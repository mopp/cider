#include "../header/cider.h"
#include "../lib/log.c/src/log.h"
#include <assert.h>
#include <stdio.h>

static int test_storage[3] = {0};

static void func(size_t argc, void* argv[]) {
    log_debug("func: started. number");

    async_sleep(50);
    test_storage[argc] = 100 + argc;

    log_debug("func: returning");
}

int main(void) {
    log_info("Begin.");

    cider_init();

    Cider* const c0 = async(func, 0, NULL);
    Cider* const c1 = async(func, 1, NULL);
    Cider* const c2 = async(func, 2, NULL);

    await(c0);
    await(c1);
    await(c2);

    assert(test_storage[0] == 100);
    assert(test_storage[1] == 101);
    assert(test_storage[2] == 102);

    log_info("Succeeded.");

    return 0;
}
