all: init da_proc

init:
	rm -rf build
	mkdir build

da_proc: process parser da/da_proc.cc
	g++ -Wall -I. -DENABLE_LOG -O3 -o da_proc da/da_proc.cc build/*.o

parser: process status da/init/parser.cc
	g++ -Wall -I. -DENABLE_LOG -O3 -c da/init/parser.cc -o build/parser.o

process: da/process/process.cc
	g++ -Wall -I. -DENABLE_LOG -O3 -c da/process/process.cc -o build/process.o

status: da/util/status.cc
	g++ -Wall -I. -DENABLE_LOG -O3 -c da/util/status.cc -o build/status.o

clean:
	rm -rf build
	find . -type f -name "*.o" | xargs -I{} bash -c 'rm -f {}'
	find . -type f -name "*.out" | xargs -I{} bash -c 'rm -f {}'

format:
	find . -type f -name "*.cc" | xargs -I{} bash -c 'clang-format -i {}'
	find . -type f -name "*.h" | xargs -I{} bash -c 'clang-format -i {}'
