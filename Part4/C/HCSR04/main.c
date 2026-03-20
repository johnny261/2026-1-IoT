#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "hcsr04.h"

#define TAG         "MAIN"
#define TRIG_PIN    GPIO_NUM_5
#define ECHO_PIN    GPIO_NUM_18
#define RTOS_delay(x) vTaskDelay(pdMS_TO_TICKS(x))

// Cola
QueueHandle_t sonar_queue;

// Configuración del sensor
hcsr04_config_t sonar_cfg = {
    .trig_pin = TRIG_PIN,
    .echo_pin = ECHO_PIN,
};

//  TAREA: HC-SR04
void task_sonar(void *arg)
{
    float distance_cm;

    for (;;) {
        esp_err_t err = hcsr04_measure_cm(&sonar_cfg, &distance_cm);
        if (err == ESP_OK) {
            xQueueSend(sonar_queue, &distance_cm, portMAX_DELAY);
        } else {
            ESP_LOGI(TAG, "HC-SR04 error: %s", esp_err_to_name(err));
        }
        RTOS_delay(200);
    }
}

//  TAREA: Monitor / Display
void task_monitor(void *arg) {
    float distance = 0.0f;
    for (;;) 
    {
        xQueueReceive(sonar_queue, &distance,  0);
        ESP_LOGI(TAG, "Dist: %.1f cm", distance);
        RTOS_delay(500);
    }
}

//  Main
void app_main(void)
{
    // Inicializar sensores
    hcsr04_init(&sonar_cfg);

    // Crear cola
    sonar_queue = xQueueCreate(5, sizeof(float));

    // Crear tareas: PinnedToCore
    xTaskCreatePinnedToCore(task_sonar, "sonar", 4096, NULL, 5, NULL, APP_CPU_NUM);
    xTaskCreatePinnedToCore(task_monitor, "monitor", 4096, NULL, 3, NULL, APP_CPU_NUM);
}
