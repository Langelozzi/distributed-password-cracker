CC      := gcc
CFLAGS  := -Wall -O2 -I./include
BUILD_DIR := ./build

# Common source shared by targets
NET_UTILS := src/network_utils.c

.PHONY: all build clean

# Default target
all: build

build: $(BUILD_DIR)/worker $(BUILD_DIR)/controller

$(BUILD_DIR)/worker: src/worker.c $(NET_UTILS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/controller: src/controller.c $(NET_UTILS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^

# Ensure output directory exists before building targets
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
