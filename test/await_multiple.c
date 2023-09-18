#include "../header/cider.h"
#include "helper.h"

static void func(size_t argc, void* argv[]) {
    log_debug("func: started. argc = %zd", argc);

    await_sleep(50);
    record_step(100 + argc);

    log_debug("func: returning");
}

int main(void) {
    log_info("Begin.");

    cider_init();

    Cider* const c0 = async(func, 0, NULL);
    Cider* const c1 = async(func, 1, NULL);
    Cider* const c2 = async(func, 2, NULL);

    await(c0);
    await(c1);
    await(c2);

    assert_steps(100, 101, 102);

    log_info("Succeeded.");

    return 0;
}
