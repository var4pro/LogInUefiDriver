#include "LogUtils.h"
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Uefi/UefiBaseType.h>

EFI_STATUS EFIAPI DriverEntryPoint(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE* SystemTable) { return EFI_SUCCESS; }

EFI_STATUS EFIAPI Test1() { return EFI_SUCCESS; } // WARNING

EFI_STATUS EFIAPI Test2() {
    TRACE_FUNCTION(); // PASS
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI Test3() {
    // TRACE_FUNCTION(); // WARNING
    return EFI_SUCCESS;
}
EFI_STATUS EFIAPI Test4() {
    /* TRACE_FUNCTION(); */ // WARNING
    return EFI_SUCCESS;
}
EFI_STATUS EFIAPI Test5() {
    Print(L"TRACE_FUNCTION();"); // WARNING
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI Test6() {
    EFI_STATUS Status = EFI_SUCCESS; // WARNING
    TRACE_FUNCTION();
    return Status;
}

#define TRACE_FUNCTION_FAKE() Print(L"Fake!")
EFI_STATUS EFIAPI Test7() {
    TRACE_FUNCTION_FAKE(); // WARNING
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI Test8() {
    { // WARNING
        TRACE_FUNCTION();
    }
    return EFI_SUCCESS;
}

#define EMPTY_TRACE()

EFI_STATUS EFIAPI Test9() {
    EMPTY_TRACE(); // WARNING
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI Test10() { // PASS
                             // comment
    TRACE_FUNCTION();
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI Test11() {
    TRACE_FUNCTION(); // PASS
    return EFI_SUCCESS;
}

// 12. Completely empty function (Tests the body_empty() branch)
VOID EFIAPI Test12_EmptyFunction() {} // WARNING

// 13. Accidental leading semicolon (Creates a NullStmt before TRACE_FUNCTION)
EFI_STATUS EFIAPI Test13_LeadingSemicolon() {
    ; // WARNING
    TRACE_FUNCTION();
    return EFI_SUCCESS;
}

// 14. Function declaration / prototype only (Must NOT crash or throw warning)
EFI_STATUS EFIAPI Test14_PrototypeOnly(IN UINTN SomeParam); // PASS

// 15. VOID return type with parameters (Ensures non-status functions are still enforced)
VOID EFIAPI Test15_VoidWithParams(IN INTN A, IN INTN B) {
    TRACE_FUNCTION(); // PASS
}

// 16. Wrapped inside a composite macro (Tests your macro-unwinding while loop!)
#define INIT_MY_DRIVER() TRACE_FUNCTION();

EFI_STATUS EFIAPI Test16_WrappedInCompositeMacro() {
    INIT_MY_DRIVER(); // PASS
    return EFI_SUCCESS;
}

#undef TRACE_FUNCTION
#define TRACE_FUNCTION()
EFI_STATUS EFIAPI Test17() {
    TRACE_FUNCTION(); // WARNING
    return EFI_SUCCESS;
}
