SRC_DIR := .
ROOT_DIR := ..
CC := clang
CXX := clang++
LIB_DIR := ../build/lib
BUILD_DIR := ../build/compiler
EXEC_NAME:=dxdp-compiler
EXEC:=$(BUILD_DIR)/$(EXEC_NAME)

SRCS := $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(SRCS:$(SRC_DIR)%.c=$(BUILD_DIR)%.o)
OBJS := $(OBJS:$(SRC_DIR)%.cpp=$(BUILD_DIR)%.o)
DEPS := $(OBJS:.o=.d)

CXXFLAGS:=-g -I$(ROOT_DIR)
LDFLAGS := -lclang-cpp -lLLVM -L$(LIB_DIR) -ldxdp-ipc

.PHONY: all clean debug

all: $(EXEC)

$(BUILD_DIR):
	@mkdir -p $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(EXEC): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

-include $(DEPS)

clean:
	rm -rf $(BUILD_DIR) 

debug:
	@echo "SRC_DIR = $(SRC_DIR)"
	@echo "CXXFLAGS = $(CXXFLAGS)"
	@echo "SRCS = $(SRCS)"
	@echo "OBJS = $(OBJS)"
	@echo "EXEC = $(EXEC)"
	@echo "DEPS = $(DEPS)"