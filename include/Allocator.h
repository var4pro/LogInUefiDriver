#pragma once
#include <ProcessorBind.h>
#include <Uefi.h>

static constexpr INTN GENERAL_ARRAY_MAX_LEN = 128;

extern VOID* Var4alloc(UINTN size);

extern void cleanup_var4free(void* pp);
extern void cleanup_zero(void* pp);
#define AUTO_FREE __attribute__((cleanup(cleanup_var4free)))
#define AUTO_SET_TO_ZERO __attribute__((cleanup(cleanup_zero)))