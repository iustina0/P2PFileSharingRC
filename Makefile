
CC := gcc
CFLAGS := -Wall -Wextra -g
LDFLAGS := -pthread

CLIENT_DIR := client
SERVER_DIR := server

CLIENT_SRC := $(wildcard src/$(CLIENT_DIR)/*.c)
SERVER_SRC := $(wildcard src/$(SERVER_DIR)/*.c)

CLIENT_OBJ := $(patsubst %.c, %.o, $(CLIENT_SRC))
SERVER_OBJ := $(patsubst %.c, %.o, $(SERVER_SRC))

CLIENT_EXE := client
SERVER_EXE := server

all: $(CLIENT_EXE) $(SERVER_EXE)

$(CLIENT_EXE): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(SERVER_EXE): $(SERVER_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(CLIENT_DIR)/%.o: $(CLIENT_DIR)/%.c $(CLIENT_DIR)/client_util.h
	$(CC) $(CFLAGS) -c $< -o $@

$(SERVER_DIR)/%.o: $(SERVER_DIR)/%.c $(SERVER_DIR)/server_util.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(CLIENT_OBJ) $(SERVER_OBJ) $(CLIENT_EXE) $(SERVER_EXE)
