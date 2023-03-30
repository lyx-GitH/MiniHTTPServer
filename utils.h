//
// Created by 刘宇轩 on 2022/11/9.
//

#ifndef MYHTTPSERVER_UTILS_H
#define MYHTTPSERVER_UTILS_H

#include <unistd.h>
#include <iomanip>
#include <fstream>
#include <map>
#include <thread>
#include <strstream>
#include <sstream>
#include "defs.h"


#define SMALL_BUF 1024

// struct to store all infos for a connection
struct Connection {
    int fd;
    std::string ip_addr;
    int ip_port;
};

const static char *day_of_weeks[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday",
                                     "Saturday"};                                    // 星期
const static char *name_of_months[] = {"Jan.", "Feb.", "Mar.", "Apr.", "May.", "Jun.", "Jul.", "Aug", "Sep", "Oct",
                                       "Nov.", "Dec"};// 月份简称
const static std::map<std::string, std::string> accounts = {{"Hello", "World"}, {"1", "1"}};

inline std::string zfill2(int num) {
    return num >= 10 ? std::to_string(num) : '0' + std::to_string(num);
}

inline std::string time_format(const tm &local_time) {
    return zfill2(local_time.tm_hour) + ':' + zfill2(local_time.tm_min) + ':' +
           zfill2(local_time.tm_sec)
           + ' ' + std::string(day_of_weeks[local_time.tm_wday]) + ' ' +
           std::string(name_of_months[local_time.tm_mon]) + ' ' + zfill2(local_time.tm_mday) + ' ' +
           std::to_string(1900 + local_time.tm_year);
}


inline std::string get_time_str() {
    auto timestamp = time(nullptr);
    tm local_time = *localtime(&timestamp);
    std::string out_buffer;

    // 输出时间戳，格式化输出时间
    out_buffer += time_format(local_time) + '\n';
    return out_buffer;
}

inline std::string get_hostname() {
    static char host_name[SMALL_BUF] = {0};
    gethostname(host_name, SMALL_BUF);
    return std::string{host_name} + '\n';
}

inline std::string
get_list(const std::map<int, std::thread::id> &thread_ids, const std::map<std::thread::id, Connection> &connections) {
    std::strstream ss;
    ss << "index" << std::setw(16) << "thread_id" << std::setw(16) << "ip address" << std::endl;
    for (auto &p: thread_ids) {
        if (p.second == std::this_thread::get_id())
            ss << "+>";
        ss << p.first << std::setw(16) << p.second << std::setw(16) << connections.at(p.second).ip_addr << ':'
           << connections.at(p.second).ip_port
           << std::endl;
    }
    return ss.str();
}

inline bool str2int(const std::string &str, int *num) {
    if (str.empty()) return false;
    short sign = 1;
    int res = 0;
    // 按位转换数字
    if (!isdigit(str[0])) {
        sign = str[0] == '+' ? 1 : str[0] == '-' ? -1 : 0;
        if (!sign) return false; // 含有非法字符直接退出
    } else {
        res = str[0] - '0';
    }

    for (int i = 1; i < str.length(); i++) {
        if (!isdigit(str[i]))
            return false;
        res = res * 10 + int(str[i] - '0');
    }

    *num = res * sign; // 组装最后的结果
    return true;
}

inline std::string get_file_contents(const std::string &file_path) {
    std::ifstream ifs(file_path);
    if (!ifs.good())
        return "";
    std::stringstream file_contents;
    file_contents << ifs.rdbuf();
    return file_contents.str();
}


inline std::string make_send_http(const std::string &contents,
                                  const unsigned int code = HTTP_OK, const std::string &status = "OK",
                                  const std::string &content_type = "text/html") {
    std::string full_content_type = "Content-Type:" + content_type + "\r\n";
    std::string header = "HTTP/1.1 " + std::to_string(code) + ' ' + status + "\r\n";
    std::string msg = header + full_content_type +
                      std::string{"Content-Length:"} + std::to_string(contents.length()) + std::string("\r\n") +
                      std::string("\r\n") + contents;
    return msg;
}

inline bool is_valid_account(const std::string &account, const std::string &pwd) {
    return accounts.count(account) && accounts.at(account) == pwd;
}

inline bool is_valid_path(const std::string &static_path, const std::string &file_path) {
    if (file_path == "/")
        return true;
    const std::string full_path = static_path + file_path;
    return access(full_path.c_str(), F_OK) == 0;
}

#endif //MYHTTPSERVER_UTILS_H
