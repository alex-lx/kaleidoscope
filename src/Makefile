all: toy

toy:
	g++ -g -03 -Wall parser.cpp `llvm-config --cppflags --ldflags --libs core` -o toy

clean:
	rm -rf *.o toy
