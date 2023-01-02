# Copyright (c) 2022 Sebastian Pipping <sebastian@pipping.org>
# Licensed under GPL v3 or later

CFLAGS += -Wall -Wextra -std=c99 -pedantic

SDL1_CFLAGS := $(shell pkg-config SDL_gfx --cflags)
SDL1_LDFLAGS := $(shell pkg-config SDL_gfx --libs)

SDL2_CFLAGS := $(shell sdl2-config --cflags)
SDL2_LDFLAGS := $(shell sdl2-config --libs)

.PHONY: all
all: sdl1_video_demo sdl2_video_demo

.PHONY: clean
clean:
	$(RM) sdl1_video_demo sdl2_video_demo

sdl1_video_demo: sdl1_video_demo.c
	$(CC) $< -o $@ $(SDL1_CFLAGS) $(CFLAGS) $(SDL1_LDFLAGS) $(LDFLAGS)

sdl2_video_demo: sdl2_video_demo.c
	$(CC) $< -o $@ $(SDL2_CFLAGS) $(CFLAGS) $(SDL2_LDFLAGS) $(LDFLAGS)
