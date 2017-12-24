CFLAGS ?= --iram-size 512

all: switcher

switcher: switcher.c
	sdcc $(CFLAGS) $^
