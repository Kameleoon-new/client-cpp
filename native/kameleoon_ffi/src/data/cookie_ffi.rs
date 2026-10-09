use kameleoon_core::data::Cookie;

use crate::{
    array_ffi::{FfiArray, FfiKeyValuePair},
    raw_str_ffi::BorrowedStr,
};

#[repr(C)]
#[derive(Clone, Copy)]
pub struct FfiCookie {
    pub cookies: FfiArray<FfiKeyValuePair<BorrowedStr, BorrowedStr>>,
}

impl From<&FfiCookie> for Cookie {
    fn from(c: &FfiCookie) -> Self {
        return Cookie::new(
            c.cookies.as_slice().iter().map(|cpair| (String::from(cpair.key), String::from(cpair.value))).collect(),
        );
    }
}
