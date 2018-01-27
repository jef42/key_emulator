#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define KEY_HOLD 2
#define KEY_PRESS 1
#define KEY_RELEASE 0
#define BUFF_LENGTH 255

static const char *uinput_file_path = "/dev/uinput";
static char buff[BUFF_LENGTH] = "/dev/input/event";

static char enabled = 1;
static char shift_pressed = 0;
static char ctrl_pressed = 0;

int uinput_fd;
int event_fd;

const char* find_event_file(const char *file_name) {
    size_t len = 0;
    ssize_t read;
    char *line;
    FILE *fd = fopen(file_name, "r");
    char tmp[BUFF_LENGTH];
    memset(tmp, 0, BUFF_LENGTH);

    while((read = getline(&line, &len, fd)) != -1) {
        if (memcmp(line, "H: Handlers=", 12) == 0) {
            memcpy(tmp, line, read);
        } else {
            if (memcmp(line, "B: EV=", 6) == 0) {
                if (memcmp(line, "B: EV=120013", 12) == 0) {
                    memcpy(buff, tmp, BUFF_LENGTH);
                } else {
                    memset(tmp, 0, BUFF_LENGTH);
                }
            }
        }
    }
    memcpy(tmp, buff, BUFF_LENGTH);
    memset(buff, 0, BUFF_LENGTH);
    strcpy(buff, "/dev/input/event");
    buff[16] = tmp[27];

    fclose(fd);
    return buff;
}

void emit(int fd, int code, int val) {
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

int uinput_init(const char* file) {
    struct uinput_user_dev usetup;
    int fd_uinput = open(file, O_WRONLY | O_NONBLOCK | O_NDELAY);
    if (fd_uinput < 0) {
        printf("Error %s\n", strerror(errno));
        return -1;
    }

    ioctl(fd_uinput, UI_SET_EVBIT, EV_KEY);
    for (int i = 0; i < 254; ++i) {
        ioctl(fd_uinput, UI_SET_KEYBIT, i);
    }
    ioctl(fd_uinput, UI_SET_EVBIT, EV_SYN);

    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x1234;
    usetup.id.product = 0x5678;
    usetup.id.version = 1;
    strcpy(usetup.name, "Test1");
    write(fd_uinput, &usetup, sizeof(usetup));
    ioctl(fd_uinput, UI_DEV_CREATE);
    return fd_uinput;
}

void uinput_close(int fd) {
    ioctl(fd, UI_DEV_DESTROY);
    close(fd);
}

void press_while_shift_key(int fd, int key) {
    emit(fd, KEY_LEFTSHIFT, 0);
    emit(fd, key, 1);
    emit(fd, key, 0);
    emit(fd, KEY_LEFTSHIFT, 1);
}

void press_while_ctrl_shift_key(int fd, int key) {
    emit(fd, KEY_LEFTSHIFT, 0);
    emit(fd, KEY_LEFTCTRL, 0);
    emit(fd, key, 1);
    emit(fd, key, 0);
    emit(fd, KEY_LEFTCTRL, 1);
    emit(fd, KEY_LEFTSHIFT, 1);
}

struct event_handler {
    // event value, event code
    char (*check_condition)(int, int);
    void (*execute)(void);
};

char is_shift_press(int value, int code) {
    return ((code == KEY_LEFTSHIFT ||
             code == KEY_RIGHTSHIFT) &&
            (value == KEY_PRESS));
}

void execute_shift_press(void) {
    shift_pressed = 1;
}

char is_shift_release(int value, int code) {
    return ((code == KEY_LEFTSHIFT ||
             code == KEY_RIGHTSHIFT) &&
            (value == KEY_RELEASE));
}

void execute_shift_release(void) {
    shift_pressed = 0;
}

char is_ctrl_press(int value, int code) {
    return ((code == KEY_LEFTCTRL ||
             code == KEY_RIGHTCTRL) &&
            (value == KEY_PRESS));
}

void execute_ctrl_press(void) {
    ctrl_pressed = 1;
}

char is_ctrl_release(int value, int code) {
    return ((code == KEY_LEFTCTRL ||
             code == KEY_RIGHTCTRL) &&
            (value == KEY_RELEASE));
}

char is_enable(int value, int code) {
    return (code == KEY_TAB &&
             value == KEY_PRESS &&
             ctrl_pressed == 1 &&
             shift_pressed == 1);
}

void execute_enable(void) {
    enabled = !enabled;
}

void execute_ctrl_release(void) {
    ctrl_pressed = 0;
}

char is_page_up(int value, int code) {
    if (value != KEY_PRESS && value != KEY_HOLD)
        return 0;
    return code == KEY_DELETE &&
            enabled &&
            shift_pressed &&
            !ctrl_pressed;
}

void execute_page_up(void) {
    press_while_shift_key(uinput_fd, KEY_PAGEUP);
}

char is_page_down(int value, int code) {
    if (value != KEY_PRESS && value != KEY_HOLD)
        return 0;
    return code == KEY_PAUSE &&
            enabled &&
            shift_pressed &&
            !ctrl_pressed;
}

void execute_page_down(void) {
    press_while_shift_key(uinput_fd, KEY_PAGEDOWN);
}

char is_home(int value, int code) {
    if (value != KEY_PRESS && value != KEY_HOLD)
        return 0;
    return code == KEY_PAUSE &&
            enabled &&
            shift_pressed &&
            ctrl_pressed;
}

void execute_home(void) {
    press_while_ctrl_shift_key(uinput_fd, KEY_HOME);
}

char is_end(int value, int code) {
    if (value != KEY_PRESS && value != KEY_HOLD)
        return 0;
    return code == KEY_DELETE &&
            enabled &&
            shift_pressed &&
            ctrl_pressed;
}

void execute_end(void) {
    press_while_ctrl_shift_key(uinput_fd, KEY_END);
}

struct event_handler handles[] = {
    {is_shift_press, execute_shift_press},
    {is_shift_release, execute_shift_release},
    {is_ctrl_press, execute_ctrl_press},
    {is_ctrl_release, execute_ctrl_release},
    {is_enable, execute_enable},
    {is_page_up, execute_page_up},
    {is_page_down, execute_page_down},
    {is_home, execute_home},
    {is_end, execute_end},
};

int main(int argc, char* argv[]) {
    const char *path = find_event_file("/proc/bus/input/devices");
    event_fd = open(path, O_RDONLY);
    if (event_fd == -1) {
        printf("Error event_fd\n");
        return -1;
    }
    uinput_fd = uinput_init(uinput_file_path);
    if (uinput_fd == -1) {
        printf("Error uinput_fd\n");
        return -1;
    }
    sleep(3);

    struct input_event event;
    while (read(event_fd, &event, sizeof(struct input_event)) > 0) {
        if (event.type == EV_KEY) {
            for(int i = 0; i < sizeof(handles) / sizeof(struct event_handler); i++){
                if (handles[i].check_condition(event.value, event.code)) {
                    handles[i].execute();
                }
            }
        }
    }
    uinput_close(uinput_fd);
    return 0;
}
