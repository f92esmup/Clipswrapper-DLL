//+------------------------------------------------------------------+
//|                                        TestClipsUniversal.mq5    |
//|                                  Copyright 2026, Pedro Escudero  |
//|                                             https://mql5.com     |
//+------------------------------------------------------------------+
#property copyright "Pedro Escudero"
#property link      "https://mql5.com"
#property version   "1.00"
#property strict

// --- IMPORTACIÓN DE LA DLL ---
#import "Clipswrapper.dll"
   long InitClips();
   int  ClipsBuild(long env, string construct);
   int  ClipsEval(long env, string command);
   void ClipsGetStr(long env, string expression, string &buffer, int bufferSize);
   void ClipsGetOutput(string &buffer, int bufferSize); // <--- Nueva función de Log
   void DeinitClips(long env);
#import

//+------------------------------------------------------------------+
//| Script program start function                                    |
//+------------------------------------------------------------------+
void OnStart()
{
   Print("=== INICIANDO AUDITORÍA DEL MOTOR CLIPS ===");

   // 1. Inicialización de instancia
   long env = InitClips();
   if(env == 0)
   {
      Print("ERROR CRÍTICO:: No se pudo inicializar la DLL.");
      return;
   }

   // --- TEST 1: CAPTURA DE ERRORES (EL "LOG" DEL MOTOR) ---
   Print("\n--- TEST 1: Depuración de errores de sintaxis ---");
   // Provocamos error: Falta cerrar el paréntesis de la regla
   int buildRes = ClipsBuild(env, "(defrule Regla_Con_Error (dato ?x) => (printout t \"Hola\") ");
   
   if(buildRes <= 0) 
   {
      Print("ADVERTENCIA:: El motor rechazó la construcción. Consultando causa...");
      ShowClipsLog(); // <--- Aquí es donde capturamos el mensaje de error
   }

   // --- TEST 2: AISLAMIENTO DE INSTANCIAS ---
   Print("\n--- TEST 2: Verificando independencia de instancias ---");
   long env2 = InitClips();
   
   ClipsBuild(env,  "(defglobal ?*instancia* = \"Soy el Cerebro A\")");
   ClipsBuild(env2, "(defglobal ?*instancia* = \"Soy el Cerebro B\")");
   
   string resA = "                    ";
   string resB = "                    ";
   
   ClipsGetStr(env,  "?*instancia*", resA, 20);
   ClipsGetStr(env2, "?*instancia*", resB, 20);
   
   Print("Instancia 1 dice: ", resA);
   Print("Instancia 2 dice: ", resB);
   
   DeinitClips(env2); // Cerramos la segunda instancia

   // --- TEST 3: MANEJO DE LISTAS (MULTIFIELD) ---
   Print("\n--- TEST 3: Recuperación de listas complejas ---");
   ClipsEval(env, "(clear)"); // Limpiamos env
   ClipsBuild(env, "(deftemplate sensor (multislot valores))");
   ClipsEval(env, "(assert (sensor (valores 1.0950 1.0965 1.0942 SELL \"Strong\")))");
   
   string multifieldRes;
   StringInit(multifieldRes, 100, 0);
   string query = "(fact-slot-value (nth$ 1 (find-all-facts ((?f sensor)) TRUE)) valores)";
   
   ClipsGetStr(env, query, multifieldRes, 100);
   Print("Datos recuperados de la lista: ", multifieldRes);

   // --- TEST 4: COMANDO PRINTOUT INTERNO ---
   Print("\n--- TEST 4: Verificación de salida stdout ---");
   ClipsEval(env, "(printout stdout \"[LOG INTERNO]: Razonamiento completado con éxito.\" crlf)");
   ShowClipsLog();

   // --- LIMPIEZA FINAL ---
   DeinitClips(env);
   Print("\n=== AUDITORÍA FINALIZADA CON ÉXITO ===");
}

//+------------------------------------------------------------------+
//| Función Auxiliar: Lee y limpia el buffer de salida de la DLL     |
//+------------------------------------------------------------------+
void ShowClipsLog()
{
   string logger;
   StringInit(logger, 1024, 0); // Reservamos 1KB para logs
   
   ClipsGetOutput(logger, 1024);
   
   // Limpiamos espacios innecesarios
   StringTrimLeft(logger);
   StringTrimRight(logger);
   
   if(StringLen(logger) > 0)
   {
      Print("MENSAJE DEL MOTOR:\n", logger);
   }
}
