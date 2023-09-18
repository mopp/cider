#include "../header/cider.h"
#include "helper.h"

static void func(size_t argc, void* argv[]) {
    log_debug("func: started");

    size_t* arr = argv[0];
    assert(argc == 1);
    assert(arr[0] == 1);
    assert(arr[1] == 2);
    assert(arr[2] == 3);

    await_sleep(1);
    record_step(100);

    log_debug("func: returning");
}

int main(void) {
    log_info("Begin.");

    cider_init();

    size_t arr[] = {1, 2, 3};
    void* argv[] = {arr};
    Cider* const f = async(func, 1, argv);
    await(f);

    assert_steps(100);

    log_info("Succeeded.");

    return 0;
}
