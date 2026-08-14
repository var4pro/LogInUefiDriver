# UEFI Secure Login Driver

A proof-of-concept UEFI driver focused on secure user authentication, memory safety, and TPM 2.0 interaction. This project goes beyond standard UEFI development by implementing RAII-style memory management in C and enforcing strict project architectures via custom Clang AST static analysis.

## Key Features

* **Secure Credential Handling:** Safe password input implementation without reliance on null-terminators (preventing classic buffer overflows). Sensitive stack arrays are automatically zeroed out upon function exit.
* **C-Style RAII (`__attribute__((cleanup))`):** Utilizes compiler extensions to ensure memory is automatically freed (`AUTO_FREE`) and sensitive data is securely wiped (`AUTO_SET_TO_ZERO`), eliminating memory leaks and data exposure even during early returns.
* **Custom Static Analysis (SAST):** Includes a custom-built Clang-Tidy plugin (written in C++ using LLVM/Clang AST Matchers) to enforce secure coding standards at compile time.
* **TPM 2.0 Integration (Mocked):** Demonstrates the conceptual workflow of unsealing secrets bound to a TPM using a password-based authorization session.

## Project Architecture & Security

### 1. Memory Safety
Standard UEFI allocators (`AllocatePool`) can leave uninitialized data. This project uses a custom wrapper (`Var4alloc`) that forces zero-initialization. Furthermore, the `AUTO_SET_TO_ZERO` macro ensures that sensitive buffers (like passwords and unsealed secrets) are zeroed out locally on the stack when they go out of scope, defending against use-of-uninitialized-value and stack-reading exploits.

### 2. Custom Clang-Tidy Plugin (`tools/clang-plugins`)
To enforce security and architectural constraints, a custom AST Matcher was built. It enforces two strict rules during the `make tidy` step:
1. **Bans standard allocators:** Fails the analysis if standard functions like `AllocatePool` or `FreePool` are used, forcing developers to use the safe project-specific `Var4alloc`.
2. **Enforces Tracing:** Ensures that every function definition (except the entry point) begins with the `TRACE_FUNCTION()` macro for consistent debugging.

## Getting Started

### Prerequisites
* **EDK2 Workspace:** You must have a configured EDK2 environment.
* **LLVM/Clang Toolchain:** Required for formatting and the custom static analyzer (tested with LLVM 14).
* **CMake:** To build the custom Clang-Tidy plugin.
* **QEMU & OVMF:** For running and testing the built UEFI app.

### Project Structure
```text
.
├── include/           # Header files (allocators, utils, macros)
├── src/               # Source files (.c)
├── tools/
│   └── clang-plugins/ # Source code for the custom Clang-Tidy AST matchers (C++)
├── Makefile           # Build and analysis orchestration
├── LogInDriver.inf    # UEFI module definition
└── LogInDriver.dsc    # UEFI package description
```

## Usage & Build Instructions

The project uses an automated Makefile. Set your specific EDK2 workspace and output paths via environment variables.

### 1. Build the Custom Static Analyzer
Before running code checks, compile the Clang-Tidy module:
```bash
make build-tools
```

### 2. Check & Format Code
Run the custom static analysis and format checker. This verifies that all security rules (no standard allocators, correct tracing) are met:
```bash
make check-all
```
*(Check `tidy_report.txt` for the detailed analysis output).*

### 3. Build the UEFI Driver
Compile the driver against the EDK2 framework. Replace the paths with your local EDK2 setup:
```bash
make build WORKSPACE_DIR_V=/path/to/your/uefi_workspace DISK_DIR_V=/path/to/virtual/disk
```

### 4. Run in QEMU
Build the EFI executable, copy it to the virtual disk, and launch QEMU with OVMF:
```bash
make run WORKSPACE_DIR_V=/path/to/your/uefi_workspace DISK_DIR_V=/path/to/virtual/disk
```

## Note on TPM Interaction
Due to constraints in the virtual testing environment, the direct TPM 2.0 interactions (`SubmitCommand` for `TPM_CC_Unseal`) are currently mocked. The codebase includes the data structures and flow necessary for a Cleartext Password Session (`TPM_RS_PW`), demonstrating the intended cryptography architecture.

---
*Built with C23 standard features, GCC, and Clang tooling.*