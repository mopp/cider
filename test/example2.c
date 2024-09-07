#include "../header/cider.h"
#include "helper.h"

static void count_even(size_t argc, void* argv[]) {
    log_debug("count_even: started. argc = %zd", argc);

    size_t total = 0;
    for (size_t i = 0; i < argc; i++) {
        await_sleep(2);

        if ((size_t)(argv[i]) % 2 == 0) {
            total += 1;
        }
    }
    printf("%zd even numbers exist.\n", total);

    record_step(101);

    log_debug("count_even: returning. argc = %zd", argc);
}

static void count_odd(size_t argc, void* argv[]) {
    log_debug("count_odd: started. argc = %zd", argc);

    size_t total = 0;
    for (size_t i = 0; i < argc; i++) {
        await_sleep(1);

        if ((size_t)(argv[i]) % 2 != 0) {
            total += 1;
        }
    }
    printf("%zd odd numbers exist.\n", total);

    record_step(102);

    log_debug("count_odd: returning. argc = %zd", argc);
}

int main(int argc, char* argv[]) {
    log_info("Begin.");

    cider_init();

    size_t numbers[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    join_ciders(
        async(count_even, 10, numbers),
        async(count_odd, 10, numbers),
    );
    assert_steps(102, 101);

    log_info("Succeeded.");

    return 0;
}
