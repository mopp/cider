CFLAGS := -DLOG_USE_COLOR

test_files := $(shell ls test/*.c)
test_bins := $(basename $(test_files))

.PHONY: test
test: $(test_bins)
	$(foreach t, $(test_bins), $(t)${newline})

$(test_bins): %: %.c lib/log.c/src/log.* src/cider.c header/cider.h

.PHONY: clean
clean:
	$(RM) $(test_bins)

# foreach で改行するためのマクロ
define newline


endef
