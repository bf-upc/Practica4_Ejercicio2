# Práctica 4 – Ejercicio 2: Sincronización de tareas con semáforo binario

## Descripción

Este proyecto forma parte de la **Práctica 4 de Sistemas Operativos en Tiempo Real**. El objetivo es comprender la **sincronización entre tareas** usando semáforos de FreeRTOS en un ESP32.

Se implementan **dos tareas** que controlan el LED integrado de forma sincronizada:

- **`TaskLedOn`** — espera su turno, enciende el LED y cede el control a `TaskLedOff`.
- **`TaskLedOff`** — espera su turno, apaga el LED y cede el control a `TaskLedOn`.

El semáforo binario garantiza una alternancia estricta: **ON → OFF → ON → OFF → ...**

---

## Hardware necesario

- Placa **ESP32** (cualquier variante compatible)
- Cable USB para programación y monitor serie

> El programa usa `LED_BUILTIN`, que corresponde al LED integrado de la placa. No se necesita hardware adicional.

---

## Software y dependencias

- [PlatformIO](https://platformio.org/) (IDE o extensión para VS Code)
- Framework: **Arduino** para ESP32
- FreeRTOS (incluido en el framework de Arduino para ESP32)

---

## Estructura del proyecto

```
Practica4_Ejercicio2/
├── src/
│   └── main.cpp        # Código fuente principal
├── include/
├── lib/
├── test/
└── platformio.ini
```

---

## Código

```cpp
#include <Arduino.h>

#define DELAY_MS   500       // Tiempo encendido/apagado en milisegundos

SemaphoreHandle_t semLedOn;   // Semáforo que autoriza encender el LED
SemaphoreHandle_t semLedOff;  // Semáforo que autoriza apagar el LED

void TaskLedOn (void *parameter);
void TaskLedOff(void *parameter);

void setup()
{
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);

    semLedOn  = xSemaphoreCreateBinary();
    semLedOff = xSemaphoreCreateBinary();

    if (semLedOn == NULL || semLedOff == NULL) {
        Serial.println("ERROR: no se pudieron crear los semaforos");
        while (true);
    }

    // Dar el primer turno a TaskLedOn
    xSemaphoreGive(semLedOn);

    xTaskCreate(TaskLedOn,  "LED ON",  2048, NULL, 1, NULL);
    xTaskCreate(TaskLedOff, "LED OFF", 2048, NULL, 1, NULL);

    Serial.println("Sistema iniciado: LED parpadeando con semaforo binario");
}

void loop()
{
    vTaskDelay(portMAX_DELAY); // No se usa: la lógica está en las tareas
}

void TaskLedOn(void *parameter)
{
    for (;;)
    {
        xSemaphoreTake(semLedOn, portMAX_DELAY);  // Esperar turno
        digitalWrite(LED_BUILTIN, HIGH);
        Serial.println("LED ON");
        vTaskDelay(DELAY_MS / portTICK_PERIOD_MS);
        xSemaphoreGive(semLedOff);                // Ceder turno a TaskLedOff
    }
}

void TaskLedOff(void *parameter)
{
    for (;;)
    {
        xSemaphoreTake(semLedOff, portMAX_DELAY); // Esperar turno
        digitalWrite(LED_BUILTIN, LOW);
        Serial.println("LED OFF");
        vTaskDelay(DELAY_MS / portTICK_PERIOD_MS);
        xSemaphoreGive(semLedOn);                 // Ceder turno a TaskLedOn
    }
}
```

---

## Explicación del funcionamiento

### ¿Por qué hace falta un semáforo?

Sin sincronización, las dos tareas competirían por el LED de forma impredecible: el planificador podría ejecutar `TaskLedOn` dos veces seguidas antes de darle paso a `TaskLedOff`, rompiendo la alternancia. El **semáforo binario** actúa como un testigo de carrera que solo puede tener una tarea a la vez, garantizando el orden correcto.

### Dos semáforos, un turno cada vez

Se usan **dos semáforos independientes** para que cada tarea tenga su propio mecanismo de bloqueo:

| Semáforo | Lo toma | Lo libera |
|---|---|---|
| `semLedOn` | `TaskLedOn` | `TaskLedOff` |
| `semLedOff` | `TaskLedOff` | `TaskLedOn` |

En `setup()` se hace `xSemaphoreGive(semLedOn)` para dar el primer turno a `TaskLedOn`. A partir de ahí cada tarea, al terminar su trabajo, hace `Give` del semáforo de la otra.

### Diagrama de flujo

```
setup()
  └─ xSemaphoreGive(semLedOn)  ← primer turno para TaskLedOn

      TaskLedOn                          TaskLedOff
          │                                  │
  Take(semLedOn) ✓               Take(semLedOff) — BLOQUEADA
  LED = HIGH                                 │
  "LED ON"                                   │
  vTaskDelay(500ms)                          │
  Give(semLedOff) ──────────────────────►    │
          │                          Take(semLedOff) ✓
  Take(semLedOn) — BLOQUEADA         LED = LOW
          │                          "LED OFF"
          │                          vTaskDelay(500ms)
          ◄──────────────────────────Give(semLedOn)
  Take(semLedOn) ✓                           │
  ...                                       ...
```

### Funciones de FreeRTOS utilizadas

| Función | Descripción |
|---|---|
| `xSemaphoreCreateBinary()` | Crea un semáforo binario (valores 0 y 1) |
| `xSemaphoreGive(sem)` | Libera el semáforo (0 → 1), desbloquea la tarea que lo espera |
| `xSemaphoreTake(sem, timeout)` | Toma el semáforo; bloquea la tarea si vale 0 |
| `portMAX_DELAY` | Espera indefinida hasta que el semáforo esté disponible |
| `vTaskDelay()` | Pausa la tarea cediendo la CPU al planificador |

---

## Salida esperada por el puerto serie

```
Sistema iniciado: LED parpadeando con semaforo binario
LED ON
LED OFF
LED ON
LED OFF
...
```

Los mensajes se alternan siempre en el mismo orden gracias al semáforo.

---

## Cómo compilar y cargar

1. Abre el proyecto con PlatformIO.
2. Conecta el ESP32 por USB.
3. Ejecuta **Build** y luego **Upload**.
4. Abre el **Monitor Serie** a **115200 baudios** para ver la salida.

---

## Referencias

- [FreeRTOS: Semaphores & Mutexes con Arduino](https://circuitdigest.com/microcontroller-projects/arduino-freertos-tutorial-using-semaphore-and-mutex-in-freertos-with-arduino)
- [ESP32 FreeRTOS: Counting Semaphores](https://techtutorialsx.com/2017/05/11/esp32-freertos-counting-semaphores/)
- [Repositorio de referencia ESP32 FreeRTOS](https://github.com/uagaviria/ESP32_FreeRtos)
- [Documentación oficial de FreeRTOS](https://www.freertos.org/)