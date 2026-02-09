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

// --- ESTRUCTURA DE CONTEXTO ---
// Encapsula el entorno y su buffer de salida para aislamiento total
struct ClipsInstance {
    Environment* env = nullptr;
    std::stringstream output_stream;
};

// --- FUNCIONES DEL ROUTER (Contextualizadas) ---
extern "C" {
    bool QueryRouter(Environment* env, const char* logicalName, void* context) {
        return true;
    }

    void WriteRouter(Environment* env, const char* logicalName, const char* str, void* context) {
        if (context) {
            // Recuperamos la instancia específica a través del puntero de contexto
            ClipsInstance* instance = static_cast<ClipsInstance*>(context);
            instance->output_stream << str;
        }
    }
}

// --- AUXILIARES DE CONVERSIÓN ---

std::string ToAnsi(const wchar_t* wstr) {
    if (!wstr || wstr[0] == L'\0') return "";
    int size = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
    if (size <= 1) return "";

    std::string str(size - 1, 0);
    WideCharToMultiByte(CP_ACP, 0, wstr, -1, &str[0], size, NULL, NULL);
    return str;
}

void ToUnicode(const char* src, wchar_t* dest, int maxLen) {
    if (!src || !dest || maxLen <= 0) return;
    MultiByteToWideChar(CP_ACP, 0, src, -1, dest, maxLen);
}

// --- API EXPORTADA ---

void* __stdcall InitClips() {
    // 1. Reservar memoria para nuestra estructura de control
    ClipsInstance* instance = new ClipsInstance();

    // 2. Crear el entorno de CLIPS
    instance->env = CreateEnvironment();

    // Si CreateEnvironment fallara, al menos env no tendría basura de memoria
    if (instance->env == nullptr) {
        delete instance;
        return nullptr;
    }
    // 3. Registrar el Router pasando la dirección de 'instance' como contexto (6º parámetro)
    AddRouter(instance->env, "MQL5Router", 10, QueryRouter, WriteRouter, NULL, NULL, NULL, instance);

    return instance; // Retornamos el puntero a nuestra estructura (el "handle")
}

int __stdcall ClipsBuild(void* handle, const wchar_t* construct) {
    if (!handle) return -1;
    ClipsInstance* instance = static_cast<ClipsInstance*>(handle);
    return Build(instance->env, ToAnsi(construct).c_str());
}

int __stdcall ClipsEval(void* handle, const wchar_t* command) {
    if (!handle) return -1;
    ClipsInstance* instance = static_cast<ClipsInstance*>(handle);
    return (int)Eval(instance->env, ToAnsi(command).c_str(), NULL);
}

void __stdcall ClipsGetStr(void* handle, const wchar_t* expression, wchar_t* buffer, int bufferSize) {
    if (!handle || !buffer || bufferSize <= 0) return;

    ClipsInstance* instance = static_cast<ClipsInstance*>(handle);
    CLIPSValue result;

    int error = (int)Eval(instance->env, ToAnsi(expression).c_str(), &result);

    std::string finalStr = "";
    if (error != 0) {
        finalStr = "EVAL_ERROR";
    }
    else {
        unsigned short type = result.header->type;

        if (type == STRING_TYPE || type == SYMBOL_TYPE || type == INSTANCE_NAME_TYPE)
            finalStr = result.lexemeValue->contents;
        else if (type == INTEGER_TYPE)
            finalStr = std::to_string(result.integerValue->contents);
        else if (type == FLOAT_TYPE)
            finalStr = std::to_string(result.floatValue->contents);
        else if (type == FACT_ADDRESS_TYPE)
            finalStr = "<Fact-" + std::to_string(result.factValue->factIndex) + ">";
        else if (type == INSTANCE_ADDRESS_TYPE)
            finalStr = "<Instance-" + std::string(result.instanceValue->name->contents) + ">";
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

                if (i < result.multifieldValue->length - 1) finalStr += " ";
            }
        }
        else {
            finalStr = (type == VOID_TYPE) ? "void" : "N/A";
        }
    }

    ToUnicode(finalStr.c_str(), buffer, bufferSize);
}

void __stdcall ClipsGetOutput(void* handle, wchar_t* buffer, int bufferSize) {
    if (!handle || !buffer) return;

    ClipsInstance* instance = static_cast<ClipsInstance*>(handle);
    std::string out = instance->output_stream.str();

    ToUnicode(out.c_str(), buffer, bufferSize);

    // Limpiar el buffer específico de esta instancia
    instance->output_stream.str("");
    instance->output_stream.clear();
}

void __stdcall DeinitClips(void* handle) {
    if (handle) {
        ClipsInstance* instance = static_cast<ClipsInstance*>(handle);
        // 1. Destruir entorno CLIPS
        DestroyEnvironment(instance->env);
        // 2. Liberar la estructura contenedora
        delete instance;
    }
}