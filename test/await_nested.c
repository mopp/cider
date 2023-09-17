#include "../header/cider.h"
#include "../lib/log.c/src/log.h"
#include <assert.h>
#include <stdio.h>

static int test_storage[3] = {0};

static void func1(size_t argc, void* argv[]) {
    log_debug("func1: started.");

    assert(test_storage[0] == 100);

    async_sleep(50);
    test_storage[1] = 101;

    log_debug("func1: returning");
}

static void func2(size_t argc, void* argv[]) {
    log_debug("func2: started.");

    test_storage[0] = 100;

    Cider* const c = async(func1, 0, NULL);
    await(c);
    test_storage[2] = 102;

    log_debug("func2: returning");
}

int main(void) {
    log_info("Begin.");

    cider_init();

    Cider* const c = async(func2, 0, NULL);
    await(c);

    assert(test_storage[0] == 100);
    assert(test_storage[1] == 101);
    assert(test_storage[2] == 102);

    log_info("Succeeded.");

    return 0;
}
