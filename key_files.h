#ifndef __KEY_FILES_H__
#define __KEY_FILES_H__

int get_event_fd(const char *file_name);
int get_uinput_fd(const char* file);
void close_event(int fd);
void close_uinput(int fd);

#endif
