#include "kameleoon/events.hpp"

#include <utility>

namespace kameleoon
{
    EventHandler::EventHandler(Handler handler) : handler_(std::move(handler)) {}

    EventHandler EventHandler::http_request(HttpRequestHandler handler)
    {
        return EventHandler(Handler{std::in_place_type<HttpRequestHandler>, std::move(handler)});
    }

    EventHandler EventHandler::datafile_update(DataFileUpdateHandler handler)
    {
        return EventHandler(Handler{std::in_place_type<DataFileUpdateHandler>, std::move(handler)});
    }
} // namespace kameleoon
