COMPILER = g++
CFLAGS = -Wall -I. -DENABLE_LOG -O3
CC = $(COMPILER) $(CFLAGS)
BIN = da_proc
BUILD = build
SRC = da
OBJS =

.PHONY: bin init clean format all

all: init bin

format:
	find . -type f -name "*.cc" | xargs -I{} bash -c 'clang-format -i {}'
	find . -type f -name "*.h" | xargs -I{} bash -c 'clang-format -i {}'

clean:
	rm -rf build
	find . -type f -name "*.o" | xargs -I{} bash -c 'rm -f {}'
	find . -type f -name "*.out" | xargs -I{} bash -c 'rm -f {}'
	rm -f $(BIN)

init: format clean
	mkdir build

bin: da_proc
	$(CC) -o $(BIN) $(OBJS)

da_proc: % : $(SRC)/%.cc util/status process/process init/parser
	mkdir -p $(shell dirname $(BUILD)/$@.o)
	$(CC) -c -o $(BUILD)/$@.o $<
	$(eval OBJS += $(BUILD)/$@.o)

init/parser: % : $(SRC)/%.cc util/status process/process
	mkdir -p $(shell dirname $(BUILD)/$@.o)
	$(CC) -c -o $(BUILD)/$@.o $<
	$(eval OBJS += $(BUILD)/$@.o)

process/process: % : $(SRC)/%.cc util/status
	mkdir -p $(shell dirname $(BUILD)/$@.o)
	$(CC) -c -o $(BUILD)/$@.o $<
	$(eval OBJS += $(BUILD)/$@.o)

util/status: % : $(SRC)/%.cc
	mkdir -p $(shell dirname $(BUILD)/$@.o)
	$(CC) -c -o $(BUILD)/$@.o $<
	$(eval OBJS += $(BUILD)/$@.o)
