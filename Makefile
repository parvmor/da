COMPILER = g++
DEBUG_CFLAGS = -Wall -I. -std=c++14 -pthread -g -DENABLE_LOG -fsanitize=undefined
CFLAGS = -Wall -I. -O3 -std=c++14 -pthread
CC = $(COMPILER) $(DEBUG_CFLAGS)
BIN = da_proc
BUILD = build
SRC = da
OBJS =

.PHONY: bin init clean format all

all: init bin

format:
	find . -type f -name "*.cc" | xargs -I{} bash -c 'clang-format -i {}' 2>/dev/null || echo >&2 "clang-format not found. Skipping."
	find . -type f -name "*.h" | xargs -I{} bash -c 'clang-format -i {}' 2>/dev/null || echo >&2 "clang-format not found. Skipping."

clean:
	rm -rf build
	find . -type f -name "*.o" | xargs -I{} bash -c 'rm -f {}'
	find . -type f -name "*.out" | xargs -I{} bash -c 'rm -f {}'
	rm -f $(BIN)

init: format clean
	mkdir build

bin: da_proc
	$(CC) -o $(BIN) $(OBJS)

da_proc: % : $(SRC)/%.cc util/status process/process init/parser socket/udp_socket executor/executor link/perfect_link receiver/receiver broadcast/uniform_reliable
	mkdir -p $(shell dirname $(BUILD)/$@.o)
	$(CC) -c -o $(BUILD)/$@.o $<
	$(eval OBJS += $(BUILD)/$@.o)

init/parser: % : $(SRC)/%.cc util/status process/process
	mkdir -p $(shell dirname $(BUILD)/$@.o)
	$(CC) -c -o $(BUILD)/$@.o $<
	$(eval OBJS += $(BUILD)/$@.o)

receiver/receiver: % : $(SRC)/%.cc util/status executor/executor socket/udp_socket broadcast/uniform_reliable link/perfect_link
	mkdir -p $(shell dirname $(BUILD)/$@.o)
	$(CC) -c -o $(BUILD)/$@.o $<
	$(eval OBJS += $(BUILD)/$@.o)

broadcast/uniform_reliable: % : $(SRC)/%.cc process/process link/perfect_link 
	mkdir -p $(shell dirname $(BUILD)/$@.o)
	$(CC) -c -o $(BUILD)/$@.o $<
	$(eval OBJS += $(BUILD)/$@.o)

link/perfect_link: % : $(SRC)/%.cc util/status process/process socket/udp_socket executor/executor
	mkdir -p $(shell dirname $(BUILD)/$@.o)
	$(CC) -c -o $(BUILD)/$@.o $<
	$(eval OBJS += $(BUILD)/$@.o)

executor/executor: % : $(SRC)/%.cc
	mkdir -p $(shell dirname $(BUILD)/$@.o)
	$(CC) -c -o $(BUILD)/$@.o $<
	$(eval OBJS += $(BUILD)/$@.o)

process/process: % : $(SRC)/%.cc util/status
	mkdir -p $(shell dirname $(BUILD)/$@.o)
	$(CC) -c -o $(BUILD)/$@.o $<
	$(eval OBJS += $(BUILD)/$@.o)

socket/udp_socket: % : $(SRC)/%.cc util/status socket/socket socket/communicating_socket
	mkdir -p $(shell dirname $(BUILD)/$@.o)
	$(CC) -c -o $(BUILD)/$@.o $<
	$(eval OBJS += $(BUILD)/$@.o)

socket/communicating_socket: % : $(SRC)/%.cc util/status socket/socket
	mkdir -p $(shell dirname $(BUILD)/$@.o)
	$(CC) -c -o $(BUILD)/$@.o $<
	$(eval OBJS += $(BUILD)/$@.o)

socket/socket: % : $(SRC)/%.cc  util/status
	mkdir -p $(shell dirname $(BUILD)/$@.o)
	$(CC) -c -o $(BUILD)/$@.o $<
	$(eval OBJS += $(BUILD)/$@.o)

util/status: % : $(SRC)/%.cc
	mkdir -p $(shell dirname $(BUILD)/$@.o)
	$(CC) -c -o $(BUILD)/$@.o $<
	$(eval OBJS += $(BUILD)/$@.o)

