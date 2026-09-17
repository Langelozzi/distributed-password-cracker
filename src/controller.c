#include "network_utils.h"
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

enum { MAX_BUFFER_SIZE = 512, MAX_LISTEN_CONN = 5 };

static volatile sig_atomic_t running = 1;

typedef struct {
    int port;
    char *password;
    char *charset_file;
    int timeout_ms;
} controller_args_t;

int main(int argc, char *argv[]) {
    // Listen IP address
    char *ip_address = "127.0.0.1";

    // Simulate args for now
    controller_args_t args = {.port = 3002,
                              .password = "a!",
                              .charset_file = "safe-charset.txt",
                              .timeout_ms = 5000};

    // Buffer for incoming message
    char buffer[MAX_BUFFER_SIZE];

    // Socket data
    int socket_fd, in_socket_fd;
    struct sockaddr_storage listen_addr_info, in_addr_info;
    struct sockaddr *listen_addr;
    socklen_t addr_len, in_addr_len = sizeof(in_addr_info);

    // Convert the IP address and port into proper structs while autodetecting
    // IPv4 vs v6
    convert_network_address(ip_address, &listen_addr_info);
    build_address(&listen_addr_info, (in_port_t)args.port, &listen_addr,
                  &addr_len);

    // 1. Create the socket
    socket_fd = socket(listen_addr_info.ss_family, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // 2. Bind the socket
    if (bind(socket_fd, listen_addr, addr_len) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // 3. Set socket to listen
    if (listen(socket_fd, MAX_LISTEN_CONN) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // 4. While true, accept connections and read data to console
    while (running) {
        in_socket_fd = accept(socket_fd, NULL, NULL);
        if (in_socket_fd < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        ssize_t bytes_read = read(in_socket_fd, buffer, MAX_BUFFER_SIZE);

        buffer[bytes_read] = '\0';

        printf("%s\n", buffer);

        close(in_socket_fd);
    }

    close(socket_fd);

    return 0;
}
