//
// Created by 刘宇轩 on 2022/11/9.
//

#ifndef MYHTTPSERVER_BUFFERARRAY_H
#define MYHTTPSERVER_BUFFERARRAY_H


#include <cstdlib>

class BufferArray {
public:
    explicit BufferArray(const std::size_t size) {
        buf = new char[size]();
    }

    ~BufferArray() {
        delete[] buf;
    }

    char *getRawBuf() {
        return buf;
    }

private:
    char *buf;

};


#endif //MYHTTPSERVER_BUFFERARRAY_H
