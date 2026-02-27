#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"

#define TAG "MUTEX_EXAMPLE"

SemaphoreHandle_t xMutex;

int sharedCounter = 0;

void task1(void *pvParameters)
{
    while (1)
    {
        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE)
        {
            int local = sharedCounter;
            vTaskDelay(pdMS_TO_TICKS(100)); // Simula trabajo
            sharedCounter = local + 1;

            ESP_LOGI(TAG, "Task1 incrementó contador a %d", sharedCounter);

            xSemaphoreGive(xMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void task2(void *pvParameters)
{
    while (1)
    {
        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE)
        {
            int local = sharedCounter;
            vTaskDelay(pdMS_TO_TICKS(100)); // Simula trabajo
            sharedCounter = local + 1;

            ESP_LOGI(TAG, "Task2 incrementó contador a %d", sharedCounter);

            xSemaphoreGive(xMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(700));
    }
}

void app_main(void)
{
    xMutex = xSemaphoreCreateMutex();

    if (xMutex == NULL)
    {
        ESP_LOGE(TAG, "Error creando el mutex");
        return;
    }

    xTaskCreate(task1, "Task1", 2048, NULL, 2, NULL);
    xTaskCreate(task2, "Task2", 2048, NULL, 2, NULL);
}
