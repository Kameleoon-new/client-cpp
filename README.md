# Kameleoon C++ SDK

## Getting Started

Our SDK gives you the possibility of running experiments and activating feature flags on your C++ platform. Integrating our SDK into your applications is easy, and its footprint (in terms of memory and network usage) is low.

You can refer to the [SDK reference](https://docs.kameleoon.com/developer-docs/sdks/web-sdks/cpp-sdk#reference) to check out all possible features of the SDK. Also make sure you check out our [Getting started tutorial](https://docs.kameleoon.com/developer-docs/sdks/web-sdks/cpp-sdk#getting-started) which we have prepared to walk you through the installation and implementation. Release notes live in [CHANGELOG.md](CHANGELOG.md).

## Installation

The SDK is a C++17 library over the Kameleoon SDK Core, which is written in Rust and bundled as source under `native/kameleoon_ffi`. CMake builds both: add the repository to your project with `FetchContent` (or `add_subdirectory` of a checkout) and link `Kameleoon::CppClient`.

```cmake
include(FetchContent)
FetchContent_Declare(kameleoon
    GIT_REPOSITORY https://github.com/Kameleoon-new/client-cpp.git
    GIT_TAG 0.8.5
)
FetchContent_MakeAvailable(kameleoon)

target_link_libraries(your_target PRIVATE Kameleoon::CppClient)
```

Requirements: CMake 3.22+, a C++17 compiler, and a Rust toolchain (`cargo` on the `PATH`, see [rustup.rs](https://rustup.rs); Rust 1.91 or newer). The first build compiles the Core and its dependencies, which takes a few minutes; later builds are incremental. Windows additionally needs [NASM](https://www.nasm.us) for the TLS library of the Core.

The Core is linked statically, so nothing has to be shipped next to your binary. To use a prebuilt Core instead of compiling it, pass `-DKAMELEOON_FFI_LIBRARY=/path/to/libkameleoon_ffi.so` (`.dylib`/`.dll`) and `-DKAMELEOON_FFI_HEADER=/path/to/sdk-core.h` generated for that library.
