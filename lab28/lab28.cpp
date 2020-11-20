#include "EdUrlParser.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <netdb.h>
#include <fcntl.h>

#define EMPTY ""
#define SPACE 32
#define HTTP "http"
#define NEW_LINE '\n'
#define BUF_SIZE 4096
#define DEFAULT_PORT "80"
#define GET_REQUEST_LEN 30
#define MAX_ALLOWED_LINES_ON_SCREEN 25
#define INVITE_TO_PRESS "Press SPACE to scroll down"
#define BACK "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"
#define CYCLICAL_BUF_INITIALIZER { 0, 1, 0, 0, 0, 0, PTHREAD_MUTEX_INITIALIZER, \
                                   PTHREAD_COND_INITIALIZER, (char *) malloc(BUF_SIZE) }

struct cyclical_buf {
    int full, empty, sock_end, lines_on_screen, up, down;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    char *buf;
};

struct cyclical_buf buffer_struct = CYCLICAL_BUF_INITIALIZER;
pthread_t socket_data_reader_pth, user_interaction_pth;
char *host = NULL, *path = NULL, *port = NULL;
struct cyclical_buf *buffer = &buffer_struct;
int sock;

int
url_parse_err_proc(char *url_line) {
    EdUrlParser *url = EdUrlParser::parseUrl(url_line);
    if (strcmp(url->scheme.c_str(), HTTP)) {
        fprintf(stderr, "Error: protocol is not supported, only http\n");
        return -1;
    }
    port = (char *) malloc(strlen(url->port.c_str()));
    memcpy(port, url->port.c_str(), strlen(url->port.c_str()));
    if (strcmp(port, EMPTY) == 0) {
        fprintf(stderr, "Note: established default port 80\n");
        port = (char *) malloc(strlen(DEFAULT_PORT));
        memcpy(port, DEFAULT_PORT, strlen(DEFAULT_PORT));
    }
    host = (char *) malloc(strlen(url->hostName.c_str()));
    memcpy(host, url->hostName.c_str(), strlen(url->hostName.c_str()));
    if (strcmp(host, EMPTY) == 0) {
        fprintf(stderr, "Error: can't parse host name\n");
        return -1;
    }
    path = (char *) malloc(strlen(url->path.c_str()));
    memcpy(path, url->path.c_str(), strlen(url->path.c_str()));
    if (strcmp(path, EMPTY) == 0) {
        fprintf(stderr, "Error: can't parse url path\n");
    }
    delete url;
    return 0;
}

int
connect_to_server() {
    int ret;
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
    if ((sock = socket(ip_struct->ai_family, ip_struct->ai_socktype,
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

void
threads_cancel() {
    pthread_cancel(user_interaction_pth);
    pthread_cancel(socket_data_reader_pth);
}

void *socket_data_reader(void *parameters) {
    int ret;
    pthread_mutex_lock(&buffer->mutex);
    while (!buffer->sock_end) {
        while (buffer->full) pthread_cond_wait(&buffer->cond, &buffer->mutex);
        if (buffer->up >= buffer->down) {
            pthread_mutex_unlock(&buffer->mutex);
            ret = read(sock, buffer->buf + buffer->up, BUF_SIZE - buffer->up);
        } else {
            pthread_mutex_unlock(&buffer->mutex);
            ret = read(sock, buffer->buf + buffer->up, buffer->down - buffer->up);
        }
        pthread_mutex_lock(&buffer->mutex);
        if (ret == -1) {
            fprintf(stderr, "Error: read() from socket failed\n");
            pthread_mutex_unlock(&buffer->mutex);
            threads_cancel();
        } else if (ret == 0) {
            buffer->sock_end = 1;
        } else {
            buffer->up += ret;
            if (buffer->up == buffer->down) buffer->full = 1;
            if (buffer->up >= buffer->down && buffer->up == BUF_SIZE) buffer->up = 0;
            buffer->empty = 0;
        }
        pthread_cond_signal(&buffer->cond);
    }
    pthread_mutex_unlock(&buffer->mutex);
}

int
listen_stdin() {
    if (buffer->lines_on_screen >= MAX_ALLOWED_LINES_ON_SCREEN) {
        pthread_mutex_unlock(&buffer->mutex);
        char click = getchar();
        pthread_mutex_lock(&buffer->mutex);
        if (click != SPACE) return 1;
        buffer->lines_on_screen = 0;
    }
    return 0;
}

void *user_interaction(void *parameters) {
    int ret;
    pthread_mutex_lock(&buffer->mutex);
    while (!buffer->empty && !buffer->sock_end) {
        while (buffer->empty && !buffer->sock_end) pthread_cond_wait(&buffer->cond, &buffer->mutex);
        if (listen_stdin() == 1) continue;

        char *data = buffer->buf + buffer->down;
        char *end_of_data = (buffer->down < buffer->up) ? buffer->buf + buffer->up : buffer->buf + BUF_SIZE;
        while (data < end_of_data && *data != NEW_LINE) data++;
        if (data < end_of_data) {
            data++;
            buffer->lines_on_screen++;
        }
        pthread_mutex_unlock(&buffer->mutex);
        if ((ret = write(1, buffer->buf + buffer->down,
                         data - buffer->buf - buffer->down)) == -1) {
            fprintf(stderr, "Error: write() page to screen failed\n");
            threads_cancel();
        }
        pthread_mutex_lock(&buffer->mutex);
        buffer->down += ret;
        if (buffer->down == BUF_SIZE) buffer->down = 0;
        if (buffer->down == buffer->up) buffer->empty = 1;
        buffer->full = 0;
        pthread_cond_signal(&buffer->cond);

        //stdin handler

        if (buffer->lines_on_screen >= MAX_ALLOWED_LINES_ON_SCREEN) {
            pthread_mutex_unlock(&buffer->mutex);
            if (write(1, INVITE_TO_PRESS, strlen(INVITE_TO_PRESS)) == -1) {
                fprintf(stderr, "Error: write() invite to press message failed\n");
                threads_cancel();
            }
            if (write(1, BACK, strlen(BACK)) == -1) {
                fprintf(stderr, "Error: write() back failed\n");
                threads_cancel();
            }
            pthread_mutex_lock(&buffer->mutex);
        }
    }
    pthread_mutex_unlock(&buffer->mutex);
}

int
load_content_from_server() {
    int ret;
    if ((ret = pthread_create(&socket_data_reader_pth, NULL, socket_data_reader, NULL)) != 0) {
        fprintf(stderr, "Error: pthread_create() socket data reader failed with %s\n", strerror(ret));
        return -1;
    }
    if ((ret = pthread_create(&user_interaction_pth, NULL, user_interaction, NULL)) != 0) {
        fprintf(stderr, "Error: pthread_create() user interaction failed with %s\n", strerror(ret));
        return -1;
    }
    if ((ret = pthread_join(socket_data_reader_pth, NULL)) != 0) {
        fprintf(stderr, "Error: pthread_join() socket data reader failed with %s\n", strerror(ret));
        return -1;
    }
    if ((ret = pthread_join(user_interaction_pth, NULL)) != 0) {
        fprintf(stderr, "Error: pthread_join() user interaction failed with %s\n", strerror(ret));
        return -1;
    }
    return 0;
}

int
start_http_client() {
    char *get_request = (char *) malloc(strlen(path) + GET_REQUEST_LEN);
    if ((sock = connect_to_server()) == -1) {
        fprintf(stderr, "Error: can't connect to server\n");
        return -1;
    }
    sprintf(get_request, "GET %s HTTP/1.0\r\n\r\n", path);
    if (write(sock, get_request, strlen(get_request)) == -1) {
        fprintf(stderr, "Error: write() GET request to socket failed with %s\n", strerror(errno));
        return -1;
    }
    free(get_request);
    if (load_content_from_server() == -1) {
        fprintf(stderr, "Error: load_content_from_server() failed\n");
        return -1;
    }
    return 0;
}

void
freeing_resources() {
    int ret;
    free(host);
    free(path);
    free(port);
    close(sock);
    if ((ret = pthread_mutex_destroy(&buffer->mutex)) != 0) {
        fprintf(stderr, "Error: pthread_mutex_destroy() failed with %s\n", strerror(ret));
    }
    if ((ret = pthread_cond_destroy(&buffer->cond)) != 0) {
        fprintf(stderr, "Error: pthread_cond_destroy() failed with %s\n", strerror(ret));
    }
    free(buffer->buf);
    system("stty echo");
}

void
sigint_handler(int signum) {
    threads_cancel();
    freeing_resources();
}

void
init_sigint_handler() {
    struct sigaction new_action;
    new_action.sa_handler = sigint_handler;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = 0;
    sigaction(SIGINT, &new_action, NULL);
}

int
main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s url\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    init_sigint_handler();
    if (url_parse_err_proc(argv[1]) != 0) {
        exit(EXIT_FAILURE);
    }
    if (start_http_client() != 0) {
        exit(EXIT_FAILURE);
    }
    freeing_resources();
    exit(EXIT_SUCCESS);
}
