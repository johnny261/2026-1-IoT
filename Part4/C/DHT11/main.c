#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "dht11.h"

#define TAG         "DTH11"
#define DHT11_PIN   GPIO_NUM_4
#define RTOS_delay(x) vTaskDelay(pdMS_TO_TICKS(x))

// Cola
QueueHandle_t dht_queue;

// TAREA: DHT11
void task_dht(void *arg)
{
    dht11_data_t data;
    while(1)
    {
        esp_err_t err = dht11_read(DHT11_PIN, &data);
        if (err == ESP_OK) {
            // xQueueOverwrite: siempre el dato más reciente (cola tamaño 1)
            xQueueOverwrite(dht_queue, &data);
        } else {
            ESP_LOGI(TAG, "DHT11 error: %s", esp_err_to_name(err));
        }
        RTOS_delay(2000);
    }
}

//  TAREA: Monitor / Display
void task_monitor(void *arg)
{
    dht11_data_t  dht_data  = {0};
    while(1)
    {
        xQueueReceive(dht_queue,   &dht_data,  0);
        ESP_LOGI(TAG, "(T, H)=(%.1f°C, %.1f%%)",
                 dht_data.temperature,
                 dht_data.humidity);
        RTOS_delay(1000);
    }
}

//  Main
void app_main(void)
{
    // Inicializar sensores
    dht11_init(DHT11_PIN);
    // Cola tamaño 1: Siempre dato más reciente
    dht_queue = xQueueCreate(1, sizeof(dht11_data_t));
    // Crear tareas
    xTaskCreatePinnedToCore(task_dht, "dht", 4096, NULL, 5, NULL, APP_CPU_NUM);
    xTaskCreatePinnedToCore(task_monitor, "monitor", 4096, NULL, 3, NULL, APP_CPU_NUM);
}
