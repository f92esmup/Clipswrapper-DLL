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

## 6. Formatos de Comunicación (I/O)

Para garantizar una integración estable, el intercambio de datos debe seguir estas reglas de formato:

### A. Entrada (MQL5  Wrapper)

El Wrapper espera cadenas **Unicode (UTF-16)**. Sin embargo, CLIPS internamente trabaja con tipos de datos específicos. Debes formatear tus strings en MQL5 según el tipo de destino:

| Tipo en CLIPS | Formato en MQL5 | Ejemplo de Comando |
| --- | --- | --- |
| **Símbolo** | Texto sin comillas | `"(assert (estado abierto))"` |
| **String** | Texto con comillas escapadas | `"(assert (mensaje \"Operación exitosa\"))"` |
| **Integer** | Número sin decimales | `StringFormat("(assert (id %d))", 123)` |
| **Float** | Número con decimales | `StringFormat("(assert (precio %.5f))", 1.1050)` |
| **Multifield** | Valores separados por espacio | `"(assert (datos 10 20.5 activo))"` |

> **Nota:** Para inyectar variables de MQL5, usa siempre `StringFormat` o la concatenación `+` para construir el comando completo antes de enviarlo a `ClipsEval`.

### B. Salida (Wrapper  MQL5)

La recuperación de datos mediante `ClipsGetStr` es el punto más crítico. El Wrapper **no crea** el string, sino que **llena** uno existente.

1. **Pre-asignación obligatoria:** MQL5 debe reservar memoria para el buffer antes de la llamada. Si el buffer es menor que el resultado de CLIPS, el texto se truncará.
```cpp
string buffer;
StringInit(buffer, 100, ' '); // Reserva 100 caracteres

```


2. **Serialización de Listas:** Si pides un valor que en CLIPS es un `multifield`, el Wrapper lo devolverá como una cadena única donde cada elemento está separado por un **espacio en blanco**.
* *Resultado en CLIPS:* `(10 20.5 "Ok")`
* *Recibido en MQL5:* `"10 20.500000 Ok"`


3. **Tipos Numéricos:** Los números se devuelven siempre convertidos a su representación en string (ej: `10` o `27.500000`).

---

## 7. Ejemplo de Flujo Completo de Datos

```cpp
// 1. Preparar entrada (MQL5)
double rsi = 70.5;
string comando = StringFormat("(assert (indicador (nombre rsi) (valor %f)))", rsi);

// 2. Enviar a la DLL
ClipsEval(env, comando);
ClipsEval(env, "(run)");

// 3. Preparar salida (MQL5)
string respuesta;
StringInit(respuesta, 50, ' '); // Espacio suficiente para la respuesta

// 4. Recuperar resultado
ClipsGetStr(env, "(find-all-facts ((?f señal)) TRUE)", respuesta, 50);

```
