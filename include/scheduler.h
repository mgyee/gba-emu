#pragma once
#include "common.h"

typedef enum {
  EVENT_TYPE_FRAME_END,
  EVENT_TYPE_HBLANK_START,
  EVENT_TYPE_HBLANK_END,
  EVENT_TYPE_VBLANK_HBLANK_START,
  EVENT_TYPE_VBLANK_HBLANK_END,
  EVENT_TYPE_TIMER_OVERFLOW,
  EVENT_TYPE_DMA_ACTIVATE,
} EventType;

typedef struct Event {
  EventType type;
  uint scheduled_time;
  void *ctx;
  struct Event *next;
} Event;

typedef struct {
  Event *head;
  uint current_time;
} Scheduler;

void scheduler_init(Scheduler *scheduler);

void scheduler_push_event(Scheduler *scheduler, EventType type,
                          uint time_from_now);

void scheduler_push_event_ctx(Scheduler *scheduler, EventType type,
                              uint time_from_now, void *ctx);

Event *scheduler_pop_event(Scheduler *scheduler);

void scheduler_cancel_event(Scheduler *scheduler, EventType type, void *ctx);

uint scheduler_peek_next_event_time(Scheduler *scheduler);

void scheduler_step(Scheduler *scheduler, uint cycles);
