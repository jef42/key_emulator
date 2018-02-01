#include "key_event_handler.h"
#include <stdio.h>
#include <linux/input.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define BOOL char
#define TRUE 1
#define FALSE 0

// TODO - update this table, make it on multiple columns
// to be easier to understand
char ascii_codes[][2] = {
/*NUL*/ {0, 0},                          /*SOH*/ {0, 0},
/*STX*/ {0, 0},                          /*ETX*/ {0, 0},
/*EOT*/ {0, 0},                          /*ENQ*/ {0, 0},
/*ACK*/ {0, 0},                          /*BEL*/ {0, 0},
/*BS */ {0, 0},                          /*TAB*/ {0, 0},
/*LF */ {KEY_ENTER, 0},                  /*VT */ {0, 0},
/*FF */ {0, 0},                          /*CR */ {KEY_ENTER, 0},
/*SO */ {0, 0},                          /*SI */ {0, 0},
/*DLE*/ {0, 0},                          /*DC1*/ {0, 0},
/*DC2*/ {0, 0},                          /*DC3*/ {0, 0},
/*DC4*/ {0, 0},                          /*NAK*/ {0, 0},
/*SYN*/ {0, 0},                          /*ETB*/ {0, 0},
/*CAN*/ {0, 0},                          /*EM */ {0, 0},
/*SUB*/ {0, 0},                          /*ESC*/ {0, 0},
/*FS */ {0, 0},                          /*GS */ {0, 0},
/*RS */ {0, 0},                          /*US */ {0, 0},
/*SPC*/ {KEY_SPACE, 0},                  /*!  */ {KEY_LEFTSHIFT, KEY_1},
/*"  */ {KEY_LEFTSHIFT, KEY_APOSTROPHE}, /*#  */ {KEY_LEFTSHIFT, KEY_3},
/*$  */ {KEY_LEFTSHIFT, KEY_4},          /*%  */ {KEY_LEFTSHIFT, KEY_5},
/*&  */ {KEY_LEFTSHIFT, KEY_7},          /*'  */ {KEY_APOSTROPHE, 0},
/*(  */ {KEY_LEFTSHIFT, KEY_9},          /*)  */ {KEY_LEFTSHIFT, KEY_0},
/**  */ {KEY_LEFTSHIFT, KEY_8},          /*+  */ {KEY_LEFTSHIFT, KEY_EQUAL},
/*,  */ {KEY_COMMA, 0},                  /*-  */ {KEY_MINUS, 0},
/*.  */ {KEY_DOT, 0},                    /*/  */ {KEY_SLASH, 0},
/*0  */ {KEY_0, 0},                      /*1  */ {KEY_1, 0},
/*2  */ {KEY_2, 0},                      /*3  */ {KEY_3, 0},
/*4  */ {KEY_4, 0},                      /*5  */ {KEY_5, 0},
/*6  */ {KEY_6, 0},                      /*7  */ {KEY_7, 0},
/*7  */ {KEY_7, 0},                      /*8  */ {KEY_8, 0},
/*9  */ {KEY_9, 0},                      /*:  */ {KEY_LEFTSHIFT, KEY_SEMICOLON},
/*;  */ {KEY_SEMICOLON, 0},              /*<  */ {KEY_LEFTSHIFT, KEY_COMMA},
/*=  */ {KEY_EQUAL, 0},                  /*>  */ {KEY_LEFTSHIFT, KEY_DOT},
/*?  */ {KEY_LEFTSHIFT, KEY_SLASH},      /*@  */ {KEY_LEFTSHIFT, KEY_2},
/*A  */ {KEY_LEFTSHIFT, KEY_A},          /*B  */ {KEY_LEFTSHIFT, KEY_B},
/*C  */ {KEY_LEFTSHIFT, KEY_C},          /*D  */ {KEY_LEFTSHIFT, KEY_D},
/*E  */ {KEY_LEFTSHIFT, KEY_E},          /*F  */ {KEY_LEFTSHIFT, KEY_F},
/*G  */ {KEY_LEFTSHIFT, KEY_G},          /*H  */ {KEY_LEFTSHIFT, KEY_H},
/*I  */ {KEY_LEFTSHIFT, KEY_I},          /*J  */ {KEY_LEFTSHIFT, KEY_J},
/*K  */ {KEY_LEFTSHIFT, KEY_K},          /*L  */ {KEY_LEFTSHIFT, KEY_L},
/*M  */ {KEY_LEFTSHIFT, KEY_M},          /*N  */ {KEY_LEFTSHIFT, KEY_N},
/*O  */ {KEY_LEFTSHIFT, KEY_O},          /*P  */ {KEY_LEFTSHIFT, KEY_P},
/*Q  */ {KEY_LEFTSHIFT, KEY_Q},          /*R  */ {KEY_LEFTSHIFT, KEY_R},
/*S  */ {KEY_LEFTSHIFT, KEY_S},          /*T  */ {KEY_LEFTSHIFT, KEY_T},
/*U  */ {KEY_LEFTSHIFT, KEY_U},          /*V  */ {KEY_LEFTSHIFT, KEY_V},
/*W  */ {KEY_LEFTSHIFT, KEY_W},          /*X  */ {KEY_LEFTSHIFT, KEY_X},
/*Y  */ {KEY_LEFTSHIFT, KEY_Y},          /*Z  */ {KEY_LEFTSHIFT, KEY_Z},
/*[  */ {KEY_LEFTBRACE, 0},              /*\  */ {KEY_BACKSLASH, 0},
/*]  */ {KEY_RIGHTBRACE, 0},             /*^  */ {KEY_LEFTSHIFT, KEY_6},
/*_  */ {KEY_LEFTSHIFT, KEY_MINUS},      /*A  */ {KEY_A, 0},
/*B  */ {KEY_B, 0},                      /*C  */ {KEY_C, 0},
/*D  */ {KEY_D, 0},                      /*E  */ {KEY_E, 0},
/*F  */ {KEY_F, 0},                      /*G  */ {KEY_G, 0},
/*H  */ {KEY_H, 0},                      /*I  */ {KEY_I, 0},
/*J  */ {KEY_J, 0},                      /*K  */ {KEY_K, 0},
/*L  */ {KEY_L, 0},                      /*M  */ {KEY_M, 0},
/*N  */ {KEY_N, 0},                      /*O  */ {KEY_O, 0},
/*P  */ {KEY_P, 0},                      /*Q  */ {KEY_Q, 0},
/*R  */ {KEY_R, 0},                      /*S  */ {KEY_S, 0},
/*T  */ {KEY_T, 0},                      /*T  */ {KEY_T, 0},
/*U  */ {KEY_U, 0},                      /*V  */ {KEY_V, 0},
/*W  */ {KEY_W, 0},                      /*X  */ {KEY_X, 0},
/*Y  */ {KEY_Y, 0},                      /*Z  */ {KEY_Z, 0},
/*{  */ {KEY_LEFTSHIFT, KEY_LEFTBRACE},  /*|  */ {KEY_LEFTSHIFT, KEY_BACKSLASH},
/*}  */ {KEY_LEFTSHIFT, KEY_RIGHTBRACE}, /*~  */ {KEY_LEFTSHIFT, KEY_GRAVE}
};

static void emit(int fd, int code, int val)
{
    struct input_event ev;
    memset(&ev, 0, sizeof(ev));

    ev.type = EV_KEY;
    ev.code = code;
    ev.value = val;
    write(fd, &ev, sizeof(ev));

    memset(&ev, 0, sizeof(ev));
    ev.type = EV_SYN;
    ev.code = 0;
    ev.value = 0;
    write(fd, &ev, sizeof(ev));
}

void insert_key(int fd, key_event_handler_t *event)
{
    int i = 0;

    // release the keys
    for (i = 0; i < event->key_entries_size; i++)
        emit(fd, event->key_entries[i], 0);

    // press the new key
    emit(fd, *((int*)event->data_ptr), 1);
    emit(fd, *((int*)event->data_ptr), 0);

    // press again the keys
    for (i = 0; i < event->key_entries_size; i++)
        emit(fd, event->key_entries[i], 1);
}

void press_keys(int fd, char *keys, size_t size) {
    int i = 0;

    // press all keys
    // example when pressing A,
    // first press shift, then press a
    // then release them in reverse order
    for (i = 0; i < size; ++i)
        if (keys[i] != 0)
            emit(fd, keys[i], 1);

    for (i = size-1; i >= 0; --i)
        if (keys[i] != 0)
            emit(fd, keys[i], 0);
}

void insert_text(int fd, key_event_handler_t *event)
{
    int i = 0;
    char *text = (char *)(event->data_ptr);
    if (text == NULL)
        return;

    // release the keys
    for (i = 0; i < event->key_entries_size; i++)
        emit(fd, event->key_entries[i], 0);

    for (i = 0; i < strlen(text); i++) {
        char *keys = ascii_codes[text[i]];
        press_keys(fd, keys, 2);
    }

    // press again the keys
    for (i = 0; i < event->key_entries_size; i++)
        emit(fd, event->key_entries[i], 1);
}

void run_script(int fd, key_event_handler_t *event)
{
    char *script_name = NULL;
    pid_t pid = -1;

    script_name = (char*)(event->data_ptr);
    if (script_name == NULL)
        return;

    switch(pid = fork()) {
    case -1:
        // error creating a new child
        // that's bad
        return;
    case 0:
        execlp(script_name, script_name, NULL);
        exit(-1);
        // this should not be reached
        return;
    default:
        // this process is ignoring signals
        // from the child so the child automatically
        // will be removed when finished. So
        // happy zombie
        ;
    }
}

void add_key_event(key_event_handler_t **events, key_event_handler_t *event)
{
    event->next = *events;
    *events = event;
}

void clear_events(key_event_handler_t **events)
{
    while (*events != NULL) {
        key_event_handler_t *tmp = *events;
        free(tmp->data_ptr);
        *events = tmp->next;
        free(tmp);
    }
}

// all the key_entries in the event_handler needs to be 
// pressed for the event to be valid
static BOOL valid_event(key_event_handler_t *event,
                        int *key_events, size_t len)
{
    size_t i;
    for (i = 0; i < event->key_entries_size; i++) {
        if (key_events[event->key_entries[i]] != 1) {
            return FALSE;
        }
    }
    return TRUE;
}

void notify_handlers(int fd, key_event_handler_t *events,
                     int *key_events, size_t len)
{
    while (events != NULL) {
        if (valid_event(events, key_events, len))
            events->event_action(fd, events);
        events = events->next;
    }
}
