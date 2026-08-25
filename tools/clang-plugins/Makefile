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

# ==============================================================================
# TEST SUITE
# ==============================================================================
TIDY_RUN = clang-tidy --quiet --load=build/libUefiTidyModule.so -p $(REPO_ROOT_V)
TESTS_DIR = $(REPO_ROOT_V)/tests

test-banned: build
	@echo "Running Banned Allocators Test..."
	@$(TIDY_RUN) --checks='-*,uefi-banned-allocator' $(TESTS_DIR)/cases/TestBanned.c 2>&1 \
		| sed 's|.*tests/cases/|tests/cases/|g' \
		| diff -u $(TESTS_DIR)/test_banned_tidy_report_expected.txt -
	@echo "  └─ Banned Allocators Test PASSED"

test-trace: build
	@echo "Running Trace Function Test..."
	@$(TIDY_RUN) --checks='-*,uefi-trace-function' $(TESTS_DIR)/cases/TestTrace.c 2>&1 \
		| sed 's|.*tests/cases/|tests/cases/|g' \
		| diff -u $(TESTS_DIR)/test_trace_tidy_report_expected.txt -
	@echo "  └─ Trace Function Test PASSED"

test-unchecked: build
	@echo "Running Unchecked Status Test..."
	@$(TIDY_RUN) --checks='-*,uefi-unchecked-status' $(TESTS_DIR)/cases/TestUnchecked.c 2>&1 \
		| sed 's|.*tests/cases/|tests/cases/|g' \
		| diff -u $(TESTS_DIR)/test_unchecked_tidy_report_expected.txt -
	@echo "  └─ Unchecked Status Test PASSED"

# Run all tests sequentially
test: test-banned test-trace test-unchecked
	@echo "\n🎉 ALL UEFI STATIC ANALYSIS TESTS PASSED SUCCESSFULLY! 🎉\n"

#manually invoke this
format-check-all: format-do hook-check

#auto invoking
hook-check: build tidy test