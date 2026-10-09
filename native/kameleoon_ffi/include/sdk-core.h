#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * Version of this C ABI. Bumped on every incompatible change to the exported
 * types or functions; consumers compare it with `ffi__abi_version()` at
 * runtime to refuse a mismatched shared library instead of corrupting memory.
 */
#define FFI_ABI_VERSION 1

/**
 * `FfiCustomData::id` of a custom data identified by `name`; the core resolves
 * the index from the data file.
 */
#define CUSTOM_DATA_UNDEFINED_ID UINT32_MAX

typedef enum FfiBrowserType {
  Chrome,
  InternetExplorer,
  Firefox,
  Safari,
  Opera,
  Other,
} FfiBrowserType;

typedef enum FfiDeviceType {
  Phone,
  Tablet,
  Desktop,
} FfiDeviceType;

typedef enum FfiOperatingSystemType {
  Windows,
  Mac,
  IOS,
  Linux,
  Android,
  WindowsPhone,
} FfiOperatingSystemType;

typedef enum FfiRequestType {
  DataFile,
  Tracking,
  RemoteVisitorData,
  RemoteData,
  AccessToken,
} FfiRequestType;

typedef enum FfiHttpRequestFailureReason {
  HttpStatus,
  Error,
  Cancelled,
  NoFailure,
} FfiHttpRequestFailureReason;

typedef enum FfiDataFileUpdateSource {
  Polling,
  Streaming,
} FfiDataFileUpdateSource;

typedef const char *StrPtr;

typedef struct BorrowedStr {
  StrPtr str;
  uint32_t size;
} BorrowedStr;

typedef struct FfiKameleoonClientConfig {
  struct BorrowedStr client_id;
  struct BorrowedStr client_secret;
  uint32_t refresh_interval_minutes;
  uint32_t session_duration_minutes;
  uint32_t default_timeout_millis;
  uint32_t tracking_interval_millis;
  struct BorrowedStr environment;
  struct BorrowedStr proxy_host;
  struct BorrowedStr top_level_domain;
  struct BorrowedStr network_domain;
} FfiKameleoonClientConfig;

typedef const void *StructPtr;

typedef uint32_t ResultCode;

typedef struct OwnedStr {
  StrPtr str;
  uint32_t size;
} OwnedStr;

/**
 * A failure returned through an `error` out-parameter. Release with `error__free`.
 */
typedef struct FfiError {
  ResultCode code;
  struct OwnedStr message;
} FfiError;

/**
 * A failure passed to a foreign callback; borrowed, never freed by the receiver.
 */
typedef struct FfiBorrowedError {
  ResultCode code;
  struct BorrowedStr message;
} FfiBorrowedError;

/**
 * Completion of an asynchronous operation without a payload.
 */
typedef struct FfiVoidCallback {
  StructPtr context;
  void (*func)(StructPtr context, const struct FfiBorrowedError *error);
} FfiVoidCallback;

typedef struct ForeignOwnedStr {
  StrPtr str;
  uint32_t size;
} ForeignOwnedStr;

typedef struct FfiCookieAccessor {
  StructPtr context;
  void (*set)(StructPtr context,
              struct BorrowedStr key,
              struct BorrowedStr value,
              uint32_t max_age,
              struct BorrowedStr top_level_domain);
  struct ForeignOwnedStr (*get)(StructPtr context, struct BorrowedStr key);
  void (*free_str)(struct ForeignOwnedStr ptr);
} FfiCookieAccessor;

typedef struct FfiBrowser {
  enum FfiBrowserType kind;
  float version;
} FfiBrowser;

typedef struct FfiArray_BorrowedStr {
  const struct BorrowedStr *ptr;
  uint32_t len;
} FfiArray_BorrowedStr;

typedef struct FfiCustomData {
  /**
   * Takes precedence over `name` unless it is `CUSTOM_DATA_UNDEFINED_ID`.
   */
  uint32_t id;
  /**
   * `NULL` when the custom data is identified by `id`.
   */
  struct BorrowedStr name;
  struct FfiArray_BorrowedStr values;
  bool overwrite;
} FfiCustomData;

typedef struct FfiArray_FfiCustomData {
  const struct FfiCustomData *ptr;
  uint32_t len;
} FfiArray_FfiCustomData;

typedef struct FfiConversion {
  uint32_t goal_id;
  float revenue;
  bool negative;
  struct FfiArray_FfiCustomData metadata;
} FfiConversion;

typedef struct FfiKeyValuePair_BorrowedStr__BorrowedStr {
  struct BorrowedStr key;
  struct BorrowedStr value;
} FfiKeyValuePair_BorrowedStr__BorrowedStr;

typedef struct FfiArray_FfiKeyValuePair_BorrowedStr__BorrowedStr {
  const struct FfiKeyValuePair_BorrowedStr__BorrowedStr *ptr;
  uint32_t len;
} FfiArray_FfiKeyValuePair_BorrowedStr__BorrowedStr;

typedef struct FfiCookie {
  struct FfiArray_FfiKeyValuePair_BorrowedStr__BorrowedStr cookies;
} FfiCookie;

typedef struct FfiDevice {
  enum FfiDeviceType kind;
} FfiDevice;

typedef struct FfiGeolocation {
  struct BorrowedStr country;
  struct BorrowedStr region;
  struct BorrowedStr city;
  struct BorrowedStr postal_code;
  float latitude;
  float longitude;
} FfiGeolocation;

typedef struct FfiOperatingSystem {
  enum FfiOperatingSystemType kind;
} FfiOperatingSystem;

typedef struct FfiArray_i32 {
  const int32_t *ptr;
  uint32_t len;
} FfiArray_i32;

typedef struct FfiPageView {
  struct BorrowedStr url;
  struct BorrowedStr title;
  struct FfiArray_i32 referrers;
} FfiPageView;

typedef struct FfiUniqueIdentifier {
  bool value;
} FfiUniqueIdentifier;

typedef struct FfiUserAgent {
  struct BorrowedStr value;
} FfiUserAgent;

typedef struct FfiApplicationVersion {
  struct BorrowedStr value;
} FfiApplicationVersion;

enum FfiData_Tag
#if defined(__cplusplus) || __STDC_VERSION__ >= 202311L
  : uint32_t
#endif // defined(__cplusplus) || __STDC_VERSION__ >= 202311L
 {
  Browser,
  Conversion,
  Cookie,
  CustomData,
  Device,
  Geolocation,
  OperatingSystem,
  PageView,
  UniqueIdentifier,
  UserAgent,
  ApplicationVersion,
};
#ifndef __cplusplus
#if __STDC_VERSION__ >= 202311L
typedef enum FfiData_Tag FfiData_Tag;
#else
typedef uint32_t FfiData_Tag;
#endif // __STDC_VERSION__ >= 202311L
#endif // __cplusplus

typedef struct FfiData {
  FfiData_Tag tag;
  union {
    struct {
      struct FfiBrowser browser;
    };
    struct {
      struct FfiConversion conversion;
    };
    struct {
      struct FfiCookie cookie;
    };
    struct {
      struct FfiCustomData custom_data;
    };
    struct {
      struct FfiDevice device;
    };
    struct {
      struct FfiGeolocation geolocation;
    };
    struct {
      struct FfiOperatingSystem operating_system;
    };
    struct {
      struct FfiPageView page_view;
    };
    struct {
      struct FfiUniqueIdentifier unique_identifier;
    };
    struct {
      struct FfiUserAgent user_agent;
    };
    struct {
      struct FfiApplicationVersion application_version;
    };
  };
} FfiData;

typedef struct FfiArray_FfiData {
  const struct FfiData *ptr;
  uint32_t len;
} FfiArray_FfiData;

enum FfiJsonValue_Tag
#if defined(__cplusplus) || __STDC_VERSION__ >= 202311L
  : uint32_t
#endif // defined(__cplusplus) || __STDC_VERSION__ >= 202311L
 {
  Boolean,
  Number,
  String,
  JSON,
  JS,
  CSS,
};
#ifndef __cplusplus
#if __STDC_VERSION__ >= 202311L
typedef enum FfiJsonValue_Tag FfiJsonValue_Tag;
#else
typedef uint32_t FfiJsonValue_Tag;
#endif // __STDC_VERSION__ >= 202311L
#endif // __cplusplus

typedef struct FfiJsonValue {
  FfiJsonValue_Tag tag;
  union {
    struct {
      bool boolean;
    };
    struct {
      double number;
    };
    struct {
      struct BorrowedStr string;
    };
    struct {
      struct BorrowedStr json;
    };
    struct {
      struct BorrowedStr js;
    };
    struct {
      struct BorrowedStr css;
    };
  };
} FfiJsonValue;

/**
 * View of a feature variable. Strings point into the `Arc<str>` data of the
 * core value retained by the enclosing payload's `owner`.
 */
typedef struct FfiVariable {
  struct BorrowedStr key;
  struct BorrowedStr kind;
  struct FfiJsonValue value;
} FfiVariable;

typedef struct FfiArray_FfiVariable {
  const struct FfiVariable *ptr;
  uint32_t len;
} FfiArray_FfiVariable;

/**
 * View of a variation; `id`/`experiment_id` are `u32::MAX` when absent.
 */
typedef struct FfiVariation {
  struct BorrowedStr key;
  struct BorrowedStr name;
  uint32_t id;
  uint32_t experiment_id;
  struct FfiArray_FfiVariable variables;
} FfiVariation;

/**
 * Result of `client__get_variation`.
 */
typedef struct FfiOwnedVariation {
  struct FfiVariation variation;
  /**
   * Boxed core `Variation` the view borrows from.
   */
  StructPtr owner;
} FfiOwnedVariation;

typedef struct FfiKeyValuePair_BorrowedStr__FfiVariation {
  struct BorrowedStr key;
  struct FfiVariation value;
} FfiKeyValuePair_BorrowedStr__FfiVariation;

typedef struct FfiArray_FfiKeyValuePair_BorrowedStr__FfiVariation {
  const struct FfiKeyValuePair_BorrowedStr__FfiVariation *ptr;
  uint32_t len;
} FfiArray_FfiKeyValuePair_BorrowedStr__FfiVariation;

/**
 * Views of a `HashMap<Arc<str>, Variation>`, keyed by feature key.
 */
typedef struct FfiArray_FfiKeyValuePair_BorrowedStr__FfiVariation FfiVariationMap;

/**
 * Result of `client__get_variations`.
 */
typedef struct FfiVariations {
  FfiVariationMap variations;
  /**
   * Boxed `HashMap<Arc<str>, Variation>` the views borrow from.
   */
  StructPtr owner;
} FfiVariations;

typedef struct FfiArray_u8 {
  const uint8_t *ptr;
  uint32_t len;
} FfiArray_u8;

/**
 * Completion of an asynchronous operation producing bytes (`data` is valid
 * only when `error` is `NULL`).
 */
typedef struct FfiBytesCallback {
  StructPtr context;
  void (*func)(StructPtr context, struct FfiArray_u8 data, const struct FfiBorrowedError *error);
} FfiBytesCallback;

typedef struct FfiRemoteVisitorDataFilter {
  uint32_t previous_visit_amount;
  bool current_visit;
  bool custom_data;
  bool page_views;
  bool geolocation;
  bool device;
  bool browser;
  bool operating_system;
  bool conversions;
  bool experiments;
  bool kcs;
  bool visitor_code;
  bool personalizations;
  bool cbs;
} FfiRemoteVisitorDataFilter;

typedef struct FfiRule {
  FfiVariationMap variations;
} FfiRule;

typedef struct FfiArray_FfiRule {
  const struct FfiRule *ptr;
  uint32_t len;
} FfiArray_FfiRule;

typedef struct FfiFeatureFlag {
  bool environment_enabled;
  struct BorrowedStr default_variation_key;
  FfiVariationMap variations;
  struct FfiArray_FfiRule rules;
} FfiFeatureFlag;

typedef struct FfiKeyValuePair_BorrowedStr__FfiFeatureFlag {
  struct BorrowedStr key;
  struct FfiFeatureFlag value;
} FfiKeyValuePair_BorrowedStr__FfiFeatureFlag;

typedef struct FfiArray_FfiKeyValuePair_BorrowedStr__FfiFeatureFlag {
  const struct FfiKeyValuePair_BorrowedStr__FfiFeatureFlag *ptr;
  uint32_t len;
} FfiArray_FfiKeyValuePair_BorrowedStr__FfiFeatureFlag;

typedef struct FfiDataFile {
  struct FfiArray_FfiKeyValuePair_BorrowedStr__FfiFeatureFlag feature_flags;
  uint64_t date_modified;
  /**
   * The retained `Arc<DataFile>` the views borrow from.
   */
  StructPtr owner;
} FfiDataFile;

typedef struct FfiHttpRequestEvent {
  enum FfiRequestType request_type;
  bool succeeded;
  /**
   * HTTP status of the response; `0` when the attempt produced none.
   */
  uint16_t http_status;
  /**
   * `NoFailure` when `succeeded` is `true`.
   */
  enum FfiHttpRequestFailureReason failure_reason;
  /**
   * `NULL` unless `failure_reason` is `Error`.
   */
  struct BorrowedStr failure_cause;
  uint64_t duration_millis;
} FfiHttpRequestEvent;

typedef struct FfiHttpRequestEventCallback {
  StructPtr context;
  void (*func)(StructPtr context, struct FfiHttpRequestEvent event);
  void (*drop)(StructPtr context);
} FfiHttpRequestEventCallback;

typedef struct FfiDataFileUpdateEvent {
  enum FfiDataFileUpdateSource source;
  uint64_t date_modified;
} FfiDataFileUpdateEvent;

typedef struct FfiDataFileUpdateEventCallback {
  StructPtr context;
  void (*func)(StructPtr context, struct FfiDataFileUpdateEvent event);
  void (*drop)(StructPtr context);
} FfiDataFileUpdateEventCallback;

/**
 * Log records emitted by the core.
 */
typedef struct FfiLogCallback {
  StructPtr context;
  void (*func)(StructPtr context, uint8_t log_level, struct BorrowedStr message);
} FfiLogCallback;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

uint32_t ffi__abi_version(void);

/**
 * Creates (or returns the cached) client for `site_code`.
 *
 * The configuration is taken from `config` (a non-`NULL` pointer) if given,
 * otherwise read from the file at `config_path` (a `BorrowedStr` with a
 * non-`NULL` pointer); one of them is required, as in the core factory. On
 * success `*out` receives a client handle to release with `client__free`; a
 * `NULL` `out` only creates and caches the client.
 */
bool client_factory__create(struct BorrowedStr sdk_name,
                            struct BorrowedStr sdk_version,
                            struct BorrowedStr site_code,
                            struct BorrowedStr config_path,
                            const struct FfiKameleoonClientConfig *config,
                            StructPtr *out,
                            struct FfiError *error);

/**
 * Removes the cached client for `site_code`; `environment` with a `NULL`
 * pointer selects the default environment.
 */
bool client_factory__forget(struct BorrowedStr site_code,
                            struct BorrowedStr environment,
                            struct FfiError *error);

/**
 * Releases a client handle obtained from `client_factory__create`. The core
 * client itself is dropped once the factory cache and every handle released it.
 */
void client__free(StructPtr client);

/**
 * Waits asynchronously until the client is ready; `timeout_millis == 0`
 * applies the configured default timeout.
 */
void client__initialize(StructPtr client, uint64_t timeout_millis, struct FfiVoidCallback cb);

bool client__is_ready(StructPtr client);

bool client__get_visitor_code(StructPtr client,
                              struct FfiCookieAccessor cookies,
                              struct BorrowedStr default_visitor_code,
                              struct OwnedStr *out,
                              struct FfiError *error);

bool client__set_legal_consent(StructPtr client,
                               struct BorrowedStr visitor_code,
                               bool consent,
                               const struct FfiCookieAccessor *cookies,
                               struct FfiError *error);

bool client__add_data(StructPtr client,
                      struct BorrowedStr visitor_code,
                      bool track,
                      struct FfiArray_FfiData data,
                      struct FfiError *error);

bool client__flush(StructPtr client, struct BorrowedStr visitor_code, struct FfiError *error);

void client__flush_instant(StructPtr client,
                           struct BorrowedStr visitor_code,
                           struct FfiVoidCallback cb);

bool client__track_conversion(StructPtr client,
                              struct BorrowedStr visitor_code,
                              uint32_t goal_id,
                              float revenue,
                              bool negative,
                              struct FfiArray_FfiCustomData metadata,
                              struct FfiError *error);

bool client__is_feature_active(StructPtr client,
                               struct BorrowedStr visitor_code,
                               struct BorrowedStr feature_key,
                               bool track,
                               bool *out,
                               struct FfiError *error);

bool client__get_variation(StructPtr client,
                           struct BorrowedStr visitor_code,
                           struct BorrowedStr feature_key,
                           bool track,
                           struct FfiOwnedVariation *out,
                           struct FfiError *error);

void variation__free(struct FfiOwnedVariation variation);

bool client__get_variations(StructPtr client,
                            struct BorrowedStr visitor_code,
                            bool only_active,
                            bool track,
                            struct FfiVariations *out,
                            struct FfiError *error);

void variations__free(struct FfiVariations variations);

bool client__evaluate_audiences(StructPtr client,
                                struct BorrowedStr visitor_code,
                                struct FfiError *error);

bool client__set_forced_variation(StructPtr client,
                                  struct BorrowedStr visitor_code,
                                  uint32_t experiment_id,
                                  struct BorrowedStr variation_key,
                                  bool force_targeting,
                                  struct FfiError *error);

bool client__get_engine_tracking_code(StructPtr client,
                                      struct BorrowedStr visitor_code,
                                      struct OwnedStr *out,
                                      struct FfiError *error);

void client__get_remote_data(StructPtr client, struct BorrowedStr key, struct FfiBytesCallback cb);

/**
 * `filter` may be `NULL` to use the default filter.
 */
void client__get_remote_visitor_data(StructPtr client,
                                     struct BorrowedStr visitor_code,
                                     const struct FfiRemoteVisitorDataFilter *filter,
                                     struct FfiVoidCallback cb);

void client__get_visitor_warehouse_audience(StructPtr client,
                                            struct BorrowedStr visitor_code,
                                            struct BorrowedStr warehouse_key,
                                            uint32_t custom_data_index,
                                            struct FfiVoidCallback cb);

bool client__get_datafile(StructPtr client, struct FfiDataFile *out, struct FfiError *error);

void datafile__free(struct FfiDataFile datafile);

void client__set_http_request_handler(StructPtr client, struct FfiHttpRequestEventCallback cb);

void client__set_datafile_update_handler(StructPtr client,
                                         struct FfiDataFileUpdateEventCallback cb);

void logger__set_log_level(uint8_t log_level);

void logger__set_logfunc(struct FfiLogCallback logfunc);

/**
 * Restores the default (stdout) logger. After this call the previously
 * registered external log function is never invoked again.
 */
void logger__reset_logfunc(void);

/**
 * Releases an `OwnedStr` handed out through an `out` parameter.
 */
void string__free(struct OwnedStr value);

void error__free(struct FfiError error);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus
