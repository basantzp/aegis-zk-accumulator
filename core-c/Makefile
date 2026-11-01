CC ?= gcc
CFLAGS ?= -Wall -Wextra -Werror -pedantic -std=c11 -O3 -fPIC -D_POSIX_C_SOURCE=199309L
INCLUDES = -Iinclude

SRCS = src/hash.c src/smt.c src/accumulator.c src/precompile_shim.c
OBJS = $(SRCS:src/%.c=build/%.o)

BUILD_DIR = build
LIB_STATIC = $(BUILD_DIR)/libaegis.a
LIB_SHARED = $(BUILD_DIR)/libaegis.so
CLI_BIN = $(BUILD_DIR)/aegis-cli

.PHONY: all clean test bench asan dirs

all: dirs $(LIB_STATIC) $(LIB_SHARED) $(CLI_BIN)

dirs:
	@mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: src/%.c | dirs
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(LIB_STATIC): $(OBJS)
	ar rcs $@ $^

$(LIB_SHARED): $(OBJS)
	$(CC) -shared -o $@ $^

$(CLI_BIN): cli/main.c $(LIB_STATIC)
	$(CC) $(CFLAGS) $(INCLUDES) $< $(LIB_STATIC) -o $@

test: dirs $(LIB_STATIC)
	$(CC) $(CFLAGS) $(INCLUDES) tests/test_hash.c $(LIB_STATIC) -o $(BUILD_DIR)/test_hash
	$(CC) $(CFLAGS) $(INCLUDES) tests/test_smt.c $(LIB_STATIC) -o $(BUILD_DIR)/test_smt
	@echo "--- Running test_hash ---"
	$(BUILD_DIR)/test_hash
	@echo "--- Running test_smt ---"
	$(BUILD_DIR)/test_smt

bench: dirs $(LIB_STATIC)
	$(CC) $(CFLAGS) $(INCLUDES) tests/bench.c $(LIB_STATIC) -o $(BUILD_DIR)/bench
	$(BUILD_DIR)/bench

asan: clean dirs
	$(CC) -Wall -Wextra -pedantic -std=c11 -g -fsanitize=address,undefined -D_POSIX_C_SOURCE=199309L $(INCLUDES) \
		$(SRCS) tests/test_hash.c -o $(BUILD_DIR)/test_hash_asan
	$(BUILD_DIR)/test_hash_asan
	$(CC) -Wall -Wextra -pedantic -std=c11 -g -fsanitize=address,undefined -D_POSIX_C_SOURCE=199309L $(INCLUDES) \
		$(SRCS) tests/test_smt.c -o $(BUILD_DIR)/test_smt_asan
	$(BUILD_DIR)/test_smt_asan

clean:
	rm -rf $(BUILD_DIR)
