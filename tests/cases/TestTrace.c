#include "LogUtils.h"
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Uefi/UefiBaseType.h>

EFI_STATUS EFIAPI DriverEntryPoint(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE* SystemTable) { return EFI_SUCCESS; }

EFI_STATUS EFIAPI Test1() { return EFI_SUCCESS; } // WARNING

EFI_STATUS EFIAPI Test2() { // PASS
    TRACE_FUNCTION();
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI Test3() { // WARNING
    // TRACE_FUNCTION();
    return EFI_SUCCESS;
}
EFI_STATUS EFIAPI Test4() { // WARNING
    /* TRACE_FUNCTION(); */
    return EFI_SUCCESS;
}
EFI_STATUS EFIAPI Test5() { // WARNING
    Print(L"TRACE_FUNCTION();");
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI Test6() { // WARNING
    EFI_STATUS Status = EFI_SUCCESS;
    TRACE_FUNCTION();
    return Status;
}

#define TRACE_FUNCTION_FAKE() Print(L"Fake!")
EFI_STATUS EFIAPI Test7() { // WARNING
    TRACE_FUNCTION_FAKE();
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI Test8() { // WARNING
    {
        TRACE_FUNCTION();
    }
    return EFI_SUCCESS;
}

#define EMPTY_TRACE()

EFI_STATUS EFIAPI Test9() { // WARNING
    EMPTY_TRACE();
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI Test10() { // PASS

    TRACE_FUNCTION();
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI Test11() { // PASS
    TRACE_FUNCTION();
    // TRACE_FUNCTION();
    return EFI_SUCCESS;
}

#undef TRACE_FUNCTION
#define TRACE_FUNCTION()
EFI_STATUS EFIAPI Test12() { // WARNING
    TRACE_FUNCTION();
    // TRACE_FUNCTION();
    return EFI_SUCCESS;
}