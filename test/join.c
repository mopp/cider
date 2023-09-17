#include "../header/cider.h"
#include "helper.h"

static void func1(size_t argc, void* _argv[]) {
    log_debug("func1: started");

    async_sleep(3);

    record(101);

    log_debug("func1: returning");
}

static void func2(size_t argc, void* _argv[]) {
    log_debug("func2: started");

    async_sleep(1);

    record(100);

    log_debug("func2: returning");
}

int main(int argc, char* argv[]) {
    log_info("Begin.");

    cider_init();

    // func2 のほうが sleep 時間が短いので先に実行完了する.
    Cider* const fs[] = {
        async(func1, 0, NULL),
        async(func2, 0, NULL),
    };
    join_ciders(fs, 2);

    assert_steps(2, {100, 101});

    log_info("Succeeded.");

    return 0;
}
