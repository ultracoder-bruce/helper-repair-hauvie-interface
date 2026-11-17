CC ?= cc
CPPFLAGS ?= -Iinclude -Ifsm/include
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Werror -O2
LDLIBS ?= -lm

CORE = src/hauvie.c src/token.c src/token_stream.c src/router.c src/receiver.c src/diagnostic.c src/remediation.c
FSM = fsm/src/hauvie_fsm.c
RUNTIME = fsm/src/hauvie_runtime.c
RECEIVER = src/receiver.c
REPAIR_QUEUE = src/repair_queue.c src/repair.c $(FSM) $(RUNTIME)

.PHONY: all clean test test-hauvie test-fsm test-runtime test-receiver test-diagnostic test-repair-queue test-router-remediation

all: hauvie-helper

hauvie-helper: src/main.c $(CORE) include/hauvie.h include/hauvie_token.h include/hauvie_token_stream.h include/hauvie_router.h include/hauvie_receiver.h include/hauvie_diagnostic.h
	$(CC) $(CPPFLAGS) $(CFLAGS) src/main.c $(CORE) $(LDLIBS) -o $@

test-hauvie: tests/test_hauvie.c $(CORE) include/hauvie.h include/hauvie_token.h include/hauvie_token_stream.h include/hauvie_router.h include/hauvie_receiver.h include/hauvie_diagnostic.h
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_hauvie.c $(CORE) $(LDLIBS) -o $@

test-fsm: tests/test_fsm.c $(FSM) fsm/include/hauvie_fsm.h
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_fsm.c $(FSM) -o $@

test-runtime: tests/test_runtime.c $(FSM) $(RUNTIME) fsm/include/hauvie_fsm.h fsm/include/hauvie_runtime.h
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_runtime.c $(FSM) $(RUNTIME) -o $@

test-receiver: tests/test_receiver.c $(RECEIVER) include/hauvie_receiver.h
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_receiver.c $(RECEIVER) $(LDLIBS) -o $@

test-diagnostic: tests/test_diagnostic.c $(CORE) include/hauvie.h include/hauvie_token.h include/hauvie_token_stream.h include/hauvie_receiver.h include/hauvie_diagnostic.h
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_diagnostic.c $(CORE) $(LDLIBS) -o $@

test-repair-queue: tests/test_repair_queue.c $(CORE) $(REPAIR_QUEUE) include/hauvie_repair.h include/hauvie_repair_queue.h
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_repair_queue.c $(CORE) $(REPAIR_QUEUE) $(LDLIBS) -o $@

test-router-remediation: tests/test_router_remediation.c src/router.c src/remediation.c include/hauvie_router.h include/hauvie_remediation.h
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_router_remediation.c src/router.c src/remediation.c -o $@

test: hauvie-helper test-hauvie test-fsm test-runtime test-receiver test-diagnostic test-repair-queue test-router-remediation
	./test-hauvie
	./test-fsm
	./test-runtime
	./test-receiver
	./test-diagnostic
	./test-repair-queue
	./test-router-remediation
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
		echo "Malformed source rejected as expected"; exit 1; \
	fi
	@if ./hauvie-helper test-patient invalid; then \
		echo "Expected invalid severity to fail"; exit 1; \
	else \
		echo "Invalid severity rejected as expected"; \
	fi

clean:
	rm -f hauvie-helper test-hauvie test-fsm test-runtime test-receiver test-diagnostic test-repair-queue test-router-remediation
