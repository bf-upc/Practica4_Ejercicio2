/*
 * Practica 4 - Ejercicio 2
 * Sistemas Operativos en Tiempo Real
 *
 * Dos tareas sincronizadas con semáforo binario:
 *   - TaskLedOn:  espera el semáforo, enciende el LED y lo libera para TaskLedOff
 *   - TaskLedOff: espera el semáforo, apaga  el LED y lo libera para TaskLedOn
 *
 * El semáforo binario garantiza que ambas tareas se alternen estrictamente:
 * ON → OFF → ON → OFF → ...
 */

#include <Arduino.h>

// ── Configuración ────────────────────────────────────────────────────────────
#define DELAY_MS   500       // Tiempo encendido/apagado en milisegundos

// ── Handles de FreeRTOS ──────────────────────────────────────────────────────
SemaphoreHandle_t semLedOn;   // Semáforo que autoriza encender el LED
SemaphoreHandle_t semLedOff;  // Semáforo que autoriza apagar el LED

// ── Prototipos ───────────────────────────────────────────────────────────────
void TaskLedOn (void *parameter);
void TaskLedOff(void *parameter);

// ── setup ────────────────────────────────────────────────────────────────────
void setup()
{
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);

    // Crear semáforos binarios
    semLedOn  = xSemaphoreCreateBinary();
    semLedOff = xSemaphoreCreateBinary();

    if (semLedOn == NULL || semLedOff == NULL) {
        Serial.println("ERROR: no se pudieron crear los semaforos");
        while (true);   // Detener ejecución si falla la inicialización
    }

    // Dar el semáforo inicial a TaskLedOn para que arranque primero
    xSemaphoreGive(semLedOn);

    // Crear las dos tareas con la misma prioridad
    xTaskCreate(TaskLedOn,  "LED ON",  2048, NULL, 1, NULL);
    xTaskCreate(TaskLedOff, "LED OFF", 2048, NULL, 1, NULL);

    Serial.println("Sistema iniciado: LED parpadeando con semaforo binario");
}

// ── loop ─────────────────────────────────────────────────────────────────────
void loop()
{
    // No se usa: la lógica está en las tareas FreeRTOS
    vTaskDelay(portMAX_DELAY);
}

// ── Tarea: encender LED ──────────────────────────────────────────────────────
void TaskLedOn(void *parameter)
{
    for (;;)
    {
        // Esperar permiso para encender (bloquea hasta recibir el semáforo)
        xSemaphoreTake(semLedOn, portMAX_DELAY);

        digitalWrite(LED_BUILTIN, HIGH);
        Serial.println("LED ON");

        // Mantener el LED encendido el tiempo configurado
        vTaskDelay(DELAY_MS / portTICK_PERIOD_MS);

        // Ceder el turno a TaskLedOff
        xSemaphoreGive(semLedOff);
    }
}

// ── Tarea: apagar LED ────────────────────────────────────────────────────────
void TaskLedOff(void *parameter)
{
    for (;;)
    {
        // Esperar permiso para apagar (bloquea hasta recibir el semáforo)
        xSemaphoreTake(semLedOff, portMAX_DELAY);

        digitalWrite(LED_BUILTIN, LOW);
        Serial.println("LED OFF");

        // Mantener el LED apagado el tiempo configurado
        vTaskDelay(DELAY_MS / portTICK_PERIOD_MS);

        // Ceder el turno a TaskLedOn
        xSemaphoreGive(semLedOn);
    }
}