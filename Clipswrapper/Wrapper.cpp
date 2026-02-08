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
    bool QueryRouter(Environment* env, const char* logicalName, void* context) {
        return true;
    }

    void WriteRouter(Environment* env, const char* logicalName, const char* str, void* context) {
        clips_output_stream << str;
    }
}

// --- AUXILIARES DE CONVERSIÓN ---

// Corregido: Ahora excluye el terminador nulo del tamaño del std::string
std::string ToAnsi(const wchar_t* wstr) {
    if (!wstr || wstr[0] == L'\0') return "";
    int size = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
    if (size <= 1) return "";

    // Restamos 1 para que el objeto string de C++ tenga el tamaño real
    std::string str(size - 1, 0);
    WideCharToMultiByte(CP_ACP, 0, wstr, -1, &str[0], size, NULL, NULL);
    return str;
}

void ToUnicode(const char* src, wchar_t* dest, int maxLen) {
    if (!src || !dest) return;
    MultiByteToWideChar(CP_ACP, 0, src, -1, dest, maxLen);
}

// --- API EXPORTADA ---

void* __stdcall InitClips() {
    Environment* env = CreateEnvironment();
    // Registro del Router para capturar errores de sintaxis y mensajes de printout
    AddRouter(env, "MQL5Router", 10, QueryRouter, WriteRouter, NULL, NULL, NULL, NULL);
    return env;
}

int __stdcall ClipsBuild(void* env, const wchar_t* construct) {
    if (env == nullptr) return -1;
    // Build devuelve 1 en éxito, 0 en error
    return Build((Environment*)env, ToAnsi(construct).c_str());
}

int __stdcall ClipsEval(void* env, const wchar_t* command) {
    if (env == nullptr) return -1;
    // Eval devuelve 0 (EE_NO_ERROR) si el comando es válido
    return (int)Eval((Environment*)env, ToAnsi(command).c_str(), NULL);
}

void __stdcall ClipsGetStr(void* env, const wchar_t* expression, wchar_t* buffer, int bufferSize) {
    if (env == nullptr || buffer == nullptr || bufferSize <= 0) return;

    Environment* clipsEnv = (Environment*)env;
    CLIPSValue result;

    // Eval en CLIPS 6.42 devuelve un código de error (0 = EE_NO_ERROR)
    int error = (int)Eval(clipsEnv, ToAnsi(expression).c_str(), &result);

    std::string finalStr = "";

    if (error != 0) {
        finalStr = "EVAL_ERROR";
    }
    else {
        unsigned short type = result.header->type;

        // --- MANEJO DE TIPOS SIMPLES ---
        if (type == STRING_TYPE || type == SYMBOL_TYPE || type == INSTANCE_NAME_TYPE) {
            finalStr = result.lexemeValue->contents;
        }
        else if (type == INTEGER_TYPE) {
            finalStr = std::to_string(result.integerValue->contents);
        }
        else if (type == FLOAT_TYPE) {
            finalStr = std::to_string(result.floatValue->contents);
        }
        else if (type == FACT_ADDRESS_TYPE) {
            // Crucial para find-all-facts: devuelve el índice del hecho <Fact-X>
            finalStr = "<Fact-" + std::to_string(result.factValue->factIndex) + ">";
        }
        else if (type == INSTANCE_ADDRESS_TYPE) {
            finalStr = "<Instance-" + std::string(result.instanceValue->name->contents) + ">";
        }

        // --- MANEJO DE MULTIFIELDS (LISTAS) ---
        else if (type == MULTIFIELD_TYPE) {
            for (unsigned long i = 0; i < result.multifieldValue->length; i++) {
                auto element = result.multifieldValue->contents[i];
                unsigned short eType = element.header->type;

                if (eType == STRING_TYPE || eType == SYMBOL_TYPE || eType == INSTANCE_NAME_TYPE)
                    finalStr += element.lexemeValue->contents;
                else if (eType == INTEGER_TYPE)
                    finalStr += std::to_string(element.integerValue->contents);
                else if (eType == FLOAT_TYPE)
                    finalStr += std::to_string(element.floatValue->contents);
                else if (eType == FACT_ADDRESS_TYPE)
                    finalStr += "<Fact-" + std::to_string(element.factValue->factIndex) + ">";
                else if (eType == INSTANCE_ADDRESS_TYPE)
                    finalStr += "<Instance-" + std::string(element.instanceValue->name->contents) + ">";
                else
                    finalStr += "UNKNOWN";

                // Añadir espacio entre elementos de la lista
                if (i < result.multifieldValue->length - 1) finalStr += " ";
            }
        }
        else if (type == VOID_TYPE) {
            finalStr = "void";
        }
        else {
            finalStr = "N/A";
        }
    }

    // Convertir la cadena final de ANSI a Unicode para MQL5
    ToUnicode(finalStr.c_str(), buffer, bufferSize);
}

void __stdcall ClipsGetOutput(wchar_t* buffer, int bufferSize) {
    std::string out = clips_output_stream.str();
    ToUnicode(out.c_str(), buffer, bufferSize);
    clips_output_stream.str("");
    clips_output_stream.clear();
}

void __stdcall DeinitClips(void* env) {
    if (env) DestroyEnvironment((Environment*)env);
}