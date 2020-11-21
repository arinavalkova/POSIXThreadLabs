#include "getRequest.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <netdb.h>
#include <fcntl.h>

#define SPACE 32
#define NEW_LINE '\n'
#define BUF_SIZE 4096
#define MAX_ALLOWED_LINES_ON_SCREEN 25
#define INVITE_TO_PRESS "Press SPACE to scroll down"
#define BACK "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"
#define CYCLICAL_BUF_INITIALIZER {0, 1, 0, 0, 0, 0, (char *) malloc(BUF_SIZE)}

struct cyclical_buf {
    int full, empty, sock_end,
            lines_on_screen, up, down;
    char *buf;
};

struct cyclical_buf buffer_struct = CYCLICAL_BUF_INITIALIZER;
struct cyclical_buf *buffer = &buffer_struct;
int sock;

int
update_select(fd_set *read_fs, fd_set *write_fs) {
    FD_ZERO(read_fs);
    FD_ZERO(write_fs);
    if (!buffer->empty && buffer->lines_on_screen < MAX_ALLOWED_LINES_ON_SCREEN) {
        FD_SET(1, write_fs);
    }
    if (!buffer->full && !buffer->sock_end) {
        FD_SET(sock, read_fs);
    }
    if (buffer->lines_on_screen >= MAX_ALLOWED_LINES_ON_SCREEN) {
        FD_SET(0, read_fs);
    }
    return select(sock + 1, read_fs, write_fs, NULL, NULL);
}

int
listen_stdin(fd_set *read_fs) {
    if (buffer->lines_on_screen >= MAX_ALLOWED_LINES_ON_SCREEN && FD_ISSET(0, read_fs)) {
        char click;
        int i;
        click = getchar();
        if (click != SPACE) return 0;
        buffer->lines_on_screen = 0;
    }
    return 0;
}

int
listen_stdout(fd_set *write_fs) {
    int ret;
    if (!buffer->empty && buffer->lines_on_screen < MAX_ALLOWED_LINES_ON_SCREEN && FD_ISSET(1, write_fs)) {
        char *data = buffer->buf + buffer->down;
        char *end_of_data = (buffer->down < buffer->up) ? buffer->buf + buffer->up : buffer->buf + BUF_SIZE;
        while (data < end_of_data && *data != NEW_LINE) data++;
        if (data < end_of_data) {
            data++;
            buffer->lines_on_screen++;
        }
        if ((ret = write(1, buffer->buf + buffer->down,
                         data - buffer->buf - buffer->down)) == -1) {
            fprintf(stderr, "Error: write() page to screen failed with %s\n", strerror(errno));
            return -1;
        }
        buffer->down += ret;
        if (buffer->down == BUF_SIZE) buffer->down = 0;
        if (buffer->down == buffer->up) buffer->empty = 1;
        buffer->full = 0;

        if (buffer->lines_on_screen >= MAX_ALLOWED_LINES_ON_SCREEN) {
            if (write(1, INVITE_TO_PRESS, strlen(INVITE_TO_PRESS)) == -1) {
                fprintf(stderr, "Error: write() invite to press message failed with %s\n", strerror(errno));
                return -1;
            }
            if (write(1, BACK, strlen(BACK)) == -1) {
                fprintf(stderr, "Error: write() back failed with %s\n", strerror(errno));
                return -1;
            }
        }
    }
    return 0;
}

int
listen_socket(fd_set *read_fs) {
    int ret;
    if (!buffer->sock_end && !buffer->full && FD_ISSET(sock, read_fs)) {
        if (buffer->up >= buffer->down) {
            ret = read(sock, buffer->buf + buffer->up, BUF_SIZE - buffer->up);
        } else {
            ret = read(sock, buffer->buf + buffer->up, buffer->down - buffer->up);
        }
        if (ret == -1) {
            fprintf(stderr, "Error: read() from socket failed with %s\n", strerror(errno));
            return -1;
        } else if (ret == 0) {
            buffer->sock_end = 1;
        } else {
            buffer->up += ret;
            if (buffer->up >= buffer->down && buffer->up == BUF_SIZE) buffer->up = 0;
            if (buffer->up == buffer->down) buffer->full = 1;
            buffer->empty = 0;
        }
    }
    return 0;
}

int
load_content_from_server() {
    int ret;
    fd_set read_fs, write_fs;
    while (1) {
        system("stty raw -echo");
        if ((ret = update_select(&read_fs, &write_fs)) < 0) {
            fprintf(stderr, "Error: select() failed with %s\n", strerror(errno));
            return -1;
        } else if (ret == 0) continue;
        system("stty cooked -echo");
        if (listen_stdin(&read_fs) != 0) {
            fprintf(stderr, "Error: listening stdin is failed\n");
            return -1;
        }
        if (listen_stdout(&write_fs) != 0) {
            fprintf(stderr, "Error: listening stdout is failed\n");
            return -1;
        }
        if (listen_socket(&read_fs) != 0) {
            fprintf(stderr, "Error: listening socket is failed\n");
            return -1;
        }
        if (buffer->sock_end && buffer->empty) { break; }
    }
    return 0;
}

void
freeing_resources() {
    close(sock);
    free(buffer->buf);
    system("stty echo");
}

void
sigint_handler(int signum) { freeing_resources(); }

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
    if ((sock = GET_Request(argv[1])) == -1) {
        fprintf(stderr, "Error: GET_Request() failed\n");
        exit(EXIT_FAILURE);
    }
    if (load_content_from_server() == -1) {
        fprintf(stderr, "Error: load_content_from_server() failed\n");
        exit(EXIT_FAILURE);
    }
    freeing_resources();
    exit(EXIT_SUCCESS);
}
