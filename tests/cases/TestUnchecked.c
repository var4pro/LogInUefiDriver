#include "LogUtils.h"
#include "Utils.h"
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Uefi/UefiBaseType.h>

EFI_STATUS EFIAPI MockEfiStatusFunc() { return EFI_SUCCESS; }
RETURN_STATUS EFIAPI MockReturnStatusFunc() { return RETURN_SUCCESS; }

VOID EFIAPI MockVoidFunc() { return; }
UINTN EFIAPI MockUintnFunc() { return 0; }

// Test Cases
EFI_STATUS EFIAPI Test1_DirectIgnoredEfiStatus() { // 1. Direct ignored EFI_STATUS call
    MockEfiStatusFunc();                           // WARNING
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI Test2_DirectIgnoredReturnStatus() { // 2. Direct ignored RETURN_STATUS call
    MockReturnStatusFunc();                           // WARNING
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI Test3_AssignedToVariable() { // PASS  3. Stored in existing variable
    EFI_STATUS Status;
    Status = MockEfiStatusFunc();
    return Status;
}

EFI_STATUS EFIAPI Test4_InitializationDeclaration() { // PASS  4. Initialized with variable declaration
    EFI_STATUS Status = MockEfiStatusFunc();
    return Status;
}

EFI_STATUS EFIAPI Test5_DirectReturn() { // PASS 5. Returned directly
    return MockEfiStatusFunc();
}

EFI_STATUS EFIAPI Test6_DirectIfCondition() { // PASS  6. Evaluated directly in an if-condition
    if (EFI_ERROR(MockEfiStatusFunc())) return EFI_DEVICE_ERROR;
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI Test7_CheckForErrorMacro() { // PASS  7. Wrapped in CHECK_FOR_ERROR macro
    CHECK_FOR_ERROR(MockEfiStatusFunc());
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI Test8_LogIfErrorMacro() { // PASS   8. Wrapped in LOG_IF_ERROR macro
    LOG_IF_ERROR(MockEfiStatusFunc());
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI Test9_ExplicitIgnoreReturn() { // PASS  9. Wrapped in IGNORE_RETURN macro / (void) cast
    IGNORE_RETURN(MockEfiStatusFunc());
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI Test10_NestedBlockIgnored() { // 10. Ignored inside a nested block
    {
        MockEfiStatusFunc(); // WARNING
    }
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI Test11_InsideIfBranch(BOOLEAN Condition) { // 11. Ignored inside an if-statement block
    if (Condition) MockEfiStatusFunc();                      // WARNING
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI Test12_NonStatusCalls() { // PASS  12. Non-status return functions (Make sure no false positives occur)
    MockVoidFunc();
    MockUintnFunc();
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI Test13_MixedCalls() {   // 13. Mixed calls: one handled, one ignored
    CHECK_FOR_ERROR(MockEfiStatusFunc()); // PASS
    MockEfiStatusFunc();                  // WARNING
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI Test14_ProtocolCallIgnored(IN EFI_BOOT_SERVICES* BS,
                                             IN EFI_EVENT Event) { // 14. Calling Protocol/Table function pointer ignored
    BS->CloseEvent(Event);                                         // WARNING
    return EFI_SUCCESS;
}

// Type Casting (Traps and Escape Hatches)
EFI_STATUS EFIAPI Test15_CastToVoid() { // PASS - The official escape hatch
    (void)MockEfiStatusFunc();
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI Test16_CStyleCastIgnored() { // 16. C-Style cast to another type, but still ignored
    (UINTN) MockEfiStatusFunc();               // WARNING
    return EFI_SUCCESS;
}

#ifdef __cplusplus
EFI_STATUS EFIAPI Test17_CppStaticCastIgnored() { // 17. C++ static_cast, but still ignored
    static_cast<UINT32>(MockEfiStatusFunc());     // WARNING
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI Test18_FunctionalCastIgnored() { // 18. C++ Functional cast, but still ignored
    UINTN(MockEfiStatusFunc());                    // WARNING
    return EFI_SUCCESS;
}
#endif

// GNU Statement Expressions
EFI_STATUS EFIAPI Test19_GnuBlockInnerIgnored() { // 19. Ignored INSIDE a GNU block that is assigned
    EFI_STATUS Status = ({
        MockEfiStatusFunc(); // WARNING (inner discarded)
        MockVoidFunc();
        EFI_SUCCESS; // This is the yielded value
    });
    return Status;
}

EFI_STATUS EFIAPI Test20_GnuBlockYieldUsed() { // PASS - Yielded from a GNU block into a variable
    EFI_STATUS Status = ({
        MockVoidFunc();
        MockEfiStatusFunc(); // Yielded value, safely assigned
    });
    return Status;
}

EFI_STATUS EFIAPI Test21_GnuBlockCompletelyIgnored() { // 21. The whole GNU block is discarded
    ({
        MockVoidFunc();
        MockEfiStatusFunc(); // WARNING (yielded, but the whole block is ignored)
    });
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI Test22_NestedGnuBlocks() { // 22. Crazy macro expansions
    EFI_STATUS Status = ({
        ({
            MockEfiStatusFunc(); // WARNING (inner block discarded)
            MockVoidFunc();
        });
        MockEfiStatusFunc(); // PASS (yielded to Status)
    });
    return Status;
}

// Brace-less Control Flow
EFI_STATUS EFIAPI Test23_BracelessIfElse(BOOLEAN Condition) { // 23. No braces on if/else
    if (Condition)
        MockEfiStatusFunc(); // WARNING
    else
        MockReturnStatusFunc(); // WARNING

    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI Test24_BracelessWhile(BOOLEAN Condition) { // 24. No braces on while
    while (Condition) MockEfiStatusFunc();                   // WARNING

    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI Test25_BracelessDoWhile(BOOLEAN Condition) { // 25. No braces on do-while
    do 
        MockEfiStatusFunc();                                    // WARNING
    while (Condition);

    return EFI_SUCCESS;
}

// For-Loop Mechanics (Init, Cond, Inc)
EFI_STATUS EFIAPI Test26_ForLoopInit() { // 26. Ignored in loop initialization
    for (MockEfiStatusFunc(); FALSE;) {  // WARNING
    }
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI Test27_ForLoopCondition() { // PASS - Used as a boolean condition!
    // The compiler evaluates this, so it is NOT discarded.
    for (; MockEfiStatusFunc() == EFI_SUCCESS;) {
    }
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI Test28_ForLoopIncrement() {   // 28. Ignored in loop increment step
    for (int i = 0; i < 5; MockEfiStatusFunc()) // WARNING
        i++;
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI Test29_BracelessForLoopBody() {       // 29. Ignored in brace-less for-loop body
    for (int i = 0; i < 5; i++) MockReturnStatusFunc(); // WARNING

    return EFI_SUCCESS;
}

#ifdef __cplusplus
EFI_STATUS EFIAPI Test30_RangeBasedForLoop() { // 30. Brace-less C++ Range-for
    int arr[] = {1, 2, 3};
    for (auto x : arr) MockEfiStatusFunc(); // WARNING

    return EFI_SUCCESS;
}
#endif

// Switch Statements (Braceless Cases)
EFI_STATUS EFIAPI Test31_SwitchCases(UINTN Value) { // 31. Switch statement scopes
    switch (Value) {
    case 1:
        MockEfiStatusFunc(); // WARNING (Braceless case)
        break;
    case 2: {
        MockReturnStatusFunc(); // WARNING (Braced case, caught by compoundStmt)
        break;
    }
    default:
        MockEfiStatusFunc(); // WARNING (Braceless default)
        break;
    }
    return EFI_SUCCESS;
}

// Edge Cases & Struct Initialization
EFI_STATUS EFIAPI Test32_StructInitialization() { // PASS - Initializer list (Not a GNU block!)
    struct {
        EFI_STATUS Status;
        UINTN Data;
    } MyStruct = {MockEfiStatusFunc(), 0}; // Used to initialize struct member

    return MyStruct.Status;
}

EFI_STATUS EFIAPI Test33_FunctionParameter() { // PASS - Passed as argument
    // Passing the return value directly into another function
    if (EFI_ERROR(Test11_InsideIfBranch(MockEfiStatusFunc() == EFI_SUCCESS))) return EFI_DEVICE_ERROR;
    return EFI_SUCCESS;
}