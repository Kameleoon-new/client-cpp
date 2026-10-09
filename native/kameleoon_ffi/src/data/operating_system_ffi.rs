use kameleoon_core::data::{OperatingSystem, OperatingSystemKind};

#[repr(C)]
#[derive(Clone, Copy)]
pub struct FfiOperatingSystem {
    pub kind: FfiOperatingSystemType,
}

impl From<&FfiOperatingSystem> for OperatingSystem {
    fn from(os: &FfiOperatingSystem) -> Self {
        return OperatingSystem::new(OperatingSystemKind::from(os.kind));
    }
}

#[repr(C)]
#[derive(Clone, Copy)]
pub enum FfiOperatingSystemType {
    Windows,
    Mac,
    IOS,
    Linux,
    Android,
    WindowsPhone,
}

impl From<FfiOperatingSystemType> for OperatingSystemKind {
    fn from(ost: FfiOperatingSystemType) -> Self {
        return match ost {
            FfiOperatingSystemType::Windows => OperatingSystemKind::Windows,
            FfiOperatingSystemType::Mac => OperatingSystemKind::Mac,
            FfiOperatingSystemType::IOS => OperatingSystemKind::IOS,
            FfiOperatingSystemType::Linux => OperatingSystemKind::Linux,
            FfiOperatingSystemType::Android => OperatingSystemKind::Android,
            FfiOperatingSystemType::WindowsPhone => OperatingSystemKind::WindowsPhone,
        };
    }
}
