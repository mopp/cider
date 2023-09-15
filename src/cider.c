#include "../header/cider.h"

#include <string.h>
#define _XOPEN_SOURCE

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ucontext.h>

// TODO: bitmask にして複数検索できるようにする？
enum state {
    UNUSED,
    READY,
    RUNNING,
    BLOCKED, // sleep や通信など何らかの副作用を待っている状態
    WAITED,  // 他の Cider を await している状態
    DONE,
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

static size_t const STACK_SIZE = 4096;
static size_t const MAX_COUNT = 64;
static Cider* ciders;
static Cider root_cider = {
    .state = RUNNING,
    .context = {0},
    .arg = NULL,
};
static Cider* current_cider = &root_cider;

static void ciderize(int);
static void switch_cider(Cider* const, Cider* const);
static void switch_runnable_cider(Cider* const);
static size_t find_cider_index(State);

int cider_init() {
    ciders = malloc(sizeof(Cider) * MAX_COUNT);
    for (Cider* f = &ciders[0]; f != &ciders[MAX_COUNT]; f++) {
        f->state = UNUSED;
    }

    return 0;
}

// 与えられた func を実行する Cider を生成する
Cider* async(AsyncFuncion const func, size_t argc, void* argv) {
    size_t i = find_cider_index(UNUSED);
    if (i == MAX_COUNT) {
        return NULL;
    }

    Cider* cider = &ciders[i];

    Context* c = &cider->context;
    if (getcontext(c) == -1) {
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

    cider->state = READY;

    return cider;
}

// next が完了するまで待つ
void await(Cider* const next) {
    // assert(next->state == READY);

    bool should_wait = true;
    do {
        current_cider->state = WAITED;
        switch_cider(current_cider, next);
        current_cider->state = RUNNING;

        switch (next->state) {
            case BLOCKED:
            case WAITED:
                // next が何らかの理由で停止しているので、その間は別の Cider を実行する
                current_cider->state = WAITED;
                switch_runnable_cider(current_cider);
                current_cider->state = RUNNING;

                should_wait = true;
                break;
            case DONE:
                should_wait = false;
                break;
            default:
                fprintf(stderr, "Unexpected state in await.");
                exit(EXIT_FAILURE);
        }
    } while (should_wait);

    current_cider->state = RUNNING;

    // 完了した Cider を破棄する.
    next->state = UNUSED;
    memset(&next->context, 0, sizeof(Context));
    free(next->arg);
}

void async_sleep(time_t seconds) {
    // assert(current_cider->state == RUNNING);

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    time_t begin = ts.tv_sec;
    time_t now = begin;

    printf("async_sleep(%zd)\n", seconds);
    while ((now - begin) <= seconds) {
        // sleep 中は暇なので他の Cider を実行する
        current_cider->state = BLOCKED;
        switch_runnable_cider(current_cider);

        clock_gettime(CLOCK_REALTIME, &ts);
        now = ts.tv_sec;
    }
    current_cider->state = RUNNING;
}

void join_ciders(Cider* const* const ciders, size_t count) {
    // assert(ciders[]->state != DONE)

    while (1) {
        for (size_t i = 0; i < count; i++) {
            current_cider->state = WAITED;
            switch_cider(current_cider, ciders[i]);
            current_cider->state = RUNNING;
        }

        for (size_t i = 0; i < count; i++) {
            if (ciders[i]->state != DONE) {
                continue;
            }
        }

        break;
    }
}

// current から next に実行を切り替える
// TODO: 引数で state の設定漏れをなくす
static void switch_cider(Cider* const current, Cider* const next) {
    printf("cider_switch\n");

    // next の実行完了後にここに戻ってくるために設定する
    next->context.uc_link = &current->context;

    int i = (size_t)(next - ciders) / sizeof(ciders[0]);
    makecontext(&next->context, (ContextFunc)ciderize, 1, i);

    current_cider = next;
    int err = swapcontext(&current->context, &next->context);
    if (err != 0) {
        fprintf(stderr, "Failed to swapcontext. err = %d", err);
        exit(EXIT_FAILURE);
    }
    current_cider = current;

    next->context.uc_link = NULL;
}

static void ciderize(int i) {
    printf("ciderize: BEGIN i = %d\n", i);

    Cider* const cider = &ciders[i];
    ciderizeArg const* const arg = cider->arg;

    cider->state = RUNNING;
    arg->func(arg->argc, arg->argv);
    cider->state = DONE;

    printf("ciderize: END\n");
}

// READY か BLOCKED な Cider に実行を切り替える
static void switch_runnable_cider(Cider* const current) {
    for (size_t i = 0; i < MAX_COUNT; i++) {
        Cider* next = &ciders[i];
        if (current != next && (next->state == READY || next->state == BLOCKED)) {
            switch_cider(current_cider, next);
        }
    }
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
