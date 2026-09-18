#include "sys_utils.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void configure_sigint_handler(void *callback) {
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = callback;
    sigemptyset(&sa.sa_mask);
    // Disable SA_RESTART so that accept doesn't resume and block the shut down
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}
