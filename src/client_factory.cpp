#include "kameleoon/client_factory.hpp"

#include "detail/abi.hpp"
#include "detail/conversions.hpp"
#include "detail/input_arena.hpp"
#include "detail/result.hpp"

#include "detail/ffi.hpp"

#include <cstdint>
#include <string>

using namespace std;

namespace kameleoon
{
    namespace ffi = detail::ffi;

    namespace
    {

        ffi::StructPtr create_client(const string &site_code,
                                     ffi::BorrowedStr config_path,
                                     const ffi::FfiKameleoonClientConfig *config)
        {
            detail::ensure_abi_compatible();
            detail::InputArena arena;
            const auto raw_site_code = arena.raw(site_code);
            ffi::StructPtr ptr = nullptr;
            detail::call_checked([&](ffi::FfiError *error)
                                 { return ffi::client_factory__create(arena.raw(CPP_SDK_NAME),
                                                                      arena.raw(CPP_SDK_VERSION),
                                                                      raw_site_code,
                                                                      config_path,
                                                                      config,
                                                                      &ptr,
                                                                      error); });
            return ptr;
        }

    } // namespace

    // Releases the Rust-side reference if the wrapper constructor throws
    // (allocation) so the core client is not leaked.
    KameleoonClient KameleoonClientFactory::wrap(const void *inner)
    {
        try
        {
            return KameleoonClient(inner);
        }
        catch (...)
        {
            ffi::client__free(inner);
            throw;
        }
    }

    KameleoonClient KameleoonClientFactory::create(const string &site_code, const KameleoonClientConfig &config)
    {
        detail::InputArena arena;
        const auto raw_config = detail::to_ffi(arena, config);
        return wrap(create_client(site_code, ffi::BorrowedStr{nullptr, 0}, &raw_config));
    }

    KameleoonClient KameleoonClientFactory::create(const string &site_code, const string &config_path)
    {
        detail::InputArena arena;
        return wrap(create_client(site_code, arena.raw(config_path), nullptr));
    }

    void KameleoonClientFactory::forget(const string &site_code)
    {
        detail::InputArena arena;
        const auto raw_site_code = arena.raw(site_code);
        detail::call_checked([&](ffi::FfiError *error)
                             { return ffi::client_factory__forget(raw_site_code, ffi::BorrowedStr{nullptr, 0}, error); });
    }

    void KameleoonClientFactory::forget(const string &site_code, const string &environment)
    {
        detail::InputArena arena;
        const auto raw_site_code = arena.raw(site_code);
        const auto raw_environment = arena.raw(environment);
        detail::call_checked([&](ffi::FfiError *error)
                             { return ffi::client_factory__forget(raw_site_code, raw_environment, error); });
    }

} // namespace kameleoon
