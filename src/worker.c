#include "network_utils.h"
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/_types/_in_port_t.h>
#include <sys/socket.h>
#include <unistd.h>

enum { MAX_BUFFER_SIZE = 512, MAX_LISTEN_CONN = 5 };

static volatile sig_atomic_t running = 1;

typedef struct {
    char *host;
    int port;
    int worker_id;
    int heartbeat_ms;
} worker_args_t;

int main(int argc, char *argv[]) {
    // Simulate args for now
    worker_args_t args = {.host = "127.0.0.1",
                          .port = 3002,
                          .worker_id = 1,
                          .heartbeat_ms = 1000};

    // Buffer for incoming message
    char buffer[MAX_BUFFER_SIZE];

    // Socket data
    int socket_fd;
    struct sockaddr_storage target_addr_info;
    struct sockaddr *target_addr;
    socklen_t addr_len;

    // Convert the IP address and port into proper structs while autodetecting
    // IPv4 vs v6
    convert_network_address(args.host, &target_addr_info);
    build_address(&target_addr_info, (in_port_t)args.port, &target_addr,
                  &addr_len);

    // 1. Create the socket
    socket_fd = socket(target_addr_info.ss_family, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // 2. Connect to the target
    if (connect(socket_fd, target_addr, addr_len) < 0) {
        perror("connect");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    // 3. Write data to the socket (send message to target)
    ssize_t bytes_sent = write(socket_fd, "Hello World", 12);
    if (bytes_sent < 0) {
        perror("write");
        exit(EXIT_FAILURE);
    }

    close(socket_fd);

    return 0;
}
