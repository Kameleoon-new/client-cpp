#pragma once

#include "input_arena.hpp"

#include "kameleoon/client_config.hpp"
#include "kameleoon/data/data.hpp"
#include "ffi.hpp"
#include "kameleoon/types/datafile.hpp"
#include "kameleoon/types/remote_visitor_data_filter.hpp"
#include "kameleoon/types/variation.hpp"

#include <optional>
#include <string>
#include <unordered_map>

namespace kameleoon::detail
{

    ffi::FfiCustomData to_ffi(InputArena &arena, const CustomData &custom_data);
    ffi::FfiData to_ffi(InputArena &arena, const Data &data);
    ffi::FfiRemoteVisitorDataFilter to_ffi(const RemoteVisitorDataFilter &filter);
    ffi::FfiKameleoonClientConfig to_ffi(InputArena &arena, const KameleoonClientConfig &config);

    Variation copy_variation(ffi::FfiVariation value);
    std::unordered_map<std::string, Variation> copy_variation_map(ffi::FfiVariationMap values);
    DataFile copy_datafile(ffi::FfiDataFile value);

} // namespace kameleoon::detail
