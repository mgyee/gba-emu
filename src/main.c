#include "common.h"
#include "cpu.h"
#include "gba.h"
#include "interrupt.h"
#include "io.h"
#include "keypad.h"
#include "ppu.h"
#include "scheduler.h"
#include <SDL.h>
#include <stdint.h>

bool turbo = false;

bool handle_input(Gba *gba) {
  SDL_Event event;
  while (SDL_PollEvent(&event) != 0) {
    switch (event.type) {
    case SDL_QUIT:
      return true;
    case SDL_KEYDOWN:
      switch (event.key.keysym.sym) {
      case SDLK_TAB:
        turbo = true;
        break;
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
      case SDLK_BACKSPACE:
        gba->keypad.keyinput &= ~(1 << BUTTON_SELECT);
        break;
      }
      break;
    case SDL_KEYUP:
      switch (event.key.keysym.sym) {
      case SDLK_TAB:
        turbo = false;
        break;
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
      case SDLK_BACKSPACE:
        gba->keypad.keyinput |= (1 << BUTTON_SELECT);
        break;
      }
      break;
    }
  }
  return false;
}

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

  int total_cycles = 0;
  while (true) {

    if (handle_input(gba)) {
      goto shutdown;
    }

    Scheduler *scheduler = &gba->scheduler;

    uint start_time = scheduler->current_time;
    scheduler_push_event(scheduler, EVENT_TYPE_FRAME_END,
                         CYCLES_PER_FRAME - total_cycles);

    bool frame_done = false;

    while (!frame_done) {

      while ((int)(scheduler->current_time -
                   scheduler_peek_next_event_time(scheduler)) >= 0) {

        Event *event = scheduler_pop_event(scheduler);
        if (!event) {
          break;
        }
        uint lateness = scheduler->current_time - event->scheduled_time;
        switch (event->type) {
        case EVENT_TYPE_FRAME_END:
          frame_done = true;
          break;

        case EVENT_TYPE_HBLANK_START:
          ppu_hblank_start(gba, lateness);
          break;
        case EVENT_TYPE_HBLANK_END:
          ppu_hblank_end(gba, lateness);
          break;
        case EVENT_TYPE_VBLANK_HBLANK_START:
          ppu_vblank_hblank_start(gba, lateness);
          break;
        case EVENT_TYPE_VBLANK_HBLANK_END:
          ppu_vblank_hblank_end(gba, lateness);
          break;
        case EVENT_TYPE_TIMER_OVERFLOW:
          timer_overflow(gba, (int)(intptr_t)event->ctx, lateness);
          break;
        case EVENT_TYPE_DMA_ACTIVATE:
          dma_transfer(gba, (int)(intptr_t)event->ctx);
          break;
        case EVENT_TYPE_IRQ:
          handle_interrupts(gba);
          break;
        }
        free(event);
        if (frame_done) {
          break;
        }
      }

      if (gba->io.power_state == POWER_STATE_HALTED) {
        int next_event_time = scheduler_peek_next_event_time(scheduler);
        scheduler_step(scheduler, next_event_time - scheduler->current_time);
      } else {
        cpu_step(gba);
      }
    }

    total_cycles += scheduler->current_time - start_time;

    total_cycles -= CYCLES_PER_FRAME;

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_UpdateTexture(texture, NULL, gba->ppu.framebuffer, 240 * sizeof(u32));
    SDL_RenderCopy(renderer, texture, NULL, NULL);

    SDL_RenderPresent(renderer);

    Uint32 frame_time = SDL_GetTicks() - frame_start_time;
    if (!turbo && frame_time < FRAME_TIME_MS) {
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
