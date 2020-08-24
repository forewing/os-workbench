NAME := $(shell basename $(PWD))
export TASK := M5
all: $(NAME)-64 $(NAME)-32

include ../Makefile
include ../Makefile.lab

test64: frecov-64
	./frecov-64 storage/fs.img

test32: frecov-32
	./frecov-32 storage/fs.img