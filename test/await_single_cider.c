#include "../header/cider.h"
#include "../lib/log.c/src/log.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

static int test_storage[1] = {0};

static void func(size_t argc, void* argv) {
    log_debug("func: started");

    size_t* arr = argv;
    assert(argc == 1);
    assert(arr[0] == 1);
    assert(arr[1] == 2);
    assert(arr[2] == 3);

    async_sleep(1);
    test_storage[0] = 100;

    log_debug("func: returning");
}

int main(void) {
    log_debug("BEGIN");

    cider_init();

    size_t arr[] = {1, 2, 3};
    Cider* const f = async(func, 1, arr);
    await(f);

    assert(test_storage[0] == 100);

    log_debug("END");

    return 0;
}
