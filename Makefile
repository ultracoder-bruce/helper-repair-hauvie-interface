CC ?= cc
CPPFLAGS ?= -Iinclude -Ifsm/include
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Werror -O2
LDLIBS ?= -lm

CORE = src/hauvie.c src/token.c src/router.c
FSM = fsm/src/hauvie_fsm.c

.PHONY: all clean test

all: hauvie-helper

hauvie-helper: src/main.c $(CORE) include/hauvie.h include/hauvie_token.h include/hauvie_router.h
	$(CC) $(CPPFLAGS) $(CFLAGS) src/main.c $(CORE) $(LDLIBS) -o $@

test-hauvie: tests/test_hauvie.c $(CORE) include/hauvie.h include/hauvie_token.h include/hauvie_router.h
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_hauvie.c $(CORE) $(LDLIBS) -o $@

test-fsm: tests/test_fsm.c $(FSM) fsm/include/hauvie_fsm.h
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_fsm.c $(FSM) -o $@

test: hauvie-helper test-hauvie test-fsm
	./test-hauvie
	./test-fsm
	./hauvie-helper
	./hauvie-helper --source 'patient=Tech-Agent-404 severity=0.2 sulking=false target=c'
	@if ./hauvie-helper test-patient 0.8; then \
		echo "Expected critical severity to fail"; exit 1; \
	else \
		echo "Critical severity handled as expected"; \
	fi
	@if ./hauvie-helper --source 'patient=critical severity=0.9 sulking=false target=java'; then \
		echo "Expected critical source input to fail diagnostic status"; exit 1; \
	else \
		echo "Critical input parsed and routed as expected"; \
	fi
	@if ./hauvie-helper --source 'patient=broken; severity=0.2'; then \
		echo "Expected malformed source input to fail"; exit 1; \
	else \
		echo "Malformed source rejected as expected"; \
	fi
	@if ./hauvie-helper test-patient invalid; then \
		echo "Expected invalid severity to fail"; exit 1; \
	else \
		echo "Invalid severity rejected as expected"; \
	fi

clean:
	rm -f hauvie-helper test-hauvie test-fsm
