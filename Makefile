CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -D_POSIX_C_SOURCE=200809L
CPPFLAGS ?= -Iinclude
LDFLAGS ?=
LDLIBS ?= -pthread

TARGET = ckv
TEST_TARGET = test_kv_store
TCP_CLIENT = examples/tcp_client
UDP_CLIENT = examples/udp_client
COMMON_OBJECTS = src/kv_store.o src/tcp_server.o src/udp_server.o

.PHONY: all clean test test-concurrent test-udp examples

all: $(TARGET)

examples: $(TCP_CLIENT) $(UDP_CLIENT)

$(TARGET): src/main.o $(COMMON_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(TEST_TARGET): tests/test_kv_store.o src/kv_store.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(TCP_CLIENT): examples/tcp_client.o
	$(CC) $(LDFLAGS) -o $@ $^

$(UDP_CLIENT): examples/udp_client.o
	$(CC) $(LDFLAGS) -o $@ $^

test: $(TEST_TARGET)
	./$(TEST_TARGET)
	$(MAKE) test-concurrent
	$(MAKE) test-udp

test-concurrent: $(TARGET)
	python3 tests/test_concurrent_tcp.py

test-udp: $(TARGET)
	python3 tests/test_udp.py

clean:
	rm -f $(TARGET) $(TEST_TARGET) $(TCP_CLIENT) $(UDP_CLIENT) src/*.o tests/*.o examples/*.o
