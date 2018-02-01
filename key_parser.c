#include "key_parser.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// returns number of digits in the key
int key_length(int key)
{
    int ret = 0;
    while (key > 0) {
        key = key / 10;
        ret++;
    }
    return ret;
}

// parse int
BOOL parse_int(char **line, key_event_handler_t *event)
{
    int key;
    int ret;
    if (*line == NULL)
        return FALSE;

    ret = sscanf(*line, "%d", &key);
    if (ret == 0 || ret == -1)
        return FALSE;

    event->key_entries[event->key_entries_size] = key;
    event->key_entries_size++;

    // update to point to the next symbol
    *line = &(*line)[key_length(key)+1];
    return TRUE;
}

// parse string
char* parse_string(char **line)
{
    char *res = NULL;
    if (*line == NULL)
        return NULL;
    res = (char*)calloc(1, strlen(*line)+1);
    memcpy(res, *line, strlen(*line));
    // remove the new line
    res[strlen(*line)-1] = '\0';
    return res;
}

#define IGNORE_COMMENTS(X)  \
        if (ignore_line(X)) \
            continue;

BOOL ignore_line(char *line)
{
    if (line == NULL)
        return TRUE;
    if (strlen(line) < 2)
        return TRUE;
    if (line[0] == '/' && line[1] == '/')
        return TRUE;
    return FALSE;
}

BOOL read_entries_keys(const char *file, key_event_handler_t **events)
{
    size_t len;
    ssize_t read;
    char *line = NULL;
    FILE *fd = fopen(file, "r");

    if (fd == NULL)
        return FALSE;

    while ((read = getline(&line, &len, fd)) != -1) {
        IGNORE_COMMENTS(line);

        // we can't change the line because we need
        // to clear it
        char *tmp = line;
        key_event_handler_t *event = (key_event_handler_t*)
                        calloc(1, sizeof(key_event_handler_t));
        event->event_action = insert_key;
        while (parse_int(&tmp, event) == TRUE);
        // the key that needs to be emulated is added at the
        // end of key_entries so we need to substract one entry
        event->data_ptr = calloc(1, sizeof(int));
        *((int*)event->data_ptr) = 
                event->key_entries[event->key_entries_size-1];
        //event->data_ptr = &event->key_entries[event->key_entries_size-1];
        event->key_entries_size--;
        add_key_event(events, event);

        // clean line
        free(line);
        line = NULL;
    }
    fclose(fd);
    return TRUE;
}

BOOL read_entries_text(const char *file, key_event_handler_t **events)
{
    size_t len = 0;
    ssize_t read = 0;
    char *line = NULL;
    FILE *fd = fopen(file, "r");

    if (fd == NULL)
        return FALSE;

    while ((read = getline(&line, &len, fd)) != -1) {
        IGNORE_COMMENTS(line);

        // we can't change the line because we need
        // to clear it;
        char *tmp = line;
        key_event_handler_t *event = (key_event_handler_t*)
                calloc(1, sizeof(key_event_handler_t));
        event->event_action = insert_text;
        // parse the keys
        while (parse_int(&tmp, event) == TRUE);
        // parse string(text)
        event->data_ptr = parse_string(&tmp);
        add_key_event(events, event);

        // clean line
        free(line);
        line = NULL;
    }
    fclose(fd);
    return TRUE;
}

BOOL read_entries_script(const char *file, key_event_handler_t **events)
{
    size_t len = 0;
    ssize_t read = 0;
    char *line = NULL;
    FILE *fd = fopen(file, "r");

    if (fd == NULL)
        return FALSE;

    while ((read = getline(&line, &len, fd)) != -1) {
        IGNORE_COMMENTS(line);

        // we can't change the line because we need
        // to clear it
        char *tmp = line;
        key_event_handler_t *event = (key_event_handler_t*)
                    calloc(1, sizeof(key_event_handler_t));
        event->event_action = run_script;
        // parse the keys
        while (parse_int(&tmp, event) == TRUE);
        // parse string(file name)
        event->data_ptr = parse_string(&tmp);
        add_key_event(events, event);

        // clean line
        free(line);
        line = NULL;
    }
    fclose(fd);
    return TRUE;
}
