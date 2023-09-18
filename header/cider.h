#ifndef _CIDER

#define _CIDER

#include <stddef.h>

typedef struct cider Cider;
typedef void (*AsyncFuncion)(size_t argc, void* argv[]);

int cider_init(void);

Cider* async(AsyncFuncion const, size_t, void*);
void await(Cider* const);

void await_sleep(long);
void join_cider_array(Cider* const* const, size_t);

#define join_ciders(...)                                   \
    do {                                                   \
        Cider* cs[] = {__VA_ARGS__};                       \
        join_cider_array(cs, sizeof(cs) / sizeof(Cider*)); \
    } while (0)

#endif
