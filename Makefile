.PHONY: test
test: test/await test/join
	./test/await
	./test/join

test/await: src/cider.c test/await.c
	$(CC) $^ -o $@

test/join: src/cider.c test/join.c
	$(CC) $^ -o $@

.PHONY: clean
clean:
	$(RM) test/await
