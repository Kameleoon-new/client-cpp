use kameleoon_core::data::PageView;

use crate::{array_ffi::FfiArray, raw_str_ffi::BorrowedStr};

#[repr(C)]
#[derive(Clone, Copy)]
pub struct FfiPageView {
    pub url: BorrowedStr,
    pub title: BorrowedStr,
    pub referrers: FfiArray<i32>,
}

impl From<&FfiPageView> for PageView {
    fn from(pv: &FfiPageView) -> Self {
        let title: Option<String> = pv.title.into();
        return PageView::new(<&str>::from(pv.url), title, pv.referrers.as_slice().to_vec());
    }
}
