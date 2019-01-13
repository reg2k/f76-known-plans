#include "windows.h"
#include "../KnownPlans.h"

/*
Minimal dxgi.dll shim.
*/

using _CreateDXGIFactory  = HRESULT(*)(REFIID riid, void** ppFactory);
using _CreateDXGIFactory2 = HRESULT(*)(UINT flags, REFIID riid, void** ppFactory);
_CreateDXGIFactory  CreateDXGIFactory_Original;
_CreateDXGIFactory  CreateDXGIFactory1_Original;
_CreateDXGIFactory2 CreateDXGIFactory2_Original;

HMODULE originalModule = NULL;

extern "C" {
    __declspec(dllexport) HRESULT __stdcall CreateDXGIFactory(REFIID riid, void** ppFactory) {
        static bool initDone = false;
        if (!initDone) {
            initDone = true;
            KnownPlans::Init();
        }
        return CreateDXGIFactory_Original(riid, ppFactory);
    }

    __declspec(dllexport) HRESULT __stdcall CreateDXGIFactory1(REFIID riid, void** ppFactory) {
        return CreateDXGIFactory1_Original(riid, ppFactory);
    }

    __declspec(dllexport) HRESULT __stdcall CreateDXGIFactory2(UINT flags, REFIID riid, void** ppFactory) {
        return CreateDXGIFactory2_Original(flags, riid, ppFactory);
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            char path[MAX_PATH];
            GetSystemDirectoryA(path, MAX_PATH);
            strcat_s(path, "\\dxgi.dll");

            originalModule = LoadLibraryA(path);
            CreateDXGIFactory_Original  = reinterpret_cast<_CreateDXGIFactory> (GetProcAddress(originalModule, "CreateDXGIFactory"));
            CreateDXGIFactory1_Original = reinterpret_cast<_CreateDXGIFactory> (GetProcAddress(originalModule, "CreateDXGIFactory1"));
            CreateDXGIFactory2_Original = reinterpret_cast<_CreateDXGIFactory2>(GetProcAddress(originalModule, "CreateDXGIFactory2"));
            break;

        case DLL_PROCESS_DETACH:
            KnownPlans::Deinit();
            FreeLibrary(originalModule);
            break;
    }
    return TRUE;
}