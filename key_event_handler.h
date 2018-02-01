#ifndef __KEY_EVENT_HANDLER_H__
#define __KEY_EVENT_HANDLER_H__

#include <stdlib.h>

#define KEY_ENTRIES_SIZE 5
#define KEY_HOLD 2
#define KEY_PRESS 1
#define KEY_RELEASE 0

struct key_event_handler;

typedef void (*event_action_t)(int fd,
                               struct key_event_handler *event);

typedef struct key_event_handler {
    size_t key_entries_size; // the number of entries in key_entries
    int key_entries[KEY_ENTRIES_SIZE]; // keys that need to be pressed, for
                                       // action to take place

    event_action_t event_action; // function_ptr to action function
    void *data_ptr; // data that is sent to action function

    struct key_event_handler *next; // it is part of an list
} key_event_handler_t;

// action functions
void insert_key(int fd, key_event_handler_t *event);
void insert_text(int fd, key_event_handler_t *event);
void run_script(int fd, key_event_handler_t *event);

// check if any of the handlers needs to take any action
void notify_handlers(int fd, key_event_handler_t *events,
                     int *key_events, size_t len);

// add new handler
void add_key_event(key_event_handler_t **events, key_event_handler_t *event);
void clear_events(key_event_handler_t **events);
#endif
