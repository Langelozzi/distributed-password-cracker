CC = /usr/bin/gcc
CFLAGS = -Wall -O2 -I./include
BUILD_DIR = ./build
COMMON_SRC = src/network_utils.c

.PHONY: all build clean

all: build

build: $(BUILD_DIR)/controller $(BUILD_DIR)/worker

$(BUILD_DIR)/.forge-directory:
	mkdir -p $(BUILD_DIR)
	touch $@

$(BUILD_DIR)/controller: src/controller.c $(COMMON_SRC) forge.toml | $(BUILD_DIR)/.forge-directory
	$(CC) $(CFLAGS) -o $@ $(filter-out forge.toml,$^)

$(BUILD_DIR)/worker: src/worker.c $(COMMON_SRC) forge.toml | $(BUILD_DIR)/.forge-directory
	$(CC) $(CFLAGS) -o $@ $(filter-out forge.toml,$^)

clean:
	rm -rf $(BUILD_DIR)
