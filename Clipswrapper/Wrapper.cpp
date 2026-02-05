#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// --- TRUCO DE RENOMBRADO PARA EVITAR COLISIONES ---
// Renombramos los símbolos de CLIPS antes de incluirlos
#define GetFocus CLIPS_GetFocus
#define TokenType CLIPS_TokenType

#include "Wrapper.h"
#include <string>

extern "C" {
#include "clips.h"
}

// Limpiamos los nombres para que no afecten al resto del proyecto
#undef GetFocus
#undef TokenType

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
// 4. Obtener un valor de CLIPS (Versión robusta para 6.42)
void __stdcall ClipsGetStr(void* env, const wchar_t* expression, wchar_t* buffer, int bufferSize) {
    if (env == nullptr) return;

    CLIPSValue result;
    // Evaluamos la expresión
    Eval((Environment*)env, ToAnsi(expression).c_str(), &result);

    // Intentamos obtener el valor como texto independientemente del tipo
    // Usamos la función interna de CLIPS para convertir el valor a string
    if (result.header->type == SYMBOL_TYPE || result.header->type == STRING_TYPE || result.header->type == INSTANCE_NAME_TYPE) {
        ToUnicode(result.lexemeValue->contents, buffer, bufferSize);
    }
    else if (result.header->type == FLOAT_TYPE) {
        std::string val = std::to_string(result.floatValue->contents);
        ToUnicode(val.c_str(), buffer, bufferSize);
    }
    else if (result.header->type == INTEGER_TYPE) {
        std::string val = std::to_string(result.integerValue->contents);
        ToUnicode(val.c_str(), buffer, bufferSize);
    }
    else {
        // Si es un multifield (lista) u otro, devolvemos un aviso descriptivo
        ToUnicode("TIPO_COMPLEJO", buffer, bufferSize);
    }
}

// 5. Liberar memoria
void __stdcall DeinitClips(void* env) {
    if (env) DestroyEnvironment((Environment*)env);
}