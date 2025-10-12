#include "headers.h"

#include <cctype>
#include <cstddef>
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
static bool compareStrings(const std::string_view s1, const std::string_view s2) {
    if (s1.length() != s2.length()) {
        return false;
    }

    for (size_t i = 0; i < s1.length(); ++i) {
        if (tolower(s1[i]) != tolower(s2[i]))
            return false;
    }

    return true;
}

void iterHeaders(std::string_view req, Callback &&callback) {
    size_t request_delimiter = req.find("\r\n");
    if (request_delimiter == std::string_view::npos) {
        return;
    }

    std::string_view headers = req.substr(request_delimiter + 2);

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
    std::string host_name;
    std::string port = "80";

    iterHeaders(req, [&](std::string_view name, std::string_view value) {
        if (!compareStrings(name, "Host")) {
            return;
        }

        size_t colon = value.find(':');
        if (colon != std::string_view::npos) {
            host_name = std::string(value.substr(0, colon));
            port = std::string(value.substr(colon + 1));
        } else {
            host_name = std::string(value);
        }
    });

    return {host_name, port};
}

std::optional<size_t> findContentLength(std::string_view rsp) {
    std::optional<size_t> content_len;

    iterHeaders(rsp, [&](std::string_view name, std::string_view value) {
        if (!compareStrings(name, "Content-Length")) {
            return;
        }

        size_t len = 0;
        auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), len);

        if (ec == std::errc() && ptr == value.data() + value.size()) {
            content_len = len;
        }
    });

    return content_len;
}
