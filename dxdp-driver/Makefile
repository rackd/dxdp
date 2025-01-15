SRC_DIR := ./
ROOT_DIR := ../
CC := gcc
BUILD_DIR := ../build/driver
LIB_DIR := ../build/lib
DXDP_DIR := $(ROOT_DIR)/dxdp
PYTHON_LIB_PATH = 
EXEC_NAME := dxdp
EXEC := $(BUILD_DIR)/$(EXEC_NAME)

SRCS := $(wildcard $(SRC_DIR)*.c)
SRCS += $(DXDP_DIR)/string_util.c
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(SRCS)))
DEPS := $(OBJS:.o=.d)

CFLAGS := -I$(ROOT_DIR) -Wall -Wextra -std=c23 -g -MMD -MP 
LDFLAGS := -L$(LIB_DIR) -ldxdp-ipc

.PHONY: all clean debug

all: $(EXEC)

$(BUILD_DIR):
	@mkdir -p $@

$(BUILD_DIR)/%.o: $(SRC_DIR)%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/string_util.o: $(DXDP_DIR)/string_util.c
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -rf $(BUILD_DIR)

debug:
	@echo "SRC_DIR = $(SRC_DIR)"
	@echo "CFLAGS = $(CFLAGS)"
	@echo "SRCS = $(SRCS)"
	@echo "OBJS = $(OBJS)"
	@echo "EXEC = $(EXEC)"
	@echo "DEPS = $(DEPS)"