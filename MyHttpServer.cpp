//
// Created by 刘宇轩 on 2022/11/9.
//

#include "MyHttpServer.h"

//std::map<int, std::thread::id> HttpServer::thread_ids;          // map for id <-> thread
//std::map<std::thread::id, Connection> HttpServer::all_connections; // map for thread <-> connection


HttpServer::HttpServer() : threadPool(MAX_CONNECTIONS), quit_flag(false) {
    prepareStaticPath();
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == INVALID_FD)
        throw std::runtime_error("socket connection failed");
    int on = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)); // set port-reuse
    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port = htons(SOCKET_PORT);

    server_sockaddr.sin_addr.s_addr = INADDR_ANY;

    memset(&server_sockaddr.sin_zero, 0, 8);
}

HttpServer::~HttpServer() {
    threadPool.stop();
}


void HttpServer::start() {
    signal(SIGPIPE, SIG_IGN); // in case the SIGPIPE signal kills the server
    // bind and listen to certain port
    if (bind(socket_fd, (const sockaddr *) (&server_sockaddr), sizeof(struct sockaddr)) == INVALID_FD)
        throw std::runtime_error("bind failed");
    if (listen(socket_fd, MAX_CONNECTIONS) == INVALID_FD)
        throw std::runtime_error("listen failed");

    socklen_t socket_length = sizeof(listen_sockaddr);

    getsockname(socket_fd, (struct sockaddr *) &listen_sockaddr, &socket_length);//获取监听的地址和端口
    printf("HttpServer Start!\nlisten address = %s:%d\n", inet_ntoa(listen_sockaddr.sin_addr),
           ntohs(listen_sockaddr.sin_port));
    threadPool.run();

    // open  a monitor thread to handle quif and other info
    std::thread monitor(&HttpServer::handleStdio, this);
    monitor.detach();
    // main loop for socket acception
    while (!isClose()) {
        int connect_fd = accept(socket_fd, (sockaddr *) (&connected_addr), &socket_length);

        if (connect_fd == INVALID_FD) {
            std::cerr << "acceptation failed" << std::endl;
            continue;
        }

        threadPool.addTask(this,&HttpServer::serve,   std::ref(connected_addr), connect_fd);

    }

}

void HttpServer::close() {
    mut_guard mutGuard(mut);
    for (const auto &p: all_connections)
        shutdown(p.second.fd, SHUT_WR);

    quit_flag.store(true);
    std::cout << "Bye!" << std::endl;
}

void HttpServer::handleMsg(char *recv_buffer, char *send_buffer) {
    memset(send_buffer, 0, MAX_BUF);
    printf("%s\n", recv_buffer);
    auto http_request = parse_http_request(recv_buffer);
    if (http_request.method == GET) {
        do_get(http_request.path, send_buffer);
    } else if (http_request.method == POST) {
        do_post(http_request.path, send_buffer, http_request);
    }

}

void HttpServer::serve(sockaddr_in &_connected_addr, int connect_fd) {
    const auto tid = std::this_thread::get_id();
    int thread_idx;

    {
        mut_guard mutGuard(mut);
        thread_idx = all_connections.empty() ? 1 : thread_ids.rbegin()->first + 1;
        all_connections.insert(std::make_pair(tid, Connection{connect_fd, inet_ntoa(_connected_addr.sin_addr),
                                                                      ntohs(_connected_addr.sin_port)}));
        thread_ids.insert(std::make_pair(thread_idx, tid));
    }


    std::strstream ss;
    ss << tid;
    const std::string tid_str = ss.str();

    sockaddr_in peer_addr;
    socklen_t len = sizeof(peer_addr);
    char peer_ip_addr[INET_ADDRSTRLEN];//保存点分十进制的地址


    BufferArray recv_buffer{MAX_BUF};
    BufferArray send_buffer{MAX_BUF};

    getsockname(connect_fd, (struct sockaddr *) &_connected_addr, &len);//获取connfd表示的连接上的本地地址
    printf("[%s] connected server address = %s:%d, fd = %d\n", tid_str.c_str(),
           inet_ntoa(_connected_addr.sin_addr),
           ntohs(_connected_addr.sin_port), connect_fd);
    getpeername(connect_fd, (struct sockaddr *) &peer_addr, &len); //获取connfd表示的连接上的对端地址
    printf("[%s] connected peer address = %s:%d\n", tid_str.c_str(),
           inet_ntop(AF_INET, &peer_addr.sin_addr, peer_ip_addr, sizeof(peer_ip_addr)), ntohs(peer_addr.sin_port));

    while (!isClose()) {
        if (recv(connect_fd, recv_buffer.getRawBuf(), MAX_BUF, 0) < 0) {
            std::cerr << '[' << tid_str << ']' << " unable to reciece any bytes, thread terminated\n" << std::endl;
            break;
        }

//        printf("[%s] data received: %s\n", tid_str.c_str(), recv_buffer.getRawBuf());
        try {
            handleMsg(recv_buffer.getRawBuf(), send_buffer.getRawBuf());
        } catch (const std::string &msg) {
            std::cout << '[' << tid_str << ']' << " info: " << msg << std::endl;
            break;
        } catch (const std::exception &e) {
            std::cout << '[' << tid_str << ']' << "error: " << e.what() << std::endl;
            break;
        }


        if (send(connect_fd, send_buffer.getRawBuf(), MAX_BUF, 0) < 0) {
            fprintf(stderr, "[%s] unable to send any bytes, thread terminated\n", tid_str.c_str());
            break;
        }
    }

    std::cout << '[' << tid_str << "] ""close connect" << std::endl;
    {
        mut_guard mutGuard(mut);
        thread_ids.erase(thread_idx);
        all_connections.erase(tid);
    }


    ::close(connect_fd);
}

void HttpServer::handleStdio() {
    std::cout << R"(monitor is running! enter 'quit' to terminate, 'list' to get all connections)" << std::endl;
    std::string cmd;
    while (std::cin >> cmd) {
        if (cmd == "quit") {
            quit_flag.store(false);
            close();
        } else if (cmd == "list") {
            std::cout << get_list(thread_ids, all_connections) << std::endl;
        } else {
            std::cout << "error: no such command " << cmd << std::endl;
        }
    }
}

void HttpServer::prepareStaticPath() {
    const std::string local_path = __FILE__;
    int i = local_path.size()-1;
    while(local_path[i] != '/')
        i--;
    const auto path = local_path.substr(0, i) + '/' + "static";
    set_static_path(path);
}

