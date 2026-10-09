use std::{sync::Arc, time::Duration};

use kameleoon_core::{
    core_client::CoreKameleoonClient,
    events::{
        DataFileUpdateEvent, DataFileUpdateHandler, DataFileUpdateSource, EventHandler, HttpRequestFailure,
        HttpRequestFailureReason, HttpRequestHandler, RequestType,
    },
};

use crate::{raw_str_ffi::BorrowedStr, FromRaw, StructPtr};

// MARK: - Enums

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum FfiRequestType {
    DataFile,
    Tracking,
    RemoteVisitorData,
    RemoteData,
    AccessToken,
}

impl From<RequestType> for FfiRequestType {
    fn from(request_type: RequestType) -> Self {
        match request_type {
            RequestType::DataFile => Self::DataFile,
            RequestType::Tracking => Self::Tracking,
            RequestType::RemoteVisitorData => Self::RemoteVisitorData,
            RequestType::RemoteData => Self::RemoteData,
            RequestType::AccessToken => Self::AccessToken,
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum FfiHttpRequestFailureReason {
    HttpStatus,
    Error,
    Cancelled,
    NoFailure,
}

impl From<HttpRequestFailureReason> for FfiHttpRequestFailureReason {
    fn from(reason: HttpRequestFailureReason) -> Self {
        match reason {
            HttpRequestFailureReason::HttpStatus => Self::HttpStatus,
            HttpRequestFailureReason::Error => Self::Error,
            HttpRequestFailureReason::Cancelled => Self::Cancelled,
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum FfiDataFileUpdateSource {
    Polling,
    Streaming,
}

impl From<DataFileUpdateSource> for FfiDataFileUpdateSource {
    fn from(source: DataFileUpdateSource) -> Self {
        match source {
            DataFileUpdateSource::Polling => Self::Polling,
            DataFileUpdateSource::Streaming => Self::Streaming,
        }
    }
}

// MARK: - Events
//
// Event payloads are passed to foreign callbacks only. `failure_cause` borrows
// from the Rust caller's stack and is valid solely for the duration of the
// callback invocation: the receiver must copy what it needs and never free it.

#[repr(C)]
#[derive(Clone, Copy)]
pub struct FfiHttpRequestEvent {
    pub request_type: FfiRequestType,
    pub succeeded: bool,
    /// HTTP status of the response; `0` when the attempt produced none.
    pub http_status: u16,
    /// `NoFailure` when `succeeded` is `true`.
    pub failure_reason: FfiHttpRequestFailureReason,
    /// `NULL` unless `failure_reason` is `Error`.
    pub failure_cause: BorrowedStr,
    pub duration_millis: u64,
}

impl FfiHttpRequestEvent {
    fn succeeded(request_type: RequestType, http_status: u16, duration: Duration) -> Self {
        Self {
            request_type: request_type.into(),
            succeeded: true,
            http_status,
            failure_reason: FfiHttpRequestFailureReason::NoFailure,
            failure_cause: BorrowedStr::NULL,
            duration_millis: duration_millis(duration),
        }
    }

    fn failed(
        request_type: RequestType,
        failure: &HttpRequestFailure,
        cause: Option<&str>,
        duration: Duration,
    ) -> Self {
        Self {
            request_type: request_type.into(),
            succeeded: false,
            http_status: failure.http_status().unwrap_or(0),
            failure_reason: failure.reason().into(),
            failure_cause: cause.into(),
            duration_millis: duration_millis(duration),
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct FfiDataFileUpdateEvent {
    pub source: FfiDataFileUpdateSource,
    pub date_modified: u64,
}

impl From<&DataFileUpdateEvent> for FfiDataFileUpdateEvent {
    fn from(event: &DataFileUpdateEvent) -> Self {
        Self {
            source: event.source.into(),
            date_modified: event.date_modified,
        }
    }
}

// MARK: - Callbacks
//
// A `NULL` `func` clears the handler registered for that event type; a
// `context` sent along with it is released through `drop` right away.
//
// `context` is owned by the SDK from registration until `drop` is called,
// which happens once the handler has been replaced or cleared *and* every
// invocation in progress has returned, possibly on an SDK thread. The
// foreign side releases `context` there and nowhere else; a `NULL` `drop`
// means nothing to release.

#[repr(C)]
#[derive(Clone, Copy)]
pub struct FfiHttpRequestEventCallback {
    pub context: StructPtr,
    pub func: Option<extern "C" fn(context: StructPtr, event: FfiHttpRequestEvent)>,
    pub drop: Option<extern "C" fn(context: StructPtr)>,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct FfiDataFileUpdateEventCallback {
    pub context: StructPtr,
    pub func: Option<extern "C" fn(context: StructPtr, event: FfiDataFileUpdateEvent)>,
    pub drop: Option<extern "C" fn(context: StructPtr)>,
}

// SAFETY: see `callback_ffi`: `context` is opaque to the SDK and the foreign
// side guarantees it can be used from any thread.
unsafe impl Send for FfiHttpRequestEventCallback {}
unsafe impl Sync for FfiHttpRequestEventCallback {}
unsafe impl Send for FfiDataFileUpdateEventCallback {}
unsafe impl Sync for FfiDataFileUpdateEventCallback {}

// MARK: - Foreign handlers

struct ForeignHttpRequestHandler {
    context: StructPtr,
    func: extern "C" fn(StructPtr, FfiHttpRequestEvent),
    drop: Option<extern "C" fn(StructPtr)>,
}

impl Drop for ForeignHttpRequestHandler {
    fn drop(&mut self) {
        if let Some(drop) = self.drop {
            drop(self.context);
        }
    }
}

// SAFETY: as for the callback structs above.
unsafe impl Send for ForeignHttpRequestHandler {}
unsafe impl Sync for ForeignHttpRequestHandler {}

impl HttpRequestHandler for ForeignHttpRequestHandler {
    fn on_request_succeeded(&self, request_type: RequestType, http_status: u16, duration: Duration) {
        (self.func)(self.context, FfiHttpRequestEvent::succeeded(request_type, http_status, duration));
    }

    fn on_request_failed(&self, request_type: RequestType, failure: &HttpRequestFailure, duration: Duration) {
        // Keep the rendered cause alive for the whole callback: the event borrows it.
        let cause = failure.error().map(ToString::to_string);
        let event = FfiHttpRequestEvent::failed(request_type, failure, cause.as_deref(), duration);
        (self.func)(self.context, event);
    }
}

struct ForeignDataFileUpdateHandler {
    context: StructPtr,
    func: extern "C" fn(StructPtr, FfiDataFileUpdateEvent),
    drop: Option<extern "C" fn(StructPtr)>,
}

impl Drop for ForeignDataFileUpdateHandler {
    fn drop(&mut self) {
        if let Some(drop) = self.drop {
            drop(self.context);
        }
    }
}

// SAFETY: as for the callback structs above.
unsafe impl Send for ForeignDataFileUpdateHandler {}
unsafe impl Sync for ForeignDataFileUpdateHandler {}

impl DataFileUpdateHandler for ForeignDataFileUpdateHandler {
    fn on_update(&self, event: &DataFileUpdateEvent) {
        (self.func)(self.context, event.into());
    }
}

// MARK: - Exports
//
// One export per event type, because each carries a differently typed
// callback. The core keeps at most one handler per event type per client:
// registering replaces the previous handler, and a `NULL` `func` clears it.

#[no_mangle]
pub extern "C" fn client__set_http_request_handler(client: StructPtr, cb: FfiHttpRequestEventCallback) {
    let client = <&CoreKameleoonClient>::from_raw(client);
    let handler = match cb.func {
        Some(func) => Some(Arc::new(ForeignHttpRequestHandler {
            context: cb.context,
            func,
            drop: cb.drop,
        }) as Arc<dyn HttpRequestHandler>),
        None => {
            release_unregistered_context(cb.context, cb.drop);
            None
        }
    };
    client.set_event_handler(EventHandler::HttpRequest(handler));
}

#[no_mangle]
pub extern "C" fn client__set_datafile_update_handler(client: StructPtr, cb: FfiDataFileUpdateEventCallback) {
    let client = <&CoreKameleoonClient>::from_raw(client);
    let handler = match cb.func {
        Some(func) => Some(Arc::new(ForeignDataFileUpdateHandler {
            context: cb.context,
            func,
            drop: cb.drop,
        }) as Arc<dyn DataFileUpdateHandler>),
        None => {
            release_unregistered_context(cb.context, cb.drop);
            None
        }
    };
    client.set_event_handler(EventHandler::DataFileUpdate(handler));
}

/// A clear request has nothing to register, so a `context` that came with a
/// `drop` would otherwise never be released: hand it back immediately.
fn release_unregistered_context(context: StructPtr, drop: Option<extern "C" fn(StructPtr)>) {
    if let Some(drop) = drop {
        drop(context);
    }
}

/// Durations cross the boundary in whole milliseconds, saturating instead of overflowing.
fn duration_millis(duration: Duration) -> u64 {
    duration.as_millis().min(u64::MAX as u128) as u64
}

// MARK: - Tests

#[cfg(test)]
mod tests {
    use std::{
        io,
        sync::{Arc, Mutex},
    };

    use super::*;

    #[test]
    fn succeeded_event_carries_status_and_duration() {
        let event = FfiHttpRequestEvent::succeeded(RequestType::Tracking, 204, Duration::from_millis(1500));

        assert!(event.succeeded);
        assert_eq!(event.request_type, FfiRequestType::Tracking);
        assert_eq!(event.http_status, 204);
        assert_eq!(event.failure_reason, FfiHttpRequestFailureReason::NoFailure);
        assert!(event.failure_cause.str.is_null());
        assert_eq!(event.duration_millis, 1500);
    }

    #[test]
    fn failed_event_with_http_status_has_no_cause() {
        let failure = HttpRequestFailure::HttpStatus(503);
        let event = FfiHttpRequestEvent::failed(RequestType::DataFile, &failure, None, Duration::from_secs(2));

        assert!(!event.succeeded);
        assert_eq!(event.request_type, FfiRequestType::DataFile);
        assert_eq!(event.failure_reason, FfiHttpRequestFailureReason::HttpStatus);
        assert_eq!(event.http_status, 503);
        assert!(event.failure_cause.str.is_null());
        assert_eq!(event.duration_millis, 2000);
    }

    #[test]
    fn failed_event_with_error_borrows_the_cause() {
        let failure = HttpRequestFailure::Error(Arc::new(io::Error::new(io::ErrorKind::TimedOut, "socket timed out")));
        let cause = failure.error().map(ToString::to_string);
        let event =
            FfiHttpRequestEvent::failed(RequestType::RemoteData, &failure, cause.as_deref(), Duration::from_millis(7));

        assert_eq!(event.failure_reason, FfiHttpRequestFailureReason::Error);
        assert_eq!(event.http_status, 0);
        assert_eq!(<&str>::from(event.failure_cause), "socket timed out");
        assert_eq!(event.failure_cause.str as *const u8, cause.as_deref().unwrap().as_ptr());
    }

    #[test]
    fn cancelled_event_has_neither_status_nor_cause() {
        let event =
            FfiHttpRequestEvent::failed(RequestType::AccessToken, &HttpRequestFailure::Cancelled, None, Duration::ZERO);

        assert_eq!(event.failure_reason, FfiHttpRequestFailureReason::Cancelled);
        assert_eq!(event.http_status, 0);
        assert!(event.failure_cause.str.is_null());
        assert_eq!(event.duration_millis, 0);
    }

    #[test]
    fn data_file_update_event_maps_source_and_date() {
        let event = FfiDataFileUpdateEvent::from(&DataFileUpdateEvent::new(DataFileUpdateSource::Streaming, 42));

        assert_eq!(event.source, FfiDataFileUpdateSource::Streaming);
        assert_eq!(event.date_modified, 42);
    }

    #[test]
    fn duration_saturates_instead_of_overflowing() {
        assert_eq!(duration_millis(Duration::MAX), u64::MAX);
    }

    // A foreign handler forwards to the registered function pointer with its context.
    struct Recorded {
        events: Mutex<Vec<(bool, u16)>>,
    }

    extern "C" fn record(context: StructPtr, event: FfiHttpRequestEvent) {
        let recorded = unsafe { &*(context as *const Recorded) };
        recorded.events.lock().unwrap().push((event.succeeded, event.http_status));
    }

    #[test]
    fn foreign_http_request_handler_forwards_both_outcomes() {
        let recorded = Recorded {
            events: Mutex::new(Vec::new()),
        };
        let handler = ForeignHttpRequestHandler {
            context: &recorded as *const Recorded as StructPtr,
            func: record,
            drop: None,
        };

        handler.on_request_succeeded(RequestType::Tracking, 200, Duration::ZERO);
        handler.on_request_failed(RequestType::Tracking, &HttpRequestFailure::HttpStatus(500), Duration::ZERO);

        assert_eq!(*recorded.events.lock().unwrap(), vec![(true, 200), (false, 500)]);
    }

    // The foreign context is released exactly once, when the last reference to
    // the handler (registration or an invocation in progress) is gone.
    extern "C" fn count_drop(context: StructPtr) {
        let drops = unsafe { &*(context as *const Mutex<u32>) };
        *drops.lock().unwrap() += 1;
    }

    extern "C" fn ignore(_: StructPtr, _: FfiDataFileUpdateEvent) {}

    #[test]
    fn clearing_releases_a_context_that_was_never_registered() {
        let drops = Mutex::new(0u32);

        release_unregistered_context(&drops as *const Mutex<u32> as StructPtr, Some(count_drop));
        assert_eq!(*drops.lock().unwrap(), 1);

        release_unregistered_context(std::ptr::null(), None);
        assert_eq!(*drops.lock().unwrap(), 1, "a NULL drop has nothing to release");
    }

    #[test]
    fn foreign_handler_releases_context_after_last_reference() {
        let drops = Mutex::new(0u32);
        let handler = Arc::new(ForeignDataFileUpdateHandler {
            context: &drops as *const Mutex<u32> as StructPtr,
            func: ignore,
            drop: Some(count_drop),
        });
        let in_flight = Arc::clone(&handler);

        drop(handler);
        assert_eq!(*drops.lock().unwrap(), 0, "released while an invocation still held the handler");

        in_flight.on_update(&DataFileUpdateEvent::new(DataFileUpdateSource::Polling, 1));
        drop(in_flight);
        assert_eq!(*drops.lock().unwrap(), 1);
    }
}
