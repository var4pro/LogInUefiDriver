SHELL := /bin/bash

CUR_DIR_V := $(notdir $(CURDIR))
C_FILES_V   := $(shell find src -type f -name "*.c" ! -name "maintest.c" 2>/dev/null)
H_FILES_V   := $(shell find include -type f -name "*.h" 2>/dev/null)
SRC_FILES_V := $(C_FILES_V) $(H_FILES_V)

# Default build variables
DSC_V       ?= $(CUR_DIR_V)/$(CUR_DIR_V).dsc
OUT_DIR_V 	?= LogInPkg
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

.PHONY: all build copy run clean generate-flags format-do tidy test-clang-plugins format-check-all-recursive hook-check
 
all: run

#default
build:
	@cd $(WORKSPACE_DIR_V) && \
	export PACKAGES_PATH="$$PWD/edk2:$$PWD/edk2-libc$(if $(EXTRA_PACKAGES_PATH_V),:$(EXTRA_PACKAGES_PATH_V))" && \
	cd edk2 && \
	export EDK_TOOLS_PATH="$$PWD/BaseTools" && \
	source edksetup.sh && \
	build -n 0 -a X64 -t $(TOOLCHAIN_V) -p $(DSC_V) -b $(TARGET_V) $(EXTRA_FLAGS_V)

copy: build
	@BUILT_EFI=$$(find $(WORKSPACE_DIR_V)/edk2/Build/$(OUT_DIR_V)/$(TARGET_V)_$(TOOLCHAIN_V)/X64 -name "$(CUR_DIR_V).efi" | head -n 1); \
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

#tidy
generate-flags: compile_flags.txt
compile_flags.txt: compile_flags.txt.in
	@echo "Generating compile_flags.txt..."
	@envsubst < $< > $@

tidy: compile_flags.txt 
	$(MAKE) -C tools/clang-plugins build
	clang-tidy --load=tools/clang-plugins/build/libUefiTidyModule.so $(C_FILES_V)

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
	$(MAKE) -C tools/clang-plugins hook-check