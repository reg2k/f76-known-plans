#include "KnownPlans.h"
#include "Utils.h"
#include "rva/RVA.h"

#include <shlobj.h>
#include <cstdio>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define XBYAK_NO_OP_NAMES
#include "xbyak/xbyak.h"

using namespace Utils;

namespace KnownPlans {
    // Globals
    void** g_entityManager = nullptr;

    // Functions
    using _GetLocalPlayer = void*(*)(void* entityManager, void** val_out);
    _GetLocalPlayer GetLocalPlayer;

    using _HasBeenRead = bool(*)(void* refr, void* book);
    _HasBeenRead HasBeenRead;

    using _CopyToBSFixedString = bool(*)(void* dest, const char* src, uint32_t unk3);
    _CopyToBSFixedString CopyToBSFixedString;

    // Hook
    uintptr_t Hook_Start;

    // ASM
    uint8_t localCodeBuf[4096 + 16]; // extra 16 bytes for alignment
    uint8_t remoteCodeBuf[16];       // have max 16 bytes at target site
    size_t remoteCodeBufSize = 0;

    // Sig
    RVA<uintptr_t> g_sig1("F6 83 D0 01 00 00 35 41 0F 95 C7 48 8D 55 A8 48 8B 0D ? ? ? ? E8 ? ? ? ? 90 48 8B D3 48 8B 08 E8 ? ? ? ? 44 0F B6 E0");
    RVA<uintptr_t> g_sig2("48 8D 55 98 49 8B CE E8 ? ? ? ? 45 33 C0 48 8B D0 48 8D 4C 24 ? E8 ? ? ? ? 49 8B CE");

    // Logging
    FILE* logfile = nullptr;
    void log(const char* format, ...) {
        static char outputBuf[8192];
        if (logfile) {
            va_list args; va_start(args, format);
            vsnprintf(outputBuf, sizeof(outputBuf), format, args);
            va_end(args);
            fprintf(logfile, "%s\n", outputBuf);
            fflush(logfile);
        }
    }

    void GetItemName(void** ppForm, void* bsStr_out, const char* name) {
        const char* result = name;
        void* form = *ppForm;
        char str[1024] = u8"[\u00B6] ";

        // Check form type
        char formType = GetOffset<char>(form, 0x22);
        if (formType == 32) {

            // Check if learnable
            char flags = GetOffset<char>(form, 0x1D0);
            bool isLearnable = flags & 0x35;

            if (isLearnable) {
                void* player = nullptr;
                GetLocalPlayer(*g_entityManager, &player);

                if (player) {
                    if (HasBeenRead(player, form)) {
                        strcat_s(str, name);
                        result = str;
                    }
                }
            }
        }

        CopyToBSFixedString(bsStr_out, result, 0);
    }

    struct CodeTrampoline : Xbyak::CodeGenerator {
        CodeTrampoline(uint8_t buf[4096], uintptr_t continueAddr) : Xbyak::CodeGenerator(4096, buf) {
            Xbyak::Label handler, retnLabel;
            mov(rcx, r14);              // a1 = TESForm**
            lea(rdx, ptr[rsp + 0x68]);  // a2 = BSFixedString
            mov(r8, rax);               // a3 = const char*
            call(ptr[rip + handler]);
            jmp(ptr[rip + retnLabel]);

            L(handler);
            dq((uintptr_t)&GetItemName);

            L(retnLabel);
            dq(continueAddr);
        }
    };
    
    // sz = 14
    struct Code : Xbyak::CodeGenerator {
        Code(uint8_t buf[16], uintptr_t branchTarget) : Xbyak::CodeGenerator(16, buf) {
            Xbyak::Label handler;
            jmp(ptr[rip + handler]); // 6 jump to trampoline
            L(handler);
            dq(branchTarget); // 8
        }
    };

    bool InitAddresses() {
        log("Sigscan start");

        RVAUtils::Timer tmr; tmr.start();

        g_sig1.Resolve();
        if (!g_sig1.IsResolved()) return false;

        log("Scan 1 done");

        g_sig2.Resolve();
        if (!g_sig2.IsResolved()) return false;

        log("Scan 2 done");

        log("Sigscan elapsed: %llu ms.", tmr.stop());

        // Globals
        g_entityManager     = reinterpret_cast<void**>               ( GetRelative(g_sig1.GetUIntPtr() + 15, 3, 7) );

        // Functions
        GetLocalPlayer      = reinterpret_cast<_GetLocalPlayer>      ( GetRelative(g_sig1.GetUIntPtr() + 22, 1, 5) );
        HasBeenRead         = reinterpret_cast<_HasBeenRead>         ( GetRelative(g_sig1.GetUIntPtr() + 34, 1, 5) );
        CopyToBSFixedString = reinterpret_cast<_CopyToBSFixedString> ( GetRelative(g_sig2.GetUIntPtr() + 23, 1, 5) );

        // Hook
        Hook_Start = g_sig2.GetUIntPtr() + 12;

        log("Base: %p", (uintptr_t)GetModuleHandle(NULL));
        log("1: %p", g_entityManager);
        log("2: %p", GetLocalPlayer);
        log("3: %p", HasBeenRead);
        log("4: %p", Hook_Start);
        log("5: %p", CopyToBSFixedString);

        return true;
    }

    void InitCodeBufs() {
        // Prepare trampoline
        uint8_t* p = Xbyak::CodeArray::getAlignedAddress(localCodeBuf);
        CodeTrampoline trampoline(p, Hook_Start + 16); // Hook_Start + 16 is return address
        trampoline.protect(p, trampoline.getSize(), true); // flag as executable
        int(*f)() = trampoline.getCode<int(*)()>();

        // Prepare code to write
        Code c(remoteCodeBuf, (uintptr_t)f);
        remoteCodeBufSize = c.getSize();
        assert(remoteCodeBufSize <= sizeof(remoteCodeBuf));
    }

    void Patch() {
        WriteMemory(Hook_Start, remoteCodeBuf, remoteCodeBufSize);
    }

    void Init() {
        // Logging
        char logPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL, NULL, logPath))) {
            strcat_s(logPath, "\\My Games\\Fallout 76\\KnownPlans.log");
            logfile = _fsopen(logPath, "w", _SH_DENYWR);
        }
        log("KnownPlans v1.0");

        // Addresses
        if (!InitAddresses()) {
            MessageBoxA(NULL, "KnownPlans is not compatible with this version of Fallout 76.\nPlease visit the mod page for updates.", "KnownPlans", MB_OK | MB_ICONEXCLAMATION);
            log("FATAL: Incompatible version");
            return;
        }
        log("Addresses set");

        // Code buffers
        InitCodeBufs();
        log("Buffers created");

        // Patch
        Patch();
        log("Done");
    }

    void Deinit() {
        log("Log closed");
        if (logfile) fclose(logfile);
    }

}
