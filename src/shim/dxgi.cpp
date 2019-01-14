#include "windows.h"
#include "../KnownPlans.h"

/*
Minimal dxgi.dll shim.
*/

using _CreateDXGIFactory  = HRESULT(*)(REFIID riid, void** ppFactory);
using _CreateDXGIFactory2 = HRESULT(*)(UINT flags, REFIID riid, void** ppFactory);
using _DXGID3D10CreateDevice = HRESULT(*)(HMODULE hModule, void* pFactory, void* pAdapter, UINT Flags, void* pUnknown, void** ppDevice);
using _DXGID3D10CreateLayeredDevice = HRESULT(*)(void* pUnknown1, void* pUnknown2, void* pUnknown3, void* pUnknown4, void* pUnknown5);
using _DXGID3D10GetLayeredDeviceSize = SIZE_T(*)(const void *pLayers, UINT NumLayers);
using _DXGID3D10RegisterLayers = HRESULT(*)(const void *pLayers, UINT NumLayers);
_CreateDXGIFactory  CreateDXGIFactory_Original;
_CreateDXGIFactory  CreateDXGIFactory1_Original;
_CreateDXGIFactory2 CreateDXGIFactory2_Original;
_DXGID3D10CreateDevice DXGID3D10CreateDevice_Original;
_DXGID3D10CreateLayeredDevice DXGID3D10CreateLayeredDevice_Original;
_DXGID3D10GetLayeredDeviceSize DXGID3D10GetLayeredDeviceSize_Original;
_DXGID3D10RegisterLayers DXGID3D10RegisterLayers_Original;

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

    __declspec(dllexport) HRESULT __stdcall DXGID3D10CreateDevice(HMODULE hModule, void* pFactory, void* pAdapter, UINT Flags, void* pUnknown, void** ppDevice) {
        return DXGID3D10CreateDevice_Original(hModule, pFactory, pAdapter, Flags, pUnknown, ppDevice);
    }

    __declspec(dllexport) HRESULT __stdcall DXGID3D10CreateLayeredDevice(void* pUnknown1, void* pUnknown2, void* pUnknown3, void* pUnknown4, void* pUnknown5) {
        return DXGID3D10CreateLayeredDevice_Original(pUnknown1, pUnknown2, pUnknown3, pUnknown4, pUnknown5);
    }

    __declspec(dllexport) SIZE_T __stdcall DXGID3D10GetLayeredDeviceSize(const void *pLayers, UINT NumLayers) {
        return DXGID3D10GetLayeredDeviceSize_Original(pLayers, NumLayers);
    }

    __declspec(dllexport) HRESULT __stdcall DXGID3D10RegisterLayers(const void *pLayers, UINT NumLayers) {
        return DXGID3D10RegisterLayers_Original(pLayers, NumLayers);
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
            DXGID3D10CreateDevice_Original = reinterpret_cast<_DXGID3D10CreateDevice>(GetProcAddress(originalModule, "DXGID3D10CreateDevice"));
            DXGID3D10CreateLayeredDevice_Original = reinterpret_cast<_DXGID3D10CreateLayeredDevice>(GetProcAddress(originalModule, "DXGID3D10CreateLayeredDevice"));
            DXGID3D10GetLayeredDeviceSize_Original = reinterpret_cast<_DXGID3D10GetLayeredDeviceSize>(GetProcAddress(originalModule, "DXGID3D10GetLayeredDeviceSize"));
            DXGID3D10RegisterLayers_Original = reinterpret_cast<_DXGID3D10RegisterLayers>(GetProcAddress(originalModule, "DXGID3D10RegisterLayers"));
            break;

        case DLL_PROCESS_DETACH:
            KnownPlans::Deinit();
            FreeLibrary(originalModule);
            break;
    }
    return TRUE;
}