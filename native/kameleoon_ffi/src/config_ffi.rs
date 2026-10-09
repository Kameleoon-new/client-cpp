use std::time::Duration;

use crate::raw_str_ffi::BorrowedStr;

use kameleoon_core::config::KameleoonClientConfig;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct FfiKameleoonClientConfig {
    pub client_id: BorrowedStr,
    pub client_secret: BorrowedStr,
    pub refresh_interval_minutes: u32,
    pub session_duration_minutes: u32,
    pub default_timeout_millis: u32,
    pub tracking_interval_millis: u32,
    pub environment: BorrowedStr,
    pub proxy_host: BorrowedStr,
    pub top_level_domain: BorrowedStr,
    pub network_domain: BorrowedStr,
}

impl From<FfiKameleoonClientConfig> for KameleoonClientConfig {
    fn from(c_config: FfiKameleoonClientConfig) -> Self {
        KameleoonClientConfig {
            refresh_interval: Duration::from_secs(c_config.refresh_interval_minutes as u64 * 60),
            session_duration: Duration::from_secs(c_config.session_duration_minutes as u64 * 60),
            default_timeout: Duration::from_millis(c_config.default_timeout_millis as u64),
            tracking_interval: Duration::from_millis(c_config.tracking_interval_millis as u64),
            client_id: <Option<&str>>::from(c_config.client_id).unwrap_or_default().to_owned(),
            client_secret: <Option<&str>>::from(c_config.client_secret).unwrap_or_default().to_owned(),
            environment: c_config.environment.into(),
            proxy_host: c_config.proxy_host.into(),
            top_level_domain: c_config.top_level_domain.into(),
            network_domain: c_config.network_domain.into(),
        }
    }
}
