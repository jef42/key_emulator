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

#define KEY_PRESS 1
#define KEY_RELEASE 0
#define BUFF_LENGTH 255

static const char *uinput_file_path = "/dev/uinput";
static char shift_pressed = 0;
static char ctrl_pressed = 0;

static char buff[BUFF_LENGTH] = "/dev/input/event";

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

char is_shift(short code) {
    return code == KEY_LEFTSHIFT ||
           code == KEY_RIGHTSHIFT;
}

char is_ctrl(short code) {
    return code == KEY_LEFTCTRL ||
           code == KEY_RIGHTCTRL;
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

int main(int argc, char* argv[]) {
    const char *path = find_event_file("/proc/bus/input/devices");
    int event_fd = open(path, O_RDONLY);
    if (event_fd == -1) {
        printf("Error event_fd\n");
        return -1;
    }
    int uinput_fd = uinput_init(uinput_file_path);
    if (uinput_fd == -1) {
        printf("Error uinput_fd\n");
        return -1;
    }
    printf("Sleep for device\n");
    sleep(3);

    struct input_event event;
    while (read(event_fd, &event, sizeof(struct input_event)) > 0) {
        if (event.type == EV_KEY) {
            if (event.value == KEY_PRESS) {
                if (is_shift(event.code)) {
                    shift_pressed = 1;
                }
                if (is_ctrl(event.code)) {
                    ctrl_pressed = 1;
                }
                if (event.code == KEY_DELETE &&
                    shift_pressed == 1 &&
                    ctrl_pressed == 1) {
                    press_while_ctrl_shift_key(uinput_fd, KEY_END);
                }
                if (event.code == KEY_DELETE &&
                    shift_pressed == 1 &&
                    ctrl_pressed == 0) {
                    press_while_shift_key(uinput_fd, KEY_PAGEUP);
                }
                if (event.code == KEY_PAUSE &&
                    shift_pressed == 1 &&
                    ctrl_pressed == 1) {
                    press_while_ctrl_shift_key(uinput_fd, KEY_HOME);
                }
                if (event.code == KEY_PAUSE &&
                    shift_pressed == 1 &&
                    ctrl_pressed == 0) {
                    press_while_shift_key(uinput_fd, KEY_PAGEDOWN);
                }
            }
            if (event.value == KEY_RELEASE) {
                if (is_shift(event.code)) {
                    shift_pressed = 0;
                }
                if (is_ctrl(event.code)) {
                    ctrl_pressed = 0;
                }
            }
        }
    }
    uinput_close(uinput_fd);
    return 0;
}
