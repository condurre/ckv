CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -D_POSIX_C_SOURCE=200809L
CPPFLAGS ?= -Iinclude
LDFLAGS ?=
LDLIBS ?= -pthread

TARGET = ckv
TEST_TARGET = test_kv_store
COMMON_OBJECTS = src/kv_store.o src/tcp_server.o

.PHONY: all clean test

all: $(TARGET)

$(TARGET): src/main.o $(COMMON_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(TEST_TARGET): tests/test_kv_store.o src/kv_store.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -f $(TARGET) $(TEST_TARGET) src/*.o tests/*.o
