//! How a `KRes<T>` crosses the C boundary.
//!
//! Fallible exports follow the classic C convention: they return `true` on
//! success and `false` on failure, writing the payload to an `out` pointer and
//! the failure to an `error` pointer. Nothing is written to `out` on failure
//! and nothing is written to `error` on success, so the caller only frees what
//! the return value tells it to.
//!
//! Callbacks receive a `*const FfiBorrowedError` instead: `NULL` means success,
//! otherwise the error borrows from the Rust caller's stack and is valid only
//! for the duration of the callback invocation (copy what you need, never free).

use kameleoon_core::{error::KameleoonError, utils::KRes};

use crate::{
    raw_str_ffi::{BorrowedStr, OwnedStr},
    FreeRaw,
};

pub type ResultCode = u32;

/// A failure returned through an `error` out-parameter. Release with `error__free`.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct FfiError {
    pub code: ResultCode,
    pub message: OwnedStr,
}

impl From<KameleoonError> for FfiError {
    /// Moves the message out of the error; `OwnedStr::from(String)` hands its
    /// buffer to the caller, so no bytes are copied.
    fn from(err: KameleoonError) -> Self {
        FfiError {
            code: err.code() as ResultCode,
            message: err.into_message().into(),
        }
    }
}

impl FreeRaw for FfiError {
    fn free(&self) {
        self.message.free();
    }
}

#[no_mangle]
pub extern "C" fn error__free(error: FfiError) {
    error.free();
}

/// A failure passed to a foreign callback; borrowed, never freed by the receiver.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct FfiBorrowedError {
    pub code: ResultCode,
    pub message: BorrowedStr,
}

impl From<&KameleoonError> for FfiBorrowedError {
    fn from(err: &KameleoonError) -> Self {
        FfiBorrowedError {
            code: err.code() as ResultCode,
            message: err.message().into(),
        }
    }
}

// MARK: - Writing a `KRes<T>` to the out-parameters

/// Returns `true` for `Ok`; for `Err`, writes the error to `error` (when
/// non-null) and returns `false`.
pub(crate) fn write_result(result: KRes<()>, error: *mut FfiError) -> bool {
    match result {
        Ok(()) => true,
        Err(err) => {
            write_error(error, err);
            false
        }
    }
}

/// Like [`write_result`], additionally converting the `Ok` payload with `convert`
/// and writing it to `out`. A null `out` drops the core value without
/// constructing an FFI payload (which can allocate a whole tree of views).
pub(crate) fn write_result_value<T, U>(
    result: KRes<T>,
    out: *mut U,
    error: *mut FfiError,
    convert: impl FnOnce(T) -> U,
) -> bool {
    match result {
        Ok(value) => {
            if !out.is_null() {
                // SAFETY: the caller supplies a writable output pointer.
                unsafe { out.write(convert(value)) };
            }
            true
        }
        Err(err) => {
            write_error(error, err);
            false
        }
    }
}

/// Writes `err` to `error` (when non-null), moving its message across the
/// boundary; a null `error` simply drops it.
fn write_error(error: *mut FfiError, err: KameleoonError) {
    if error.is_null() {
        return;
    }
    // SAFETY: the caller passes a valid, writable pointer or null (checked above).
    unsafe { error.write(FfiError::from(err)) };
}

/// Runs a foreign callback with the borrowed view of `result`.
pub(crate) fn with_borrowed_error<T>(result: &KRes<T>, call: impl FnOnce(Option<&T>, *const FfiBorrowedError)) {
    match result {
        Ok(value) => call(Some(value), std::ptr::null()),
        Err(err) => {
            let error = FfiBorrowedError::from(err);
            call(None, &error);
        }
    }
}

// MARK: - Tests

#[cfg(test)]
mod tests {
    use kameleoon_core::error::{ErrorCode, KameleoonError};

    use super::*;
    use crate::test_utils::watch;

    fn network_error() -> KameleoonError {
        KameleoonError::new(ErrorCode::Network, "network failed")
    }

    #[test]
    fn write_result_ok_returns_true_and_leaves_error_untouched() {
        let mut error = FfiError {
            code: 42,
            message: OwnedStr::NULL,
        };

        assert!(write_result(Ok(()), &mut error));
        assert_eq!(error.code, 42);
    }

    #[test]
    fn write_result_err_writes_error_and_returns_false() {
        let mut error = FfiError {
            code: 0,
            message: OwnedStr::NULL,
        };

        assert!(!write_result(Err(network_error()), &mut error));
        assert_eq!(error.code, ErrorCode::Network as u32);
        assert_eq!(<&str>::from(error.message), "network failed");
        error.free();
    }

    #[test]
    fn write_result_err_moves_the_message_buffer_instead_of_copying() {
        // A message built from a `String` with no spare capacity keeps its
        // buffer through `into_boxed_str`, so the bytes must not move.
        let message = String::from("network failed");
        let message_ptr = message.as_ptr();
        let mut error = FfiError {
            code: 0,
            message: OwnedStr::NULL,
        };

        assert!(!write_result(Err(KameleoonError::new(ErrorCode::Network, message)), &mut error));
        assert_eq!(error.message.str as *const u8, message_ptr);
        error.free();
    }

    #[test]
    fn error_free_releases_the_message_buffer() {
        let mut error = FfiError {
            code: 0,
            message: OwnedStr::NULL,
        };
        assert!(!write_result(Err(network_error()), &mut error));
        let buffer = watch(error.message.str);

        error__free(error);

        assert_eq!(buffer.freed_size(), Some("network failed".len()), "error__free must release the message");
    }

    #[test]
    fn error_free_of_an_empty_message_is_a_no_op() {
        let mut error = FfiError {
            code: 0,
            message: OwnedStr::NULL,
        };
        assert!(!write_result(Err(KameleoonError::new(ErrorCode::Network, String::new())), &mut error));
        let buffer = watch(error.message.str);

        error__free(error);

        assert_eq!(buffer.freed_size(), None, "an empty message owns no allocation");
    }

    #[test]
    fn write_result_tolerates_null_error_pointer() {
        assert!(!write_result(Err(network_error()), std::ptr::null_mut()));
    }

    #[test]
    fn write_result_value_writes_converted_payload_only_on_success() {
        let mut out = 0_u32;
        let mut error = FfiError {
            code: 0,
            message: OwnedStr::NULL,
        };

        assert!(write_result_value(Ok(7_u8), &mut out, &mut error, u32::from));
        assert_eq!(out, 7);

        assert!(!write_result_value(Err(network_error()), &mut out, &mut error, |_: u8| 99_u32));
        assert_eq!(out, 7, "out must not be written on failure");
        assert_eq!(error.code, ErrorCode::Network as u32);
        error.free();
    }

    #[test]
    fn write_result_value_with_null_output_drops_source_without_building_views() {
        let source = std::sync::Arc::new(7_u32);
        let weak = std::sync::Arc::downgrade(&source);
        let mut error = FfiError {
            code: 42,
            message: OwnedStr::NULL,
        };

        assert!(write_result_value(Ok(source), std::ptr::null_mut::<u32>(), &mut error, |_| {
            panic!("discarded outputs must not allocate FFI views")
        }));
        assert!(weak.upgrade().is_none(), "discarding a payload must release its source");
        assert_eq!(error.code, 42, "success must leave the error untouched");
    }

    #[test]
    fn borrowed_error_borrows_message_bytes() {
        let err = network_error();
        let result: KRes<()> = Err(err);
        with_borrowed_error(&result, |value, error| {
            assert!(value.is_none());
            assert!(!error.is_null());
            let error = unsafe { *error };
            assert_eq!(error.code, ErrorCode::Network as u32);
            assert_eq!(error.message.str as *const u8, result.as_ref().unwrap_err().message().as_ptr());
        });
    }

    #[test]
    fn borrowed_success_passes_null_error() {
        with_borrowed_error(&Ok(5), |value, error| {
            assert_eq!(value, Some(&5));
            assert!(error.is_null());
        });
    }
}
