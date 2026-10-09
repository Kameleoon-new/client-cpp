//! C ABI of the Kameleoon SDK Core, consumed by the C++ SDK (and any other
//! language able to call C).
//!
//! Conventions, shared by every export:
//!
//! * Fallible functions return `true` on success. On failure they return
//!   `false` and write the error to the `error` out-parameter (release it with
//!   `error__free`); the `out` parameter is left untouched.
//! * Payloads written to `out` are owned by the caller and released with the
//!   free function of their type (`string__free`, `variation__free`, …).
//!   Structured payloads (`FfiVariation`, `FfiVariations`, `FfiDataFile`) are
//!   *views*: their strings borrow from the core value retained by the
//!   payload's `owner` and are valid until that free call, so no string bytes
//!   are copied on the Rust side. Copy them before freeing the root payload;
//!   never free its nested views individually.
//! * Strings are length-prefixed UTF-8 and never NUL-terminated. `BorrowedStr`
//!   is never freed by the receiver: inputs and callback arguments are valid
//!   only during the call, strings inside structured results until that
//!   result's `*__free`. `OwnedStr` is the caller's, released with
//!   `string__free`; `ForeignOwnedStr` is foreign memory Rust hands back
//!   through the accessor's `free_str`.
//! * Optional inputs are nullable pointers (`NULL` = absent); optional strings
//!   are `BorrowedStr` with a `NULL` pointer; optional floats are `NaN`.
//! * Asynchronous functions return immediately and invoke their callback
//!   exactly once, possibly before returning and possibly from another thread.
//!   Callback payloads borrow from the core and are valid only during the call.
//! * Arrays are pointer + length (`FfiArray`); maps are arrays of key/value
//!   pairs. `ffi__abi_version()` reports [`FFI_ABI_VERSION`] so a consumer can
//!   refuse a shared library built for another revision of this ABI.

use kameleoon_core::core_client::CoreKameleoonClient;

use std::ffi::c_void;

pub mod array_ffi;
pub mod callback_ffi;
pub mod client_factory_ffi;
pub mod client_ffi;
pub mod config_ffi;
pub mod cookie_accessor_ffi;
pub mod data_ffi;
pub mod datafile_ffi;
pub mod events_ffi;
#[cfg(feature = "jni")]
pub mod jni;
pub mod logging_ffi;
#[cfg(feature = "jni")]
pub mod proto;
pub mod raw_str_ffi;
pub mod remote_visitor_data_filter_ffi;
pub mod result_ffi;
pub mod variable_ffi;
pub mod variation_ffi;

pub type StructPtr = *const c_void;

/// Version of this C ABI. Bumped on every incompatible change to the exported
/// types or functions; consumers compare it with `ffi__abi_version()` at
/// runtime to refuse a mismatched shared library instead of corrupting memory.
pub const FFI_ABI_VERSION: u32 = 1;

#[no_mangle]
pub extern "C" fn ffi__abi_version() -> u32 {
    FFI_ABI_VERSION
}

pub trait FreeRaw {
    fn free(&self);
}

#[cfg(test)]
mod test_utils;

#[cfg(test)]
#[global_allocator]
static GLOBAL: test_utils::TrackingAllocator = test_utils::TrackingAllocator;

pub trait FromRaw<RawPtr> {
    fn from_raw(raw: RawPtr) -> Self;
}

impl FromRaw<StructPtr> for &CoreKameleoonClient {
    fn from_raw(ptr: StructPtr) -> Self {
        // SAFETY: handles come from `Arc::into_raw` in `client_factory__create`
        // and stay valid until `client__free`.
        unsafe { &*(ptr as *const CoreKameleoonClient) }
    }
}
