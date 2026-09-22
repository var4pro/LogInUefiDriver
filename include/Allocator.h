#pragma once
#include <ProcessorBind.h>
#include <Uefi.h>

static constexpr INTN GENERAL_ARRAY_MAX_LEN = 128;

extern VOID* Var4alloc(UINTN size);

// extern void _internal_cleanup_var4free(void* pp); // NOLINT(cert-dcl37-c,bugprone-reserved-identifier,cert-dcl51-cpp)
extern void _internal_cleanup_zero_char( // NOLINT(cert-dcl37-c,bugprone-reserved-identifier,cert-dcl51-cpp)
    char (*pp)[GENERAL_ARRAY_MAX_LEN]);
extern void _internal_cleanup_zero_uint8( // NOLINT(cert-dcl37-c,bugprone-reserved-identifier,cert-dcl51-cpp)
    UINT8 (*pp)[GENERAL_ARRAY_MAX_LEN]);

// #define AUTO_FREE __attribute__((cleanup(_internal_cleanup_var4free)))
#define AUTO_SET_TO_ZERO_UINT8 __attribute__((cleanup(_internal_cleanup_zero_uint8)))
#define AUTO_SET_TO_ZERO_CHAR __attribute__((cleanup(_internal_cleanup_zero_char)))

typedef struct {
    UINTN size;
    UINT8 buffer[];
} Uint8Buffer;
