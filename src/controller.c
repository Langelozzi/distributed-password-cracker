#include "network_utils.h"
#include "sys_utils.h"
#include <errno.h>
#include <getopt.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

enum { MAX_BUFFER_SIZE = 512, MAX_LISTEN_CONN = 5, PROG_NAME_ARG_IDX = 0 };

typedef struct {
    int port;
    char *password;
    char *charset_file;
    int timeout_ms;
} controller_args_t;

static volatile sig_atomic_t running = 1;

static void run_controller(const controller_args_t *args);
static void parse_arguments(int argc, char *argv[], controller_args_t *args);
static void handle_parsing_failure(const char *prog_name);
static void print_help(const char *prog_name);
static void handle_sigint(int sig);

int main(int argc, char *argv[]) {
    // // Simulate args for now
    // controller_args_t args = {.port = 3002,
    //                           .password = "a!",
    //                           .charset_file = "safe-charset.txt",
    //                           .timeout_ms = 5000};
    controller_args_t args;
    parse_arguments(argc, argv, &args);

    configure_sigint_handler(handle_sigint);

    run_controller(&args);

    return 0;
}

static void run_controller(const controller_args_t *args) {
    // Listen IP address
    char *ip_address = "127.0.0.1";

    // Buffer for incoming message
    char buffer[MAX_BUFFER_SIZE];

    // Socket data
    int socket_fd, in_socket_fd;
    struct sockaddr_storage listen_addr_info, in_addr_info;
    struct sockaddr *listen_addr;
    socklen_t addr_len = sizeof(in_addr_info);

    // Convert the IP address and port into proper structs while autodetecting
    // IPv4 vs v6
    convert_network_address(ip_address, &listen_addr_info);
    build_address(&listen_addr_info, (in_port_t)args->port, &listen_addr,
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
        if (in_socket_fd == -1) {
            if (errno == EINTR) {
                // Signal was caught and handler set running to 0
                break;
            } else {
                perror("accept");
                exit(EXIT_FAILURE);
            }
        }

        ssize_t bytes_read = read(in_socket_fd, buffer, MAX_BUFFER_SIZE);

        buffer[bytes_read] = '\0';

        printf("%s\n", buffer);

        close(in_socket_fd);
    }

    printf("\nCTRL+C pressed\n");
    close(socket_fd);
}

static void parse_arguments(int argc, char *argv[], controller_args_t *args) {
    // Set defaults
    args->port = -1;
    args->password = NULL;
    args->charset_file = NULL;
    args->timeout_ms = -1;

    // Define options
    static struct option long_options[] = {
        {"port", required_argument, 0, 'p'},
        {"password", required_argument, 0, 's'},
        {"charset-file", required_argument, 0, 'f'},
        {"timeout-ms", required_argument, 0, 't'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0},
    };

    const char *prog_name = argv[PROG_NAME_ARG_IDX];

    int opt;
    int option_index;

    // Parse each option
    while ((opt = getopt_long(argc, argv, "h", long_options, &option_index)) !=
           -1) {
        switch (opt) {
        case 'p':
            args->port = atoi(optarg);
            if (args->port <= 0 || args->port > 65535) {
                fprintf(stderr, "Invalid port number: %s (must be1-65535)\n",
                        optarg);
                handle_parsing_failure(prog_name);
            }
            break;
        case 's':
            args->password = optarg;
            break;
        case 'f':
            args->charset_file = optarg;
            break;
        case 't':
            args->timeout_ms = atoi(optarg);
            if (args->timeout_ms < 0) {
                fprintf(stderr, "Cannot have negative timeout value.\n");
                handle_parsing_failure(prog_name);
            }
            break;
        case 'h':
            print_help(prog_name);
            exit(EXIT_SUCCESS);
        case '?':
            fprintf(stderr, "Unknown option: -%c\n", optopt);
            handle_parsing_failure(prog_name);
        default:
            handle_parsing_failure(prog_name);
        }
    }

    // Validate that all required options are set
    if (args->port == -1) {
        fprintf(stderr, "--port is required\n");
        handle_parsing_failure(prog_name);
    }

    if (!args->password) {
        fprintf(stderr, "--password is required\n");
        handle_parsing_failure(prog_name);
    }

    if (!args->charset_file) {
        fprintf(stderr, "--charset-file is required\n");
        handle_parsing_failure(prog_name);
    }

    if (args->timeout_ms == -1) {
        fprintf(stderr, "--timeout-ms is required\n");
        handle_parsing_failure(prog_name);
    }

    // Check for unexpected positional arguments
    if (optind < argc) {
        fprintf(stderr, "Unexpected postitiional argument: %s\n", argv[optind]);
        handle_parsing_failure(prog_name);
    }
}

static void handle_parsing_failure(const char *prog_name) {
    print_help(prog_name);
    exit(EXIT_FAILURE);
}

static void print_help(const char *prog_name) {
    printf(
        "Usage: %s [OPTIONS]\n"
        "\n"
        "Distributed password cracker for performing concurrent brute force "
        "cracking of hashed passwords.\n"
        "\n"
        "Options:\n"
        "  --port PORT                  Port number to listen on (required)\n"
        "  --password PASSWORD          The hashed password you want to crack\n"
        "  --charset-file FILE_PATH     The path to the file containing the "
        "available character set of the password\n"
        "  --timeout-ms TIMEOUT_MS      The heartbeat age that will "
        "consider the worker STALE\n"
        "  -h, --help                   Show this help message and exit\n"
        "\n"
        "Example:\n"
        "  %s --port 9876 --password \"a!\" --charset-file "
        "\"safe-charset.txt\" --timeout-ms 5000\n",
        prog_name, prog_name);
}

static void handle_sigint(int sig) { running = 0; }
