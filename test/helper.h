#ifndef _HELPER

#define _HELPER

#include "../lib/log.c/src/log.h"
#include <assert.h>
#include <stddef.h>
#include <string.h>

#define MAX_STEPS 128
static int steps[MAX_STEPS] = {0};
static size_t step_index = 0;

inline static void record_step(int n) {
    steps[step_index++] = n;
}

inline static void clear_steps(void) {
    step_index = 0;
    memset(steps, 0, sizeof(int));
}

static void assert_last_step(int n) {
    assert(step_index != 0);
    assert(steps[step_index - 1] == n);
}

static void _assert_steps(int expected[], size_t count) {
    bool fail = false;

    for (size_t i = 0; i < count; ++i) {
        if (expected[i] != steps[i]) {
            fail = true;
            break;
        }
    }

    if (fail == true) {
        for (size_t i = 0; i < count; ++i) {
            log_error("expected: %d, actual: %d", expected[i], steps[i]);
        }
        assert(false);
    }
}

#define assert_steps(n, ...) _assert_steps((int[n])__VA_ARGS__, n)

#endif
