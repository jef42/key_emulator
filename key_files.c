#include "key_files.h"

#include <stdlib.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#define BUFF_LENGTH 255
static char buff[BUFF_LENGTH] = "/dev/input/event";

static const char* find_event_file(const char *file_name)
{
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

int get_event_fd(const char *file_name)
{
    const char *file = find_event_file(file_name);
    return open(file, O_RDONLY);
}

int get_uinput_fd(const char* file) 
{
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

void close_uinput(int fd)
{
    ioctl(fd, UI_DEV_DESTROY);
    close(fd);
}

void close_event(int fd)
{
    close(fd);
}
