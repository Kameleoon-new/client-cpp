use kameleoon_core::data::KameleoonData;

use application_version_ffi::FfiApplicationVersion;
use browser_ffi::FfiBrowser;
use conversion_ffi::FfiConversion;
use cookie_ffi::FfiCookie;
use custom_data_ffi::FfiCustomData;
use device_ffi::FfiDevice;
use geolocation_ffi::FfiGeolocation;
use operating_system_ffi::FfiOperatingSystem;
use page_view_ffi::FfiPageView;
use unique_identifier_ffi::FfiUniqueIdentifier;
use user_agent_ffi::FfiUserAgent;

#[path = "data/application_version_ffi.rs"]
pub mod application_version_ffi;
#[path = "data/browser_ffi.rs"]
pub mod browser_ffi;
#[path = "data/conversion_ffi.rs"]
pub mod conversion_ffi;
#[path = "data/cookie_ffi.rs"]
pub mod cookie_ffi;
#[path = "data/custom_data_ffi.rs"]
pub mod custom_data_ffi;
#[path = "data/device_ffi.rs"]
pub mod device_ffi;
#[path = "data/geolocation_ffi.rs"]
pub mod geolocation_ffi;
#[path = "data/operating_system_ffi.rs"]
pub mod operating_system_ffi;
#[path = "data/page_view_ffi.rs"]
pub mod page_view_ffi;
#[path = "data/unique_identifier_ffi.rs"]
pub mod unique_identifier_ffi;
#[path = "data/user_agent_ffi.rs"]
pub mod user_agent_ffi;

#[repr(C, u32)]
#[derive(Clone, Copy)]
pub enum FfiData {
    Browser(FfiBrowser),
    Conversion(FfiConversion),
    Cookie(FfiCookie),
    CustomData(FfiCustomData),
    Device(FfiDevice),
    Geolocation(FfiGeolocation),
    OperatingSystem(FfiOperatingSystem),
    PageView(FfiPageView),
    UniqueIdentifier(FfiUniqueIdentifier),
    UserAgent(FfiUserAgent),
    ApplicationVersion(FfiApplicationVersion),
}

impl FfiData {
    pub fn to_data(&self) -> KameleoonData {
        match self {
            FfiData::Browser(b) => KameleoonData::Browser(b.into()),
            FfiData::Conversion(c) => KameleoonData::Conversion(c.into()),
            FfiData::Cookie(c) => KameleoonData::Cookie(c.into()),
            FfiData::CustomData(cd) => KameleoonData::CustomData(cd.into()),
            FfiData::Device(d) => KameleoonData::Device(d.into()),
            FfiData::Geolocation(g) => KameleoonData::Geolocation(g.into()),
            FfiData::OperatingSystem(os) => KameleoonData::OperatingSystem(os.into()),
            FfiData::PageView(pv) => KameleoonData::PageView(pv.into()),
            FfiData::UniqueIdentifier(ui) => KameleoonData::UniqueIdentifier(ui.into()),
            FfiData::UserAgent(ua) => KameleoonData::UserAgent(ua.into()),
            FfiData::ApplicationVersion(av) => KameleoonData::ApplicationVersion(av.into()),
        }
    }
}

/// Optional floats travel as NaN (the C side sends `NAN` for "absent").
pub(crate) fn opt_f32_from_raw(value: f32) -> Option<f32> {
    if value.is_nan() {
        None
    } else {
        Some(value)
    }
}

#[cfg(test)]
mod tests {
    use super::opt_f32_from_raw;

    #[test]
    fn f32_value_is_some() {
        assert_eq!(opt_f32_from_raw(12.5), Some(12.5));
    }

    #[test]
    fn f32_nan_is_none() {
        assert_eq!(opt_f32_from_raw(f32::NAN), None);
    }
}
