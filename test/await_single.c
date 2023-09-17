#include "../header/cider.h"
#include "../lib/log.c/src/log.h"
#include <assert.h>
#include <stdio.h>

static int test_storage[1] = {0};

static void func(size_t argc, void* argv[]) {
    log_debug("func: started");

    size_t* arr = argv[0];
    assert(argc == 1);
    assert(arr[0] == 1);
    assert(arr[1] == 2);
    assert(arr[2] == 3);

    await_sleep(1);
    test_storage[0] = 100;

    log_debug("func: returning");
}

int main(void) {
    log_info("Begin.");

    cider_init();

    size_t arr[] = {1, 2, 3};
    void* argv[] = {arr};
    Cider* const f = async(func, 1, argv);
    await(f);

    assert(test_storage[0] == 100);

    log_info("Succeeded.");

    return 0;
}
