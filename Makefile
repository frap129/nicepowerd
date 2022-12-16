DAEMON := nicepowerd
CTL := nicepowerctl

include config.mk

CFLAGS := $(CFLAGS) -pthread \
          -DAC_PATH=\"$(AC_PATH)\" \
          -DBAT_LOW_THRESH=$(BAT_LOW_THRESH) \
          -DBAT_HIGH_THRESH=$(BAT_HIGH_THRESH)

CFLAGS_DEBUG := -ggdb -DDEBUG -Wall -Wextra
 
PHONY: clean build
clean:
	@rm -f $(DAEMON) $(CTL) $<
build:
	@echo "Compiling $(DAEMON)..." $<
	@clang src/$(DAEMON).c $(CFLAGS) $(CFLAGS_DEBUG) -o $(DAEMON) $<
	@echo "Compiling $(CTL)..." $<
	@clang src/$(CTL).c $(CFLAGS) $(CFLAGS_DEBUG) -o $(CTL) $<

