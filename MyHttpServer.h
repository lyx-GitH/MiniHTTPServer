//
// Created by 刘宇轩 on 2022/11/9.
//

#ifndef MYHTTPSERVER_MYHTTPSERVER_H
#define MYHTTPSERVER_MYHTTPSERVER_H


#include <iostream>
#include <sstream>
#include <strstream>
#include <exception>
#include <thread>
#include <map>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cassert>
#include <mutex>
#include "defs.h"
#include "utils.h"
#include "BufferArray.h"
#include "parse_http.h"
#include "ThreadPool.h"


class HttpServer {
public:
    // open the HttpServer's socket
    HttpServer();

    ~HttpServer();

    void start();


    // close all connections
    void close();


private:
    using mut_guard = std::lock_guard<std::mutex>;

    inline bool isClose() {
        return quit_flag.load();
    }

    std::mutex mut;
    int socket_fd;
    sockaddr_in server_sockaddr{};
    sockaddr_in listen_sockaddr{};
    sockaddr_in connected_addr{};
    ThreadPool threadPool;

    std::map<int, std::thread::id> thread_ids;
    std::map<std::thread::id, Connection> all_connections;

    volatile std::atomic_bool quit_flag;

    void prepareStaticPath();


    void handleStdio();

    // handle clients' requests
    void serve(sockaddr_in &_connected_addr, int connect_fd);

    static void handleMsg(char recv_buffer[MAX_BUF], char send_buffer[MAX_BUF]);
};


#endif //MYHTTPSERVER_MYHTTPSERVER_H
