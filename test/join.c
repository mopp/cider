#include "../header/cider.h"
#include "helper.h"

static void func(size_t argc, void* _argv[]) {
    log_debug("func: started. argc = %zd", argc);

    async_sleep(argc);
    record(argc);

    log_debug("func: returning. argc = %zd", argc);
}

int main(int argc, char* argv[]) {
    log_info("Begin.");

    cider_init();

    // sleep 時間の短い順に実行完了する
    Cider* const fs[] = {
        async(func, 5, NULL),
        async(func, 1, NULL),
        async(func, 3, NULL),
    };
    join_ciders(fs, 3);

    assert_steps(3, {1, 3, 5});

    log_info("Succeeded.");

    return 0;
}
