#include "../header/cider.h"
#include "../lib/log.c/src/log.h"
#include "helper.h"
#include <assert.h>
#include <stdio.h>

static void func1(size_t argc, void* argv[]) {
    log_debug("func1: started.");

    assert_last_step(100);

    await_sleep(50);

    record_step(101);

    log_debug("func1: returning");
}

static void func2(size_t argc, void* argv[]) {
    log_debug("func2: started.");

    record_step(100);

    Cider* const c = async(func1, 0, NULL);
    await(c);

    assert_last_step(101);
    record_step(102);

    log_debug("func2: returning");
}

int main(void) {
    log_info("Begin.");

    cider_init();

    Cider* const c = async(func2, 0, NULL);
    await(c);

    assert_steps(3, {100, 101, 102});

    log_info("Succeeded.");

    return 0;
}
