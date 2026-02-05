# CLIPS Universal Wrapper for MQL5 (x64)

Este proyecto es un **puente de alto rendimiento** que integra el motor de inferencia **CLIPS 6.42** dentro de la terminal **MetaTrader 5**. Permite la creación de sistemas expertos híbridos donde MQL5 actúa como sensor/actuador y CLIPS como el núcleo de razonamiento lógico.

## 1. Arquitectura del Sistema

La comunicación se basa en un flujo de datos asíncrono en tipos, pero síncrono en ejecución:

1. **MQL5 (Capa de Aplicación):** Gestiona strings en formato **Unicode (UTF-16)**.
2. **Wrapper (Capa de Traducción):** Realiza la conversión de codificación y gestiona los punteros de memoria de los entornos CLIPS.
3. **CLIPS (Capa de Inferencia):** Procesa lógica simbólica en **ANSI (C puro)**.

---

## 2. Funcionamiento Esencial de `Wrapper.cpp`

El código fuente de `Wrapper.cpp` resuelve tres problemas críticos de ingeniería:

### A. Gestión de Colisiones (Conflict Resolution)

Dado que CLIPS y la API de Windows comparten nombres de funciones, el Wrapper utiliza un **mapeo de preprocesador**:

* **Problema:** `GetFocus` y `TokenType` existen en ambos mundos.
* **Solución:** Se renombran los símbolos de CLIPS antes de la inclusión (`#define GetFocus CLIPS_GetFocus`) para evitar conflictos de vinculación en el enlazador (Linker).

### B. Traducción de Memoria y Strings

MQL5 usa `wchar_t` (2 bytes por carácter). CLIPS usa `char` (1 byte).

* **`ToAnsi`:** Convierte comandos de MQL5 para que el motor CLIPS pueda interpretarlos.
* **`ToUnicode`:** Reconstruye la respuesta del motor para que sea legible en la terminal de MT5.

### C. Serialización de Datos (El motor de `ClipsGetStr`)

Esta es la función más compleja. Su objetivo es la **introspección del motor**.

1. **Evaluación:** Ejecuta una expresión en CLIPS y captura un objeto `CLIPSValue`.
2. **Identificación de Tipo:** Determina si el resultado es un entero, flotante, símbolo o lista (Multifield).
3. **Aplanamiento (Flattening):** Si el resultado es una lista, la función recorre cada elemento, lo convierte a texto y lo concatena en una sola cadena separada por espacios. Esto permite que MQL5 reciba estructuras complejas en un solo buffer.

---

## 3. Referencia de la API (Exportaciones)

| Función | Parámetros | Descripción |
| --- | --- | --- |
| `InitClips` | Ninguno | Reserva memoria para un nuevo entorno (Environment). Retorna un puntero `long`. |
| `ClipsBuild` | `env`, `construct` | Inyecta reglas (`defrule`) o plantillas (`deftemplate`). |
| `ClipsEval` | `env`, `command` | Ejecuta acciones inmediatas como `(reset)`, `(run)` o `(assert)`. |
| `ClipsGetStr` | `env`, `expr`, `buffer`, `size` | Extrae el valor de variables o resultados de funciones hacia MQL5. |
| `DeinitClips` | `env` | Destruye el entorno y libera la memoria RAM. |

---

## 4. Implementación en MQL5 (Debug rápido)

Para instanciar el motor, se requiere la definición del puntero como tipo `long` para compatibilidad con x64:

```cpp
#import "Clipswrapper.dll"
   long InitClips();
   int  ClipsBuild(long env, string construct);
   void ClipsGetStr(long env, string expression, string &buffer, int bufferSize);
#import

// Ejemplo de uso:
long env = InitClips();
ClipsBuild(env, "(defglobal ?*v* = 100)");
string res; StringInit(res, 10, ' ');
ClipsGetStr(env, "?*v*", res, 10); // Res ahora contiene "100"

```

---

## 5. Notas Técnicas para Compilación

* **Configuración:** Debe compilarse exclusivamente en **x64**.
* **Archivos CLIPS:** Todos los archivos `.c` de la librería original deben tener desactivada la opción de "Encabezados Precompilados" y estar configurados como "Compilar como código C (/TC)".
* **Flags de Preprocesador:** Se requiere `_CRT_SECURE_NO_WARNINGS` para permitir el uso de funciones de manejo de strings estándar de C.
