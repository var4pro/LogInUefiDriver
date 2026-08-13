#pragma once
#include <Uefi.h>

extern VOID* Var4alloc(UINTN size);

extern void cleanup_var4free(void* pp);
#define AUTO_FREE __attribute__((cleanup(cleanup_var4free)))