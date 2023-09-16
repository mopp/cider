#include "../header/cider.h"
#include "../lib/log.c/src/log.h"
#include <stdio.h>

static void func1(size_t argc, void* _argv) {
    log_debug("func1: started");
    async_sleep(5);
    log_debug("func1: returning");
}

static void func2(size_t argc, void* _argv) {
    log_debug("func2: started");
    async_sleep(3);
    log_debug("func2: returning");
}

static void func3(size_t argc, void* _argv) {
    log_debug("func3: started");
    async_sleep(1);
    log_debug("func3: returning");
}

static void func4(size_t argc, void* _argv) {
    log_debug("func4: started");

    Cider* const f = async(func3, 0, NULL);
    await(f);

    log_debug("func4: returning");
}

int main(int argc, char* argv[]) {
    log_debug("BEGIN");

    cider_init();

    Cider* const f1 = async(func1, 0, NULL);
    Cider* const f2 = async(func2, 0, NULL);

    Cider* const fs[] = {f1, f2};
    join_ciders(fs, 2);

    log_debug("END");

    return 0;
}
