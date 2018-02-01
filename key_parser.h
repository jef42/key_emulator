#ifndef __KEY_PARSER_H__
#define __KEY_PARSER_H__

#include "key_event_handler.h"

#define BOOL char
#define TRUE 1
#define FALSE 0

// functions to parse/read the file
// these will fill up the events
BOOL read_entries_keys(const char *file, key_event_handler_t **events);
BOOL read_entries_text(const char *file, key_event_handler_t **events);
BOOL read_entries_script(const char *file, key_event_handler_t **events);

#endif

