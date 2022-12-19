// Copyright (c) 2022 Sebastian Pipping <sebastian@pipping.org>
// Licensed under GPL v3 or later

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>

void render_with_scaling_respecting_aspect_ratio(SDL_Renderer *renderer,
                                                 SDL_Texture *texture) {
  SDL_Rect texture_rect;
  if (SDL_QueryTexture(texture, NULL, NULL, &texture_rect.w, &texture_rect.h) !=
      0) {
    fprintf(stderr, "SDL_QueryTexture failed: %s\n", SDL_GetError());
    return;
  }

  SDL_Rect window_rect;
  if (SDL_GetRendererOutputSize(renderer, &window_rect.w, &window_rect.h) !=
      0) {
    fprintf(stderr, "SDL_GetRendererOutputSize failed: %s\n", SDL_GetError());
    return;
  }

  const float texture_aspect_ratio = texture_rect.w / (float)texture_rect.h;
  const float window_aspect_ratio = window_rect.w / (float)window_rect.h;
  SDL_Rect target_rectangle;

  if (texture_aspect_ratio > window_aspect_ratio) {
    // Apply letterboxing
    // https://en.wikipedia.org/wiki/Letterboxing_(filming)
    target_rectangle.w = window_rect.w;
    target_rectangle.h = window_rect.w / texture_aspect_ratio;
    target_rectangle.x = 0;
    target_rectangle.y = (window_rect.h - target_rectangle.h) / 2;
  } else {
    // Apply windowboxing
    // https://en.wikipedia.org/wiki/Windowbox_(filmmaking)
    target_rectangle.w = window_rect.h * texture_aspect_ratio;
    target_rectangle.h = window_rect.h;
    target_rectangle.x = (window_rect.w - target_rectangle.w) / 2;
    target_rectangle.y = 0;
  }

  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, NULL, &target_rectangle);
}

int main() {
  const int initial_window_width = 1024;
  const int initial_window_height = 768;
  // https://en.wikipedia.org/wiki/21:9_aspect_ratio
  const int video_width = 800;
  const int video_height = (int)(video_width * 9.0 / 21.0);

  const int grid_speed = 150; // in texture pixels per second

  const int bits_per_pixel = 32;
  const int pixel_format = SDL_PIXELFORMAT_ABGR8888;

  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    return EXIT_FAILURE;
  }

  SDL_Window *const window =
      SDL_CreateWindow("SDL2 video demo", 100, 100, initial_window_width,
                       initial_window_height, SDL_WINDOW_RESIZABLE);
  if (window == NULL) {
    fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
    return EXIT_FAILURE;
  }

  SDL_Renderer *const renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (renderer == NULL) {
    fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_FAILURE;
  }

  SDL_Texture *const texture =
      SDL_CreateTexture(renderer, pixel_format, SDL_TEXTUREACCESS_STREAMING,
                        video_width, video_height);
  if (texture == NULL) {
    fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_FAILURE;
  }

  SDL_Event event;
  bool running = true;

  const Uint64 frequency = SDL_GetPerformanceFrequency();

  float seconds_since_last_fps_dump = 1.0f;
  int grid_index = 0;
  int grid_width = 40;

  const Uint64 initial = SDL_GetPerformanceCounter();

  int window_width = initial_window_width;
  int window_height = initial_window_height;

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
          static bool fullscreen = false;
          if (SDL_SetWindowFullscreen(
                  window, fullscreen ? 0 : SDL_WINDOW_FULLSCREEN) != 0) {
            fprintf(stderr, "SDL_SetWindowFullscreen failed: %s\n",
                    SDL_GetError());
            break;
          }
          fullscreen = !fullscreen;
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
      case SDL_WINDOWEVENT:
        switch (event.window.event) {
        case SDL_WINDOWEVENT_SIZE_CHANGED:
          window_width = event.window.data1;
          window_height = event.window.data2;
          break;
        }
        break;
      case SDL_QUIT:
        running = false;
        break;
      }
    }

    const Uint64 before = SDL_GetPerformanceCounter();

    uint8_t *pixels = NULL;
    int pitch = 0; // bytes per row including padding

    if (SDL_LockTexture(texture, NULL, (void **)&pixels, &pitch) != 0) {
      continue; // e.g. while going fullscreen
    }

    for (int x = 0; x < video_width; x++) {
      for (int y = 0; y < video_height; y++) {
        const int index = (bits_per_pixel / 8) * x + y * pitch;
        const uint8_t red =
            (y % grid_width == grid_index) ? 0xff : (y * 255 / video_height);
        const uint8_t green =
            (x % grid_width == grid_index) ? 0xff : (x * 255 / video_width);
        const uint8_t blue = x + y;
        const uint8_t alpha = 0xFF;

        pixels[index] = red;
        pixels[index + 1] = green;
        pixels[index + 2] = blue;
        pixels[index + 3] = alpha;
      }
    }

    SDL_UnlockTexture(texture);

    render_with_scaling_respecting_aspect_ratio(renderer, texture);
    SDL_RenderPresent(renderer);
    SDL_Delay(1); // to avoid 100% CPU usage

    const Uint64 after = SDL_GetPerformanceCounter();
    const double seconds_elapsed = (after - before) / (double)frequency;
    seconds_since_last_fps_dump += seconds_elapsed;
    const int frames_per_second = (int)(1.0 / seconds_elapsed);
    if (seconds_since_last_fps_dump > 1) {
      fprintf(stderr, "[SDL 2] %3d FPS at %dx%dx%d (took %2dms)\n",
              frames_per_second, window_width, window_height, bits_per_pixel,
              (int)(seconds_elapsed * 1000));
      seconds_since_last_fps_dump = 0.0f;
    }

    const double total_runtime_seconds = (after - initial) / (double)frequency;
    grid_index = (int)(total_runtime_seconds * grid_speed) % grid_width;
  }

  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return EXIT_SUCCESS;
}
