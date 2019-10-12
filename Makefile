all: da_proc

da_proc: da/da_proc.cc
	g++ -Wall -O3 -o da_proc da/da_proc.cc

clean:
	rm -f da_proc **/*.o **/*.out

format:
	find . -type f -name "*.cc" | xargs -I{} bash -c 'clang-format -i {}'
	find . -type f -name "*.h" | xargs -I{} bash -c 'clang-format -i {}'
