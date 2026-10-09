use kameleoon_core::data::application_version::ApplicationVersion;

use crate::raw_str_ffi::BorrowedStr;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct FfiApplicationVersion {
    value: BorrowedStr,
}

impl From<&FfiApplicationVersion> for ApplicationVersion {
    fn from(av: &FfiApplicationVersion) -> Self {
        ApplicationVersion::new(String::from(av.value))
    }
}
