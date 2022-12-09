DAEMON := nicepowerd
CTL := nicepowerctl

CFLAGS := $(CFLAGS) -pthread
CFLAGS_DEBUG := -ggdb -DDEBUG -Wall -Wextra
 
PHONY: clean build
clean:
	@rm -f $(DAEMON) $(CTL) $<
build:
	@echo "Compiling $(DAEMON)..." $<
	@clang src/$(DAEMON).c $(CFLAGS) $(CFLAGS_DEBUG) -o $(DAEMON) $<
	@echo "Compiling $(CTL)..." $<
	@clang src/$(CTL).c $(CFLAGS) $(CFLAGS_DEBUG) -o $(CTL) $<

