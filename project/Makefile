COMPILER = g++
CFLAGS = -Wall -ggdb -O0
INCL_PATHS = config_parser.cpp datagram_parser.cpp

all: server

server: main.cpp $(INCL_PATHS)
	$(COMPILER) -std=c++20 $(INCL_PATHS) $< -o $@ $(CFLAGS)

clean:
	rm -rf server
	rm -rf *.dSYM
