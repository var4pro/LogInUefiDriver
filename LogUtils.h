#pragma once
#include <Base.h>
#include <Library/DebugLib.h>

__attribute__((always_inline)) static inline void trace_exit(const char** func_name) {
    DEBUG((DEBUG_VERBOSE, "[ <- EXIT ] %a\n", *func_name));
}

#define TRACE_FUNCTION()                                  \
    DEBUG((DEBUG_VERBOSE, "[ -> ENTER] %a\n", __func__)); \
    const char* __trace_dummy_var __attribute__((cleanup(trace_exit))) = __func__