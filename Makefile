SHELL := /bin/bash

C_FILES_V   := $(shell find src -type f -name "*.c" 2>/dev/null)
H_FILES_V   := $(shell find include -type f -name "*.h" 2>/dev/null)
SRC_FILES_V := $(C_FILES_V) $(H_FILES_V)

# Default build variables
DSC_V       := LogInDriver/LogInDriver.dsc
OUT_DIR_V 	?= LogInPkg
TARGET_V    ?= RELEASE
TOOLCHAIN_V := GCC
EXTRA_FLAGS_V ?=

# paths
WORKSPACE_DIR_V ?= 
DISK_DIR_V      ?= 

CURRENT_GOALS_V := $(or $(MAKECMDGOALS),all)
# Goals that require WORKSPACE_DIR_V
WORKSPACE_GOALS_V := all build copy run clean
# Goals that strictly require DISK_DIR_V (build and clean excluded)
DISK_GOALS_V := all copy run

ifneq ($(filter $(WORKSPACE_GOALS_V),$(CURRENT_GOALS_V)),)
ifeq ($(strip $(WORKSPACE_DIR_V)),)
    $(error [ERROR] Variable WORKSPACE_DIR_V isn't set! Set it on invoking make)
endif
endif

ifneq ($(filter $(DISK_GOALS_V),$(CURRENT_GOALS_V)),)
ifeq ($(strip $(DISK_DIR_V)),)
    $(error [ERROR] Variable DISK_DIR_V isn't set! Set it on invoking make)
endif
endif

.PHONY: all build copy run clean generate-flags format-do tidy format-check-all-recursive hook-check
 
all: run

#default
build:
	@cd $(WORKSPACE_DIR_V) && \
	export PACKAGES_PATH="$$PWD/edk2:$$PWD/edk2-libc:$(abspath $(CURDIR)/..)" && \
	cd edk2 && \
	export EDK_TOOLS_PATH="$$PWD/BaseTools" && \
	source edksetup.sh && \
	build -n 0 -a X64 -t $(TOOLCHAIN_V) -p $(DSC_V) -b $(TARGET_V) $(EXTRA_FLAGS_V)

copy: build
	@BUILT_EFI=$$(find $(WORKSPACE_DIR_V)/edk2/Build/$(OUT_DIR_V)/$(TARGET_V)_$(TOOLCHAIN_V)/X64 -name "LogInDriver.efi" | head -n 1); \
	if [ -z "$$BUILT_EFI" ]; then \
		echo "[ERROR] EFI not found"; exit 1; \
	fi; \
	mkdir -p $(DISK_DIR_V); \
	cp -f "$$BUILT_EFI" $(TARGET_EFI_V)

# not necessary
TARGET_EFI_V := $(DISK_DIR_V)/App.efi
run: copy
	qemu-system-x86_64 \
		-drive if=pflash,format=raw,readonly=on,file=/usr/share/edk2/x64/OVMF_CODE.4m.fd \
		-drive format=raw,file=fat:rw:$(DISK_DIR_V) \
		-net none

clean:
	rm -rf $(WORKSPACE_DIR_V)/edk2/Build/$(OUT_DIR_V)	

#tidy
export EDK2_PATH_V := $(WORKSPACE_DIR_V)/edk2

generate-flags: compile_flags.txt
compile_flags.txt: compile_flags.txt.in
	@if [ -z "$(strip $(WORKSPACE_DIR_V))" ]; then \
		echo "[ERROR] WORKSPACE_DIR_V is not set! Please set it before running."; \
		exit 1; \
	fi
	@echo "Generating compile_flags.txt..."
	@envsubst < $< > $@

tidy: compile_flags.txt 
	$(MAKE) -C tools/clang-plugins build
	clang-tidy --warnings-as-errors='*' --load=tools/clang-plugins/build/libUefiTidyModule.so $(C_FILES_V)

#format
format-do:
	@echo "Formatting code with clang-format..."
	@if [ -n "$(SRC_FILES_V)" ]; then \
		clang-format -i $(SRC_FILES_V); \
		echo "Formatting done!"; \
	else \
		echo "No source files found to format."; \
	fi
	@$(MAKE) -C tools/clang-plugins format-do

#manually invoke this
format-check-all-recursive: format-do hook-check

#auto invoking
hook-check: compile_flags.txt tidy
	$(MAKE) -C tools/clang-plugins hook-check WORKSPACE_DIR_V=$(WORKSPACE_DIR_V)