#include "Allocator.h"
#include <Uefi.h>
#include "Library/BaseMemoryLib.h"
#include "Library/DebugLib.h"
#include <Library/UefiBootServicesTableLib.h>

VOID* Var4alloc(UINTN size) {
    VOID* buffer = NULL;

    if (EFI_ERROR(gBS->AllocatePool(EfiBootServicesData, size, &buffer))){ 
        DEBUG((DEBUG_ERROR, "Failed to allocate memory"));
        return NULL;
    }

    ZeroMem(buffer, size);

    return buffer;
}

VOID cleanup_var4free(void* pp) {
    void** ptr_to_ptr = (void**)pp;

    if (ptr_to_ptr && *ptr_to_ptr) {
        gBS->FreePool(*ptr_to_ptr);
        *ptr_to_ptr = NULL;//stack ptr cleanup
    }
}