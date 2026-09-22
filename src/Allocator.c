#include "LogUtils.h"
#include "Allocator.h"

#include <Uefi.h>
#include <Base.h>

// #include <Library/BaseCryptLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <ProcessorBind.h>
#include <Protocol/GraphicsOutput.h>
// #include <Protocol/Tcg2Protocol.h>
#include <Uefi/UefiBaseType.h>

VOID* Var4alloc(UINTN size) {
    TRACE_FUNCTION();
    VOID* buffer = NULL;

    if (EFI_ERROR(gBS->AllocatePool(EfiBootServicesData, size, &buffer))) {
        DEBUG((DEBUG_ERROR, "Failed to allocate memory"));
        return NULL;
    }

    ZeroMem(buffer, size);

    return buffer;
}

VOID _internal_cleanup_var4free(void* pp) { // NOLINT(cert-dcl37-c,bugprone-reserved-identifier,cert-dcl51-cpp)
    TRACE_FUNCTION();
    void** ptr_to_ptr = (void**)pp;

    if (ptr_to_ptr && *ptr_to_ptr) {
        LOG_IF_ERROR(gBS->FreePool(*ptr_to_ptr));
        *ptr_to_ptr = NULL; // stack ptr cleanup
    }
}
static VOID* SecureZeroMem(void* ptr, INTN len) {
    TRACE_FUNCTION();
    if (!ptr) return ptr;

    volatile UINT8* vptr = (volatile UINT8*)ptr;
    for (INTN i = 0; i < len; i++) vptr[i] = 0;

    MemoryFence();
    return ptr;
}

VOID _internal_cleanup_zero_char(
    char (*pp)[GENERAL_ARRAY_MAX_LEN]) { // NOLINT(cert-dcl37-c,bugprone-reserved-identifier,cert-dcl51-cpp)
    TRACE_FUNCTION();
    SecureZeroMem(pp, GENERAL_ARRAY_MAX_LEN);
}
VOID _internal_cleanup_zero_uint8(
    UINT8 (*pp)[GENERAL_ARRAY_MAX_LEN]) { // NOLINT(cert-dcl37-c,bugprone-reserved-identifier,cert-dcl51-cpp)
    TRACE_FUNCTION();
    SecureZeroMem(pp, GENERAL_ARRAY_MAX_LEN);
}