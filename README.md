# cider

Just hobby Fiber library written in C

## Examples

```c
static void hoge(size_t argc, void* _argv[]) {
    log_debug("hoge: started. argc = %zd", argc);

    await_sleep(argc);
    record_step(argc);

    log_debug("hoge: returning. argc = %zd", argc);
}

static void func(size_t argc, void* _argv[]) {
    log_debug("func: started. argc = %zd", argc);

    log_info("1st await");
    await(async(hoge, 10, NULL));
    log_info("1st awaited");

    log_info("2nd await");
    await(async(hoge, 2, NULL));
    log_info("2nd awaited");

    log_debug("func: returning. argc = %zd", argc);
}

int main(int argc, char* argv[]) {
    log_info("Begin.");

    cider_init();

    join_ciders(
        async(hoge, 20, NULL),
        async(func, 0, NULL));
    assert_steps(10, 2, 20);

    log_info("Succeeded.");

    return 0;
}
```

See the other tests in more details.

## How to develop

```console
make test

# To execute each test file.
make test/example && test/example

make clean
```

NOTE: It don't aware of the change of headers.

## TODOs

- Add more tests
- Improve readability
- Introduce N Threads M Fibers model
