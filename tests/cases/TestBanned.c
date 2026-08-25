#include "Allocator.h"

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>

// Mock Definitions

// 1. Direct library functions (from MemoryAllocationLib)
extern EFI_STATUS EFIAPI AllocatePool(IN EFI_MEMORY_TYPE PoolType, IN UINTN Size, OUT VOID** Buffer);
extern EFI_STATUS EFIAPI AllocatePages(IN EFI_ALLOCATE_TYPE Type, IN EFI_MEMORY_TYPE MemoryType, IN UINTN Pages,
                                       IN OUT EFI_PHYSICAL_ADDRESS* Memory);
extern EFI_STATUS EFIAPI FreePool(IN VOID* Buffer);
extern EFI_STATUS EFIAPI FreePages(IN EFI_PHYSICAL_ADDRESS Memory, IN UINTN Pages);

// =============================================================================
// Test Cases
// =============================================================================

// --- DIRECT LIBRARY CALLS (declRefExpr) ---

VOID Test1_BannedDirectAlloc(VOID) {
    VOID** Ptr = NULL;
    AllocatePool(EfiBootServicesData, 128, Ptr); // WARNING
}

VOID Test2_BannedDirectFree(VOID* Ptr) { FreePool(Ptr); } // WARNING

// --- BOOT SERVICES TABLE CALLS (memberExpr) ---
VOID Test3_BannedTableAlloc(VOID** Ptr) {
    if (TRUE) gBS->AllocatePool(EfiBootServicesData, 128, Ptr); // WARNING
}

VOID Test4_BannedTableFree(VOID* Ptr) {
    if (TRUE) gBS->FreePool(Ptr); // WARNING
}

// --- ALLOWED ALTERNATIVES & UNRELATED CALLS (PASS) ---

VOID Test5_AllowedCustomAllocator(VOID) { // PASS
    AUTO_FREE VOID* Ptr = Var4alloc(128);
}
VOID Test6_AllowedUnrelatedTableCall(EFI_EVENT Event) { // PASS
    gBS->CloseEvent(Event);
}

// =============================================================================
// EXTRA TESTS: Pages Allocation (Direct & Table)
// =============================================================================

VOID Test7_BannedDirectPages(VOID) {
    EFI_PHYSICAL_ADDRESS Addr = 0;
    AllocatePages(AllocateAnyPages, EfiBootServicesData, 1, &Addr); // WARNING
    FreePages(Addr, 1);                                             // WARNING
}

VOID Test8_BannedTablePages(VOID) {
    EFI_PHYSICAL_ADDRESS Addr = 0;
    gBS->AllocatePages(AllocateAnyPages, EfiBootServicesData, 1, &Addr); // WARNING
    gBS->FreePages(Addr, 1);                                             // WARNING
}

// Tricky C Syntax & Macro Evasions

VOID Test9_ParenthesizedCalls(VOID** Ptr) {
    // Developers sometimes wrap function names in parentheses to bypass macros
    (AllocatePool)(EfiBootServicesData, 128, Ptr);        // WARNING
    ((gBS)->AllocatePool)(EfiBootServicesData, 128, Ptr); // WARNING
}

VOID Test10_InsideConditionOrMacro(VOID** Ptr) {
    // Calling banned allocator inside an if-condition
    if (gBS->AllocatePool(EfiBootServicesData, 128, Ptr) == EFI_SUCCESS) { // WARNING
        // ...
    }
}

// False Positive Guards (Must all PASS)

// Helper mock functions with similar names
VOID AllocatePoolCustom(UINTN Size) { (VOID) Size; }
VOID FreePoolCustom(VOID* Ptr) { (VOID) Ptr; }

VOID Test11_SimilarFunctionNames(VOID) { // PASS
    // Make sure 'hasAnyName' is doing exact matches, not substring matches!
    AllocatePoolCustom(128);
    FreePoolCustom(NULL);
}

VOID Test12_CustomStructMemberWithSimilarName(VOID) { // PASS
    struct CustomAllocator {
        VOID (*AllocatePoolCustom)(UINTN Size);
    } MyAlloc;

    MyAlloc.AllocatePoolCustom = AllocatePoolCustom;
    MyAlloc.AllocatePoolCustom(128);
}