#include "pch.h" // Requerido por VS para DLLs
#include "Wrapper.h"

// Función de prueba para verificar que la DLL carga bien
int __stdcall GetClipsVersion() {
    return 641; // Representa la versión 6.4.1
}

void* __stdcall InitClips() {
    // Aquí más adelante inicializarás el entorno de CLIPS
    return nullptr;
}