#include "UdpServer.h"
#include <iostream>
#include <boost/asio.hpp>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

UdpServer::UdpServer(std::string address, unsigned short port)
    : io_service_(), socket_(io_service_), running_(false), receiving_(false) {
    try {
        boost::asio::ip::udp::endpoint endpoint(boost::asio::ip::address::from_string(address), port);
        socket_.open(boost::asio::ip::udp::v4());
        socket_.bind(endpoint);
    } catch (const boost::system::system_error& e) {
        //
    } catch (const std::exception& e) {
        //
    }
}

void UdpServer::processMessage(const std::string& message) {
    if (message.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (!message.empty()) {
        messages.push(message);
    }
}

void UdpServer::startReceive() {
    if (receiving_) return;

    receiving_ = true;
    socket_.async_receive_from(
        boost::asio::buffer(data_), remote_endpoint_,
        [this](boost::system::error_code ec, std::size_t bytes_received) {
            handleReceive(ec, bytes_received);
        });
}

void UdpServer::handleReceive(const boost::system::error_code& ec, std::size_t bytes_received) {
    if (!ec && bytes_received > 0) {
        std::string message(std::begin(data_), std::begin(data_) + bytes_received);
        processMessage(message);
    } else if (ec) {
        std::lock_guard<std::mutex> lock(mutex_);
        messages.push("Ошибка при получении данных: " + ec.message());
    }

    receiving_ = false;
    if (running_) {
        startReceive();  // Возобновляем прием
    }
}

std::string UdpServer::getFirstMessage() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!messages.empty()) {
        std::string message = messages.front();
        messages.pop();
        return message;
    }
    return u8"non";
}

void UdpServer::run() {
    running_ = true;
    startReceive();
    std::thread server_thread([this]() {
        try {
            io_service_.run();
        } catch (const std::exception& e) {
            //
        }
    });
    server_thread.detach();
}

void UdpServer::stop() {
    running_ = false;
    try {
        io_service_.stop();
        socket_.close();
    } catch (const boost::system::system_error& e) {
        //
    } catch (const std::exception& e) {
        //
    }
}

bool UdpServer::isRunning() const {
    return running_;
}
