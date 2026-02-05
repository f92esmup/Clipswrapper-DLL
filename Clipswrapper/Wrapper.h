#pragma once

// Definimos la macro para exportar funciones con la convención de llamada de MT5
#define MT5_EXPORT __declspec(dllexport)

extern "C" {
    // Usamos __stdcall para que MT5 entienda la comunicación
    MT5_EXPORT int __stdcall GetClipsVersion();
    MT5_EXPORT void* __stdcall InitClips();
}