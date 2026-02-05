#include "pch.h"
#include <windows.h> // Aseguramos que Windows esté cargado
#include "Wrapper.h"
#include <string>

// --- TRUCO TÉCNICO ---
// Eliminamos la definición de Windows para GetFocus 
// antes de que CLIPS intente definir la suya.
#undef GetFocus 

extern "C" {
#include "clips.h"
}

// Auxiliar: MQL5 (UTF-16) -> CLIPS (ANSI)
std::string ToAnsi(const wchar_t* wstr) {
    if (!wstr) return "";
    int size = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
    std::string str(size, 0);
    WideCharToMultiByte(CP_ACP, 0, wstr, -1, &str[0], size, NULL, NULL);
    return str;
}

// Auxiliar: CLIPS (ANSI) -> MQL5 (UTF-16)
void ToUnicode(const char* src, wchar_t* dest, int maxLen) {
    MultiByteToWideChar(CP_ACP, 0, src, -1, dest, maxLen);
}

// 1. Inicializar el motor
void* __stdcall InitClips() {
    return CreateEnvironment();
}

// 2. Definir estructuras (defrule, deffacts, deftemplate)
int __stdcall ClipsBuild(void* env, const wchar_t* construct) {
    if (env == nullptr) return -1;
    return Build((Environment*)env, ToAnsi(construct).c_str());
}

// 3. Ejecutar comandos (reset, run, assert)
int __stdcall ClipsEval(void* env, const wchar_t* command) {
    if (env == nullptr) return -1;
    return Eval((Environment*)env, ToAnsi(command).c_str(), NULL);
}

// 4. Obtener un valor de CLIPS como String
void __stdcall ClipsGetStr(void* env, const wchar_t* expression, wchar_t* buffer, int bufferSize) {
    if (env == nullptr) return;

    CLIPSValue result;
    Eval((Environment*)env, ToAnsi(expression).c_str(), &result);

    // En CLIPS 6.42 el miembro se llama 'lexemeValue'
    if (result.header->type == SYMBOL_TYPE || result.header->type == STRING_TYPE) {
        ToUnicode(result.lexemeValue->contents, buffer, bufferSize);
    }
    else {
        ToUnicode("N/A", buffer, bufferSize);
    }
}

// 5. Liberar memoria
void __stdcall DeinitClips(void* env) {
    if (env) DestroyEnvironment((Environment*)env);
}