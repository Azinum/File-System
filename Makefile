# Makefile

CC = gcc

FLAGS = -o vfs *.c -std=c99 -Os -Iinclude

all: build run

build:
	$(CC) $(FLAGS)

run:
	./vfs