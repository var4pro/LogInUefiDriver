SHELL := /bin/bash

REPO_ROOT_V := ../..
SRC_FILES_V := $(shell find src -type f -name "*.cpp" 2>/dev/null)
TEST_EXP_V   := $(REPO_ROOT_V)/tests/tests_tidy_report_expected.txt
TEST_FILES_V := $(wildcard $(REPO_ROOT_V)/tests/cases/*.c)

.PHONY: all build clean tidy format-do test format-check-all hook-check

all: build

#default
build:
	@echo "Building Clang Plugin..."
	@cmake -S . -B build
	@cmake --build build
	
clean:
	rm -rf build

#tidy
tidy:
	clang-tidy src/*.cpp -p build/

#format
format-do:
	@echo "Formatting .cpp plugin's code with clang-format..."
	@if [ -n "$(SRC_FILES_V)" ]; then \
		clang-format -i $(SRC_FILES_V); \
		echo "Formatting done!"; \
	else \
		echo "No source files found to format."; \
	fi

#testing
test: build
	clang-tidy --quiet --load=build/libUefiTidyModule.so -p $(REPO_ROOT_V) $(TEST_FILES_V) 2>&1 \
		| sed 's|.*tests/cases/|tests/cases/|g' \
		| diff -u $(TEST_EXP_V) -

#manually invoke this
format-check-all: format-do hook-check

#auto invoking
hook-check: build tidy test