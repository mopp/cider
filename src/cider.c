#define _XOPEN_SOURCE 700

#include "../header/cider.h"
#include "../lib/log.c/src/log.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ucontext.h>

typedef uint8_t State;
#define FREE (1 << 0)              // async することで ALLOCATED になる
#define ALLOCATED (1 << 1)         // await や join をすると READY になる
#define READY (1 << 2)             // switch すると RUNNING になる
#define RUNNING (1 << 3)           // 一つの Thread で常にただ一つの Cider しかこの状態になれない
#define POLLING (1 << 4)           // sleep や通信などの副作用の完了を Polling しながら待っている
#define WAITED (1 << 5)            // 他の Cider の実行完了を待っている
#define DONE (1 << 6)              // 実行完了した Cider でリソースの開放待ち
#define RUNNABLE (READY | POLLING) // Polling するために POLLING も実行可能なもの扱いする

struct ciderize_arg {
    AsyncFuncion func;
    size_t argc;
    void* argv;
};
typedef struct ciderize_arg CiderizeArg;

typedef ucontext_t Context;

struct cider {
    State state;
    Context context;
    CiderizeArg* arg;
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

static void ciderize(void);
static void switch_cider(State, Cider* const);
static Cider* find_cider(State);
static size_t to_index(Cider const* const);
static char* to_state_str(State);
static void log_cider(char const*, Cider const* const);

int cider_init(void) {
    ciders = malloc(sizeof(Cider) * MAX_COUNT);
    for (Cider* f = &ciders[0]; f != &ciders[MAX_COUNT]; f++) {
        f->state = FREE;
    }

    return 0;
}

// 与えられた AsyncFuncion を実行する Cider を生成する
Cider* async(AsyncFuncion const func, size_t argc, void* argv) {
    Cider* const cider = find_cider(FREE);
    if (cider == NULL) {
        log_error("No more cider.");
        return NULL;
    }

    log_cider("async:", cider);

    Context* context = &cider->context;
    if (getcontext(context) == -1) {
        log_error("Failed to getcontext. errno = %d", strerror(errno));
        return NULL;
    }

    // TODO: Config uc_sigmask.
    void* ptr = malloc(STACK_SIZE + sizeof(CiderizeArg));
    context->uc_stack.ss_sp = ptr + sizeof(CiderizeArg);
    context->uc_stack.ss_size = STACK_SIZE;
    context->uc_link = &current_cider->context; // async を呼び出した Cider に戻るために保存する
    makecontext(context, ciderize, 0);

    cider->arg = ptr;
    CiderizeArg* const arg = cider->arg;
    arg->func = func;
    arg->argc = argc;
    arg->argv = argv;

    cider->state = ALLOCATED;

    return cider;
}

// next が実行完了するまで待つ
// next が RUNNABLE ではないとき、他の Cider を実行する
void await(Cider* const next) {
    log_cider("await:", next);

    assert(current_cider != next);
    assert(current_cider->state == RUNNING);
    assert(next->state == ALLOCATED || next->state == READY || next->state == POLLING);

    next->state = READY;
    switch_cider(WAITED, next);

    bool should_wait = true;
    do {
        switch (next->state) {
            case POLLING:
            case WAITED: {
                // next が副作用を待っている
                // その間は別の Cider を実行する
                // このときの実行対象に next も含むことで Polling を実現する
                Cider* t = find_cider(RUNNABLE);
                if (t == NULL) {
                    log_error("No runnable cider but need to poll");
                    exit(EXIT_FAILURE);
                }
                switch_cider(WAITED, t);

                should_wait = true;
                break;
            }
            case FREE:
                should_wait = false;
                break;
            default:
                log_error("Unexpected state in await. state = %d", next->state);
                exit(EXIT_FAILURE);
        }
    } while (should_wait);

    assert(next->state == FREE);
}

// 指定されたミリ秒待つ
// 待っているときに RUNNABLE な Cider があれば実行する
void await_sleep(long msec) {
    log_debug("await_sleep(%ld)", msec);
    assert(current_cider->state == RUNNING);

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    long nsec = msec * (1000 * 1000);
    long begin_nsec = ts.tv_sec * (1000 * 1000 * 1000) + ts.tv_nsec;
    long now_nsec = begin_nsec;

    while ((now_nsec - begin_nsec) <= nsec) {
        // sleep 中は暇なので他の Cider を実行する
        Cider* next = find_cider(RUNNABLE);
        if (next != NULL) {
            switch_cider(POLLING, next);
        }

        clock_gettime(CLOCK_MONOTONIC, &ts);
        now_nsec = ts.tv_sec * (1000 * 1000 * 1000) + ts.tv_nsec;
    }
}

// 指定された N 個の Cider を concurrent に実行し、全て完了するのを待つ
void join_ciders(Cider* const* const ciders, size_t count) {
    log_debug("join_ciders(count = %zd)", count);

    assert(current_cider->state == RUNNING);

    for (size_t i = 0; i < count; i++) {
        assert(ciders[i]->state == ALLOCATED);

        // 全て READY にして concurrent に実行可能にする
        ciders[i]->state = READY;
    }

    for (size_t i = 0; i < count; i++) {
        if (ciders[i]->state & RUNNABLE) {
            await(ciders[i]);
        }
    }

    assert(current_cider->state == RUNNING);
    for (size_t i = 0; i < count; i++) {
        assert(ciders[i]->state == FREE);
    }
}

// current_cider から next に実行を切り替える
static void switch_cider(State prev_state, Cider* const next) {
    assert(current_cider != next);
    assert(current_cider->state == RUNNING);
    assert(next->state == READY || next->state == POLLING);

    Cider* prev = current_cider;
    prev->state = prev_state;

    Context* next_context = next->context.uc_link;
    next->state = RUNNING;
    next->context.uc_link = &prev->context; // next の実行完了後にここに戻ってくるために設定する

    current_cider = next;
    int err = swapcontext(&prev->context, &next->context);
    if (err != 0) {
        log_error("Failed to swapcontext. err = %d", err);
        exit(EXIT_FAILURE);
    }
    current_cider = prev;

    current_cider->state = RUNNING;
    next->context.uc_link = next_context;

    // 実行完了していたら Cider を破棄
    if (next->state == DONE) {
        memset(&next->context, 0, sizeof(Context));
        free(next->arg);
        next->state = FREE;
    }
}

// Context の実行開始直後と終了直前に処理を挟むためのラッパー
static void ciderize(void) {
    assert(current_cider->state == RUNNING);

    Cider* const cider = current_cider;
    CiderizeArg const* const arg = cider->arg;

    arg->func(arg->argc, arg->argv);

    // 完了した Cider を破棄する.
    cider->state = DONE;
    // NOTE: ここでメモリを破棄するともとの Context に復帰できなくなる
}

static Cider* find_cider(State s) {
    // First-fit にすると Polling 時に同じ Cider ばかり実行されてしまうので Next-fit にする
    static Cider* next = NULL;

    if (next == NULL) {
        next = &ciders[0];
    }

    Cider* const begin = next;
    do {
        if (current_cider != next && ((next->state & s) != 0)) {
            return next;
        }

        if (++next == &ciders[MAX_COUNT]) {
            next = &ciders[0];
        }
    } while (begin != next);

    return NULL;
}

static size_t to_index(Cider const* const cider) {
    if (cider == &root_cider) {
        return MAX_COUNT;
    }

    return ((uintptr_t)cider - (uintptr_t)&ciders[0]) / sizeof(Cider);
}

static char* to_state_str(State s) {
    switch (s) {
        case FREE:
            return "FREE";
        case ALLOCATED:
            return "ALLOCATED";
        case READY:
            return "READY";
        case RUNNING:
            return "RUNNING";
        case POLLING:
            return "POLLING";
        case WAITED:
            return "WAITED";
        case DONE:
            return "DONE";
        default:
            log_error("unexpected state. 0b%b", s);
            exit(EXIT_FAILURE);
    }
}

static void log_cider(char const* msg, Cider const* const cider) {
    log_debug("%s index = %zd, state = %s", msg, to_index(cider), to_state_str(cider->state));
}
