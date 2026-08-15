#include "Allocator.h"
#include "LogUtils.h"
#include "Utils.h"

#include <Uefi.h>
#include <Base.h>

#include <IndustryStandard/Tpm20.h>
#include <Library/BaseCryptLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/TimerLib.h>
#include <Library/Tpm2CommandLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <ProcessorBind.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/Tcg2Protocol.h>
#include <Uefi/UefiBaseType.h>

static constexpr INTN MAX_PASS_LEN = 256;
static constexpr INTN MAX_SECRET_LEN = MAX_SYM_DATA;

static constexpr TPMI_DH_OBJECT g_master = 0x81000001;
static constexpr TPMI_DH_OBJECT g_itemHandle = 0x81010001;
static UINTN g_terminaСols = 0, g_terminalRows = 0;

[[nodiscard]] extern EFI_STATUS EFIAPI PrintForm1Time();
[[nodiscard]] extern EFI_STATUS EFIAPI GetUserPassword(OUT char userPass[], OUT INTN* i);
[[nodiscard]] extern EFI_STATUS EFIAPI UnsealSecret(IN char userPass[], INTN userLen, OUT UINT8 secretBuffer[], INTN maxSecretLen,
                                                    OUT INTN* actualSecretLen);
[[nodiscard]] extern EFI_STATUS EFIAPI MeasureSecretToTpm(IN UINT8 secretData[], INTN secretSize);

EFI_STATUS EFIAPI DriverEntryPoint(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE* SystemTable) {
    AUTO_SET_TO_ZERO char userPass[GENERAL_ARRAY_MAX_LEN] = {0};
    INTN i = 0;
    CHECK_FOR_ERROR(PrintForm1Time());
    CHECK_FOR_ERROR(GetUserPassword(userPass, &i));

    AUTO_SET_TO_ZERO UINT8 secretBuffer[GENERAL_ARRAY_MAX_LEN] = {0};
    INTN actualSize = 0;
    // Print(L"\'%a\'", userPass);
    CHECK_FOR_ERROR(UnsealSecret(userPass, i, secretBuffer, MAX_SECRET_LEN, &actualSize));
    CHECK_FOR_ERROR(MeasureSecretToTpm(secretBuffer, actualSize));

    return EFI_SUCCESS;
}

// print text and set cursor 1 row below in the center
EFI_STATUS EFIAPI PrintForm1Time() {
    TRACE_FUNCTION();
    CHECK_FOR_ERROR(gST->ConOut->ClearScreen(gST->ConOut));
    CHECK_FOR_ERROR(gST->ConOut->EnableCursor(gST->ConOut, FALSE));
    CHECK_FOR_ERROR(gST->ConOut->QueryMode(gST->ConOut, gST->ConOut->Mode->Mode, &g_terminaСols, &g_terminalRows));

    CHAR16 text[] = L"Enter the password below:";

    INTN textStartCol = (INTN)(g_terminaСols - STR16_LEN(text)) / 2;

    CHECK_FOR_ERROR(gST->ConOut->SetCursorPosition(gST->ConOut, textStartCol, g_terminalRows / 2));
    Print(text);

    return EFI_SUCCESS;
}

// gets user password from ConIn, shows user how many chars he typed, but doesn't show them
EFI_STATUS EFIAPI GetUserPassword(OUT char userPass[], OUT INTN* i) {
    TRACE_FUNCTION();
    CHECK_FOR_ERROR(gST->ConIn->Reset(gST->ConIn, false));
    EFI_INPUT_KEY key = {0};
    UINTN eventIndex = 0;

    INTN inputRow = (INTN)(g_terminalRows / 2) + 2;

    while (TRUE) {
        gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &eventIndex);
        if (gST->ConIn->ReadKeyStroke(gST->ConIn, &key) != EFI_SUCCESS) continue;

        if (key.UnicodeChar != 0) {
            if (key.UnicodeChar == 0x08 && *i > 0) { // backspace
                (*i)--;
                userPass[*i] = 0;
            } else if (key.UnicodeChar == 0x0D) { // enter
                break;
            } else if (*i < MAX_PASS_LEN && key.UnicodeChar >= 0x20 && key.UnicodeChar <= 0x7E) {
                userPass[*i] = (char)key.UnicodeChar;
                (*i)++;
            }
        }

        INTN clearWidth = MAX_PASS_LEN + 4;
        INTN clearStartCol = MIN(((INTN)g_terminaСols - clearWidth) / 2, 0);
        CHECK_FOR_ERROR(gST->ConOut->SetCursorPosition(gST->ConOut, clearStartCol, inputRow));
        for (INTN j = 0; j < clearWidth; j++) Print(L" ");

        if (*i > 0) {
            INTN asterisksStartCol = MIN(((INTN)g_terminaСols - *i) / 2, 0);
            CHECK_FOR_ERROR(gST->ConOut->SetCursorPosition(gST->ConOut, asterisksStartCol, inputRow));

            for (INTN j = 0; j < *i; j++) Print(L"*");
        }
    }

    CHECK_FOR_ERROR(gST->ConOut->SetCursorPosition(gST->ConOut, 0, inputRow + 2));
    DEBUG((DEBUG_INFO, "Password received from user"));
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI UnsealSecret(IN char userPass[], INTN userLen, OUT UINT8 secretBuffer[], INTN maxSecretLen,
                               OUT INTN* actualSecretLen) {
    TRACE_FUNCTION();
    return EFI_SUCCESS;
}; // mock
EFI_STATUS EFIAPI MeasureSecretToTpm(IN UINT8 secretData[], INTN secretSize) {
    TRACE_FUNCTION();
    return EFI_SUCCESS;
}; // mock

// #pragma pack(1)
// typedef struct {
//     TPMI_ST_COMMAND_TAG tag;
//     UINT32 commandSize;
//     TPM_CC commandCode;

//     TPMI_DH_OBJECT itemHandle;
//     UINT32 authorizationSize;
//     UINT32 authHandle;
//     UINT16 nonceCallerSize;  // 0 in my case
//     UINT8 sessionAttributes; // may be only 1
//     UINT16 hmacSize;
//     UINT8 hmac[MAX_PASS_LEN];
// } TPM2_UNSEAL_COMMAND_LOCAL;

// typedef struct {
//     TPM_ST tag;
//     UINT32 responseSize;
//     TPM_RC responseCode;
//     UINT8 outData[0];
// } TPM2_UNSEAL_RESPONSE_LOCAL;
// #pragma pack()

// EFI_STATUS EFIAPI UnsealSecret(IN char userPass[], INTN userLen, OUT UINT8 secretBuffer[], INTN maxSecretLen,
//                                OUT INTN* actualSecretLen) {
//     TRACE_FUNCTION();
//     if (userPass == NULL || secretBuffer == NULL || actualSecretLen == NULL) return EFI_INVALID_PARAMETER;
//     EFI_TCG2_PROTOCOL* tcg2Protocol;
//     CHECK_FOR_ERROR(gBS->LocateProtocol(&gEfiTcg2ProtocolGuid, NULL, (VOID**)&tcg2Protocol));

//     UINT32 authSize = sizeof(TPM2_UNSEAL_COMMAND_LOCAL) -
//                       (sizeof(TPMI_ST_COMMAND_TAG) + sizeof(UINT32) + sizeof(TPM_CC) + sizeof(TPMI_DH_OBJECT) +
//                       sizeof(UINT32));

//     TPM2_UNSEAL_COMMAND_LOCAL cmd;
//     ZeroMem(&cmd, sizeof(cmd));
//     cmd.tag = SwapBytes16(TPM_ST_SESSIONS);       // 0x8002
//     cmd.commandSize = SwapBytes32(sizeof(cmd));   // Total command size
//     cmd.commandCode = SwapBytes32(TPM_CC_Unseal); // 0x0000015E
//     cmd.itemHandle = SwapBytes32(g_itemHandle);
//     cmd.authorizationSize = SwapBytes32(authSize);    // Size of auth section
//     cmd.authHandle = SwapBytes32(TPM_RS_PW);          // 0x40000009
//     cmd.nonceCallerSize = SwapBytes16(0);             // No nonce
//     cmd.sessionAttributes = 0x00;                     // Session flags
//     cmd.hmacSize = SwapBytes16((UINT16)MAX_PASS_LEN); // Size of HMAC buffer
//     Sha256HashAll(userPass, userLen, cmd.hmac);

//     UINT8 respBuffer[1024];
//     ZeroMem(respBuffer, sizeof(respBuffer));
//     DEBUG((DEBUG_INFO, "The command will send now"));
//     CHECK_FOR_ERROR(tcg2Protocol->SubmitCommand(tcg2Protocol, sizeof(cmd), (UINT8*)&cmd, sizeof(respBuffer), respBuffer));
//     TPM2_UNSEAL_RESPONSE_LOCAL* resp = (TPM2_UNSEAL_RESPONSE_LOCAL*)respBuffer;

//     resp->responseCode = SwapBytes32(resp->responseCode);
//     if (resp->responseCode != TPM_RC_SUCCESS){
//         DEBUG((DEBUG_INFO, "Sealing secret from tpm failed!"));
//         return EFI_ACCESS_DENIED;
//     }
//     DEBUG((DEBUG_INFO, "Sealing secret from tpm succeed!"));

//     // UINT16 responseTag = SwapBytes16(resp->tag);

//     return EFI_SUCCESS;
// }

// EFI_STATUS EFIAPI MeasureSecretToTpm(IN UINT8 secretData[], INTN secretSize) {
//     TRACE_FUNCTION();
//     if (secretSize == 0) return EFI_INVALID_PARAMETER;

//     EFI_TCG2_PROTOCOL* tcg2Protocol;
//     CHECK_FOR_ERROR(gBS->LocateProtocol(&gEfiTcg2ProtocolGuid, NULL, (VOID**)&tcg2Protocol));

//     // shit for logging
//     char eventString[] = "Unsealed Secret Measured";
//     INTN totalEventSize = sizeof(EFI_TCG2_EVENT_HEADER) + sizeof(UINT32) + sizeof(eventString);

//     AUTO_FREE EFI_TCG2_EVENT* tcgEvent = Var4alloc(totalEventSize);
//     if (tcgEvent == NULL) return EFI_OUT_OF_RESOURCES;

//     tcgEvent->Size = (UINT32)totalEventSize;
//     tcgEvent->Header.HeaderSize = sizeof(EFI_TCG2_EVENT_HEADER);
//     tcgEvent->Header.HeaderVersion = EFI_TCG2_EVENT_HEADER_VERSION;
//     tcgEvent->Header.PCRIndex = 12;
//     tcgEvent->Header.EventType = EV_COMPACT_HASH;
//     CopyMem(tcgEvent->Event, eventString, sizeof(eventString));

//     CHECK_FOR_ERROR(tcg2Protocol->HashLogExtendEvent(tcg2Protocol, 0, (EFI_PHYSICAL_ADDRESS)(UINTN)secretData,
//     (UINT64)secretSize,tcgEvent));

//     return EFI_SUCCESS;
// }
