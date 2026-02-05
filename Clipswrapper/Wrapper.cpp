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
void __stdcall ClipsGetStr(void* env, const wchar_t* expression, wchar_t* buffer, int bufferSize) {
    if (env == nullptr || buffer == nullptr || bufferSize <= 0) return;

    CLIPSValue result;
    Eval((Environment*)env, ToAnsi(expression).c_str(), &result);

    std::string finalStr = "";
    unsigned short type = result.header->type;

    // Caso 1: Valores simples (Símbolo, String, Instancia)
    if (type == STRING_TYPE || type == SYMBOL_TYPE || type == INSTANCE_NAME_TYPE) {
        finalStr = result.lexemeValue->contents;
    }
    // Caso 2: Listas (Multifield) - Recorremos todos los elementos
    else if (type == MULTIFIELD_TYPE) {
        for (unsigned long i = 0; i < result.multifieldValue->length; i++) {
            auto element = result.multifieldValue->contents[i];

            if (element.header->type == STRING_TYPE || element.header->type == SYMBOL_TYPE) {
                finalStr += element.lexemeValue->contents;
            }
            else if (element.header->type == INTEGER_TYPE) {
                finalStr += std::to_string(element.integerValue->contents);
            }
            else if (element.header->type == FLOAT_TYPE) {
                finalStr += std::to_string(element.floatValue->contents);
            }

            // Añadimos espacio entre elementos si no es el último
            if (i < result.multifieldValue->length - 1) finalStr += " ";
        }
    }
    // Caso 3: Números
    else if (type == INTEGER_TYPE) {
        finalStr = std::to_string(result.integerValue->contents);
    }
    else if (type == FLOAT_TYPE) {
        finalStr = std::to_string(result.floatValue->contents);
    }
    else if (type == VOID_TYPE) {
        finalStr = "void";
    }
    else {
        finalStr = "ERROR_TIPO_DESCONOCIDO";
    }

    // Copiamos el resultado al buffer de MQL5
    ToUnicode(finalStr.c_str(), buffer, bufferSize);
}
// 5. Liberar memoria
void __stdcall DeinitClips(void* env) {
    if (env) DestroyEnvironment((Environment*)env);
}