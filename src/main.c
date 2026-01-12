#include "common.h"
#include "cpu.h"
#include "gba.h"
#include "interrupt.h"
#include "io.h"
#include "keypad.h"
#include "ppu.h"
#include <SDL.h>
#include <stdio.h>
#include <string.h>

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

  SDL_Texture *texture =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                        SDL_TEXTUREACCESS_STREAMING, 240, 160);
  if (!texture) {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  char *bios_file = "gba_bios.bin";
  if (argc == 3) {
    bios_file = argv[2];
  } else if (argc != 2) {
    printf("Usage: %s <rom_file> [bios_file]\n", argv[0]);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  Gba *gba = malloc(sizeof(Gba));
  if (!gba_init(gba, bios_file, argv[1])) {
    gba_free(gba);
    free(gba);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  Uint32 frame_start_time = SDL_GetTicks();

  int frame_count = 0;
  Uint32 last_time = frame_start_time;
  char fps_buffer[32];

  while (true) {
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
      switch (event.type) {
      case SDL_QUIT:
        goto shutdown;
      case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
        case SDLK_UP:
          gba->keypad.keyinput &= ~(1 << BUTTON_UP);
          break;
        case SDLK_DOWN:
          gba->keypad.keyinput &= ~(1 << BUTTON_DOWN);
          break;
        case SDLK_LEFT:
          gba->keypad.keyinput &= ~(1 << BUTTON_LEFT);
          break;
        case SDLK_RIGHT:
          gba->keypad.keyinput &= ~(1 << BUTTON_RIGHT);
          break;
        case SDLK_z:
          gba->keypad.keyinput &= ~(1 << BUTTON_B);
          break;
        case SDLK_x:
          gba->keypad.keyinput &= ~(1 << BUTTON_A);
          break;
        case SDLK_a:
          gba->keypad.keyinput &= ~(1 << BUTTON_L);
          break;
        case SDLK_s:
          gba->keypad.keyinput &= ~(1 << BUTTON_R);
          break;
        case SDLK_RETURN:
          gba->keypad.keyinput &= ~(1 << BUTTON_START);
          break;
        case SDLK_RSHIFT:
          gba->keypad.keyinput &= ~(1 << BUTTON_SELECT);
          break;
        }
        break;
      case SDL_KEYUP:
        switch (event.key.keysym.sym) {
        case SDLK_UP:
          gba->keypad.keyinput |= (1 << BUTTON_UP);
          break;
        case SDLK_DOWN:
          gba->keypad.keyinput |= (1 << BUTTON_DOWN);
          break;
        case SDLK_LEFT:
          gba->keypad.keyinput |= (1 << BUTTON_LEFT);
          break;
        case SDLK_RIGHT:
          gba->keypad.keyinput |= (1 << BUTTON_RIGHT);
          break;
        case SDLK_z:
          gba->keypad.keyinput |= (1 << BUTTON_B);
          break;
        case SDLK_x:
          gba->keypad.keyinput |= (1 << BUTTON_A);
          break;
        case SDLK_a:
          gba->keypad.keyinput |= (1 << BUTTON_L);
          break;
        case SDLK_s:
          gba->keypad.keyinput |= (1 << BUTTON_R);
          break;
        case SDLK_RETURN:
          gba->keypad.keyinput |= (1 << BUTTON_START);
          break;
        case SDLK_RSHIFT:
          gba->keypad.keyinput |= (1 << BUTTON_SELECT);
          break;
        }
        break;
      }
    }
    int total_cycles = 0;
    while (total_cycles < CYCLES_PER_FRAME) {
      int cycles = 0;

      gba->bus.cycle_count = 0;

      if (dma_active(&gba->dma)) {
        dma_step(gba);
      } else {
        if (gba->io.power_state == POWER_STATE_HALTED) {
          if (interrupt_pending(gba)) {
            gba->io.power_state = POWER_STATE_NORMAL;
          }
        }

        if (gba->io.power_state == POWER_STATE_HALTED) {
          cycles += 16;
        } else {
          if (interrupt_pending(gba)) {
            handle_interrupts(gba);
          }
          cycles += cpu_step(gba);
        }
      }

      cycles += gba->bus.cycle_count;

      ppu_step(gba, cycles);
      total_cycles += cycles;
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_UpdateTexture(texture, NULL, gba->ppu.framebuffer, 240 * sizeof(u32));
    SDL_RenderCopy(renderer, texture, NULL, NULL);

    SDL_RenderPresent(renderer);

    Uint32 frame_time = SDL_GetTicks() - frame_start_time;
    if (frame_time < FRAME_TIME_MS) {
      SDL_Delay(FRAME_TIME_MS - frame_time);
    }

    frame_start_time = SDL_GetTicks();

    frame_count++;
    Uint32 elapsed_time = frame_start_time - last_time;
    if (elapsed_time >= 1000) {
      double fps = (double)frame_count * 1000.0 / elapsed_time;
      snprintf(fps_buffer, sizeof(fps_buffer), "gba-emu | FPS: %.1f", fps);
      SDL_SetWindowTitle(window, fps_buffer);
      frame_count = 0;
      last_time = frame_start_time;
    }
  }

shutdown:
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  gba_free(gba);
  free(gba);

  return 0;
}
