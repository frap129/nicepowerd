DAEMON := nicepowerd
CFLAGS := $(CFLAGS) -pthread -Iinclude
CFLAGS_DEBUG := -ggdb -DDEBUG -Wall -Wextra
SRC := src/nicepowerd.c
 
PHONY: clean build
clean:
	@rm -f $(DAEMON) $<
build:
	@echo "Compiling $(DAEMON)..." $<
	@clang $(SRC) $(CFLAGS) $(CFLAGS_DEBUG) -o $(DAEMON) $<
