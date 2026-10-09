use std::{sync::Arc, time::Duration};

use kameleoon_core::core_client::CoreKameleoonClient;

use crate::{
    array_ffi::FfiArray,
    callback_ffi::{FfiBytesCallback, FfiVoidCallback},
    cookie_accessor_ffi::FfiCookieAccessor,
    data_ffi::{custom_data_ffi::FfiCustomData, FfiData},
    datafile_ffi::FfiDataFile,
    raw_str_ffi::{BorrowedStr, OwnedStr},
    remote_visitor_data_filter_ffi::FfiRemoteVisitorDataFilter,
    result_ffi::{with_borrowed_error, write_result, write_result_value, FfiError},
    variation_ffi::{FfiOwnedVariation, FfiVariations},
    FreeRaw, FromRaw, StructPtr,
};

// MARK: - Handle

/// Releases a client handle obtained from `client_factory__create`. The core
/// client itself is dropped once the factory cache and every handle released it.
#[no_mangle]
pub extern "C" fn client__free(client: StructPtr) {
    if !client.is_null() {
        // SAFETY: handles are produced by `Arc::into_raw` in `client_factory__create`.
        drop(unsafe { Arc::<CoreKameleoonClient>::from_raw(client.cast()) });
    }
}

// MARK: - Initialization

/// Waits asynchronously until the client is ready; `timeout_millis == 0`
/// applies the configured default timeout.
#[no_mangle]
pub extern "C" fn client__initialize(client: StructPtr, timeout_millis: u64, cb: FfiVoidCallback) {
    let client = <&CoreKameleoonClient>::from_raw(client);
    client.initialize(
        (timeout_millis > 0).then(|| Duration::from_millis(timeout_millis)),
        Box::new(move |result| with_borrowed_error(result, |_, error| cb.call(error))),
    );
}

#[no_mangle]
pub extern "C" fn client__is_ready(client: StructPtr) -> bool {
    <&CoreKameleoonClient>::from_raw(client).is_ready()
}

// MARK: - Visitor

#[no_mangle]
pub extern "C" fn client__get_visitor_code(
    client: StructPtr,
    cookies: FfiCookieAccessor,
    default_visitor_code: BorrowedStr,
    out: *mut OwnedStr,
    error: *mut FfiError,
) -> bool {
    let client = <&CoreKameleoonClient>::from_raw(client);
    let mut cookies = cookies;
    let result = client.get_visitor_code(&mut cookies, default_visitor_code.into());
    write_result_value(result, out, error, OwnedStr::from)
}

#[no_mangle]
pub extern "C" fn client__set_legal_consent(
    client: StructPtr,
    visitor_code: BorrowedStr,
    consent: bool,
    cookies: *const FfiCookieAccessor,
    error: *mut FfiError,
) -> bool {
    let client = <&CoreKameleoonClient>::from_raw(client);
    // SAFETY: `cookies` is null or points to a valid accessor for the duration of the call.
    let mut cookies = unsafe { cookies.as_ref() }.copied();
    write_result(client.set_legal_consent(visitor_code.into(), consent, cookies.as_mut()), error)
}

// MARK: - Data

#[no_mangle]
pub extern "C" fn client__add_data(
    client: StructPtr,
    visitor_code: BorrowedStr,
    track: bool,
    data: FfiArray<FfiData>,
    error: *mut FfiError,
) -> bool {
    let client = <&CoreKameleoonClient>::from_raw(client);
    let data = data.as_slice().iter().map(FfiData::to_data).collect();
    write_result(client.add_data(visitor_code.into(), data, track), error)
}

#[no_mangle]
pub extern "C" fn client__flush(client: StructPtr, visitor_code: BorrowedStr, error: *mut FfiError) -> bool {
    let client = <&CoreKameleoonClient>::from_raw(client);
    write_result(client.flush(visitor_code.into()), error)
}

#[no_mangle]
pub extern "C" fn client__flush_instant(client: StructPtr, visitor_code: BorrowedStr, cb: FfiVoidCallback) {
    let client = <&CoreKameleoonClient>::from_raw(client);
    client.flush_instant(visitor_code.into(), move |result| with_borrowed_error(&result, |_, error| cb.call(error)));
}

#[no_mangle]
pub extern "C" fn client__track_conversion(
    client: StructPtr,
    visitor_code: BorrowedStr,
    goal_id: u32,
    revenue: f32,
    negative: bool,
    metadata: FfiArray<FfiCustomData>,
    error: *mut FfiError,
) -> bool {
    let client = <&CoreKameleoonClient>::from_raw(client);
    let metadata = metadata.as_slice().iter().map(Into::into).collect();
    write_result(client.track_conversion(visitor_code.into(), goal_id, revenue, negative, metadata), error)
}

// MARK: - Evaluation

#[no_mangle]
pub extern "C" fn client__is_feature_active(
    client: StructPtr,
    visitor_code: BorrowedStr,
    feature_key: BorrowedStr,
    track: bool,
    out: *mut bool,
    error: *mut FfiError,
) -> bool {
    let client = <&CoreKameleoonClient>::from_raw(client);
    let result = client.is_feature_active(visitor_code.into(), feature_key.into(), track);
    write_result_value(result, out, error, |active| active)
}

#[no_mangle]
pub extern "C" fn client__get_variation(
    client: StructPtr,
    visitor_code: BorrowedStr,
    feature_key: BorrowedStr,
    track: bool,
    out: *mut FfiOwnedVariation,
    error: *mut FfiError,
) -> bool {
    let client = <&CoreKameleoonClient>::from_raw(client);
    let result = client.get_variation(visitor_code.into(), feature_key.into(), track);
    write_result_value(result, out, error, FfiOwnedVariation::owned)
}

#[no_mangle]
pub extern "C" fn variation__free(variation: FfiOwnedVariation) {
    variation.free();
}

#[no_mangle]
pub extern "C" fn client__get_variations(
    client: StructPtr,
    visitor_code: BorrowedStr,
    only_active: bool,
    track: bool,
    out: *mut FfiVariations,
    error: *mut FfiError,
) -> bool {
    let client = <&CoreKameleoonClient>::from_raw(client);
    let result = client.get_variations(visitor_code.into(), only_active, track);
    write_result_value(result, out, error, FfiVariations::owned)
}

#[no_mangle]
pub extern "C" fn variations__free(variations: FfiVariations) {
    variations.free();
}

#[no_mangle]
pub extern "C" fn client__evaluate_audiences(
    client: StructPtr,
    visitor_code: BorrowedStr,
    error: *mut FfiError,
) -> bool {
    let client = <&CoreKameleoonClient>::from_raw(client);
    write_result(client.evaluate_audiences(visitor_code.into()), error)
}

#[no_mangle]
pub extern "C" fn client__set_forced_variation(
    client: StructPtr,
    visitor_code: BorrowedStr,
    experiment_id: u32,
    variation_key: BorrowedStr,
    force_targeting: bool,
    error: *mut FfiError,
) -> bool {
    let client = <&CoreKameleoonClient>::from_raw(client);
    let result = client.set_forced_variation(visitor_code.into(), experiment_id, variation_key.into(), force_targeting);
    write_result(result, error)
}

// MARK: - Hybrid integration

#[no_mangle]
pub extern "C" fn client__get_engine_tracking_code(
    client: StructPtr,
    visitor_code: BorrowedStr,
    out: *mut OwnedStr,
    error: *mut FfiError,
) -> bool {
    let client = <&CoreKameleoonClient>::from_raw(client);
    write_result_value(client.get_engine_tracking_code(visitor_code.into()), out, error, OwnedStr::from)
}

// MARK: - Remote data

#[no_mangle]
pub extern "C" fn client__get_remote_data(client: StructPtr, key: BorrowedStr, cb: FfiBytesCallback) {
    let client = <&CoreKameleoonClient>::from_raw(client);
    client.get_remote_data(key.into(), move |result| {
        with_borrowed_error(&result, |bytes, error| {
            cb.call(FfiArray::borrowed(bytes.map(Vec::as_slice).unwrap_or_default()), error)
        })
    });
}

/// `filter` may be `NULL` to use the default filter.
#[no_mangle]
pub extern "C" fn client__get_remote_visitor_data(
    client: StructPtr,
    visitor_code: BorrowedStr,
    filter: *const FfiRemoteVisitorDataFilter,
    cb: FfiVoidCallback,
) {
    let client = <&CoreKameleoonClient>::from_raw(client);
    // SAFETY: `filter` is null or points to a valid filter for the duration of the call.
    let filter = unsafe { filter.as_ref() }.map(|filter| (*filter).into());
    client.get_remote_visitor_data(visitor_code.into(), filter, move |result| {
        with_borrowed_error(&result, |_, error| cb.call(error))
    });
}

#[no_mangle]
pub extern "C" fn client__get_visitor_warehouse_audience(
    client: StructPtr,
    visitor_code: BorrowedStr,
    warehouse_key: BorrowedStr,
    custom_data_index: u32,
    cb: FfiVoidCallback,
) {
    let client = <&CoreKameleoonClient>::from_raw(client);
    client.get_visitor_warehouse_audience(
        visitor_code.into(),
        warehouse_key.into(),
        custom_data_index,
        move |result| with_borrowed_error(&result, |_, error| cb.call(error)),
    );
}

// MARK: - Data file

#[no_mangle]
pub extern "C" fn client__get_datafile(client: StructPtr, out: *mut FfiDataFile, error: *mut FfiError) -> bool {
    let client = <&CoreKameleoonClient>::from_raw(client);
    write_result_value(client.get_types_datafile(), out, error, FfiDataFile::owned)
}

#[no_mangle]
pub extern "C" fn datafile__free(datafile: FfiDataFile) {
    datafile.free();
}
