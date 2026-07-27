
CC=arm-linux-gnueabihf-gcc
CFLAGS = --static -I. -Icore
LDFLAGS = -pthread



all:
	$(CC) $(CFLAGS) $(LDFLAGS) main.c core/draw.c core/touch.c core/peashooter.c core/digit_bitmaps.c -o main 
clean:
	rm -rf main

.PHONY: all clean