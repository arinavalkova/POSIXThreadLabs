#include "EdUrlParser.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <netdb.h>
#include <fcntl.h>

#define EMPTY ""
#define HTTP "http"
#define DEFAULT_PORT "80"
#define GET_REQUEST_LEN 30

int
url_parse_err_proc(char *url_line, char **host, char **port, char **path) {
    EdUrlParser *url = EdUrlParser::parseUrl(url_line);
    if (strcmp(url->scheme.c_str(), HTTP)) {
        fprintf(stderr, "Error: protocol is not supported, only http\n");
        return -1;
    }
    *port = (char *) malloc(strlen(url->port.c_str()));
    memcpy(*port, url->port.c_str(), strlen(url->port.c_str()));
    if (strcmp(*port, EMPTY) == 0) {
        fprintf(stderr, "Note: established default port 80\n");
        *port = (char *) malloc(strlen(DEFAULT_PORT));
        memcpy(*port, DEFAULT_PORT, strlen(DEFAULT_PORT));
    }
    *host = (char *) malloc(strlen(url->hostName.c_str()));
    memcpy(*host, url->hostName.c_str(), strlen(url->hostName.c_str()));
    if (strcmp(*host, EMPTY) == 0) {
        fprintf(stderr, "Error: can't parse host name\n");
        return -1;
    }
    *path = (char *) malloc(strlen(url->path.c_str()));
    memcpy(*path, url->path.c_str(), strlen(url->path.c_str()));
    if (strcmp(*path, EMPTY) == 0) {
        fprintf(stderr, "Error: can't parse url path\n");
    }
    delete url;
    return 0;
}

int
connect_to_server(char *host, char *port) {
    int ret, sock;
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

int
start_http_client(char *host, char *port, char *path) {
    int sock;
    char *get_request = (char *) malloc(strlen(path) + GET_REQUEST_LEN);
    if ((sock = connect_to_server(host, port)) == -1) {
        fprintf(stderr, "Error: can't connect to server\n");
        return -1;
    }
    sprintf(get_request, "GET %s HTTP/1.0\r\n\r\n", path);
    if (write(sock, get_request, strlen(get_request)) == -1) {
        fprintf(stderr, "Error: write() GET request to socket failed with %s\n", strerror(errno));
        return -1;
    }
    free(get_request);
    return sock;
}

int GET_Request(char* url_line) {
    int sock;
    char *host = NULL, *path = NULL, *port = NULL;
    if (url_parse_err_proc(url_line, &host, &port, &path) != 0) {
        return -1;
    }
    if ((sock = start_http_client(host, port, path)) == -1) {
        return -1;
    }
    free(host);
    free(path);
    free(port);
    return sock;
}
