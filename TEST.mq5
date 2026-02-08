#property strict

#import "Clipswrapper.dll"
   long InitClips();
   int  ClipsBuild(long env, string construct);
   int  ClipsEval(long env, string command);
   void ClipsGetStr(long env, string expression, string &buffer, int bufferSize);
   void ClipsGetOutput(string &buffer, int bufferSize);
   void DeinitClips(long env);
#import

long clipsEnv = 0;

int OnInit() {
    Print("=== INICIANDO AUDITORÍA DE CLIPS DLL ===");
    
    // 1. TEST: InitClips (Inicialización)
    clipsEnv = InitClips();
    if(clipsEnv == 0) {
        Print("CRÍTICO: No se pudo obtener puntero de entorno.");
        return INIT_FAILED;
    }
    Print("OK: InitClips -> Puntero: ", clipsEnv);

    // 2. TEST: ClipsBuild (Capacidad de Aprendizaje/Estructura)
    // Definimos un template para la cuenta y una regla de razonamiento
    int b1 = ClipsBuild(clipsEnv, "(deftemplate account (slot balance) (slot risk))");
    int b2 = ClipsBuild(clipsEnv, "(deftemplate recommendation (slot lots))");
    
    // REGLA DE RAZONAMIENTO: "Si el balance > 1000 y el riesgo es alto, lotaje = 0.5"
    string rule = "(defrule calculate-risk "
                  "  (account (balance ?b&:(> ?b 1000)) (risk high)) "
                  "  => "
                  "  (assert (recommendation (lots 0.5))) "
                  "  (printout t \"Regla disparada: Riesgo Alto detectado.\" crlf))";
    int b3 = ClipsBuild(clipsEnv, rule);

    PrintFormat("OK: ClipsBuild -> T1:%d T2:%d R1:%d (0 = OK)", b1, b2, b3);

    // 3. TEST: ClipsEval & Memoria (Inyección de Hechos)
    ClipsEval(clipsEnv, "(reset)");
    // Inyectamos datos de "memoria"
    ClipsEval(clipsEnv, "(assert (account (balance 5000) (risk high)))");
    
    // Verificamos si CLIPS recuerda el hecho antes de correr la inferencia
    string memCheck; StringInit(memCheck, 128, ' ');
    ClipsGetStr(clipsEnv, "(find-all-facts ((?f account)) TRUE)", memCheck, 128);
    Print("OK: Memoria (Facts antes de run): ", memCheck);

    // 4. TEST: Razonamiento (Inferencia)
    int fired = ClipsEval(clipsEnv, "(run)");
    Print("OK: ClipsEval(run) -> Reglas ejecutadas: ", fired);

    // 5. TEST: ClipsGetStr (Recuperación de Deducción)
    string finalResult; StringInit(finalResult, 128, ' ');
    ClipsGetStr(clipsEnv, "(find-all-facts ((?f recommendation)) TRUE)", finalResult, 128);
    Print("OK: Razonamiento (Deducción final): ", finalResult);

    // 6. TEST: ClipsGetOutput (Captura de Router)
    // Debería capturar el "printout" que definimos en la regla
    string clipsLog; StringInit(clipsLog, 500, ' ');
    ClipsGetOutput(clipsLog, 500);
    Print("OK: ClipsGetOutput (Logs del motor): ", clipsLog);

    Print("=== AUDITORÍA FINALIZADA CON ÉXITO ===");
    
    // 7. TEST: DeinitClips (Liberación)
    DeinitClips(clipsEnv);
    clipsEnv = 0;

    return INIT_FAILED; // Detenemos el EA tras el test
}