#ifndef UDPSERVER_H
#define UDPSERVER_H
#pragma once

#include <boost/asio.hpp>
#include <vector>
#include <string>
#include <mutex>
#include <atomic>
#include <queue>
#include <unordered_set>
#include <list>

using boost::asio::ip::udp;

class UdpServer {
public:
    // Конструктор класса
    UdpServer(std::string adress, unsigned short port);

    // Метод для получения сообщений от клиента асинхронно
    void startReceive();

    // Метод для получения первого сообщения и удаления его из вектора
    std::string getFirstMessage();

    // Метод для запуска работы сервера (io_service)
    void run();

    // Метод для завершения работы сервера
    void stop();

    // Метод для проверки состояния работы сервера
    bool isRunning() const;

    // Метод для получения сообщений от клиента асинхронно
    void processMessage(const std::string& message);
    //void processMessage(const std::vector<char>& message);

    // Метод для обработки полученных данных
    void handleReceive(const boost::system::error_code& ec, std::size_t bytes_received);
    
private:
    boost::asio::io_service io_service_;  // Сервис ввода-вывода Boost
    udp::socket socket_;                  // UDP сокет
    udp::endpoint remote_endpoint_;       // Удаленный эндпоинт
    // 67000 начальный размер буфера, взят с запасом.
    char data_[65536]; // Буфер для хранения полученных данных
    std::queue<std::string> messages;     // Хранилище сообщений
    std::mutex mutex_;                    // Мьютекс для защиты доступа к сообщениям
    std::atomic<bool> running_;           // Флаг для контроля работы сервера
    std::atomic<bool> receiving_;
    std::string data_buffer_;
    std::list<std::vector<char>>data_packets_;
};

#endif // UDPSERVER_H