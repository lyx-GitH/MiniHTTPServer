//
// Created by 刘宇轩 on 2022/11/9.
//

#ifndef MYHTTPSERVER_PARSE_HTTP_H
#define MYHTTPSERVER_PARSE_HTTP_H

#include "defs.h"
#include "utils.h"
#include "BufferArray.h"

#include <iostream>
#include <string>
#include <map>
#include <vector>


#define UNKNOWN -1
#define POST 0
#define GET 1

struct HttpRequest {
    int method;
    std::string path;
    std::string protocol;
    std::string payload;
    std::map<std::string, std::string> keys;
};

void set_static_path(const std::string& path);

HttpRequest parse_http_request(const char packet[MAX_BUF]);

std::map<std::string, std::string> split_post_payload(const std::string &post_payload);


std::string create_html_output_from_error(const std::string &error_msg);

std::string create_html_output_from_html(const std::string &full_path);

std::string create_html_output_from_img(const std::string &full_path, const std::string& format);

std::string create_html_output_from_txt(const std::string &full_path);

void do_get(const std::string &path, char send_buffer[MAX_BUF]);

void do_post(const std::string &path, char send_buffer[MAX_BUF], const HttpRequest &httpRequest);

#endif //MYHTTPSERVER_PARSE_HTTP_H
