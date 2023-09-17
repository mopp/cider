#include "../header/cider.h"
#include "../lib/log.c/src/log.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define _XOPEN_SOURCE
#include <ucontext.h>

// TODO: bitmask にして複数検索できるようにする？
enum state {
    // 未使用の Cider
    // async することで利用可能
    UNUSED,

    // 確保済みの Cider
    // await, join をすると READY になる
    USED,

    // 実行可能状態の Cider
    // switch すると RUNNING になる
    READY,

    // 実行中の Cider
    // 一つの Thread で常に唯一つになる
    RUNNING,

    // sleep や通信などの副作用の完了を Polling しながら待っている状態
    POLLING,

    // 他の Cider の実行完了を待っている状態
    WAITED,
};
typedef enum state State;

struct ciderize_arg {
    AsyncFuncion func;
    size_t argc;
    void* argv;
};
typedef struct ciderize_arg ciderizeArg;

typedef ucontext_t Context;
typedef void (*ContextFunc)(void);

struct cider {
    State state;
    Context context;
    ciderizeArg* arg;
};

static size_t const STACK_SIZE = 8 * 1024; // 8KiB
static size_t const MAX_COUNT = 64;
static Cider* ciders;
static Cider root_cider = {
    .state = RUNNING,
    .context = {0},
    .arg = NULL,
};
static Cider* current_cider = &root_cider;

static void ciderize(int);
static void switch_cider(State, Cider* const);
static Cider* find_runnable_cider();
static size_t find_cider_index(State);
static size_t to_index(Cider const* const);

int cider_init() {
    ciders = malloc(sizeof(Cider) * MAX_COUNT);
    // TODO: マクロ化
    for (Cider* f = &ciders[0]; f != &ciders[MAX_COUNT]; f++) {
        f->state = UNUSED;
    }

    return 0;
}

// 与えられた func を実行する Cider を生成する
Cider* async(AsyncFuncion const func, size_t argc, void* argv) {
    size_t i = find_cider_index(UNUSED);
    log_debug("allocate cider: %zd", i);
    if (i == MAX_COUNT) {
        return NULL;
    }

    Cider* cider = &ciders[i];

    Context* c = &cider->context;
    if (getcontext(c) == -1) {
        log_error("No more cider.");
        return NULL;
    }

    // TODO: Config uc_sigmask.
    void* ptr = malloc(STACK_SIZE + sizeof(ciderizeArg));
    c->uc_stack.ss_sp = ptr + sizeof(ciderizeArg);
    c->uc_stack.ss_size = STACK_SIZE;
    c->uc_link = NULL;

    cider->arg = ptr;
    ciderizeArg* const arg = cider->arg;
    arg->func = func;
    arg->argc = argc;
    arg->argv = argv;

    cider->state = USED;

    return cider;
}

// next が完了するまで待つ
void await(Cider* const next) {
    assert(current_cider->state == RUNNING);
    assert(next->state == USED);

    next->state = READY;

    bool should_wait = true;
    do {
        switch_cider(WAITED, next);

        switch (next->state) {
            case POLLING:
            case WAITED: {
                // next が何らかの理由で停止しているので、その間は別の Cider を実行する
                Cider* t = find_runnable_cider();
                if (t != NULL) {
                    switch_cider(WAITED, t);
                }

                should_wait = true;
                break;
            }
            case UNUSED:
                should_wait = false;
                break;
            default:
                log_error("Unexpected state in await. state = %d", next->state);
                exit(EXIT_FAILURE);
        }
    } while (should_wait);
}

// 指定された秒数待つ
// 待つ間、別の Fiber を実行する
void async_sleep(time_t seconds) {
    log_debug("async_sleep(%zd)", seconds);
    assert(current_cider->state == RUNNING);

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    time_t begin = ts.tv_sec;
    time_t now = begin;

    while ((now - begin) <= seconds) {
        // sleep 中は暇なので他の Cider を実行する
        Cider* next = find_runnable_cider();
        if (next != NULL) {
            switch_cider(POLLING, next);
        }

        clock_gettime(CLOCK_REALTIME, &ts);
        now = ts.tv_sec;
    }
}

void join_ciders(Cider* const* const ciders, size_t count) {
    assert(current_cider->state == RUNNING);
    for (size_t i = 0; i < count; i++) {
        assert(ciders[i]->state == READY);
    }

    while (1) {
        // TODO: improve performance.
        for (size_t i = 0; i < count; i++) {
            await(ciders[i]);
        }

        for (size_t i = 0; i < count; i++) {
            if (ciders[i]->state != UNUSED) {
                continue;
            }
        }

        break;
    }

    assert(current_cider->state == RUNNING);
}

// current_cider から next に実行を切り替える
static void switch_cider(State prev_state, Cider* const next) {
    assert(current_cider != next);
    assert(current_cider->state == RUNNING);
    assert(next->state == READY || next->state == POLLING);

    log_debug("switch from %zd to %zd", to_index(current_cider), to_index(next));

    Cider* prev = current_cider;
    prev->state = prev_state;

    next->state = RUNNING;
    next->context.uc_link = &prev->context; // next の実行完了後にここに戻ってくるために設定する
    makecontext(&next->context, (ContextFunc)ciderize, 1, to_index(next));

    current_cider = next;
    int err = swapcontext(&prev->context, &next->context);
    if (err != 0) {
        log_error("Failed to swapcontext. err = %d", err);
        exit(EXIT_FAILURE);
    }
    current_cider = prev;
    current_cider->state = RUNNING;

    // 実行完了していたら Cider を破棄
    if (next->state == UNUSED) {
        memset(&next->context, 0, sizeof(Context));
        free(next->arg);
    }
}

// TODO: current_cider 経由で取得すれば引数不要かも
static void ciderize(int i) {
    Cider* const cider = &ciders[i];
    ciderizeArg const* const arg = cider->arg;

    arg->func(arg->argc, arg->argv);

    // 完了した Cider を破棄する.
    cider->state = UNUSED;
    // NOTE: ここでメモリを破棄するともとの Context に復帰できなくなる
}

// READY か POLLING な Cider を取得する
static Cider* find_runnable_cider() {
    for (Cider* next = &ciders[0]; next != &ciders[MAX_COUNT]; ++next) {
        if (current_cider != next && (next->state == READY || next->state == POLLING)) {
            log_debug("current_cider = %zd, next = %zd", to_index(current_cider), to_index(next));
            return next;
        }
    }

    return NULL;
}

static size_t find_cider_index(State s) {
    // TODO: next fit にする?
    for (size_t i = 0; i < MAX_COUNT; i++) {
        if (ciders[i].state == s) {
            return i;
        }
    }

    return MAX_COUNT;
}

static size_t to_index(Cider const* const cider) {
    if (cider == &root_cider) {
        return MAX_COUNT;
    }

    return ((uintptr_t)cider - (uintptr_t)&ciders[0]) / sizeof(Cider);
}
