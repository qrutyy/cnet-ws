SHELL = /bin/sh

CC = gcc
CFLAGS = -Wall -Wpedantic -Wextra -O3 -DNDEBUG -std=c99

SUBDIRS := tcp-server 

TARGETS := $(SUBDIRS)

all:
	$(TARGETS)

$(TARGETS):
	$(CC) $(CCFLAGS) $(CFLAGS) $(wildcard $@/*.c) -o $@.out


clean:
	rm -f $(addsuffix .out,$(SUBDIRS))


.PHONY: all clean $(TARGETS)
