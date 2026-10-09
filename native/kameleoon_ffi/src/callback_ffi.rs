//! Foreign callbacks. Each callback is a plain `context` + function pointer
//! pair; the SDK never interprets `context`, it only hands it back.
//!
//! Callbacks are stored by the core and invoked from its worker threads, so
//! the foreign side must make `context` safe to use from any thread. Payload
//! arguments (errors, byte arrays, strings) borrow from the Rust caller and are
//! valid only for the duration of the invocation.

use crate::{array_ffi::FfiArray, raw_str_ffi::BorrowedStr, result_ffi::FfiBorrowedError, StructPtr};

/// Completion of an asynchronous operation without a payload.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct FfiVoidCallback {
    pub context: StructPtr,
    pub func: extern "C" fn(context: StructPtr, error: *const FfiBorrowedError),
}

impl FfiVoidCallback {
    pub fn call(&self, error: *const FfiBorrowedError) {
        (self.func)(self.context, error);
    }
}

/// Completion of an asynchronous operation producing bytes (`data` is valid
/// only when `error` is `NULL`).
#[repr(C)]
#[derive(Clone, Copy)]
pub struct FfiBytesCallback {
    pub context: StructPtr,
    pub func: extern "C" fn(context: StructPtr, data: FfiArray<u8>, error: *const FfiBorrowedError),
}

impl FfiBytesCallback {
    pub fn call(&self, data: FfiArray<u8>, error: *const FfiBorrowedError) {
        (self.func)(self.context, data, error);
    }
}

/// Log records emitted by the core.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct FfiLogCallback {
    pub context: StructPtr,
    pub func: extern "C" fn(context: StructPtr, log_level: u8, message: BorrowedStr),
}

// SAFETY: `context` is an opaque foreign pointer that the SDK never
// dereferences; the foreign side guarantees it can be used from any thread
// (see module docs). Function pointers are inherently thread-safe.
unsafe impl Send for FfiVoidCallback {}
unsafe impl Sync for FfiVoidCallback {}
unsafe impl Send for FfiBytesCallback {}
unsafe impl Sync for FfiBytesCallback {}
unsafe impl Send for FfiLogCallback {}
unsafe impl Sync for FfiLogCallback {}
