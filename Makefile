CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99
LDFLAGS = -lssl -lcrypto

SRC_DIR = src
BIN_DIR = bin
OBJ_DIR = obj

TARGETS = $(BIN_DIR)/client $(BIN_DIR)/server

# Source files
COMMON_SRCS = $(SRC_DIR)/common.c
CLIENT_SRCS = $(SRC_DIR)/client.c $(COMMON_SRCS)
SERVER_SRCS = $(SRC_DIR)/server.c $(COMMON_SRCS)

# Object files
CLIENT_OBJS = $(SRC_DIR)/client.o $(OBJ_DIR)/common.o
SERVER_OBJS = $(SRC_DIR)/server.o $(OBJ_DIR)/common.o

.PHONY: all clean test

all: $(TARGETS)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/common.o: $(SRC_DIR)/common.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(SRC_DIR)/client.o: $(SRC_DIR)/client.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(SRC_DIR)/server.o: $(SRC_DIR)/server.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/client: $(CLIENT_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BIN_DIR)/server: $(SERVER_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -rf $(BIN_DIR) $(OBJ_DIR) $(SRC_DIR)/*.o

test: all
	@echo "Create test directory..."
	@mkdir -p test_data test_receive
	@echo "Generating 1GB test file..."
	@dd if=/dev/zero of=test_data/testfile_1gb.bin bs=1M count=1024 2>/dev/null
	@echo "Test file created: test_data/testfile_1gb.bin"
	@echo ""
	@echo "To test the transfer:"
	@echo "1. Start server: ./$(BIN_DIR)/server ./test_receive 9999"
	@echo "2. In another terminal: ./$(BIN_DIR)/client localhost ./test_data/testfile_1gb.bin 9999"

help:
	@echo "Available targets:"
	@echo "  all   - Build client and server"
	@echo "  clean - Remove all build artifacts"
	@echo "  test  - Build and create test files"
	@echo "  help  - Show this help message"
