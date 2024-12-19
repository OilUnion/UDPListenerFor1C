/*
#include "UdpServer.h"

#include <iostream>
#include <boost/asio.hpp>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <spdlog/spdlog.h>

// Конструктор класса
UdpServer::UdpServer(std::string address, unsigned short port)
    : io_service_(), socket_(io_service_), running_(false), receiving_(false) {
    try {
        boost::asio::ip::udp::endpoint endpoint(boost::asio::ip::address::from_string(address), port);
        socket_.open(boost::asio::ip::udp::v4());
        socket_.bind(endpoint);
    } catch (const boost::system::system_error& e) { 
        std::cerr << "Error binding socket: " << e.what() << std::endl;
    }
}

// Метод для обработки полученных сообщений
void UdpServer::processMessage(const std::string& message) {
    if (message.empty()) {
        spdlog::warn("Получено пустое сообщение.");
        return;
    }

    // Логируем сообщение
    spdlog::info("Получено сообщение: {}", message);
    
    // Добавляем сообщение в очередь
    std::lock_guard<std::mutex> lock(mutex_);
    if (message != ""){
      messages.push(message);
    }
}

// Метод для получения сообщений от клиента асинхронно
void UdpServer::startReceive() {
    if (receiving_) return;

    receiving_ = true;
    socket_.async_receive_from(
        boost::asio::buffer(data_), remote_endpoint_,
        [this](boost::system::error_code ec, std::size_t bytes_received) {
            handleReceive(ec, bytes_received);
        });
}

// Метод для обработки полученных данных
void UdpServer::handleReceive(const boost::system::error_code& ec, std::size_t bytes_received) {
    if (!ec && bytes_received > 0) {
        std::string message(std::begin(data_), std::begin(data_) + bytes_received);
        processMessage(message);  // Используем processMessage для обработки сообщения
    } else if (ec) {
        spdlog::info("Ошибка при получении данных: {}. Код ошибки: {}", ec.message(), ec.value());
        std::lock_guard<std::mutex> lock(mutex_);
        messages.push("Ошибка при получении данных: " + ec.message());
    }

    receiving_ = false;
    if (running_) {
        startReceive();
    }
}

// Метод для получения первого сообщения и удаления его из вектора
std::string UdpServer::getFirstMessage() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!messages.empty()) {
        std::string message = messages.front();
        messages.pop();
        return message;
    }
    return u8"non";
}

// Метод для запуска работы сервера (io_service)
void UdpServer::run() {
    running_ = true;
    startReceive();
    std::thread server_thread([&]() {
      io_service_.run();
    });
    server_thread.detach();
}


// Метод для завершения работы сервера
void UdpServer::stop() {
    running_ = false;
    io_service_.stop();
    socket_.close();
}

// Метод для проверки состояния работы сервера
bool UdpServer::isRunning() const{
    return running_;
}

*/















/*
#include "UdpServer.h"

#include <iostream>
#include <boost/asio.hpp>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <spdlog/spdlog.h>

// Конструктор класса
UdpServer::UdpServer(std::string address, unsigned short port)
    : io_service_(), socket_(io_service_), running_(false), receiving_(false) {
    try {
        boost::asio::ip::udp::endpoint endpoint(boost::asio::ip::address::from_string(address), port);
        socket_.open(boost::asio::ip::udp::v4());
        socket_.bind(endpoint);
    } catch (const boost::system::system_error& e) { 
        std::cerr << "Error binding socket: " << e.what() << std::endl;
    }
}

// Метод для обработки полученных сообщений
void UdpServer::processMessage(const std::string& message) {
    if (message.empty()) {
        spdlog::warn("Получено пустое сообщение.");
        return;
    }

    // Логируем сообщение
    spdlog::info("Получено сообщение: {}", message);
    
    // Добавляем сообщение в очередь
    std::lock_guard<std::mutex> lock(mutex_);
    if (message != "") {
        messages.push(message);
    }
}

// Метод для получения сообщений от клиента асинхронно
void UdpServer::startReceive() {
    if (receiving_) return;

    receiving_ = true;
    socket_.async_receive_from(
        boost::asio::buffer(data_), remote_endpoint_,
        [this](boost::system::error_code ec, std::size_t bytes_received) {
            handleReceive(ec, bytes_received);
        });
}

// Метод для обработки полученных данных
void UdpServer::handleReceive(const boost::system::error_code& ec, std::size_t bytes_received) {
    if (!ec && bytes_received > 0) {
        // Преобразуем полученные данные в строку
        std::string received_data(data_.begin(), data_.begin() + bytes_received);

        // Добавляем данные в буфер
        {
            std::lock_guard<std::mutex> lock(mutex_);
            data_buffer_ += received_data;
        }

        // Разбиваем данные по разделителю '|'
        size_t pos = 0;
        while ((pos = data_buffer_.find("~")) != std::string::npos) {
            // Извлекаем сообщение и удаляем его из буфера
            std::string message = data_buffer_.substr(0, pos);
            data_buffer_ = data_buffer_.substr(pos + 1);  // Убираем разделитель

            // Обрабатываем сообщение
            processMessage(message);
        }
    } else if (ec) {
        spdlog::info("Ошибка при получении данных: {}. Код ошибки: {}", ec.message(), ec.value());
        std::lock_guard<std::mutex> lock(mutex_);
        messages.push("Ошибка при получении данных: " + ec.message());
    }

    // После получения данных, снова вызываем startReceive
    receiving_ = false;
    if (running_) {
        startReceive();
    }
}

// Метод для получения первого сообщения и удаления его из очереди
std::string UdpServer::getFirstMessage() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!messages.empty()) {
        std::string message = messages.front();
        messages.pop();
        return message;
    }
    return u8"non";
}

// Метод для запуска работы сервера (io_service)
void UdpServer::run() {
    running_ = true;
    startReceive();
    std::thread server_thread([&]() {
        io_service_.run();
    });
    server_thread.detach();
}

// Метод для завершения работы сервера
void UdpServer::stop() {
    running_ = false;
    io_service_.stop();
    socket_.close();
}

// Метод для проверки состояния работы сервера
bool UdpServer::isRunning() const {
    return running_;
}
*/














/*
#include "UdpServer.h"
#include <iostream>
#include <boost/asio.hpp>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <spdlog/spdlog.h>
#include <list>

// Конструктор класса
UdpServer::UdpServer(std::string address, unsigned short port)
    : io_service_(), socket_(io_service_), running_(false), receiving_(false) {
    try {
        boost::asio::ip::udp::endpoint endpoint(boost::asio::ip::address::from_string(address), port);
        socket_.open(boost::asio::ip::udp::v4());
        socket_.bind(endpoint);
    } catch (const boost::system::system_error& e) {
        std::cerr << "Error binding socket: " << e.what() << std::endl;
    }
}

// Метод для обработки полученных сообщений
void UdpServer::processMessage(const std::vector<char>& message) {
    if (message.empty()) {
        spdlog::warn("Получено пустое сообщение.");
        return;
    }

    // Логируем сообщение
    spdlog::info("Получено сообщение, размер: {}", message.size());
    
    // Обрабатываем сообщение как массив байтов
    std::lock_guard<std::mutex> lock(mutex_);
   // messages.push(std::string(message.begin(), message.end()));
}

// Метод для получения сообщений от клиента асинхронно
void UdpServer::startReceive() {
    if (receiving_) return;

    receiving_ = true;
    socket_.async_receive_from(
        boost::asio::buffer(data_), remote_endpoint_,
        [this](boost::system::error_code ec, std::size_t bytes_received) {
            handleReceive(ec, bytes_received);
        });
}

// Метод для обработки полученных данных
void UdpServer::handleReceive(const boost::system::error_code& ec, std::size_t bytes_received) {
    if (!ec && bytes_received > 0) {
        // Создаем вектор для нового пакета и добавляем в него полученные данные
        std::vector<char> packet(std::begin(data_), std::begin(data_) + bytes_received);
  
        //throw "BBBBBBBBBLEt";
        // Логируем размер пакета
        spdlog::info("Принят пакет, размер: {}", packet.size());
         if (packet.empty()){
            throw "BBBBBBBBBLEt";
         }
        // Добавляем пакет в контейнер
        {
            std::lock_guard<std::mutex> lock(mutex_);
            data_packets_.push_back(packet);
        }

        // Обрабатываем новый пакет
        //processMessage(packet);
    } else if (ec) {
        spdlog::info("Ошибка при получении данных: {}. Код ошибки: {}", ec.message(), ec.value());
        std::lock_guard<std::mutex> lock(mutex_);
        messages.push({"Ошибка при получении данных: " + ec.message()});
    }

    receiving_ = false;
    if (running_) {
        startReceive();  // Начинаем новый прием
    }
}

// Метод для получения первого сообщения и удаления его из контейнера
std::string UdpServer::getFirstMessage() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!data_packets_.empty()) {
        std::vector<char> packet = data_packets_.front();
        data_packets_.pop_front();
        return std::string(packet.begin(), packet.end());
    }
    return u8"non";
}

// Метод для запуска работы сервера (io_service)
void UdpServer::run() {
    running_ = true;
    startReceive();
    std::thread server_thread([&]() {
        io_service_.run();
    });
    server_thread.detach();
}

// Метод для завершения работы сервера
void UdpServer::stop() {
    running_ = false;
    io_service_.stop();
    socket_.close();
}

// Метод для проверки состояния работы сервера
bool UdpServer::isRunning() const {
    return running_;
}
*/




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

    // Настройка логгера для записи в файл
    try {
        // Пробуем связать сокет
        boost::asio::ip::udp::endpoint endpoint(boost::asio::ip::address::from_string(address), port);
        socket_.open(boost::asio::ip::udp::v4());
        socket_.bind(endpoint);
    } catch (const boost::system::system_error& e) {
    } catch (const std::exception& e) {
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
    } catch (const std::exception& e) {
    }
}

bool UdpServer::isRunning() const {
    return running_;
}