CC := clang
CFLAGS := --std=c2x -DLOG_USE_COLOR

test_files := $(shell ls test/*.c)
test_bins := $(basename $(test_files))

.PHONY: test
test: $(test_bins)
	$(foreach t, $(test_bins), $(t)${newline})

$(test_bins): %: %.c lib/log.c/src/log.c src/cider.c

.PHONY: clean
clean:
	$(RM) $(test_bins)

# foreach で改行するためのマクロ
define newline


endef
