#include "../header/cider.h"
#include "helper.h"

static void func(size_t argc, void* _argv[]) {
    log_debug("func: started. argc = %zd", argc);

    await_sleep(argc);
    record_step(argc);

    log_debug("func: returning. argc = %zd", argc);
}

static void test_join1() {
    log_info("%s: Begin", __func__);

    join_ciders((Cider* [3]){
                    async(func, 30, NULL),
                    async(func, 1, NULL),
                    async(func, 10, NULL),
                },
                3);

    assert_steps(1, 10, 30);

    log_info("%s: Succeeded", __func__);
}

static void test_join2() {
    log_info("%s: Begin", __func__);

    join_ciders((Cider* [3]){
                    async(func, 1, NULL),
                    async(func, 30, NULL),
                    async(func, 10, NULL),
                },
                3);
    assert_steps(1, 10, 30);

    log_info("%s: Succeeded", __func__);
}

static void test_join3() {
    log_info("%s: Begin", __func__);

    join_ciders((Cider* [3]){
                    async(func, 1, NULL),
                    async(func, 10, NULL),
                    async(func, 30, NULL),
                },
                3);
    assert_steps(1, 10, 30);

    log_info("%s: Succeeded", __func__);
}

static void test_join4() {
    log_info("%s: Begin", __func__);

    join_ciders((Cider* [3]){
                    async(func, 30, NULL),
                    async(func, 20, NULL),
                    async(func, 10, NULL),
                },
                3);
    assert_steps(10, 20, 30);

    log_info("%s: Succeeded", __func__);
}

int main(int argc, char* argv[]) {
    log_info("Begin.");

    cider_init();

    clear_steps();
    test_join1();

    clear_steps();
    test_join2();

    clear_steps();
    test_join3();

    clear_steps();
    test_join4();

    log_info("Succeeded.");

    return 0;
}
