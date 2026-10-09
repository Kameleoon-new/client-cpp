use kameleoon_core::data::UserAgent;

use crate::raw_str_ffi::BorrowedStr;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct FfiUserAgent {
    value: BorrowedStr,
}

impl From<&FfiUserAgent> for UserAgent {
    fn from(ua: &FfiUserAgent) -> Self {
        return UserAgent::new(String::from(ua.value));
    }
}
