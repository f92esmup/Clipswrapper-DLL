//+------------------------------------------------------------------+
//|                                              AuditoriaClips.mq5 |
//|                                  Copyright 2026, Pedro Escudero  |
//+------------------------------------------------------------------+
#property strict

// Incluimos la definición de la clase
#include <clips.mqh> 

int OnInit() {
    Print("=== INICIANDO AUDITORÍA DE CLIPS DLL (MODO CLASE) ===");
    
    // 1. TEST: Inicialización (Constructor)
    // El constructor invoca internamente a InitClips()
    CClipsEngine engine; 
    
    if(!engine.IsReady()) {
        Print("CRÍTICO: No se pudo obtener el handle del entorno.");
        return INIT_FAILED;
    }
    Print("OK: Motor inicializado correctamente.");

    // 2. TEST: ClipsBuild (Definición de Lógica)
    // Definimos templates y reglas
    bool b1 = engine.Build("(deftemplate account (slot balance) (slot risk))");
    bool b2 = engine.Build("(deftemplate recommendation (slot lots))");
    
    string rule = "(defrule calculate-risk "
                  "  (account (balance ?b&:(> ?b 1000)) (risk high)) "
                  "  => "
                  "  (assert (recommendation (lots 0.5))) "
                  "  (printout t \"Regla disparada: Riesgo Alto detectado.\" crlf))";
    
    bool b3 = engine.Build(rule);

    PrintFormat("OK: Build de Estructuras -> T1:%s T2:%s R1:%s", 
                (string)b1, (string)b2, (string)b3);

    // 3. TEST: ClipsEval & Memoria (Inyección de Hechos)
    engine.Eval("(reset)");
    engine.Eval("(assert (account (balance 5000) (risk high)))");
    
    // Verificamos si CLIPS recuerda el hecho usando GetStr
    string memCheck = engine.GetStr("(find-all-facts ((?f account)) TRUE)");
    Print("OK: Memoria (Facts antes de run): ", memCheck);

    // 4. TEST: Razonamiento (Inferencia)
    // Eval devuelve el número de reglas disparadas
    int fired = engine.Run(); 
    Print("OK: Eval(run) -> Reglas ejecutadas: ", fired);

    // 5. TEST: GetStr (Recuperación de Deducción)
    // Extraemos el resultado de la recomendación calculada
    string finalResult = engine.GetStr("(find-all-facts ((?f recommendation)) TRUE)");
    Print("OK: Razonamiento (Deducción final): ", finalResult);

    // 6. TEST: GetLog (Captura de Router)
    // Recupera lo que el motor imprimió con 'printout'
    string clipsLog = engine.GetLog();
    Print("OK: Logs del motor (Router): ", clipsLog);

    Print("=== AUDITORÍA FINALIZADA CON ÉXITO ===");
    
    // 7. TEST: Liberación (Destructor)
    // No hace falta llamar a DeinitClips; ocurre automáticamente al salir de OnInit.
    return INIT_FAILED; 
}

void OnDeinit(const int reason) {}
void OnTick() {}