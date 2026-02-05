# CLIPS Universal Wrapper for MQL5 (x64)

Este proyecto es un **puente de alto rendimiento** que integra el motor de inferencia **CLIPS 6.42** dentro de la terminal **MetaTrader 5**. Permite la creación de sistemas expertos híbridos donde MQL5 actúa como sensor/actuador (recolector de datos y ejecutor de órdenes) y CLIPS como el núcleo de razonamiento lógico y toma de decisiones.

## 1. Arquitectura del Sistema

La comunicación se basa en un flujo de datos asíncrono en tipos, pero síncrono en ejecución, estructurado en tres niveles:

1.  **MQL5 (Capa de Aplicación):** Gestiona strings en formato **Unicode (UTF-16)** y controla el ciclo de vida del motor.
2.  **Wrapper (Capa de Traducción):** Realiza la conversión de codificación (Unicode <-> ANSI), gestiona punteros de memoria de múltiples entornos y redirige el flujo de salida.
3.  **CLIPS (Capa de Inferencia):** Procesa lógica simbólica basada en el algoritmo Rete en **ANSI (C puro)**.

---

## 2. Funcionamiento Esencial de `Wrapper.cpp`

El código resuelve retos críticos de integración entre C++ moderno y código C legado:

### A. Resolución de Conflictos de Nombres
Se implementa un **mapeo de preprocesador** para evitar colisiones con la API de Windows:
* **Problema:** Símbolos como `GetFocus` y `TokenType` están definidos tanto en `Win32` como en `CLIPS`.
* **Solución:** Uso de `#define` antes de la inclusión de cabeceras para renombrar los símbolos de CLIPS internamente.

### B. Sistema de Router Universal (Logging)
El Wrapper registra un "Router" personalizado en el motor CLIPS que captura **toda** la salida de texto (logs, errores de sintaxis y comandos `printout`):
* **Captura Total:** Redirige los canales `stdout`, `werror` (errores de parsing) y `wtrace` hacia un buffer dinámico (`std::stringstream`).
* **Consulta bajo demanda:** MQL5 puede vaciar este buffer en cualquier momento para mostrar mensajes del motor en la pestaña de "Expertos".

### C. Serialización de Tipos de Datos
La función `ClipsGetStr` realiza introspección en los resultados de evaluación:
* **Aplanamiento de Multifields:** Si el motor devuelve una lista, el Wrapper la descompone y concatena en un único string separado por espacios, facilitando su consumo en MQL5.

---

## 3. Referencia de la API (Exportaciones)

| Función | Parámetros | Descripción |
| :--- | :--- | :--- |
| `InitClips` | Ninguno | Crea un entorno (`Environment`) y registra el Router de logs. Retorna un puntero `long`. |
| `ClipsBuild` | `env`, `construct` | Inyecta estructuras (`defrule`, `deftemplate`). Retorna `1` si es éxito, `0` si hay error. |
| `ClipsEval` | `env`, `command` | Ejecuta comandos operativos (`reset`, `run`, `assert`). |
| `ClipsGetStr` | `env`, `expr`, `buffer`, `size` | Evalúa una expresión y extrae el valor resultante (Símbolo, String, Número o Lista). |
| `ClipsGetOutput` | `buffer`, `size` | Recupera el log de errores y mensajes acumulados, limpiando el buffer interno. |
| `DeinitClips` | `env` | Destruye la instancia y libera la memoria RAM. |

---

## 4. Formatos de Comunicación (I/O)

### A. Entrada (MQL5 -> CLIPS)
| Tipo en CLIPS | Formato en MQL5 | Ejemplo de Comando |
| :--- | :--- | :--- |
| **Símbolo** | Texto sin comillas | `"(assert (tendencia alcista))"` |
| **String** | Comillas escapadas | `"(assert (mensaje \"Señal detectada\"))"` |
| **Float** | Punto decimal | `StringFormat("(assert (precio %.5f))", 1.1050)` |
| **Multifield** | Espacios | `"(assert (datos 10 20.5 OK))"` |

### B. Salida (CLIPS -> MQL5)
* **Pre-asignación:** MQL5 debe inicializar el buffer antes de la llamada: `StringInit(res, size, 0)`.
* **Tratamiento de Listas:** Las listas de CLIPS se reciben como strings planos (Ej: `(1 2 OK)` -> `"1 2 OK"`).

---

## 5. Ejemplo de Implementación con Depuración



```cpp
#import "Clipswrapper.dll"
   long InitClips();
   int  ClipsBuild(long env, string construct);
   void ClipsGetOutput(string &buffer, int bufferSize);
   void DeinitClips(long env);
#import

void OnStart() {
   long env = InitClips();
   
   // Intento de inyectar regla con error de sintaxis
   if(ClipsBuild(env, "(defrule error (item ?x) => )") <= 0) {
      string log; StringInit(log, 1024, 0);
      ClipsGetOutput(log, 1024);
      Print("❌ CLIPS Error: ", log);
   }

   DeinitClips(env);
}
```

6. Notas Técnicas para Compilación
Plataforma: Windows x64.
Configuración VC++: * Desactivar Encabezados Precompilados en archivos .c de CLIPS.
Configurar Compilar como código C (/TC) para el núcleo del motor.
Definir _CRT_SECURE_NO_WARNINGS en el preprocesador.
Desactivar Comprobaciones SDL (/sdl-) para compatibilidad con código C legado.
Desarrollado por: Pedro Escudero Murcia (2026)
