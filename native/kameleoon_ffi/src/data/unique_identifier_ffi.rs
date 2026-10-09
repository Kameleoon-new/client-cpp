use kameleoon_core::data::UniqueIdentifier;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct FfiUniqueIdentifier {
    pub value: bool,
}

impl From<&FfiUniqueIdentifier> for UniqueIdentifier {
    fn from(ui: &FfiUniqueIdentifier) -> Self {
        return UniqueIdentifier::new(ui.value);
    }
}
