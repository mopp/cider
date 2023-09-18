#include "../header/cider.h"
#include "helper.h"

static void func(size_t argc, void* _argv[]) {
    log_debug("func: started. argc = %zd", argc);

    await_sleep(argc);
    record_step(argc);

    log_debug("func: returning. argc = %zd", argc);
}

static void inner1(size_t _argc, void* _argv[]) {
    log_debug("inner1: started.");

    join_ciders((Cider*[]){
                    async(func, 3, NULL),
                },
                1);
    record_step(200);

    log_debug("inner1: returning.");
}

static void inner2(size_t _argc, void* _argv[]) {
    log_debug("inner2: started.");

    await_sleep(1);
    record_step(100);

    log_debug("inner2: returning.");
}

int main(int argc, char* argv[]) {
    log_info("Begin.");

    cider_init();

    // sleep 時間の短い順に実行完了する
    join_ciders((Cider*[]){
                    async(inner1, 0, NULL),
                    async(inner2, 0, NULL),
                },
                2);
    assert_steps(3, {100, 3, 200});

    log_info("Succeeded.");

    return 0;
}
