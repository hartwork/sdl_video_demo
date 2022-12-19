// Copyright (c) 2022 Sebastian Pipping <sebastian@pipping.org>
// Licensed under GPL v3 or later

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL/SDL.h>
#include <SDL/SDL_rotozoom.h> // of sdl-gfx

void render_with_scaling_respecting_aspect_ratio(SDL_Surface *window,
                                                 SDL_Surface *texture) {
  const float texture_aspect_ratio = texture->w / (float)texture->h;
  const float window_aspect_ratio = window->w / (float)window->h;
  SDL_Rect target_rectangle;

  if (texture_aspect_ratio > window_aspect_ratio) {
    // Apply letterboxing
    // https://en.wikipedia.org/wiki/Letterboxing_(filming)
    target_rectangle.w = window->w;
    target_rectangle.h = window->w / texture_aspect_ratio;
    target_rectangle.x = 0;
    target_rectangle.y = (window->h - target_rectangle.h) / 2;
  } else {
    // Apply windowboxing
    // https://en.wikipedia.org/wiki/Windowbox_(filmmaking)
    target_rectangle.w = window->h * texture_aspect_ratio;
    target_rectangle.h = window->h;
    target_rectangle.x = (window->w - target_rectangle.w) / 2;
    target_rectangle.y = 0;
  }

  const double zoom_x = target_rectangle.w / (double)texture->w;
  const double zoom_y = target_rectangle.h / (double)texture->h;

  SDL_Surface *const scaled_texture =
      zoomSurface(texture, zoom_x, zoom_y, 0 /* i.e. no SMOOTHING_ON */);
  if (scaled_texture == NULL) {
    fprintf(stderr, "zoomSurface failed: %s\n", SDL_GetError());
    return;
  }

  if (SDL_BlitSurface(scaled_texture, NULL, window, &target_rectangle) == -1) {
    fprintf(stderr, "SDL_BlitSurface failed: %s\n", SDL_GetError());
  }
  SDL_FreeSurface(scaled_texture);
}

int main() {
  const int initial_window_width = 1024;
  const int initial_window_height = 768;
  // https://en.wikipedia.org/wiki/21:9_aspect_ratio
  const int video_width = 800;
  const int video_height = (int)(video_width * 9.0 / 21.0);

  const int grid_speed = 150; // in texture pixels per second

  const int bits_per_pixel = 32;

  // See
  // https://www.libsdl.org/release/SDL-1.2.15/docs/html/sdlcreatergbsurface.html
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
  const Uint32 rmask = 0xff000000;
  const Uint32 gmask = 0x00ff0000;
  const Uint32 bmask = 0x0000ff00;
  const Uint32 amask = 0x000000ff;
#else
  const Uint32 rmask = 0x000000ff;
  const Uint32 gmask = 0x0000ff00;
  const Uint32 bmask = 0x00ff0000;
  const Uint32 amask = 0xff000000;
#endif

  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    return EXIT_FAILURE;
  }

  SDL_WM_SetCaption("SDL1 video demo", NULL);
  const Uint32 window_flags = SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_RESIZABLE;
  SDL_Surface *window =
      SDL_SetVideoMode(initial_window_width, initial_window_height,
                       bits_per_pixel, window_flags);
  if (window == NULL) {
    fprintf(stderr, "SDL_SetVideoMode failed: %s\n", SDL_GetError());
    return EXIT_FAILURE;
  }

  SDL_Surface *const texture =
      SDL_CreateRGBSurface(SDL_HWSURFACE, video_width, video_height,
                           bits_per_pixel, rmask, gmask, bmask, amask);
  if (texture == NULL) {
    fprintf(stderr, "SDL_CreateRGBSurface failed: %s\n", SDL_GetError());
    return EXIT_FAILURE;
  }

  SDL_Event event;
  bool running = true;

  const Uint32 frequency = 1000; // i.e. milliseconds

  float seconds_since_last_fps_dump = 1.0f;
  int grid_index = 0;
  int grid_width = 40;

  const Uint32 initial = SDL_GetTicks();

  while (running) {
    while (SDL_PollEvent(&event) != 0) {
      switch (event.type) {
      case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
        case SDLK_ESCAPE:
        case SDLK_q:
          running = false;
          break;
        case SDLK_f:
        case SDLK_F11: {
          if (SDL_WM_ToggleFullScreen(window) == 0) {
            fprintf(stderr, "SDL_WM_ToggleFullScreen failed: %s\n",
                    SDL_GetError());
            break;
          }
          const int cursor_visibility = SDL_ShowCursor(SDL_QUERY);
          if (SDL_ShowCursor((cursor_visibility != SDL_ENABLE)
                                 ? SDL_ENABLE
                                 : SDL_DISABLE) < 0) {
            fprintf(stderr, "SDL_ShowCursor failed: %s\n", SDL_GetError());
            break;
          }
        } break;
        }
        break;
      case SDL_VIDEORESIZE:
        SDL_FreeSurface(window);
        window = SDL_SetVideoMode(event.resize.w, event.resize.h,
                                  bits_per_pixel, window_flags);
        break;
      case SDL_QUIT:
        running = false;
        break;
      }
    }

    const Uint32 before = SDL_GetTicks();

    if (SDL_LockSurface(texture) == -1) {
      continue; // e.g. while going fullscreen
    }

    uint8_t *const pixels = texture->pixels;
    const int pitch = texture->pitch; // bytes per row including padding

    for (int x = 0; x < video_width; x++) {
      for (int y = 0; y < video_height; y++) {
        const int index = (bits_per_pixel / 8) * x + y * pitch;
        const uint8_t red =
            (y % grid_width == grid_index) ? 0xff : (y * 255 / video_height);
        const uint8_t green =
            (x % grid_width == grid_index) ? 0xff : (x * 255 / video_width);
        const uint8_t blue = x + y;
        const uint8_t alpha = 0xFF;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        pixels[index] = alpha;
        pixels[index + 1] = blue;
        pixels[index + 2] = green;
        pixels[index + 3] = red;
#else
        pixels[index] = red;
        pixels[index + 1] = green;
        pixels[index + 2] = blue;
        pixels[index + 3] = alpha;
#endif
      }
    }

    SDL_UnlockSurface(texture);

    render_with_scaling_respecting_aspect_ratio(window, texture);
    if (SDL_Flip(window) == -1) {
      fprintf(stderr, "SDL_Flip failed: %s\n", SDL_GetError());
      break;
    }
    SDL_Delay(1); // to avoid 100% CPU usage

    const Uint32 after = SDL_GetTicks();
    const double seconds_elapsed = (after - before) / (double)frequency;
    seconds_since_last_fps_dump += seconds_elapsed;
    const int frames_per_second = (int)(1.0 / seconds_elapsed);
    if (seconds_since_last_fps_dump > 1) {
      fprintf(stderr, "[SDL 1] %3d FPS at %dx%dx%d (took %2dms)\n",
              frames_per_second, window->w, window->h, bits_per_pixel,
              (int)(seconds_elapsed * 1000));
      seconds_since_last_fps_dump = 0.0f;
    }

    const double total_runtime_seconds = (after - initial) / (double)frequency;
    grid_index = (int)(total_runtime_seconds * grid_speed) % grid_width;
  }

  SDL_FreeSurface(texture);
  SDL_FreeSurface(window);
  SDL_Quit();

  return EXIT_SUCCESS;
}
