SHELL := /bin/bash

SRC_DIR_V := $(notdir $(CURDIR))
C_FILES_V   := $(shell find src -type f -name "*.c" ! -name "maintest.c" 2>/dev/null)
H_FILES_V   := $(shell find include -type f -name "*.h" 2>/dev/null)
TEST_C_FILES_V := $(shell find tests -type f -name "*.c" 2>/dev/null)
SRC_FILES_V := $(C_FILES_V) $(H_FILES_V)
PLUGIN_PATH_V := tools/clang-plugins/build/libUefiTidyModule.so

# Default build variables
DSC_V       ?= MdeModulePkg/MdeModulePkg.dsc
OUT_DIR_V 	?= MdeModule
TARGET_V    ?= RELEASE
TOOLCHAIN_V ?= GCC
EXTRA_FLAGS_V ?=

# paths
# necessary
WORKSPACE_DIR_V ?= 
DISK_DIR_V      ?= 
export EDK2_PATH_V := $(WORKSPACE_DIR_V)/edk2

# not necessary
EXTRA_PACKAGES_PATH_V ?= 
TARGET_EFI_V := $(DISK_DIR_V)/App.efi

CURRENT_GOALS_V := $(or $(MAKECMDGOALS),all)
# goals that need paths
EDK2_GOALS_V := all build copy run clean

ifneq ($(filter $(EDK2_GOALS_V),$(CURRENT_GOALS_V)),)
ifeq ($(strip $(WORKSPACE_DIR_V)),)
$(error [ERROR] Variable WORKSPACE_DIR_V isn't set! Set it on invoking make)
endif

ifeq ($(strip $(DISK_DIR_V)),)
$(error [ERROR] Variable DISK_DIR_V isn't set! Set it on invoking make)
endif
endif

.PHONY: all build copy run clean build-tools generate-flags format-do format-check tidy test check-all
 
all: run

build:
	@cd $(WORKSPACE_DIR_V) && \
	export PACKAGES_PATH="$$PWD/edk2:$$PWD/edk2-libc$(if $(EXTRA_PACKAGES_PATH_V),:$(EXTRA_PACKAGES_PATH_V))" && \
	cd edk2 && \
	export EDK_TOOLS_PATH="$$PWD/BaseTools" && \
	source edksetup.sh && \
	build -n 0 -a X64 -t $(TOOLCHAIN_V) -p $(DSC_V) -b $(TARGET_V) $(EXTRA_FLAGS_V)

copy: build
	@BUILT_EFI=$$(find $(WORKSPACE_DIR_V)/edk2/Build/$(OUT_DIR_V)/$(TARGET_V)_$(TOOLCHAIN_V)/X64 -name "$(SRC_DIR_V).efi" | head -n 1); \
	if [ -z "$$BUILT_EFI" ]; then \
		echo "[ERROR] EFI not found"; exit 1; \
	fi; \
	mkdir -p $(DISK_DIR_V); \
	cp -f "$$BUILT_EFI" $(TARGET_EFI_V)

run: copy
	qemu-system-x86_64 \
		-drive if=pflash,format=raw,readonly=on,file=/usr/share/edk2/x64/OVMF_CODE.4m.fd \
		-drive format=raw,file=fat:rw:$(DISK_DIR_V) \
		-net none

clean:
	rm -rf $(WORKSPACE_DIR_V)/edk2/Build/$(OUT_DIR_V)

build-tools: 
	@echo "Building custom tools..."
	@cmake -S tools/clang-plugins -B tools/clang-plugins/build
	@cmake --build tools/clang-plugins/build

compile_flags.txt: compile_flags.txt.in
	@echo "Generating compile_flags.txt..."
	@envsubst < $< > $@

generate-flags: compile_flags.txt

format-do:
	@echo "Formatting code with clang-format..."
	@if [ -n "$(SRC_FILES_V)" ]; then \
		clang-format -i $(SRC_FILES_V); \
		echo "Formatting done!"; \
	else \
		echo "No source files found to format."; \
	fi

format-check:
	@echo "Checking code formatting..."
	@if [ -n "$(SRC_FILES_V)" ]; then \
		if ! clang-format --dry-run --Werror $(SRC_FILES_V); then \
			echo -e "\n[ERROR] Code is not formatted properly. Run 'make format-do' to fix it."; \
			exit 1; \
		fi; \
		echo "Formatting check passed!"; \
	else \
		echo "No source files found to check."; \
	fi
 
tidy: compile_flags.txt build-tools
	@echo "Running clang-tidy and writing report to tidy_report.txt..."
	@if [ -n "$(C_FILES_V)" ]; then \
		clang-tidy --load=$(PLUGIN_PATH_V) --quiet $(C_FILES_V) > tidy_report.txt 2>&1 || true; \
		echo "Analysis complete! Check tidy_report.txt for details."; \
	else \
		echo "No source files found for clang-tidy."; \
	fi

test: compile_flags.txt build-tools
	@echo "Running tests and writing report to tests/tests_tidy_report.txt..."
	@if [ -n "$(TEST_C_FILES_V)" ]; then \
		clang-tidy --load=$(PLUGIN_PATH_V) --checks='-*,uefi-*' --quiet $(TEST_C_FILES_V) > tests/tests_tidy_report.txt 2>&1 || true; \
		echo "Tests complete! Check tests/tests_tidy_report.txt for details."; \
	else \
		echo "No test files found for clang-tidy."; \
	fi

check-all: tidy format-check