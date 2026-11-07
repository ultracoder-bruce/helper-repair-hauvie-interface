CC ?= cc
CPPFLAGS ?= -Iinclude
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Werror -O2
LDLIBS ?= -lm

.PHONY: all clean test

all: hauvie-helper

hauvie-helper: src/main.c src/hauvie.c include/hauvie.h
	$(CC) $(CPPFLAGS) $(CFLAGS) src/main.c src/hauvie.c $(LDLIBS) -o $@

test-hauvie: tests/test_hauvie.c src/hauvie.c include/hauvie.h
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_hauvie.c src/hauvie.c $(LDLIBS) -o $@

test: hauvie-helper test-hauvie
	./test-hauvie
	./hauvie-helper
	@if ./hauvie-helper test-patient 0.8; then \
		echo "Expected critical severity to fail"; exit 1; \
	else \
		echo "Critical severity handled as expected"; \
	fi
	@if ./hauvie-helper test-patient invalid; then \
		echo "Expected invalid severity to fail"; exit 1; \
	else \
		echo "Invalid severity rejected as expected"; \
	fi

clean:
	rm -f hauvie-helper test-hauvie
