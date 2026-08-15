SHELL := /bin/bash

CPP_FILES_V := $(shell find src -type f -name "*.cpp" 2>/dev/null)
SRC_FILES_V := $(CPP_FILES_V)

.PHONY: all build clean tidy format-do format-check-all

all: build

#default
build:
	@echo "Building Clang Plugin..."
	@cmake -S . -B build
	@cmake --build build
	
clean:
	@echo "Cleaning Clang Plugin build..."
	@rm -rf build

#tidy
tidy:
	@echo "Running clang-tidy on C++ plugin..."
	@clang-tidy src/*.cpp -p build/

#format
format-do:
	@echo "Formatting .cpp plugin's code with clang-format..."
	@if [ -n "$(SRC_FILES_V)" ]; then \
		clang-format -i $(SRC_FILES_V); \
		echo "Formatting done!"; \
	else \
		echo "No source files found to format."; \
	fi

format-check-all: build format-do tidy