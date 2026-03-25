#ifndef OS_H
#define OS_H

#include <stddef.h>

typedef long os_time_t;

struct os_time {
    os_time_t sec;
    os_time_t usec;
};

void os_sleep(os_time_t sec, os_time_t usec);
int os_get_time(struct os_time *t);
char * os_strdup(const char *s);
size_t os_strlen(const char *s);
int os_strcmp(const char *a, const char *b);
void os_free(void *ptr);
void * os_malloc(size_t size);
void * os_realloc(void *ptr, size_t size);

#endif /* OS_H */
