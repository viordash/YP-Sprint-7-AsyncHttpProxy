#include "headers.h"

#include <ranges>
#include <string_view>

using namespace std::string_view_literals;

static void trim(std::string_view &s) {
    auto start = s.find_first_not_of(" \t");
    auto end = s.find_last_not_of(" \t");
    if (start != std::string_view::npos && end != std::string_view::npos) {
        s = s.substr(start, end - start + 1);
    }
}

void iterHeaders(std::string_view req, Callback &&callback) {
    size_t request_delimeter = req.find("\r\n");
    if (request_delimeter == std::string_view::npos) {
        return;
    }

    std::string_view headers = req.substr(request_delimeter + 2);

    for (auto line_range : headers | std::views::split("\r\n"sv)) {
        std::string_view line(line_range.begin(), line_range.end());

        if (line.empty()) {
            break;
        }

        size_t colon = line.find(':');
        if (colon != std::string_view::npos) {
            std::string_view name = line.substr(0, colon);
            std::string_view value = line.substr(colon + 1);
            
            trim(name);
            trim(value);

            callback(name, value);
        }
    }
}

std::pair<std::string, std::string> findHostPort(std::string_view req) {
    // code here
    return {};
}

std::optional<size_t> findContentLength(std::string_view rsp) {
    // code here
    return {};
}
