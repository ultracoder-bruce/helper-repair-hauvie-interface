CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Werror -O2

.PHONY: all clean test

all: hauvie-helper

hauvie-helper: src/hauvie_helper.c
	$(CC) $(CFLAGS) $< -o $@

test: hauvie-helper
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
	rm -f hauvie-helper
