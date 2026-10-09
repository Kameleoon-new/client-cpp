#pragma once

#include <exception>
#include <functional>
#include <version>

#if defined(__cpp_impl_coroutine) && defined(__cpp_lib_coroutine)
#define KAMELEOON_HAS_COROUTINES 1
#include <atomic>
#include <coroutine>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>
#endif

namespace kameleoon
{
    namespace detail
    {
        template <typename T>
        struct CompletionFor
        {
            using type = std::function<void(std::exception_ptr error, T value)>;
        };

        template <>
        struct CompletionFor<void>
        {
            using type = std::function<void(std::exception_ptr error)>;
        };
    } // namespace detail

    /// Completion of an asynchronous SDK call (`*_async` methods of
    /// `KameleoonClient`):
    ///
    ///     Completion<T>    = std::function<void(std::exception_ptr error, T value)>
    ///     Completion<void> = std::function<void(std::exception_ptr error)>
    ///
    /// `error` is null on success, and only then is `value` meaningful;
    /// otherwise `error` holds the `KameleoonException` that the blocking form
    /// of the same method would have thrown.
    ///
    /// Omitting the completion (or passing `{}`) makes the call
    /// fire-and-forget: it still runs to completion and keeps the client
    /// alive meanwhile, but its outcome is discarded; failures are visible
    /// only in the SDK log. Input validation still throws from the call
    /// itself, before anything is started.
    ///
    /// Threading contract:
    /// - Invoked exactly once per call.
    /// - Usually invoked on the SDK's single worker thread. It may also be
    ///   invoked on the calling thread, before the `*_async` call returns, when
    ///   the core fails fast (client not ready, invalid visitor code, ...).
    /// - Must not throw; an escaping exception is caught and discarded.
    /// - Must not block, and must not call blocking SDK methods: they wait for
    ///   the same worker thread. Hand off to your own event loop instead
    ///   (`post`, `dispatch`, queue push, ...) and finish the work there.
    template <typename T>
    using Completion = typename detail::CompletionFor<T>::type;

#ifdef KAMELEOON_HAS_COROUTINES

    /// Schedules a coroutine resumption on the caller's own event loop, e.g.
    /// `[&io](auto resume) { boost::asio::post(io, std::move(resume)); }`.
    using Executor = std::function<void(std::function<void()> resume)>;

    namespace detail
    {
        template <typename T>
        struct AwaitState
        {
            using Value = std::conditional_t<std::is_void_v<T>, std::monostate, T>;

            // Set once by whichever of await_suspend / completion runs second;
            // that side resumes the coroutine.
            std::atomic<bool> settled{false};
            // Set by the awaitable's destructor. Once the coroutine frame is
            // gone (the awaitable lives in it), a completion or a resumption
            // still queued on the executor must not touch `handle`.
            std::atomic<bool> abandoned{false};
            std::exception_ptr error;
            std::optional<Value> value;
            Executor executor;
            std::coroutine_handle<> handle;
        };

        // Shared ownership keeps the state alive for a resumption queued on the
        // executor, which checks for a destroyed frame right before resuming.
        template <typename T>
        void resume(const std::shared_ptr<AwaitState<T>> &state)
        {
            if (state->executor)
            {
                state->executor([state]
                                {
                    if (!state->abandoned.load(std::memory_order_acquire))
                    {
                        state->handle.resume();
                    } });
            }
            else if (!state->abandoned.load(std::memory_order_acquire))
            {
                state->handle.resume();
            }
        }
    } // namespace detail

    /// Result of the `co_*` methods of `KameleoonClient`; `co_await` it from a
    /// coroutine whose promise accepts foreign awaitables (a hand-written task,
    /// cppcoro, folly, ...).
    ///
    ///     co_await client.co_get_remote_visitor_data(visitor_code);
    ///
    /// Boost.Asio's `awaitable<T>` is not such a type: its promise only awaits
    /// Asio's own asynchronous operations, so with Asio wrap the `*_async`
    /// methods in `boost::asio::async_initiate` (posting the completion back
    /// through the handler's associated executor) and `co_await` that with
    /// `use_awaitable`; the SDK documentation shows the adapter.
    ///
    /// The SDK call starts when the coroutine suspends on it, not when the
    /// awaitable is created; an awaitable that is never awaited starts nothing.
    /// On failure `co_await` throws the `KameleoonException`.
    ///
    /// By default the coroutine is resumed inline on the SDK's worker thread
    /// (see `Completion`), which is fine for short continuations that only
    /// call the in-memory SDK methods. Call `via(executor)` to be resumed on
    /// your own event loop instead; a server should always do that.
    ///
    /// The `KameleoonClient` must outlive the awaited operation. The awaiting
    /// coroutine need not: a framework may destroy it while it is suspended
    /// here (cancellation, shutdown of its loop), in which case the operation
    /// completes against nothing, its outcome is discarded and the frame is
    /// never resumed. That destruction must not run concurrently with the
    /// completion or with a resumption posted through `via` — destroying a
    /// coroutine another thread is resuming is a race in any framework — so
    /// on an event loop destroy it from the loop that owns it.
    template <typename T>
    class [[nodiscard]] Awaitable
    {
    public:
        using Start = std::function<void(Completion<T> on_done)>;

        explicit Awaitable(Start start) : start_(std::move(start)), state_(std::make_shared<detail::AwaitState<T>>()) {}

        Awaitable(const Awaitable &) = delete;
        Awaitable &operator=(const Awaitable &) = delete;
        Awaitable(Awaitable &&) noexcept = default;
        Awaitable &operator=(Awaitable &&) noexcept = default;

        // Runs when the frame holding this awaitable is destroyed: after a
        // normal resumption (nothing left to do) or while still suspended
        // (the completion must now leave the frame alone). A moved-from
        // awaitable has no state.
        ~Awaitable()
        {
            if (state_)
            {
                state_->abandoned.store(true, std::memory_order_release);
            }
        }

        /// Resumes the awaiting coroutine through `executor` instead of inline.
        Awaitable via(Executor executor) &&
        {
            state_->executor = std::move(executor);
            return std::move(*this);
        }

        bool await_ready() const noexcept
        {
            return false;
        }

        // After `start` runs, the completion may resume and even finish the
        // coroutine on another thread, so nothing below touches `this`.
        bool await_suspend(std::coroutine_handle<> handle)
        {
            auto state = state_;
            state->handle = handle;
            auto start = std::move(start_);
            // `value` is empty for `Completion<void>` (emplaces the monostate),
            // a single `T` otherwise.
            start([state](std::exception_ptr error, auto &&...value) {
                state->error = error;
                state->value.emplace(std::forward<decltype(value)>(value)...);
                if (state->settled.exchange(true, std::memory_order_acq_rel))
                {
                    detail::resume(state);
                }
            });
            // false: the completion already ran, continue without suspending.
            return !state->settled.exchange(true, std::memory_order_acq_rel);
        }

        T await_resume()
        {
            if (state_->error)
            {
                std::rethrow_exception(state_->error);
            }
            if constexpr (!std::is_void_v<T>)
            {
                return std::move(*state_->value);
            }
        }

    private:
        Start start_;
        std::shared_ptr<detail::AwaitState<T>> state_;
    };

#endif // KAMELEOON_HAS_COROUTINES
} // namespace kameleoon
