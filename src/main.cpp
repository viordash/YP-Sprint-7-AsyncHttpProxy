#include "headers.h"

#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <iostream>
#include <ostream>
#include <print>
#include <string_view>

using boost::asio::async_read_until;
using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::io_service;
using boost::asio::use_awaitable;
using boost::asio::ip::tcp;
using boost::system::error_code;

constexpr std::string_view delimiter = "\r\n\r\n";
constexpr size_t chunk_size = 4096;

awaitable<void> session(tcp::socket client_socket, io_service &io_service) {
    try {
        boost::asio::streambuf client_buffer;
        co_await async_read_until(client_socket, client_buffer, delimiter, use_awaitable);

        auto client_data = client_buffer.data();
        std::string client_req(boost::asio::buffer_cast<const char *>(client_data),
                               boost::asio::buffer_size(client_data));

        auto [host, port] = findHostPort(client_req);
        if (host.empty()) {
            std::println(stderr, "'Host' header is empty");
            co_return;
        }

        tcp::resolver resolver(io_service);
        auto endpoints = co_await resolver.async_resolve(host, port, use_awaitable);

        tcp::socket server_socket(io_service);
        co_await boost::asio::async_connect(server_socket, endpoints, use_awaitable);

        co_await async_write(server_socket, boost::asio::buffer(client_req), use_awaitable);

        boost::asio::streambuf server_buffer;
        co_await async_read_until(server_socket, server_buffer, delimiter, use_awaitable);

        auto server_data = server_buffer.data();
        size_t response_size = boost::asio::buffer_size(server_data);
        std::string server_rsp(boost::asio::buffer_cast<const char *>(server_data), response_size);

        size_t headers_end = server_rsp.find(delimiter);
        if (headers_end == std::string::npos) {
            std::println(stderr, "No header delimiter found");
            co_return;
        }

        auto content_length = findContentLength(server_rsp);
        if (!content_length.has_value()) {
            std::println(stderr, "'Content Length' header is empty");
            co_return;
        }

        co_await async_write(client_socket, boost::asio::buffer(server_rsp), use_awaitable);

        size_t transferred = response_size - (headers_end + delimiter.length());
        std::array<char, chunk_size> temp_buffer;
        while (transferred < content_length.value()) {
            size_t to_read = std::min(content_length.value() - transferred, chunk_size);

            size_t read_count = co_await async_read(server_socket, boost::asio::buffer(temp_buffer.data(), to_read),
                                                    boost::asio::transfer_at_least(to_read), use_awaitable);
            if (read_count == 0) {
                break;
            }
            co_await async_write(client_socket, boost::asio::buffer(temp_buffer.data(), read_count), use_awaitable);
            transferred += read_count;
        }

    } catch (const std::exception &e) {
        std::println(stderr, "Session error: {}", e.what());
    }
}

class Server {
public:
    Server(io_service &io_service, short port)
        : io_service_(io_service), acceptor_(io_service, tcp::endpoint(tcp::v4(), port)) {
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept([this](error_code ec, tcp::socket socket) {
            if (!ec) {
                co_spawn(io_service_, session(std::move(socket), io_service_), boost::asio::detached);
            } else {
                std::cerr << "Accept error: " << ec.message() << std::endl;
            }

            do_accept();
        });
    }

    io_service &io_service_;
    tcp::acceptor acceptor_;
};

int main(int argc, char *argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Usage: proxy_server";
            std::cerr << " <listen_port>\n";
            return 1;
        }
        io_service io_service(1);
        Server server(io_service, std::atoi(argv[1]));
        io_service.run();

    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}
