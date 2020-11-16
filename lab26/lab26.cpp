#include <stdio.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <iostream>
#include "EdUrlParser.h"

#define HTTP "http"
#define EMPTY ""
#define DEFAULT_PORT "80"
#define BUF_SIZE 4096

char buf[BUF_SIZE];
int buf_full = 0, socket_end = 0, curr_pos_in_buf = 0, simbols_in_buf,
        buf_empty = 1, lines_on_screen = 0, max_allowed_lines_on_screen = 25;

int
url_parse_err_proc(char *url_line, char **port, char **host, char **path) {
    EdUrlParser* url = EdUrlParser::parseUrl(url_line);
    if (strcmp(url->scheme.c_str(), HTTP)) {
        fprintf(stderr, "Error: protocol is not supported, only http\n");
        return -1;
    }
    *port = (char*) malloc(strlen(url->port.c_str()));
    memcpy(*port, url->port.c_str(), strlen(url->port.c_str()));
    if (strcmp(*port, EMPTY) == 0) {
        fprintf(stderr, "Note: established default port 80\n");
        *port = (char*) malloc(strlen(DEFAULT_PORT));
        memcpy(*port, DEFAULT_PORT, strlen(DEFAULT_PORT));
    }
    *host = (char*) malloc(strlen(url->hostName.c_str()));
    memcpy(*host, url->hostName.c_str(), strlen(url->hostName.c_str()));
    if (strcmp(*host, EMPTY) == 0) {
        fprintf(stderr, "Error: can't parse host name\n");
        return -1;
    }
    *path = (char*) malloc(strlen(url->path.c_str()));
    memcpy(*path, url->path.c_str(), strlen(url->path.c_str()));
    if (strcmp(*path, EMPTY) == 0) {
        fprintf(stderr, "Error: can't parse url path\n");
    }
    delete url;
    return 0;
}

int
connect_to_server(char *host, char *port) {
    int sock, ret;
    struct sockaddr_in server_addr;
    struct addrinfo *ip_struct;
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;
    if ((ret = getaddrinfo(host, port, &hints, &ip_struct)) != 0) {
        fprintf(stderr, "getaddrinfo() failed with %s\n", gai_strerror(ret));
        return -1;
    }
    if((sock = socket(ip_struct->ai_family, ip_struct->ai_socktype,
                      ip_struct->ai_protocol)) == -1) {
        fprintf(stderr, "Error: socket() failed with %s\n", strerror(errno));
        return -1;
    }
    if (connect(sock, ip_struct->ai_addr, ip_struct->ai_addrlen) != 0) {
        fprintf(stderr, "Error: connect() failed with %s\n", strerror(errno));
        return -1;
    }
    return sock;
}

int
update_select(int sock, fd_set *read_fs, fd_set *write_fs) {
    FD_ZERO(read_fs);
    FD_ZERO(write_fs);
    if (!buf_empty && lines_on_screen < max_allowed_lines_on_screen) {
        FD_SET(1, write_fs);
    }
    if (!buf_full && !socket_end) {
        FD_SET(sock, read_fs);
    }
    if (lines_on_screen >= max_allowed_lines_on_screen) {
        FD_SET(0, read_fs);
    }
    return select(sock + 1, read_fs, write_fs, NULL, NULL);
}

int
listen_stdin(fd_set *read_fs) {
    if (lines_on_screen >= max_allowed_lines_on_screen && FD_ISSET(0, read_fs)) {
//        char* click;
//        read(0, click, 1);//handle read
//        lines_on_screen = 0;
    }
    return 0;
}

int
listen_stdout(fd_set *write_fs) {
    int ret;
    if (!buf_empty && lines_on_screen < max_allowed_lines_on_screen && FD_ISSET(1, write_fs)) {
//        char *new_line_checker = buf + curr_pos_in_buf,
//             *end_buf =  buf + sizeof(buf);
//        int line_size = 0;
//        for (; new_line_checker != end_buf && *new_line_checker != '\n'; new_line_checker++) {
//            line_size++;
//        }
//        if (new_line_checker < end_buf) {
//            new_line_checker++;
//            lines_on_screen++;
//        }
//        write(1, buf + curr_pos_in_buf, new_line_checker - (buf + curr_pos_in_buf));
//        curr_pos_in_buf +=
    }
    return 0;
}

int
listen_socket(int sock, fd_set *read_fs) {
    if (!socket_end && buf_empty && FD_ISSET(sock, read_fs)) {
        printf("*\n");
//        simbols_in_buf = read(sock, buf, sizeof(buf));//read handler
//        if (simbols_in_buf == 0) {
//            socket_end = 1;
//        }
//        buf_empty = 0;
    }
    return 0;
}

int
load_content_from_server(int sock) {
    int ret;
    fd_set read_fs, write_fs;
    while (1) {
        if ((ret = update_select(sock, &read_fs, &write_fs)) < 0) {
            fprintf(stderr, "Error: select() failed with %s\n", strerror(errno));
            return -1;
        } else if (ret == 0) continue;
        char buf[4096 * 10];
          if (listen_stdin(&read_fs) != 0) {
              fprintf(stderr, "Error: listening stdin is failed\n");
              return -1;
          }
        if (listen_stdout(&write_fs) != 0) {
            fprintf(stderr, "Error: listening stdout is failed\n");
            return -1;
        }
        if (listen_socket(sock, &read_fs) != 0) {
            fprintf(stderr, "Error: listening socket is failed\n");
            return -1;
        }

        if (!buf_full && !socket_end && FD_ISSET(sock, &read_fs)) {
            read(sock, buf, 4096 * 10);
            write(1, buf, 4096 * 10);
            buf_full = 1;
        }
        //if ((socket_end || buf_full) && buf_empty) break;
    }
}

int
start_http_client(char *host, char *port, char *path) {
    char get_request[BUF_SIZE];
    int sock;
    if ((sock = connect_to_server(host, port)) == -1) {
        fprintf(stderr, "Error: can't connect to server\n");
        return -1;
    }
    sprintf(get_request, "GET %s HTTP/1.0\r\n\r\n", path);
    write(sock, get_request, strlen(get_request));
    if (load_content_from_server(sock) == -1) {
        fprintf(stderr, "Error: load_content_from_server() failed\n");
        return -1;
    }
    return 0;
}

int
main(int argc, char **argv) {
    char *host = NULL, *path = NULL, *port = NULL;
    if (argc < 2) {
        fprintf(stderr, "Usage: %s url\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    if (url_parse_err_proc(argv[1], &port, &host, &path) != 0) {
        exit(EXIT_FAILURE);
    }
    if (start_http_client(host, port, path) != 0) {
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}//close socket
