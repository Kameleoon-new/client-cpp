use std::sync::Arc;

use kameleoon_core::core_factory::{CoreKameleoonClientFactory, ForgetParams};

use crate::{
    config_ffi::FfiKameleoonClientConfig,
    raw_str_ffi::BorrowedStr,
    result_ffi::{write_result, write_result_value, FfiError},
    StructPtr,
};

/// Creates (or returns the cached) client for `site_code`.
///
/// The configuration is taken from `config` (a non-`NULL` pointer) if given,
/// otherwise read from the file at `config_path` (a `BorrowedStr` with a
/// non-`NULL` pointer); one of them is required, as in the core factory. On
/// success `*out` receives a client handle to release with `client__free`; a
/// `NULL` `out` only creates and caches the client.
#[no_mangle]
pub extern "C" fn client_factory__create(
    sdk_name: BorrowedStr,
    sdk_version: BorrowedStr,
    site_code: BorrowedStr,
    config_path: BorrowedStr,
    config: *const FfiKameleoonClientConfig,
    out: *mut StructPtr,
    error: *mut FfiError,
) -> bool {
    // SAFETY: `config` is null or points to a valid config for the duration of the call.
    let config = unsafe { config.as_ref() }.copied().map(Into::into);
    let result = CoreKameleoonClientFactory::create(
        site_code.into(),
        config_path.into(),
        config,
        sdk_name.into(),
        sdk_version.into(),
    );
    write_result_value(result, out, error, |client| Arc::into_raw(client) as StructPtr)
}

/// Removes the cached client for `site_code`; `environment` with a `NULL`
/// pointer selects the default environment.
#[no_mangle]
pub extern "C" fn client_factory__forget(
    site_code: BorrowedStr,
    environment: BorrowedStr,
    error: *mut FfiError,
) -> bool {
    let site_code: &str = site_code.into();
    let environment: Option<&str> = environment.into();
    let result = match environment {
        Some(environment) => {
            CoreKameleoonClientFactory::forget_with_params(site_code, ForgetParams::Environment(environment))
        }
        None => CoreKameleoonClientFactory::forget(site_code),
    };
    write_result(result, error)
}
