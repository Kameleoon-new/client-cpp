use kameleoon_core::types::RemoteVisitorDataFilter;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct FfiRemoteVisitorDataFilter {
    pub previous_visit_amount: u32,
    pub current_visit: bool,
    pub custom_data: bool,
    pub page_views: bool,
    pub geolocation: bool,
    pub device: bool,
    pub browser: bool,
    pub operating_system: bool,
    pub conversions: bool,
    pub experiments: bool,
    pub kcs: bool,
    pub visitor_code: bool,
    pub personalizations: bool,
    pub cbs: bool,
}

impl From<FfiRemoteVisitorDataFilter> for RemoteVisitorDataFilter {
    fn from(filter: FfiRemoteVisitorDataFilter) -> Self {
        return RemoteVisitorDataFilter {
            previous_visit_amount: filter.previous_visit_amount,
            current_visit: filter.current_visit,
            custom_data: filter.custom_data,
            page_views: filter.page_views,
            geolocation: filter.geolocation,
            device: filter.device,
            browser: filter.browser,
            operating_system: filter.operating_system,
            conversions: filter.conversions,
            experiments: filter.experiments,
            kcs: filter.kcs,
            visitor_code: filter.visitor_code,
            personalizations: filter.personalizations,
            cbs: filter.cbs,
        };
    }
}
