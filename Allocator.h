#pragma once
#include <Uefi.h>

extern VOID* Var4alloc(UINTN size);
extern VOID Var4free(VOID* ptr);

static inline void _auto_var4free(void* pp) {
    // Приводим к указателю на указатель
    void** ptr_to_ptr = (void**)pp;

    if (ptr_to_ptr != NULL && *ptr_to_ptr != NULL) {
        // 1. Освобождаем память в куче
        Var4free(*ptr_to_ptr);

        // 2. Зануляем саму переменную на стеке!
        *ptr_to_ptr = NULL;
    }
}

#define AUTO_FREE __attribute__((cleanup(_auto_var4free)))