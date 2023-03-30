//
// Created by 刘宇轩 on 2022/11/9.
//

#include "parse_http.h"

static std::string static_path;
static const std::string default_html = "test.html";

std::pair<std::string, std::string> split_post_item(const std::string &item) {
    auto pos = item.find('=');
    auto key = item.substr(0, pos);
    auto value = item.substr(pos + 1);
    return {key, value};
}

std::string create_html_output_from_img(const std::string &full_path, const std::string &format) {
    const char *file_name = full_path.c_str();

    FILE *file_stream = fopen(file_name, "rb");

    std::string file_str;

    size_t file_size;

    std::vector<char> buffer;

    if (file_stream != nullptr) {
        fseek(file_stream, 0, SEEK_END);

        long file_length = ftell(file_stream);

        rewind(file_stream);

        buffer.resize(file_length);


        file_size = fread(buffer.data(), 1, file_length, file_stream);

        std::stringstream out;

        for (int i = 0; i < file_size; i++) {
            out << buffer[i];
        }

        std::string copy = out.str();

        file_str = copy;

    } else {
        printf("file_stream is null! file name -> %s\n", file_name);
    }

    fclose(file_stream);

    std::string html;

    if (file_str.length() > 0) {
        std::string file_size_str = std::to_string(file_str.length());
        html = "HTTP/1.1 200 Okay\r\nContent-Type: image/" + format +
               "; Content-Transfer-Encoding: binary; Content-Length: " +
               file_size_str + ";charset=ISO-8859-4 \r\n\r\n" + file_str;
    } else {
        html = "HTTP/1.1 200 Okay\r\nContent-Type: text/html; charset=ISO-8859-4 \r\n\r\n" +
               std::string("FILE NOT FOUND!!");
    }

    return html;
}

HttpRequest parse_http_request(const char packet[MAX_BUF]) {
    HttpRequest httpRequest;
    std::istringstream packet_strm(packet);
    std::string method_str;
    std::string path;
    packet_strm >> method_str;
    packet_strm >> httpRequest.path;
    httpRequest.method = method_str == "GET" ? GET : method_str == "POST" ? POST : UNKNOWN;
    packet_strm >> httpRequest.protocol;
    std::string line;
    getline(packet_strm, line, '\n');
    if (line != "\r")
        throw std::string("not a valid http packet");

    while (getline(packet_strm, line, '\n') && line != "\r") {
        std::cout << line << std::endl;
        int key_end = 0, value_start = 0, value_end = 0;
        while (line[key_end] != ':') ++key_end;
        //--key_end;
        value_start = key_end + 1;
        while (line[value_start] == ' ') value_start++;
        std::string key = line.substr(0, key_end);
        std::string value = line.substr(value_start);
        value.pop_back();
        httpRequest.keys.insert(std::make_pair(std::move(key), std::move(value)));
    }

    getline(packet_strm, line, '\n');
    swap(httpRequest.payload, line);

    return httpRequest;
}

std::string get_http_payload(const char packet[MAX_BUF]) {
    std::string line;
    std::istringstream packet_strm(packet);
    while (getline(packet_strm, line) && !line.empty());
    getline(packet_strm, line);
    return line;
}

void do_get(const std::string &path, char buffer[MAX_BUF]) {
    if (!is_valid_path(static_path, path)) {
        strcpy(buffer, create_html_output_from_error("INVALID PATH: " + path).c_str());
        return;
    }

    const std::string full_path = static_path + path;
    // handle different GET formats
    if (path == "/") {
        const auto default_path = static_path + '/' + default_html;
        const std::string http_cont = create_html_output_from_html(default_path);
        strcpy(buffer, http_cont.c_str());
    } else if (path.find(".jpeg") != std::string::npos || path.find(".jpg") != std::string::npos) {
        // .jpeg or .jpg are the same
        auto http_str = create_html_output_from_img(full_path, "jpg");
        memcpy(buffer, http_str.c_str(), http_str.size());
    } else if (path.find(".html") != std::string::npos) {
        const auto http_cont = create_html_output_from_html(full_path);
        strcpy(buffer, http_cont.c_str());
    } else if (path.find(".txt") != std::string::npos) {
        const auto txt_cont = create_html_output_from_txt(full_path);
        strcpy(buffer, txt_cont.c_str());
    } else if (path.find(".GIF") != std::string::npos) {
        auto http_str = create_html_output_from_img(full_path, "gif");
        memcpy(buffer, http_str.c_str(), http_str.size());
    } else {
        const auto err_cont = create_html_output_from_error("format not support");
        strcpy(buffer, err_cont.c_str());
    }
}

void do_post(const std::string &path, char send_buffer[MAX_BUF], const HttpRequest &httpRequest) {

    const std::string login_fail_path = static_path + "/login_fail.html";
    const std::string login_success_path = static_path + "/login_success.html";

    if (path == "/dopost") {
        auto post_pairs = split_post_payload(httpRequest.payload);
        auto account = post_pairs.at("login");
        auto pwd = post_pairs.at("pass");
        if (is_valid_account(account, pwd)) {
            strcpy(send_buffer, create_html_output_from_html(login_success_path).c_str());
        } else {
            strcpy(send_buffer, create_html_output_from_html(login_fail_path).c_str());
        }
    }
}

std::string create_html_output_from_html(const std::string &full_path) {
    return make_send_http(get_file_contents(full_path));
}

std::string create_html_output_from_error(const std::string &error_msg) {
    return make_send_http(error_msg, HTTP_NOT_FOUND, HTTP_NOT_FOUND_MSG);
}

std::string create_html_output_from_txt(const std::string &full_path) {
    const auto str = get_file_contents(full_path);
    return make_send_http(str, HTTP_OK, HTTP_OK_MSG, "text/plain");
}


std::map<std::string, std::string> split_post_payload(const std::string &post_payload) {
    std::map<std::string, std::string> post_pairs;
    std::string item;
    for (auto c: post_payload) {
        if (c == '&') {
            // split
            post_pairs.emplace(split_post_item(item));
            item.clear();

        } else {
            item += c;
        }
    }
    post_pairs.emplace(split_post_item(item));
    return post_pairs;
}

void set_static_path(const std::string &path) {
    static_path = path;
}
