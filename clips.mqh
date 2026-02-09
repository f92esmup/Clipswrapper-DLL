//+------------------------------------------------------------------+
//|                                                        clips.mqh |
//|                                  Copyright 2026, Pedro Escudero. |
//+------------------------------------------------------------------+
#property strict

// Importación de funciones desde la DLL
#import "Clipswrapper.dll"
   long  InitClips();                                     
   int   ClipsBuild(long handle, const string construct); 
   int   ClipsEval(long handle, const string command);    
   void  ClipsGetStr(long handle, const string expression, string &buffer, int bufferSize);
   void  ClipsGetOutput(long handle, string &buffer, int bufferSize);
   void  DeinitClips(long handle);                        
#import

class CClipsEngine {
private:
    long     m_handle;       // Puntero a la instancia ClipsInstance en C++
    int      m_buffer_size;
    string   m_last_log;

public:
    // Inicializa el entorno y el router contextual
    CClipsEngine(int buffer_size = 4096) : m_buffer_size(buffer_size) {
        m_handle = InitClips();
    }

    // Libera la memoria de la instancia al destruir el objeto
    ~CClipsEngine() {
        if(m_handle != 0) {
            DeinitClips(m_handle);
        }
    }

    // Define constructos: defrule, deftemplate, deffunction
    bool Build(const string construct) {
        if(m_handle == 0) return false;
        if(ClipsBuild(m_handle, construct) <= 0) {
            string err = GetLog();
            if(StringLen(err) > 0) Print("CLIPS Syntax Error: ", err);
            return false;
        }
        return true;
    }

    // Ejecuta comandos: (run), (reset), (assert ...), (clear)
    int Eval(const string command) {
        if(m_handle == 0) return -1;
        return ClipsEval(m_handle, command);
    }

    // Extrae el valor de una expresión (Símbolos, Multifields, Números)
    string GetStr(const string expression) {
        if(m_handle == 0) return "";
        string res;
        StringInit(res, m_buffer_size);
        ClipsGetStr(m_handle, expression, res, m_buffer_size);
        return res;
    }

    // Recupera y vacía el buffer de salida del router
    string GetLog() {
        if(m_handle == 0) return "Handle inválido";
        string buffer;
        StringInit(buffer, m_buffer_size);
        ClipsGetOutput(m_handle, buffer, m_buffer_size);
        m_last_log = buffer;
        return m_last_log;
    }

    bool IsReady() { return m_handle != 0; }
};