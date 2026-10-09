use kameleoon_core::data::{Device, DeviceKind};

#[repr(C)]
#[derive(Clone, Copy)]
pub struct FfiDevice {
    pub kind: FfiDeviceType,
}

impl From<&FfiDevice> for Device {
    fn from(d: &FfiDevice) -> Self {
        return Device::new(DeviceKind::from(d.kind));
    }
}

#[repr(C)]
#[derive(Clone, Copy)]
pub enum FfiDeviceType {
    Phone,
    Tablet,
    Desktop,
}

impl From<FfiDeviceType> for DeviceKind {
    fn from(dt: FfiDeviceType) -> Self {
        return match dt {
            FfiDeviceType::Phone => DeviceKind::Phone,
            FfiDeviceType::Tablet => DeviceKind::Tablet,
            FfiDeviceType::Desktop => DeviceKind::Desktop,
        };
    }
}
