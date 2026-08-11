SHELL := /bin/bash

SRC_DIR_V := $(notdir $(CURDIR))

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
export EDK2_PATH := $(WORKSPACE_DIR_V)/edk2

# not necessary
EXTRA_PACKAGES_PATH_V ?= 
TARGET_EFI_V := $(DISK_DIR_V)/App.efi

ifeq ($(strip $(WORKSPACE_DIR_V)),)
$(error [ERROR] Variable WORKSPACE_DIR_V isn't set! Set it on invoking make)
endif

ifeq ($(strip $(DISK_DIR_V)),)
$(error [ERROR] Variable DISK_DIR_V isn't set! Set it on invoking make)
endif

.PHONY: all build copy run clean build-tools trace-check generate-flags

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
		exit 1; \
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
	@cmake -S tools/trace-check -B tools/trace-check/build
	@cmake --build tools/trace-check/build

generate-flags:
	@envsubst < compile_flags.txt.in > compile_flags.txt

trace-check: build-tools generate-flags
	@echo "Running trace-check on all Main.c..."
	@./tools/trace-check/build/trace-check Main.c -- $$(cat compile_flags.txt)