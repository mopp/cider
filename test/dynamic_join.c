#include "../header/cider.h"
#include "helper.h"
#include <alloca.h>

static void dynamic_join(size_t count, void* argv[]) {
    if (count == 0) {
        await_sleep(10);
        record_step(100);
        return;
    }

    log_debug("dynamic_join: started. count = %zd", count);

    Cider** ciders = alloca(count * sizeof(Cider*));
    for (size_t i = 0; i < count; i++) {
        ciders[i] = async(dynamic_join, count - 1, argv);
    }
    join_cider_array(ciders, count);

    log_debug("dynamic_join: returning. count = %zd", count);
}

int main(int argc, char* argv[]) {
    log_info("Begin.");

    cider_init();

    // 16 * 3 = 48 Ciders
    join_ciders(
        async(dynamic_join, 3, NULL),
        async(dynamic_join, 3, NULL),
        async(dynamic_join, 3, NULL));

    assert_steps(100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100);

    log_info("Succeeded.");

    return 0;
}
