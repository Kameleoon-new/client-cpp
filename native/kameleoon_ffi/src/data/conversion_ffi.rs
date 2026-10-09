use kameleoon_core::data::{Conversion, CustomData};

use crate::{array_ffi::FfiArray, data_ffi::custom_data_ffi::FfiCustomData};

#[repr(C)]
#[derive(Clone, Copy)]
pub struct FfiConversion {
    pub goal_id: u32,
    pub revenue: f32,
    pub negative: bool,
    pub metadata: FfiArray<FfiCustomData>,
}

impl From<&FfiConversion> for Conversion {
    fn from(c: &FfiConversion) -> Self {
        let metadata = Vec::from_iter(c.metadata.as_slice().iter().map(CustomData::from));
        return Conversion::new_with_opts(
            c.goal_id,
            kameleoon_core::data::ConversionOpts::new().revenue(c.revenue).negative(c.negative).metadata(metadata),
        );
    }
}
