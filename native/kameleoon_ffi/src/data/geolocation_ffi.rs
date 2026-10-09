use kameleoon_core::data::Geolocation;

use crate::{data_ffi::opt_f32_from_raw, raw_str_ffi::BorrowedStr};

#[repr(C)]
#[derive(Clone, Copy)]
pub struct FfiGeolocation {
    pub country: BorrowedStr,
    pub region: BorrowedStr,
    pub city: BorrowedStr,
    pub postal_code: BorrowedStr,
    pub latitude: f32,
    pub longitude: f32,
}

impl From<&FfiGeolocation> for Geolocation {
    fn from(g: &FfiGeolocation) -> Self {
        Geolocation::new(
            String::from(g.country),
            g.region.into(),
            g.city.into(),
            g.postal_code.into(),
            opt_f32_from_raw(g.latitude),
            opt_f32_from_raw(g.longitude),
        )
    }
}
