#include <sys/socket.h>
#include "getRequest.h"
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
#include <aio.h>

#define SPACE 32
#define READ_SOCK 0
#define READ_STDIN 2
#define NEW_LINE '\n'
#define BUF_SIZE 4096
#define WRITE_STDOUT 1
#define RW_HANDLERS_COUNT 3
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
struct aiocb *rw_in_progress[3]={ NULL, NULL, NULL};
struct aiocb socket_read_struct, stdout_write_struct, stdin_read_struct;
struct cyclical_buf *buffer = &buffer_struct;
int sock;

void
freeing_resources() {
    close(sock);
    free(buffer->buf);
    system("stty cooked echo");
}

void cancel_all_rw() {
    if (aio_cancel(0, stdin_read_struct) == -1) {
        fprintf(stderr, "Error: aio_cancel() stdin read struct failed with %s\n", strerror(errno));
    }
    if (aio_cancel(1, stdout_write_struct) == -1) {
        fprintf(stderr, "Error: aio_cancel() stdout write struct failed with %s\n", strerror(errno));
    }
    if(aio_cancel(sock, socket_read_struct) == -1) {
        fprintf(stderr, "Error: aio_cancel() socket read struct failed with %s\n", strerror(errno));
    }
}

void
sigint_handler(int signum) {
    cancel_all_rw();
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

int stdin_read() {
    if (buffer->lines_on_screen >= MAX_ALLOWED_LINES_ON_SCREEN && aio_error(&stdin_read_struct) != EINPROGRESS) {
        stdin_read_struct.aio_nbytes = 1;
        if (aio_read(&stdin_read_struct) == -1) {
            fprintf(stderr, "Error: aio_read() from stdin failed with %s\n", strerror(errno));
            return -1;
        }
        rw_in_progress[READ_STDIN] = &stdin_read_struct;
    }
    return 0;
}

int stdout_write() {
    int ret;
    if (!buffer->empty && buffer->lines_on_screen < MAX_ALLOWED_LINES_ON_SCREEN
                       && aio_error(&stdout_write_struct) != EINPROGRESS) {
        char *data = buffer->buf + buffer->down;
        char *end_of_data = (buffer->down < buffer->up) ? buffer->buf + buffer->up : buffer->buf + BUF_SIZE;
        while (data < end_of_data && *data != NEW_LINE) data++;
        if (data < end_of_data) {
            data++;
            buffer->lines_on_screen++;
        }
        stdout_write_struct.aio_buf = buffer->buf + buffer->down;
        stdout_write_struct.aio_nbytes = data - buffer->buf - buffer->down;
        if (aio_write(&stdout_write_struct) == -1) {
            fprintf(stderr, "Error: aio_write() to stdout failed with %s\n", strerror(errno));
            return -1;
        }
        rw_in_progress[WRITE_STDOUT] = &stdout_write_struct;
    }
    return 0;
}

int socket_read() {
    int ret;
    if (!buffer->sock_end && !buffer->full && aio_error(&socket_read_struct) != EINPROGRESS) {
        socket_read_struct.aio_buf = buffer->buf + buffer->up;
        if (buffer->up >= buffer->down) {
            socket_read_struct.aio_nbytes = BUF_SIZE - buffer->up;
        } else {
            socket_read_struct.aio_nbytes = buffer->down - buffer->up;
        }
        if (aio_read(&socket_read_struct) == -1) {
            fprintf(stderr, "Error: aio_read from socket failed with %s\n", strerror(errno));
            return -1;
        }
        rw_in_progress[READ_SOCK] = &socket_read_struct;
    }
    return 0;
}

int check_stdin_read() {
    int ret, click;
    if (stdin_read_struct.aio_nbytes != 0 && aio_error(&stdin_read_struct) != EINPROGRESS) {
        if ((ret = aio_error(&stdin_read_struct)) != 0) {
            fprintf(stderr, "Error: aio_error() when checking stdin read failed with %s\n", strerror(ret));
            return -1;
        }
        click = aio_return(&socket_read_struct);
        stdin_read_struct.aio_nbytes = 0;
        if (click != SPACE) return 0;

        buffer->lines_on_screen = 0;
    }
    return 0;
}

int check_stdout_write() {
    int ret;
    if (stdout_write_struct.aio_nbytes != 0 && aio_error(&stdout_write_struct) != EINPROGRESS) {
        if ((ret = aio_error(&stdout_write_struct)) != 0) {
            fprintf(stderr, "Error: aio_error() in checking stdout write failed with %s\n", strerror(ret));
            return -1;
        }
        ret = aio_return(&stdout_write_struct);
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
        stdout_write_struct.aio_nbytes = 0;
    }
    return 0;
}

int check_socket_read() {
    int ret;
    if (socket_read_struct.aio_nbytes != 0 && aio_error(&socket_read_struct) != EINPROGRESS) {
        if ((ret = aio_error(&socket_read_struct)) != 0) {
            fprintf(stderr, "Error: aio_error() in check socket read failed with %s\n", strerror(ret));
            return -1;
        }
        ret = aio_return(&socket_read_struct);
        if (ret == 0) {
            buffer->sock_end = 1;
        } else {
            buffer->up += ret;
            if (buffer->up >= buffer->down && buffer->up == BUF_SIZE) buffer->up = 0;
            if (buffer->up == buffer->down) buffer->full = 1;
            buffer->empty = 0;
        }
        socket_read_struct.aio_nbytes = 0;
    }
    return 0;
}

int
load_content_from_server() {
    memset(&socket_read_struct, 0, sizeof socket_read_struct);
    socket_read_struct.aio_fildes = sock;
    memset(&stdout_write_struct, 0, sizeof stdout_write_struct);
    stdout_write_struct.aio_fildes = 1;
    memset(&stdin_read_struct, 0, sizeof stdin_read_struct);
    stdin_read_struct.aio_fildes = 0;
    stdin_read_struct.aio_buf = click;
    system("stty raw -echo");
    while (1) {
        if (stdin_read() != 0) {
            fprintf(stderr, "Error: stdin_read() failed\n");
            return -1;
        }
        if (stdout_write() != 0) {
            fprintf(stderr, "Error: stdout_write() failed\n");
            return -1;
        }
        if (socket_read() != 0) {
            fprintf(stderr, "Error: socket_read() failed\n");
            return -1;
        }

        aio_suspend((const struct aiocb * const)rw_in_progress, RW_HANDLERS_COUNT, NULL);

        if (check_stdin_read() != 0) {
            fprintf(stderr, "Error: check_stdin_read() failed\n");
            return -1;
        }
        if (check_stdout_write() != 0) {
            fprintf(stderr, "Error: check_stdout_write() failed\n");
            return -1;
        }
        if (check_socket_read() != 0) {
            fprintf(stderr, "Error: check_socket_read() failed\n");
            return -1;
        }
        if (buffer->sock_end && buffer->empty) break;
    }
    return 0;
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
