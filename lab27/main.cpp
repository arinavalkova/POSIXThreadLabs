#include "../getRequest.h"
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
#include <termios.h>
#include <fcntl.h>
#include <aio.h>
#include "../common/queuebuffer.h"

#define READ_SOCK 0
#define READ_STDIN 2
#define WRITE_STDOUT 1
#define RW_HANDLERS_COUNT 3

#define SPACE 32
#define NEW_LINE '\n'
#define BUF_SIZE 4096
#define MAX_ALLOWED_LINES_ON_SCREEN 25
#define INVITE_TO_PRESS "Press SPACE to scroll down\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"
#define INVITE_LEN 52


struct queuebuffer queuebuffer = QUEUE_BUFFER_INITIALIZER;
int lines_on_screen = 0;

struct aiocb *rw_in_progress[3] = {NULL, NULL, NULL};
struct aiocb socket_read_struct, stdout_write_struct, stdin_read_struct;
struct termios tty, savtty;
int sock;
char click;

char socket_buffer[BUF_SIZE];
char *stdout_buffer;
int stdout_len, running = 1;

void freeing_resources() {
    close(sock);
    queuebuffer_clear(&queuebuffer);
}

void cancel_rw(struct aiocb *rw_struct) {
    int ret;
    if (aio_cancel(0, &stdin_read_struct) == AIO_NOTCANCELED) {
        while (aio_suspend((const aiocb *const*) rw_struct, 1, NULL) == -1) {
            fprintf(stderr, "Error: aio_suspend() end work with %s\n", strerror(errno));
            if (errno == ENOSYS) {
                break;
            }
        }
    }
}

void cancel_all_rw() {
    cancel_rw(&stdin_read_struct);
    cancel_rw(&stdout_write_struct);
    cancel_rw(&socket_read_struct);
}

void sigint_handler(int signum) {
    running = 0;
    cancel_all_rw();
}

void init_sigint_handler() {
    struct sigaction new_action;
    new_action.sa_handler = sigint_handler;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = 0;
    sigaction(SIGINT, &new_action, NULL);
}

int stdin_read() {
    if (lines_on_screen >= MAX_ALLOWED_LINES_ON_SCREEN && aio_error(&stdin_read_struct) == 0) {
        tcsetattr(0, TCSAFLUSH, &tty);
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
    int ret, stdout_i;
    if (!queuebuffer_is_empty(&queuebuffer) && lines_on_screen < MAX_ALLOWED_LINES_ON_SCREEN
        && aio_error(&stdout_write_struct) == 0) {
        stdout_i = 0;
        queuebuffer_get_bytes(&queuebuffer, &stdout_buffer, &stdout_len);
        while (stdout_i + 1 < stdout_len && stdout_buffer[stdout_i] != NEW_LINE) {
            stdout_i++;
        }
        stdout_write_struct.aio_buf = stdout_buffer;
        stdout_write_struct.aio_nbytes = stdout_i + 1;
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
    if (!queuebuffer.finished && aio_error(&socket_read_struct) == 0) {
        socket_read_struct.aio_buf = socket_buffer;
        socket_read_struct.aio_nbytes = BUF_SIZE;

        if (aio_read(&socket_read_struct) == -1) {
            fprintf(stderr, "Error: aio_read() from socket failed with %s\n", strerror(errno));
            return -1;
        }
        rw_in_progress[READ_SOCK] = &socket_read_struct;
    }
    return 0;
}

int check_stdin_read() {
    int ret;
    if (stdin_read_struct.aio_nbytes != 0 && aio_error(&stdin_read_struct) == 0) {
        if (aio_return(&stdin_read_struct) != stdin_read_struct.aio_nbytes) {
            fprintf(stderr, "Error: aio_return() when checking stdin read failed\n");
            return -1;
        }
        stdin_read_struct.aio_nbytes = 0;
        if (click != SPACE) return 0;

        lines_on_screen = 0;
        rw_in_progress[READ_STDIN] = NULL;
        tcsetattr(0, TCSANOW, &savtty);
    }
    return 0;
}

int check_stdout_write() {
    int len, res, ret;
    if (stdout_write_struct.aio_nbytes != 0 && aio_error(&stdout_write_struct) == 0) {
        if ((res = aio_return(&stdout_write_struct)) == -1) {
            fprintf(stderr, "Error: aio_return() in check socket read failed with %s\n", strerror(errno));
            return -1;
        }
        queuebuffer_add_handled_bytes(&queuebuffer, res);
        if (res == stdout_write_struct.aio_nbytes &&
            stdout_buffer[stdout_write_struct.aio_nbytes - 1] == NEW_LINE) {
            lines_on_screen++;
        }

        if (lines_on_screen == MAX_ALLOWED_LINES_ON_SCREEN) {
            ret = write(1, INVITE_TO_PRESS, INVITE_LEN);
            if (ret < 0) {
                fprintf(stderr, "Error: write() invite to screen failed with %s\n", strerror(errno));
                return -1;
            }
        }
        stdout_write_struct.aio_nbytes = 0;
        rw_in_progress[WRITE_STDOUT] = NULL;
    }
    return 0;
}

int check_socket_read() {
    int res;
    if (socket_read_struct.aio_nbytes != 0 && aio_error(&socket_read_struct) == 0) {
        if ((res = aio_return(&socket_read_struct)) == -1) {
            fprintf(stderr, "Error: aio_return() in check socket read failed with %s\n", strerror(errno));
            return -1;
        }
        if (res == 0) {
            queuebuffer_finish(&queuebuffer);
            return 0;
        }
        queuebuffer_add_bytes(&queuebuffer, socket_buffer, res);
        socket_read_struct.aio_nbytes = 0;
        rw_in_progress[READ_SOCK] = NULL;
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
    stdin_read_struct.aio_buf = &click;

    while (running) {
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

        aio_suspend((const aiocb *const *) rw_in_progress, RW_HANDLERS_COUNT, NULL);

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
        if (queuebuffer_is_empty(&queuebuffer) && queuebuffer.finished) {
            break;
        }
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
    tcgetattr(0, &tty);
    savtty = tty;
    tty.c_lflag &= ~(ISIG | ICANON | ECHO);
    tty.c_cc[VMIN] = 1;
    if (load_content_from_server() == -1) {
        fprintf(stderr, "Error: load_content_from_server() failed\n");
        exit(EXIT_FAILURE);
    }
    freeing_resources();
    exit(EXIT_SUCCESS);
}
