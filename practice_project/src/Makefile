CROSS_COMPILE ?= arm-linux-gnueabihf-
CC := $(CROSS_COMPILE)gcc
HOST_CC ?= gcc
STATIC ?= 1

CPPFLAGS := -I. -Icore
COMMON_CFLAGS := -std=c11 -Wall -Wextra -Wpedantic
APP_CFLAGS ?= -O2
HOST_CFLAGS ?= -O0 -g
THREAD_FLAGS := -pthread

BUILD_DIR := build
TARGET_DIR := $(BUILD_DIR)/target
HOST_DIR := $(BUILD_DIR)/host

APP_SOURCES := \
	main.c \
	core/assets.c \
	core/digit_bitmaps.c \
	core/display.c \
	core/draw.c \
	core/event_queue.c \
	core/game.c \
	core/input_service.c \
	core/renderer.c \
	core/touch.c \
	core/ui.c
APP_OBJECTS := $(patsubst %.c,$(TARGET_DIR)/%.o,$(APP_SOURCES))
APP_BINARY := $(TARGET_DIR)/pvz

TEST_SOURCES := \
	tests/test_main.c \
	core/digit_bitmaps.c \
	core/draw.c \
	core/event_queue.c \
	core/game.c \
	core/touch.c \
	core/ui.c
HEADERS := $(wildcard core/*.h) config.h
TEST_BINARY := $(HOST_DIR)/pvz_tests
SANITIZE_BINARY := $(HOST_DIR)/pvz_tests_sanitize

ifeq ($(STATIC),1)
APP_LDFLAGS := -static
endif

.PHONY: all app check sanitize clean

all: app

app: $(APP_BINARY)

$(APP_BINARY): $(APP_OBJECTS)
	@mkdir -p $(dir $@)
	$(CC) $(APP_LDFLAGS) $^ $(THREAD_FLAGS) -o $@

$(TARGET_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(COMMON_CFLAGS) $(APP_CFLAGS) -MMD -MP -c $< -o $@

$(TEST_BINARY): $(TEST_SOURCES) $(HEADERS)
	@mkdir -p $(dir $@)
	$(HOST_CC) $(CPPFLAGS) $(COMMON_CFLAGS) -Werror $(HOST_CFLAGS) \
		$(TEST_SOURCES) $(THREAD_FLAGS) -o $@

check: $(TEST_BINARY)
	$(TEST_BINARY)

$(SANITIZE_BINARY): $(TEST_SOURCES) $(HEADERS)
	@mkdir -p $(dir $@)
	$(HOST_CC) $(CPPFLAGS) $(COMMON_CFLAGS) -Werror $(HOST_CFLAGS) \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(TEST_SOURCES) $(THREAD_FLAGS) -o $@

sanitize: $(SANITIZE_BINARY)
	ASAN_OPTIONS=detect_leaks=1 $(SANITIZE_BINARY)

clean:
	$(RM) -r $(BUILD_DIR)

-include $(APP_OBJECTS:.o=.d)
