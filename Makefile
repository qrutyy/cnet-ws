SHELL := /bin/sh

CC := gcc
CFLAGS := -Wall -Wpedantic -Wextra -O3 -DNDEBUG -std=c99

SUBDIRS := tcp-server

BINDIR := bin

TARGETS := $(addprefix $(BINDIR)/,$(SUBDIRS))

all: $(TARGETS)

$(BINDIR)/%: %
	@mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) $(wildcard $</*.c) -o $@

clean:
	rm -rf $(BINDIR)

.PHONY: all clean $(SUBDIRS)

