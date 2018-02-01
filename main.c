#include <stdio.h>
#include <stdlib.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include "key_event_handler.h"
#include "key_parser.h"
#include "key_files.h"

static key_event_handler_t *events = NULL;
static int event_fd;
static int uinput_fd;

static BOOL enable = 1;

#define KEY_EVENTS_LENGTH 200
static int key_events[KEY_EVENTS_LENGTH];

void update_current_keys(struct input_event event)
{
    key_events[event.code] = event.value == KEY_RELEASE ? 0 : 1;
}

void load_entries()
{
    read_entries_keys("/home/oroles/Programming/C/keyboard/entries_keys", &events);
    read_entries_text("/home/oroles/Programming/C/keyboard/entries_text", &events);
    read_entries_script("/home/oroles/Programming/C/keyboard/entries_script", &events);
}

void update_enable()
{
    if (key_events[KEY_LEFTSHIFT] == 1 &&
        key_events[KEY_LEFTCTRL] == 1 &&
        key_events[KEY_TAB] == 1) {
        enable = !enable;
        if (enable) {
            clear_events(&events);
            load_entries();
        }
    }
}

BOOL is_enable()
{
    return enable == TRUE;
}

int main(int argc, char *argv[])
{
    event_fd = get_event_fd("/proc/bus/input/devices");
    if (event_fd == -1) {
        fprintf(stderr, "Error event_fd\n");
        return -1;
    }

    uinput_fd = get_uinput_fd("/dev/uinput");
    if (uinput_fd == -1) {
        fprintf(stderr, "Error uinput_fd\n");
        return -1;
    }

    load_entries();
    signal(SIGCHLD, SIG_IGN);

    // beauty sleep
    printf("Before sleep\n");
    sleep(3);
    printf("After sleep\n");

    struct input_event event;
    while(read(event_fd, &event, sizeof(event))>0) {
        if (event.type == EV_KEY) {
            update_current_keys(event);

            // check if it is enabled
            // enable combination is hardcoded
            update_enable();
            if (!is_enable())
                continue;

            if (event.value == KEY_PRESS ||
                event.value == KEY_HOLD)
                printf("Code: %d\n", event.code);
                notify_handlers(uinput_fd, events,
                                key_events, KEY_EVENTS_LENGTH);
        }
    }

    close_uinput(uinput_fd);
    close_event(event_fd);
    return 0;
}
