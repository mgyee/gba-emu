#include "common.h"
#include "cpu.h"
#include <SDL.h>
#include <stdio.h>
#include <string.h>

#define SCALE 4
#define SCREEN_WIDTH 240 * SCALE
#define SCREEN_HEIGHT 160 * SCALE

#define CYCLES_PER_FRAME 280896
#define FRAME_TIME_MS (1000.0 / 59.73)

int main(int argc, char *argv[]) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    return 1;
  }

  SDL_Window *window =
      SDL_CreateWindow("gba-emu", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
  if (!window) {
    SDL_Quit();
    return 1;
  }

  SDL_Renderer *renderer =
      SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (!renderer) {
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  Bus *bus = malloc(sizeof(Bus));
  bus_init(bus);

  if (argc > 1) {
    if (bus_load_rom(bus, argv[1])) {
      printf("ROM loaded: %s\n", argv[1]);
    } else {
      printf("Failed to load ROM: %s\n", argv[1]);
      return 1;
    }
  } else {
    printf("Usage: %s <rom_file>\n", argv[0]);
    return 1;
  }

  CPU *cpu = malloc(sizeof(CPU));
  cpu_init(cpu, bus);

  bool running = true;
  Uint32 frame_start_time = SDL_GetTicks();

  int frame_count = 0;
  Uint32 last_time = SDL_GetTicks();
  char fps_buffer[32];

  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
      if (event.type == SDL_QUIT) {
        running = false;
      }
    }
    int total_cyles = 0;
    while (total_cyles < CYCLES_PER_FRAME) {
      int cycles = cpu_step(cpu, bus);
      total_cyles += cycles;
      // ppu_step(ppu, cycles);
      // timer_step(timer, cycles);
      // dma_step(dma, cycles);
      // sound_step(sound, cycles);
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    Uint32 frame_time = SDL_GetTicks() - frame_start_time;
    if (frame_time < FRAME_TIME_MS) {
      SDL_Delay(FRAME_TIME_MS - frame_time);
    }
    frame_start_time = SDL_GetTicks();

    frame_count++;
    if (frame_start_time - last_time >= 1000) {
      snprintf(fps_buffer, sizeof(fps_buffer), "gba-emu | FPS: %d",
               frame_count);
      SDL_SetWindowTitle(window, fps_buffer);
      frame_count = 0;
      last_time = frame_start_time;
    }
  }

  free(bus);

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
