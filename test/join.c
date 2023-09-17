#include "../header/cider.h"
#include "../lib/log.c/src/log.h"
#include <stdio.h>

static int test_codes[10] = {0};

static void func1(size_t argc, void* _argv) {
    log_debug("func1: started");
    async_sleep(1);

    test_codes[0] = 1;

    log_debug("func1: returning");
}

static void func2(size_t argc, void* _argv) {
    log_debug("func2: started");
    async_sleep(2);

    test_codes[1] = 2;

    log_debug("func2: returning");
}

int main(int argc, char* argv[]) {
    log_debug("BEGIN");

    cider_init();

    Cider* const fs[] = {
        async(func1, 0, NULL),
        async(func2, 0, NULL),
    };
    join_ciders(fs, 2);

    log_debug("END");

    return 0;
}
