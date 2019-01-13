#include "Utils.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// reg2k
namespace Utils {
    bool ReadMemory(uintptr_t addr, void* data, size_t len) {
        DWORD oldProtect;
        if (VirtualProtect((void*)addr, len, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            memcpy(data, (void*)addr, len);
            if (VirtualProtect((void*)addr, len, oldProtect, &oldProtect))
                return true;
        }
        return false;
    }

    void WriteMemory(uintptr_t addr, void* data, size_t len) {
        DWORD oldProtect;
        VirtualProtect((void*)addr, len, PAGE_EXECUTE_READWRITE, &oldProtect);
        memcpy((void*)addr, data, len);
        VirtualProtect((void*)addr, len, oldProtect, &oldProtect);
    }

    uintptr_t GetRelative(uintptr_t start, int offset, int instructionLength) {
        int32_t rel32 = 0;
        ReadMemory(start + offset, &rel32, sizeof(int32_t));
        return start + instructionLength + rel32;
    }
}