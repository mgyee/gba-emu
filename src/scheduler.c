#include "scheduler.h"

void scheduler_init(Scheduler *scheduler) {
  scheduler->head = NULL;
  scheduler->current_time = 0;
}

void scheduler_push_event_ctx(Scheduler *scheduler, EventType type,
                              uint time_from_now, void *ctx) {
  Event *new_event = (Event *)malloc(sizeof(Event));
  new_event->type = type;
  new_event->scheduled_time = scheduler->current_time + time_from_now;
  new_event->ctx = ctx;
  new_event->next = NULL;

  if (scheduler->head == NULL ||
      (int)(scheduler->head->scheduled_time - new_event->scheduled_time) > 0) {
    new_event->next = scheduler->head;
    scheduler->head = new_event;
  } else {
    Event *current = scheduler->head;
    while (current->next != NULL && (int)(new_event->scheduled_time -
                                          current->next->scheduled_time) >= 0) {
      current = current->next;
    }
    new_event->next = current->next;
    current->next = new_event;
  }
}

void scheduler_push_event(Scheduler *scheduler, EventType type,
                          uint time_from_now) {
  scheduler_push_event_ctx(scheduler, type, time_from_now, NULL);
}

Event *scheduler_pop_event(Scheduler *scheduler) {
  if (scheduler->head == NULL) {
    return NULL;
  }
  Event *event = scheduler->head;
  scheduler->head = scheduler->head->next;
  return event;
}

void scheduler_cancel_event(Scheduler *scheduler, EventType type, void *ctx) {
  Event *current = scheduler->head;
  Event *prev = NULL;

  while (current != NULL) {
    if (current->type == type && current->ctx == ctx) {
      if (prev == NULL) {
        scheduler->head = current->next;
      } else {
        prev->next = current->next;
      }
      free(current);
      return;
    }
    prev = current;
    current = current->next;
  }
}

uint scheduler_peek_next_event_time(Scheduler *scheduler) {
  if (scheduler->head == NULL) {
    return -1;
  }
  return scheduler->head->scheduled_time;
}

void scheduler_step(Scheduler *scheduler, uint cycles) {
  scheduler->current_time += cycles;
}
