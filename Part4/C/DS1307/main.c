#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "ds1307.h"

static const char *TAG = "I2C";

#define RTOS_delay(x) vTaskDelay(pdMS_TO_TICKS(x))

// Cola global
QueueHandle_t rtc_queue;

// TASK PRODUCTOR
void rtc_reader_task(void *pvParameters)
{
    ds1307_time_t time;
    while (1)
    {
        ds1307_get_time(&time);
        // Enviar a la cola
        if (xQueueSend(rtc_queue, &time, pdMS_TO_TICKS(100)) != pdPASS) {
            ESP_LOGW(TAG, "Cola llena, dato perdido");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// TASK CONSUMIDOR
void rtc_logger_task(void *pvParameters)
{
    ds1307_time_t time;
    while (1)
    {
        // Espera bloqueante
        if (xQueueReceive(rtc_queue, &time, portMAX_DELAY)) {
            ESP_LOGI(TAG,
                "Fecha: %02d/%02d/20%02d Hora: %02d:%02d:%02d",
                time.date, time.month, time.year,
                time.hour, time.min, time.sec);
        }
    }
}

// Escáner
void i2c_scanner() {
    ESP_LOGI("SCAN", "Escaneando bus I2C...");

    for (uint8_t addr = 1; addr < 127; addr++) {

        i2c_cmd_handle_t cmd = i2c_cmd_link_create();

        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);

        esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 50 / portTICK_PERIOD_MS);

        i2c_cmd_link_delete(cmd);

        if (ret == ESP_OK) {
            ESP_LOGI("SCAN", "Dispositivo encontrado en 0x%02X", addr);
        }
    }
}

// Main
void app_main(void)
{
    ds1307_init();
    RTOS_delay(5000);
    i2c_scanner();
    // Ajustar hora (solo una vez)
    ds1307_set_time(0, 45, 14, 3, 19, 3, 26);
    // Crear cola (capacidad de 5 elementos)
    rtc_queue = xQueueCreate(5, sizeof(ds1307_time_t));
    if (rtc_queue == NULL) {
        ESP_LOGE(TAG, "Error creando la cola");
        return;
    }
    // Crear tareas
    xTaskCreate(rtc_reader_task, "rtc_reader", 4096, NULL, 5, NULL);
    xTaskCreate(rtc_logger_task, "rtc_logger", 4096, NULL, 5, NULL);
}
