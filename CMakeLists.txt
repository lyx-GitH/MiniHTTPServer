cmake_minimum_required(VERSION 3.22)
project(MyHttpServer)

set(CMAKE_CXX_STANDARD 14)

add_executable(MyHttpServer main.cpp MyHttpServer.cpp MyHttpServer.h BufferArray.cpp BufferArray.h defs.h utils.h parse_http.cpp parse_http.h ThreadSafeQueue.h ThreadPool.h)
