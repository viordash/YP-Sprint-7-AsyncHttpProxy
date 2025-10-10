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
    std::string_view req = "GET / HTTP/1.1\r\nHost: example.com\r\n\r\n";
    std::vector<Header> headers;

    iterHeaders(req, [&](std::string_view name, std::string_view value) {
        headers.emplace_back(std::string(name), std::string(value));
    });

    ASSERT_EQ(headers.size(), 1);
    ASSERT_EQ(headers[0].name, "Host");
    ASSERT_EQ(headers[0].value, "example.com");
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
    // code here
}

TEST(findHostPort, NoHost) {
    // code here
}

TEST(findContentLength, Simple) {
    // code here
}

TEST(findContentLength, NoContentLength) {
    // code here
}
