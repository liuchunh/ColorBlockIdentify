# ===== 可配置变量 =====
# 源代码文件名（不含扩展名）
TARGET_NAME := MergeRunsToComponent

# 编译选项
CXXFLAGS := -Wall -Wextra -Wpedantic

# C++ 标准
CXXSTD := c++20

# 编译器
CXX := g++

# ===== 派生变量（根据文件名计算）=====
SRC     := $(TARGET_NAME).cpp
EXE     := $(TARGET_NAME).exe

# ===== 目标 =====
.PHONY: build clean all

# 默认执行 build
all: clean build

# make build 时自动编译
build: $(EXE)

$(EXE): $(SRC)
	$(CXX) -std=$(CXXSTD) $(CXXFLAGS) $< -o $@

clean:
	del /f $(EXE)