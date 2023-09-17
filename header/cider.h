#ifndef _CIDER

#define _CIDER

#include <stddef.h>

typedef struct cider Cider;
typedef void (*AsyncFuncion)(size_t argc, void* argv[]);

int cider_init(void);

Cider* async(AsyncFuncion const, size_t, void*);
void await(Cider* const);

void async_sleep(long);
void join_ciders(Cider* const* const, size_t);

#endif
