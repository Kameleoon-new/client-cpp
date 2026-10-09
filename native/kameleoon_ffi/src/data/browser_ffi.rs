use kameleoon_core::data::{Browser, BrowserKind};

use crate::data_ffi::opt_f32_from_raw;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct FfiBrowser {
    pub kind: FfiBrowserType,
    pub version: f32,
}

impl From<&FfiBrowser> for Browser {
    fn from(b: &FfiBrowser) -> Self {
        return Browser::new(BrowserKind::from(b.kind), opt_f32_from_raw(b.version));
    }
}

#[repr(C)]
#[derive(Clone, Copy)]
pub enum FfiBrowserType {
    Chrome,
    InternetExplorer,
    Firefox,
    Safari,
    Opera,
    Other,
}

impl From<FfiBrowserType> for BrowserKind {
    fn from(bt: FfiBrowserType) -> Self {
        return match bt {
            FfiBrowserType::Chrome => BrowserKind::Chrome,
            FfiBrowserType::InternetExplorer => BrowserKind::InternetExplorer,
            FfiBrowserType::Firefox => BrowserKind::Firefox,
            FfiBrowserType::Safari => BrowserKind::Safari,
            FfiBrowserType::Opera => BrowserKind::Opera,
            FfiBrowserType::Other => BrowserKind::Other,
        };
    }
}
