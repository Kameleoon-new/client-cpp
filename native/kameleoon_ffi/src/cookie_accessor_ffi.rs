use kameleoon_core::cookies::accessor::CoreCookieAccessor;

use crate::{
    raw_str_ffi::{BorrowedStr, ForeignOwnedStr},
    StructPtr,
};

// The string parameters of `set` and `get` borrow from the calling Rust stack
// and are valid only for the duration of the callback invocation; the
// receiver must copy what it needs and must not free them. The string
// returned by `get` is owned by the foreign side and is released through
// `free_str` once Rust has copied it.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct FfiCookieAccessor {
    pub context: StructPtr,
    pub set: extern "C" fn(
        context: StructPtr,
        key: BorrowedStr,
        value: BorrowedStr,
        max_age: u32,
        top_level_domain: BorrowedStr,
    ),
    pub get: extern "C" fn(context: StructPtr, key: BorrowedStr) -> ForeignOwnedStr,
    pub free_str: extern "C" fn(ptr: ForeignOwnedStr),
}

impl CoreCookieAccessor for FfiCookieAccessor {
    fn set(&mut self, key: &str, value: &str, max_age: u32, top_level_domain: Option<&str>) {
        (self.set)(self.context, key.into(), value.into(), max_age, top_level_domain.into());
    }

    fn get(&self, key: &str) -> Option<String> {
        let value = (self.get)(self.context, key.into());
        if value.str.is_null() {
            None
        } else {
            let managed_value = <&str>::from(value).to_owned();
            (self.free_str)(value);
            Some(managed_value)
        }
    }
}
