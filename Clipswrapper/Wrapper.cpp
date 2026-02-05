#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <sstream>

// --- TRUCO DE RENOMBRADO PARA EVITAR COLISIONES ---
#define GetFocus CLIPS_GetFocus
#define TokenType CLIPS_TokenType

#include "Wrapper.h"

extern "C" {
#include "clips.h"
}

#undef GetFocus
#undef TokenType

// Buffer global para capturar la salida de CLIPS
std::stringstream clips_output_stream;

// --- FUNCIONES DEL ROUTER (Captura de logs) ---
extern "C" {
    // 1. QueryRouter ahora devuelve bool y acepta un parámetro de contexto
    bool QueryRouter(Environment* env, const char* logicalName, void* context) {
        if (strcmp(logicalName, "stdout") == 0 ||
            strcmp(logicalName, "wtrace") == 0 ||
            strcmp(logicalName, "werror") == 0) return true;
        return false;
    }
    // 2. WriteRouter ahora acepta un parámetro de contexto
    void WriteRouter(Environment* env, const char* logicalName, const char* str, void* context) {
        clips_output_stream << str;
    }
}

// --- AUXILIARES DE CONVERSIÓN ---
std::string ToAnsi(const wchar_t* wstr) {
    if (!wstr) return "";
    int size = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
    std::string str(size, 0);
    WideCharToMultiByte(CP_ACP, 0, wstr, -1, &str[0], size, NULL, NULL);
    return str;
}

void ToUnicode(const char* src, wchar_t* dest, int maxLen) {
    MultiByteToWideChar(CP_ACP, 0, src, -1, dest, maxLen);
}

// --- API EXPORTADA ---

// 1. Inicializar el motor y registrar el Router
void* __stdcall InitClips() {
    Environment* env = CreateEnvironment();
    // AddRouter en 6.42 requiere 9 argumentos (el último es el contexto NULL)
    AddRouter(env, "MQL5Router", 10, QueryRouter, WriteRouter, NULL, NULL, NULL, NULL);
    return env;
}

// 2. Definir estructuras
int __stdcall ClipsBuild(void* env, const wchar_t* construct) {
    if (env == nullptr) return -1;
    return Build((Environment*)env, ToAnsi(construct).c_str());
}

// 3. Ejecutar comandos
int __stdcall ClipsEval(void* env, const wchar_t* command) {
    if (env == nullptr) return -1;
    return Eval((Environment*)env, ToAnsi(command).c_str(), NULL);
}

// 4. Obtener valor (Soporta Multifields, Strings, Números)
void __stdcall ClipsGetStr(void* env, const wchar_t* expression, wchar_t* buffer, int bufferSize) {
    if (env == nullptr || buffer == nullptr || bufferSize <= 0) return;

    CLIPSValue result;
    Eval((Environment*)env, ToAnsi(expression).c_str(), &result);

    std::string finalStr = "";
    unsigned short type = result.header->type;

    if (type == STRING_TYPE || type == SYMBOL_TYPE || type == INSTANCE_NAME_TYPE) {
        finalStr = result.lexemeValue->contents;
    }
    else if (type == MULTIFIELD_TYPE) {
        for (unsigned long i = 0; i < result.multifieldValue->length; i++) {
            auto element = result.multifieldValue->contents[i];
            if (element.header->type == STRING_TYPE || element.header->type == SYMBOL_TYPE)
                finalStr += element.lexemeValue->contents;
            else if (element.header->type == INTEGER_TYPE)
                finalStr += std::to_string(element.integerValue->contents);
            else if (element.header->type == FLOAT_TYPE)
                finalStr += std::to_string(element.floatValue->contents);

            if (i < result.multifieldValue->length - 1) finalStr += " ";
        }
    }
    else if (type == INTEGER_TYPE) finalStr = std::to_string(result.integerValue->contents);
    else if (type == FLOAT_TYPE) finalStr = std::to_string(result.floatValue->contents);
    else if (type == VOID_TYPE) finalStr = "void";
    else finalStr = "N/A";

    ToUnicode(finalStr.c_str(), buffer, bufferSize);
}

// 5. Recuperar y limpiar el log de salida (Router)
void __stdcall ClipsGetOutput(wchar_t* buffer, int bufferSize) {
    std::string out = clips_output_stream.str();
    ToUnicode(out.c_str(), buffer, bufferSize);
    clips_output_stream.str(""); // Limpiar
    clips_output_stream.clear();
}

// 6. Liberar memoria
void __stdcall DeinitClips(void* env) {
    if (env) DestroyEnvironment((Environment*)env);
}