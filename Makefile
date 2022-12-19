# Copyright (c) 2022 Sebastian Pipping <sebastian@pipping.org>
# Licensed under GPL v3 or later

CFLAGS += -Wall -Wextra -std=c99 -pedantic -Wno-switch

SDL1_CFLAGS := $(shell pkg-config sdl --cflags) $(shell pkg-config SDL_gfx --cflags)
SDL1_LDFLAGS := $(shell pkg-config sdl --libs) $(shell pkg-config SDL_gfx --libs)

SDL2_CFLAGS := $(shell pkg-config sdl2 --cflags)
SDL2_LDFLAGS := $(shell pkg-config sdl2 --libs)

.PHONY: all
all: sdl1_video_demo sdl2_video_demo

.PHONY: clean
clean:
	$(RM) sdl1_video_demo sdl2_video_demo

sdl1_video_demo: sdl1_video_demo.c
	$(CC) $(SDL1_CFLAGS) $(CFLAGS) $(SDL1_LDFLAGS) $(LDFLAGS) $< -o $@

sdl2_video_demo: sdl2_video_demo.c
	$(CC) $(SDL2_CFLAGS) $(CFLAGS) $(SDL2_LDFLAGS) $(LDFLAGS) $< -o $@
