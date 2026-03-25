#include "os.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

void os_sleep(os_time_t sec, os_time_t usec)
{
    struct timeval tv;
    tv.tv_sec = sec;
    tv.tv_usec = usec;
    select(0, NULL, NULL, NULL, &tv);
}

int os_get_time(struct os_time *t)
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL) < 0)
        return -1;
    t->sec = tv.tv_sec;
    t->usec = tv.tv_usec;
    return 0;
}

char * os_strdup(const char *s)
{
    return strdup(s);
}

size_t os_strlen(const char *s)
{
    return strlen(s);
}

int os_strcmp(const char *a, const char *b)
{
    return strcmp(a, b);
}

void os_free(void *ptr)
{
    free(ptr);
}

void * os_malloc(size_t size)
{
    return malloc(size);
}

void * os_realloc(void *ptr, size_t size)
{
    return realloc(ptr, size);
}
