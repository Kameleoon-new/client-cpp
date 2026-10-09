#include "kameleoon/logging.hpp"

#include "detail/abi.hpp"
#include "detail/input_arena.hpp"
#include "detail/result.hpp"

#include "detail/ffi.hpp"

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <new>
#include <utility>

using namespace std;

namespace kameleoon
{
    namespace ffi = detail::ffi;

    namespace
    {
        using LoggerPtr = shared_ptr<const KameleoonLogger::Logger>;

        // An atomic shared_ptr gives the two guarantees the logger needs
        // without a mutex: a callback in flight keeps the logger it copied
        // alive while another thread replaces it, and the replaced logger is
        // destroyed after the atomic operation completes, so a user logger
        // whose destructor calls set_logger cannot deadlock.
#if defined(__cpp_lib_atomic_shared_ptr)
        using LoggerSlot = atomic<LoggerPtr>;
        LoggerPtr load_logger(LoggerSlot &slot) { return slot.load(); }
        LoggerPtr exchange_logger(LoggerSlot &slot, LoggerPtr logger) { return slot.exchange(std::move(logger)); }
#else
        // C++17: the free-function overloads for shared_ptr.
        using LoggerSlot = LoggerPtr;
        LoggerPtr load_logger(LoggerSlot &slot) { return atomic_load(&slot); }
        LoggerPtr exchange_logger(LoggerSlot &slot, LoggerPtr logger) { return atomic_exchange(&slot, std::move(logger)); }
#endif

        // Constructed in static storage and never destroyed: Rust background
        // threads may emit logs during and after static destruction (after
        // main() returns), so the slot referenced by the registered callback
        // must outlive every C++ static. Being static storage rather than a
        // heap allocation, it is invisible to leak checkers.
        template <typename T>
        T &immortal()
        {
            alignas(T) static unsigned char storage[sizeof(T)];
            static T *object = new (storage) T();
            return *object;
        }

        LoggerSlot &logger_slot()
        {
            return immortal<LoggerSlot>();
        }

        // Serialises the two steps of a registration (the C++ slot and the
        // native callback) against competing calls: a reset interleaved
        // between the two steps of a set would otherwise leave the native
        // trampoline registered with an empty slot, silently dropping every
        // log. Never held while user code runs: callbacks only load the slot,
        // and the previous logger is released after unlocking.
        mutex &registration_mutex()
        {
            return immortal<mutex>();
        }

        void logger_callback(ffi::StructPtr, uint8_t level, ffi::BorrowedStr message) noexcept
        {
            try
            {
                if (const auto logger = load_logger(logger_slot()))
                {
                    (*logger)(static_cast<LogLevel>(level), detail::copy_raw(message));
                }
            }
            catch (...)
            {
                // User loggers must not throw; see KameleoonLogger docs.
            }
        }

    } // namespace

    // Both entry points are commonly called before any client exists, so they
    // perform the ABI handshake themselves rather than relying on `create`.
    void KameleoonLogger::set_log_level(LogLevel level)
    {
        detail::ensure_abi_compatible();
        ffi::logger__set_log_level(static_cast<uint8_t>(level));
    }

    void KameleoonLogger::set_logger(Logger logger)
    {
        detail::ensure_abi_compatible();
        // Declared before the lock so the previous logger is destroyed after
        // it is released: its destructor may call set_logger itself.
        LoggerPtr previous;
        {
            const lock_guard<mutex> lock(registration_mutex());
            if (!logger)
            {
                // Restore the core default logger first so no further
                // callbacks are dispatched, then drop the C++ logger.
                ffi::logger__reset_logfunc();
                previous = exchange_logger(logger_slot(), nullptr);
            }
            else
            {
                // Publish the logger first so a callback racing with
                // registration already sees it.
                previous = exchange_logger(logger_slot(), make_shared<const Logger>(std::move(logger)));
                ffi::logger__set_logfunc(ffi::FfiLogCallback{nullptr, logger_callback});
            }
        }
    }
} // namespace kameleoon
