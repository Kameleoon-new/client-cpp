use kameleoon_core::data::{CustomData, CustomDataOpts};

use crate::{array_ffi::FfiArray, raw_str_ffi::BorrowedStr};

/// `FfiCustomData::id` of a custom data identified by `name`; the core resolves
/// the index from the data file.
pub const CUSTOM_DATA_UNDEFINED_ID: u32 = u32::MAX;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct FfiCustomData {
    /// Takes precedence over `name` unless it is `CUSTOM_DATA_UNDEFINED_ID`.
    pub id: u32,
    /// `NULL` when the custom data is identified by `id`.
    pub name: BorrowedStr,
    pub values: FfiArray<BorrowedStr>,
    pub overwrite: bool,
}

impl From<&FfiCustomData> for CustomData {
    fn from(cd: &FfiCustomData) -> Self {
        let values = Vec::from_iter(cd.values.as_slice().iter().map(|v| String::from(*v)));
        let opts = CustomDataOpts::new().overwrite(cd.overwrite);
        match Option::<&str>::from(cd.name) {
            Some(name) if cd.id == CUSTOM_DATA_UNDEFINED_ID => CustomData::new_with_name_opts(name, values, opts),
            _ => CustomData::new_with_index_opts(cd.id, values, opts),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn ffi(id: u32, name: Option<&str>, values: &[BorrowedStr], overwrite: bool) -> FfiCustomData {
        FfiCustomData {
            id,
            name: name.into(),
            values: FfiArray::borrowed(values),
            overwrite,
        }
    }

    #[test]
    fn by_id() {
        let values = [BorrowedStr::from("v1"), BorrowedStr::from("v2")];
        let cd = CustomData::from(&ffi(7, None, &values, true));
        assert_eq!(7, cd.index);
        assert_eq!(None, cd.name);
        assert_eq!(vec!["v1".to_owned(), "v2".to_owned()], cd.values);
        assert!(cd.overwrite);
    }

    #[test]
    fn by_name() {
        let values = [BorrowedStr::from("premium")];
        let cd = CustomData::from(&ffi(CUSTOM_DATA_UNDEFINED_ID, Some("plan"), &values, false));
        assert_eq!(CUSTOM_DATA_UNDEFINED_ID, cd.index);
        assert_eq!(Some("plan".to_owned()), cd.name);
        assert_eq!(vec!["premium".to_owned()], cd.values);
        assert!(!cd.overwrite);
    }

    #[test]
    fn id_wins_over_name() {
        let cd = CustomData::from(&ffi(3, Some("plan"), &[], true));
        assert_eq!(3, cd.index);
        assert_eq!(None, cd.name);
    }
}
