SRC_DIR := ./
ROOT_DIR := ../
CC := gcc
BUILD_DIR := ../build
LIB_DIR := ../build/lib
MAKE_PATH := /usr/bin/make
EXEC_NAME := dxdpd
EXEC := $(BUILD_DIR)/$(EXEC_NAME)

SRCS := $(wildcard $(SRC_DIR)*.c)
OBJS := $(SRCS:$(SRC_DIR)%.c=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

CFLAGS := -I$(ROOT_DIR) -Wall -Wextra -std=c23 -g -MMD -MP -D_GNU_SOURCE -m64
LDFLAGS := -lbpf -lelf -lz  -lm -L$(LIB_DIR) -ldxdp-ipc

BIN_PREFIX := /usr/bin
CONFIG_PREFIX := /etc/dxdp
LIB_PREFIX := /lib/dxdp
SYSD_SRV_PREFIX := /etc/systemd/system
SOCK_PATH := /var/run/dxdpd/dxdpd

UID := $(shell id -u)

.PHONY: all clean debug

all:
	@echo -e "Make targets:"
	@echo -e "\tbuild (targets everything)"
	@echo -e "\tipc"
	@echo -e "\tdriver"
	@echo -e "\tcompiler"
	@echo
	@echo -e "Other targets (systemd based systems):"
	@echo -e "\tinstall"
	@echo -e "\tuninstall"
help: all

$(EXEC): $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	@mkdir -p $@

-include $(MAIN_OBJS:.o=.d)

clean:
	rm -rf $(BUILD_DIR)

debug:
	@echo "SRC_DIR = $(SRC_DIR)"
	@echo "CFLAGS = $(CFLAGS)"
	@echo "SRCS = $(SRCS)"
	@echo "OBJS = $(OBJS)"
	@echo "EXEC = $(EXEC)"
	@echo "LIB_TARGETS = $(LIB_TARGETS)"

ipc:
ifeq ($(UID),0)
	$(error "Do not run 'make ipc' as root. Please run as a regular user.")
endif
	$(MAKE_PATH) -C ../dxdp-ipc

driver:
ifeq ($(UID),0)
	$(error "Do not run 'make driver' as root. Please run as a regular user.")
endif
	$(MAKE_PATH) -C ../dxdp-driver

compiler: check-not-root
ifeq ($(UID),0)
	$(error "Do not run 'make compiler' as root. Please run as a regular user.")
endif
	$(MAKE_PATH) -C ../dxdp-compiler

check-not-root:
ifeq ($(UID),0)
	$(error "Do not run 'make build' as root. Please run as a regular user.")
endif

build: check-not-root ipc driver $(EXEC) compiler

install:
ifneq ($(UID),0)
	$(error "'make install' requires root.")
endif
	mkdir -p -m 0755 $(LIB_PREFIX)
	mkdir -p -m 0755 $(LIB_PREFIX)/tmp
	mkdir -p -m 0755 $(CONFIG_PREFIX)
	mkdir -p -m 0755 $(SOCK_PATH)
	install -m 0755 $(BUILD_DIR)/dxdpd $(BIN_PREFIX)/dxdpd
	install -m 0755 $(BUILD_DIR)/compiler/dxdp-compiler $(BIN_PREFIX)/dxdp-compiler
	install -m 0755 $(BUILD_DIR)/driver/dxdp $(BIN_PREFIX)/dxdp
	install -m 0755 $(ROOT_DIR)dxdp-db-update/dxdp-db-update $(BIN_PREFIX)/dxdp-db-update
	install -m 0644 $(ROOT_DIR)dxdp-db-update/update_db.py $(LIB_PREFIX)/update_db.py
	install -m 0644 $(ROOT_DIR)app/dxdpd.service $(SYSD_SRV_PREFIX)/dxdpd.service
	install -m 0644 $(ROOT_DIR)app/sample.conf $(CONFIG_PREFIX)/sample.conf
	systemctl daemon-reload

uninstall:
ifneq ($(UID),0)
	$(error "'make uninstall' requires root.")
endif
	rm -f $(BIN_PREFIX)/dxdpd
	rm -f $(BIN_PREFIX)/dxdp
	rm -f $(BIN_PREFIX)/dxdp-db-update
	rm -f $(LIB_PREFIX)/update_db.py
	rm -f $(SYSD_SRV_PREFIX)/dxdpd.service
	rm -f $(CONFIG_PREFIX)/sample.conf
	systemctl daemon-reload
	rm -rf $(LIB_PREFIX)
	rm -rf $(CONFIG_PREFIX)
	rm -rf $(SOCK_PATH)