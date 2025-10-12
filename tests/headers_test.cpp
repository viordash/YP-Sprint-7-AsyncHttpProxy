#include "headers.h"
#include <gtest/gtest.h>

struct Header {
    std::string name;
    std::string value;
};

TEST(iterHeaders, Empty) {
    iterHeaders("", [&](std::string_view, std::string_view) { FAIL(); });
    SUCCEED();
}

TEST(iterHeaders, OnlyRequestLine) {
    iterHeaders("GET / HTTP/1.1\r\n\r\n", [&](std::string_view, std::string_view) { FAIL(); });
    SUCCEED();
}

TEST(iterHeaders, OnlyRequestLineNoCRLF) {
    iterHeaders("GET / HTTP/1.1", [&](std::string_view, std::string_view) { FAIL(); });
    SUCCEED();
}

TEST(iterHeaders, SkipRequestLine) {
    std::string_view req = "GET / HTTP/1.1\r\nHost: example.com:80\r\n\r\n";
    std::vector<Header> headers;

    iterHeaders(req, [&](std::string_view name, std::string_view value) {
        headers.emplace_back(std::string(name), std::string(value));
    });

    ASSERT_EQ(headers.size(), 1);
    ASSERT_EQ(headers[0].name, "Host");
    ASSERT_EQ(headers[0].value, "example.com:80");
}

TEST(iterHeaders, SingleHeader) {
    std::string_view req = "GET / HTTP/1.1\r\nContent-Type: text/html\r\n\r\n";
    std::vector<Header> headers;

    iterHeaders(req, [&](std::string_view name, std::string_view value) {
        headers.emplace_back(std::string(name), std::string(value));
    });

    ASSERT_EQ(headers.size(), 1);
    ASSERT_EQ(headers[0].name, "Content-Type");
    ASSERT_EQ(headers[0].value, "text/html");
}

TEST(iterHeaders, MultipleHeaders) {
    std::string_view req = "GET / HTTP/1.1\r\n"
                           "Host: example.com\r\n"
                           "User-Agent: TestClient\r\n"
                           "Accept: */*\r\n"
                           "\r\n";
    std::vector<Header> headers;

    iterHeaders(req, [&](std::string_view name, std::string_view value) {  //
        headers.emplace_back(std::string(name), std::string(value));
    });

    ASSERT_EQ(headers.size(), 3);
    ASSERT_EQ(headers[0].name, "Host");
    ASSERT_EQ(headers[0].value, "example.com");
    ASSERT_EQ(headers[1].name, "User-Agent");
    ASSERT_EQ(headers[1].value, "TestClient");
    ASSERT_EQ(headers[2].name, "Accept");
    ASSERT_EQ(headers[2].value, "*/*");
}

TEST(iterHeaders, MultipleSameHeaders) {
    std::string_view req = "GET / HTTP/1.1\r\n"
                           "Set-Cookie: a=1\r\n"
                           "Set-Cookie: b=2\r\n"
                           "\r\n";
    std::vector<Header> headers;
    iterHeaders(req, [&](std::string_view name, std::string_view value) {
        headers.emplace_back(std::string(name), std::string(value));
    });
    ASSERT_EQ(headers.size(), 2);
    ASSERT_EQ(headers[0].name, "Set-Cookie");
    ASSERT_EQ(headers[0].value, "a=1");
    ASSERT_EQ(headers[1].name, "Set-Cookie");
    ASSERT_EQ(headers[1].value, "b=2");
}

TEST(iterHeaders, HeaderWithValueContainingColon) {
    std::string_view req = "GET / HTTP/1.1\r\n"
                           "Content-Type: application/json; charset=utf-8\r\n"
                           "\r\n";
    std::vector<Header> headers;

    iterHeaders(req, [&](std::string_view name, std::string_view value) {
        headers.emplace_back(std::string(name), std::string(value));
    });

    ASSERT_EQ(headers.size(), 1);
    ASSERT_EQ(headers[0].name, "Content-Type");
    ASSERT_EQ(headers[0].value, "application/json; charset=utf-8");
}

TEST(iterHeaders, HeaderWithSpacesInName) {
    std::string_view req = "GET / HTTP/1.1\r\n"
                           " \t X-Custom-Header \t : some value\r\n"
                           "\r\n";
    std::vector<Header> headers;
    iterHeaders(req, [&](std::string_view name, std::string_view value) {
        headers.emplace_back(std::string(name), std::string(value));
    });
    ASSERT_EQ(headers.size(), 1);
    ASSERT_EQ(headers[0].name, "X-Custom-Header");
    ASSERT_EQ(headers[0].value, "some value");
}

TEST(iterHeaders, HeaderWithSpacesInValue) {
    std::string_view req = "GET / HTTP/1.1\r\n"
                           "X-Custom-Header:  \t  some value  \t \r\n"
                           "\r\n";
    std::vector<Header> headers;
    iterHeaders(req, [&](std::string_view name, std::string_view value) {
        headers.emplace_back(std::string(name), std::string(value));
    });
    ASSERT_EQ(headers.size(), 1);
    ASSERT_EQ(headers[0].name, "X-Custom-Header");
    ASSERT_EQ(headers[0].value, "some value");
}

TEST(findHostPort, Simple) {
    std::string_view req = "GET / HTTP/1.1\r\n"
                           "Host: example.com:443\r\n"
                           "User-Agent: TestClient\r\n"
                           "\r\n";
    auto [host, port] = findHostPort(req);
    ASSERT_EQ(host, "example.com");
    ASSERT_EQ(port, "443");
}

TEST(findHostPort, NoHost) {
    std::string_view req = "GET / HTTP/1.1\r\n"
                           "User-Agent: TestClient\r\n"
                           "\r\n";
    auto [host, port] = findHostPort(req);
    ASSERT_TRUE(host.empty());
    ASSERT_EQ(port, "80");
}

TEST(findHostPort, HostCaseInsensitive) {
    std::string_view req_lower = "GET / HTTP/1.1\r\nhost: lower.com\r\n\r\n";
    auto [host_lower, port_lower] = findHostPort(req_lower);
    ASSERT_EQ(host_lower, "lower.com");
    ASSERT_EQ(port_lower, "80");

    std::string_view req_mixed = "GET / HTTP/1.1\r\nHosT: mixed.com:9000\r\n\r\n";
    auto [host_mixed, port_mixed] = findHostPort(req_mixed);
    ASSERT_EQ(host_mixed, "mixed.com");
    ASSERT_EQ(port_mixed, "9000");
}

TEST(findHostPort, OnlyRequestLine) {
    std::string_view req = "GET / HTTP/1.1\r\n\r\n";
    auto [host, port] = findHostPort(req);
    ASSERT_TRUE(host.empty());
    ASSERT_EQ(port, "80");
}

TEST(findHostPort, EmptyRequest) {
    std::string_view req = "";
    auto [host, port] = findHostPort(req);
    ASSERT_TRUE(host.empty());
    ASSERT_EQ(port, "80");
}

TEST(findContentLength, Simple) {
    std::string_view resp = "HTTP/1.1 200 OK\r\n"
                            "Content-Length: 12345\r\n"
                            "\r\n"
                            "<body>...</body>";
    std::optional<size_t> length = findContentLength(resp);
    ASSERT_TRUE(length.has_value());
    ASSERT_EQ(length.value(), 12345);
}

TEST(findContentLength, NoContentLength) {
    std::string_view resp = "HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/html\r\n"
                            "\r\n"
                            "<body>...</body>";
    std::optional<size_t> length = findContentLength(resp);
    ASSERT_FALSE(length.has_value());
}

TEST(findContentLength, ContentLengthCaseInsensitive) {
    std::string_view resp = "HTTP/1.1 200 OK\r\n"
                            "content-length: 12345\r\n"
                            "\r\n";
    std::optional<size_t> length = findContentLength(resp);
    ASSERT_TRUE(length.has_value());
    ASSERT_EQ(length.value(), 12345);

    std::string_view resp_mixed = "HTTP/1.1 200 OK\r\n"
                                  "ContEnT-LenGtH: 6789\r\n"
                                  "\r\n";
    length = findContentLength(resp_mixed);
    ASSERT_TRUE(length.has_value());
    ASSERT_EQ(length.value(), 6789);
}

TEST(findContentLength, InvalidContentLengthValue) {
    std::string_view resp_invalid = "HTTP/1.1 200 OK\r\n"
                                    "Content-Length: abc\r\n"
                                    "\r\n";
    std::optional<size_t> length = findContentLength(resp_invalid);
    ASSERT_FALSE(length.has_value());

    std::string_view rsp_part_num = "HTTP/1.1 200 OK\r\n"
                                    "Content-Length: 123abc\r\n"
                                    "\r\n";
    length = findContentLength(rsp_part_num);
    ASSERT_FALSE(length.has_value());
}

TEST(findContentLength, MaxContentLengthLimit) {
    std::string_view resp = "HTTP/1.1 200 OK\r\n"
                            "Content-Length: 1073741824\r\n"
                            "\r\n"
                            "<body>...</body>";
    std::optional<size_t> length = findContentLength(resp);
    ASSERT_TRUE(length.has_value());
    ASSERT_EQ(length.value(), 1073741824);

    std::string_view resp_over = "HTTP/1.1 200 OK\r\n"
                            "Content-Length: 1073741825\r\n"
                            "\r\n"
                            "<body>...</body>";
    length = findContentLength(resp_over);
    ASSERT_FALSE(length.has_value());
}

TEST(findContentLength, OnlyStatusLine) {
    std::string_view resp = "HTTP/1.1 200 OK\r\n\r\n";
    std::optional<size_t> length = findContentLength(resp);
    ASSERT_FALSE(length.has_value());
}

TEST(findContentLength, EmptyResponse) {
    std::string_view resp = "";
    std::optional<size_t> length = findContentLength(resp);
    ASSERT_FALSE(length.has_value());
}
