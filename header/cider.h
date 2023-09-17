#ifndef _CIDER

#define _CIDER

#include <stdlib.h>

typedef struct cider Cider;
// typedef void (*AsyncFuncion)(size_t argc, void* argv[]); に直す
typedef void (*AsyncFuncion)(size_t argc, void* argv);

int cider_init();

Cider* async(AsyncFuncion const, size_t, void*);
void await(Cider* const);

void async_sleep(time_t);
void join_ciders(Cider* const* const, size_t);

#endif
