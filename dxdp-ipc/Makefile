SRC_DIR := ./
ROOT_DIR := ../
CC := gcc
BUILD_DIR := ../build/lib/ipc
MAKE_PATH := /usr/bin/make
LIB_DIR := ../build/lib
LIB_NAME := libdxdp-ipc.a

LIB := $(LIB_DIR)/$(LIB_NAME)
SRCS := $(wildcard $(SRC_DIR)*.c)
OBJS := $(SRCS:$(SRC_DIR)%.c=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

CFLAGS := -I$(ROOT_DIR) -Wall -Wextra -std=c23 -g -MMD -MP -D_GNU_SOURCE -fPIC
LDFLAGS := 

.PHONY: all clean debug

all: $(LIB)

$(LIB): $(OBJS)
	ar rcs $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	@mkdir -p $@

-include $(DEPS)

clean:
	rm -rf $(BUILD_DIR)

debug:
	@echo "SRC_DIR = $(SRC_DIR)"
	@echo "CFLAGS = $(CFLAGS)"
	@echo "SRCS = $(SRCS)"
	@echo "OBJS = $(OBJS)"
	@echo "LIB = $(LIB)"