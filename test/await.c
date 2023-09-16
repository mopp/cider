#include "../header/cider.h"
#include "../lib/log.c/src/log.h"
#include <stdio.h>

static void func1(size_t _argc, void* _argv) {
    log_debug("func: started");
    async_sleep(1);
    log_debug("func: returning");
}

static int test_assert_without_args() {
    Cider* const f1 = async(func1, 0, NULL);
    await(f1);

    return 0;
}

static void func2(size_t argc, void* argv) {
    log_debug("func: started\n");

    // assert(argc == 1);
    // assert(agrv[0] == 1);
    // assert(agrv[1] == 2);
    // assert(agrv[2] == 3);

    async_sleep(1);
    log_debug("func: returning\n");
}

static int test_assert_with_args() {
    size_t arr[] = {1, 2, 3};
    Cider* const f = async(func1, 1, arr);
    await(f);

    return 0;
}

int main(void) {
    log_debug("BEGIN");

    cider_init();

    test_assert_with_args();
    test_assert_without_args();

    log_debug("END");

    return 0;
}
