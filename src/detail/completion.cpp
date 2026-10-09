#include "completion.hpp"

#include "result.hpp"

#include <exception>
#include <memory>
#include <type_traits>
#include <string>
#include <utility>

using namespace std;

namespace kameleoon::detail
{
    namespace
    {
        // Takes back the box handed to the core and settles its completion.
        // Completions must not throw (see `Completion<T>` docs); anything
        // escaping is discarded.
        template <typename T, typename... Args>
        void settle(ffi::StructPtr context, Args &&...args) noexcept
        {
            unique_ptr<CompletionContext<T>> box(static_cast<CompletionContext<T> *>(const_cast<void *>(context)));
            try
            {
                box->on_done(std::forward<Args>(args)...);
            }
            catch (...)
            {
            }
        }

        void completion_callback(ffi::StructPtr context, const ffi::FfiBorrowedError *error) noexcept
        {
            settle<void>(context, error_to_exception(error));
        }

        void bytes_completion_callback(ffi::StructPtr context,
                                       ffi::FfiArray_u8 bytes,
                                       const ffi::FfiBorrowedError *error) noexcept
        {
            auto failure = error_to_exception(error);
            string value;
            if (!failure)
            {
                try
                {
                    // The payload borrows from the core: copy before returning.
                    const auto *ptr = reinterpret_cast<const char *>(bytes.ptr);
                    if (ptr != nullptr)
                    {
                        value.assign(ptr, ptr + bytes.len);
                    }
                }
                catch (...)
                {
                    failure = current_exception();
                }
            }
            settle<string>(context, std::move(failure), std::move(value));
        }

        // An empty completion is fire-and-forget: the outcome is discarded. The
        // box is still needed so the native client outlives the call.
        template <typename T>
        CompletionContext<T> *box(const shared_ptr<ClientCallbackState> &client, Completion<T> on_done)
        {
            if (!on_done)
            {
                if constexpr (is_void_v<T>)
                {
                    on_done = [](exception_ptr) {};
                }
                else
                {
                    on_done = [](exception_ptr, T) {};
                }
            }
            return new CompletionContext<T>{client, std::move(on_done)};
        }
    } // namespace

    ffi::FfiVoidCallback completion(const shared_ptr<ClientCallbackState> &client, Completion<void> on_done)
    {
        return ffi::FfiVoidCallback{box<void>(client, std::move(on_done)), completion_callback};
    }

    ffi::FfiBytesCallback completion(const shared_ptr<ClientCallbackState> &client, Completion<string> on_done)
    {
        return ffi::FfiBytesCallback{box<string>(client, std::move(on_done)), bytes_completion_callback};
    }

} // namespace kameleoon::detail
