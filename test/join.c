#include "../header/cider.h"
#include <stdio.h>

static void func1(size_t argc, void* _argv) {
    printf("func1: started\n");
    async_sleep(5);
    printf("func1: returning\n");
}

static void func2(size_t argc, void* _argv) {
    printf("func2: started\n");
    async_sleep(3);
    printf("func2: returning\n");
}

static void func3(size_t argc, void* _argv) {
    printf("func3: started\n");
    async_sleep(1);
    printf("func3: returning\n");
}

static void func4(size_t argc, void* _argv) {
    printf("func4: started\n");

    Cider* const f = async(func3, 0, NULL);
    await(f);

    printf("func4: returning\n");
}

int main(int argc, char* argv[]) {
    printf("=== BEGIN ===\n");

    cider_init();
    printf("cider_init DONE\n");

    Cider* const f1 = async(func1, 0, NULL);
    Cider* const f2 = async(func2, 0, NULL);

    Cider* const fs[] = {f1, f2};
    printf("before cider_join\n");
    join_ciders(fs, 2);
    printf("after cider_join\n");

    printf("=== END ===\n");

    return 0;
}
