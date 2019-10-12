all: da_proc

da_proc: src/da_proc.cc
	g++ -Wall -o da_proc src/da_proc.cc

clean:
	rm -f da_proc **/*.o **/*.out

format:
	clang-format -i **/*.cc
	clang-format -i **/*.h
