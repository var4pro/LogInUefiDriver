# UEFI Secure Login Driver

A proof-of-concept UEFI driver focused on secure user authentication, memory safety, and TPM 2.0 interaction. This project goes beyond standard UEFI development by implementing RAII-style memory management in C and enforcing strict project architectures via custom Clang AST static analysis(see https://github.com/var4pro/ClangTidyUefi).

## Key Features

* **Secure Credential Handling:** Sensitive stack arrays are automatically zeroed out upon function exit.
* **C-Style RAII (`__attribute__((cleanup))`):** Utilizes compiler extensions to ensure memory is automatically freed (`AUTO_FREE`) and sensitive data is securely wiped (`AUTO_SET_TO_ZERO`), eliminating memory leaks and data exposure even during early returns.
* **Custom Static Analysis (SAST):** Includes a custom-built Clang-Tidy plugin (written in C++ using LLVM/Clang AST Matchers) to enforce secure coding standards at compile time.
* **TPM 2.0 Integration (Mocked for now(will be updated up to 30.09.26)):** Demonstrates the conceptual workflow of unsealing secrets bound to a TPM using a password-based authorization session.

## Project Architecture & Security

### 1. Memory Safety
Standard UEFI allocators (`AllocatePool`) can leave uninitialized data. This project uses a custom wrapper (`Var4alloc`) that forces zero-initialization. Furthermore, the `AUTO_SET_TO_ZERO` macro ensures that sensitive buffers (like passwords and unsealed secrets) are zeroed out locally on the stack when they go out of scope, defending against use-of-uninitialized-value and stack-reading exploits.

### 2. Custom Clang-Tidy Plugin as a submodule (`tools/clang-plugins`)
See https://github.com/var4pro/ClangTidyUefi for the info


## Getting Started

### Prerequisites
* **EDK2 Workspace:** You must have a configured EDK2 environment.
* **LLVM Toolchain:** Required for formatting and the custom static analyzer (tested with LLVM 14).
* **CMake:** To build the custom Clang-Tidy plugin.
* **GNU Make:** Required for top-level build orchestration, test suite execution, code formatting, and QEMU runner automation.
* **QEMU & OVMF:** For running and testing the built UEFI app.
* **envsubst:** For generating flags

### Project Structure
```text
.
├── include/           # Header files (allocators, utils, macros)
├── src/               # Source files (.c)
├── tools/
│   └── clang-tidy-uefi/ # Source code for the custom Clang-Tidy AST matchers (C++)
├── Makefile           # Build and analysis orchestration
├── LogInDriver.inf    # UEFI module definition
└── LogInDriver.dsc    # UEFI package description
```

## Usage & Build Instructions(Linux)

```bash
export WORKSPACE_DIR_V=/home/var4p/Documents/uefi_workspace/edk2/..
make build WORKSPACE_DIR_V=/home/var4p/Documents/uefi_workspace/edk2/.. # building the driver
make run DISK_DIR_V=/home/var4p/Documents/disk
```

make -C edk2/BaseTools -j$(nproc)

