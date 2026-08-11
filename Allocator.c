#include "Allocator.h"
#include "Utils.h"
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Uefi.h>
#include <Library/DebugLib.h>


VOID* Var4alloc(UINTN size) {
    VOID* buffer = NULL;

    if (EFI_ERROR(gBS->AllocatePool(EfiBootServicesData, size, &buffer))){ 
        DEBUG((DEBUG_ERROR, "Failed to allocate memory"));
        return NULL;
    }

    ZeroMem(buffer, size);

    return buffer;
}

VOID Var4free(VOID* ptr) {
    if (ptr == NULL) return;
    gBS->FreePool(ptr);
}