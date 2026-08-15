#pragma once
#include <Uefi.h>
#include <Library/CpuLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <ProcessorBind.h>

#define STR16_LEN(str) ((sizeof(str) / sizeof(CHAR16)) - 1)

#define ASSERT_RELEASE(Condition)                                                    \
    do {                                                                             \
        if (!(Condition)) {                                                          \
            DEBUG((DEBUG_ERROR, "Assertion failed in %a:%d\n", __FILE__, __LINE__)); \
            CpuDeadLoop();                                                           \
        }                                                                            \
    } while (0)

#define IGNORE_RETURN(Expr) ((void)(Expr))

#define CHECK_FOR_ERROR(EfiCall)                                                                                    \
    do {                                                                                                            \
        EFI_STATUS _macro_status = (EfiCall);                                                                       \
        if (EFI_ERROR(_macro_status)) {                                                                             \
            DEBUG((DEBUG_ERROR, "Error executing %a in %a:%d: %r\n", #EfiCall, __FILE__, __LINE__, _macro_status)); \
            return _macro_status;                                                                                   \
        }                                                                                                           \
    } while (0)