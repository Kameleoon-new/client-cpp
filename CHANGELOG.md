# Changelog

All notable changes to this project will be documented in this file.
## 0.8.7 - 2026-11-23
### Features
* Added the "greater than or equal to" (`>=`) and "less than or equal to" (`<=`) operators to the following targeting conditions:
  - App version
  - SDK type
  - Browser
  - Custom data (number)
  - Number of page views
  - Total number of visits
  - Number of visits today
  - Elapsed time since first visit
  - Elapsed time since last visit

  These operators will be available in the Kameleoon app with the platform release planned for December 2026.

### Bug fixes
* Fixed the Custom data condition with an operator unknown to the SDK targeting every visitor who does not have the custom data: the operator was treated as `Undefined`. Such a condition is now never met.
* Fixed the Custom data condition with the `Among values` operator never being met when the list contained a decimal number (for example, `[1.5, 10]`).
* Fixed the Browser condition with a version and an unknown or missing operator ignoring the version. Such a condition is now never met.
* Fixed the SDK type condition without a version never being met when its operator was unknown or missing. The operator is ignored without a version.
* Fixed the SDK type condition with an invalid version being met for any version. Such a condition is now never met.
* A condition with invalid parameters (for example, an operator unknown to the SDK) is not an error when the configuration is parsed anymore: an error is logged when such a condition is evaluated, and it is evaluated as not met.

## 0.8.6 - 2026-11-19
### Features
* Real-time configuration [streaming](https://docs.kameleoon.com/developer-docs/feature-experimentation/technical-reference/technical-considerations#streaming-premium-option) can now be enabled for server-side SDKs independently of client-side SDKs.
* The real-time (SSE) connection now honors the `proxy_host` setting of [`KameleoonClientConfig`][additional_config], like the other SDK requests.
* Added support for version strings containing pre-release and build metadata, in accordance with [Semantic Versioning](https://semver.org/#semantic-versioning-200). Pre-release and build metadata components are currently ignored, and the version is normalized to `major.minor.patch`.

### Bug fixes
* Fixed the SDK getting stuck without configuration updates when the real-time streaming connection kept being established without delivering an event (for example, behind a proxy which accepts the connection but forwards nothing). When the connection cannot be established or delivers nothing, the SDK now retries it once after 10 seconds and then falls back to polling, with the first poll after one minute; each successful poll switches back to streaming once it is available again.
* Fixed configuration updates published while the real-time streaming connection was down (reconnection, server-side close) being missed until the next update. The SDK now resynchronizes the configuration on every (re)connection.
* Fixed the SDK getting stuck without configuration updates on a stalled streaming connection (for example, one silently dropped by a NAT or load balancer). The SDK now treats 6 minutes of silence as a connection failure: it reconnects, and falls back to polling if the new connection stalls as well.
* A configuration fetch which fails while the SDK is in streaming mode is now retried after one minute, with streaming restored once a fetch succeeds. Previously the SDK kept the outdated configuration until the next streaming event.
* Fixed an older configuration being applied over a newer one when two configuration fetches (for example, a poll and a fetch triggered by a streaming event) completed out of order.
* Fixed the configuration being polled more and more often in polling mode: every configuration update started one more polling loop.

## 0.8.5 - 2026-10-09
### Features
* Initial release of the Kameleoon C++ SDK, a C++17 wrapper over the Kameleoon SDK Core.

[client_factory]: https://docs.kameleoon.com/developer-docs/sdks/web-sdks/cpp-sdk#create
[additional_config]: https://docs.kameleoon.com/developer-docs/sdks/web-sdks/cpp-sdk#additional-configuration
[initialize]: https://docs.kameleoon.com/developer-docs/sdks/web-sdks/cpp-sdk#initialize
[is_ready]: https://docs.kameleoon.com/developer-docs/sdks/web-sdks/cpp-sdk#is_ready
[get_visitor_code]: https://docs.kameleoon.com/developer-docs/sdks/web-sdks/cpp-sdk#get_visitor_code
[add_data]: https://docs.kameleoon.com/developer-docs/sdks/web-sdks/cpp-sdk#add_data
[flush]: https://docs.kameleoon.com/developer-docs/sdks/web-sdks/cpp-sdk#flush
[track_conversion]: https://docs.kameleoon.com/developer-docs/sdks/web-sdks/cpp-sdk#track_conversion
[is_feature_active]: https://docs.kameleoon.com/developer-docs/sdks/web-sdks/cpp-sdk#is_feature_active
[get_variation]: https://docs.kameleoon.com/developer-docs/sdks/web-sdks/cpp-sdk#get_variation
[get_variations]: https://docs.kameleoon.com/developer-docs/sdks/web-sdks/cpp-sdk#get_variations
[set_forced_variation]: https://docs.kameleoon.com/developer-docs/sdks/web-sdks/cpp-sdk#set_forced_variation
[evaluate_audiences]: https://docs.kameleoon.com/developer-docs/sdks/web-sdks/cpp-sdk#evaluate_audiences
[set_legal_consent]: https://docs.kameleoon.com/developer-docs/sdks/web-sdks/cpp-sdk#set_legal_consent
[get_engine_tracking_code]: https://docs.kameleoon.com/developer-docs/sdks/web-sdks/cpp-sdk#get_engine_tracking_code
[get_remote_data]: https://docs.kameleoon.com/developer-docs/sdks/web-sdks/cpp-sdk#get_remote_data
[get_remote_visitor_data]: https://docs.kameleoon.com/developer-docs/sdks/web-sdks/cpp-sdk#get_remote_visitor_data
[get_visitor_warehouse_audience]: https://docs.kameleoon.com/developer-docs/sdks/web-sdks/cpp-sdk#get_visitor_warehouse_audience
[get_datafile]: https://docs.kameleoon.com/developer-docs/sdks/web-sdks/cpp-sdk#get_datafile
[set_event_handler]: https://docs.kameleoon.com/developer-docs/sdks/web-sdks/cpp-sdk#set_event_handler
