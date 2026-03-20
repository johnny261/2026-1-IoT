#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/adc.h"
#include "esp_log.h"

#define TAG "ADC_RTOS"

#define ADC_CHANNEL ADC1_CHANNEL_6   // GPIO7
#define ADC_WIDTH ADC_WIDTH_BIT_12
#define ADC_ATTEN ADC_ATTEN_DB_11

#define RTOS_delay(x) vTaskDelay(pdMS_TO_TICKS(x))

QueueHandle_t adcQueue;

void adc_init()
{
    adc1_config_width(ADC_WIDTH);
    adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN);
}

void taskADC(void *pvParameters)
{
    int adc_reading;
    while (1)
    {
        adc_reading = adc1_get_raw(ADC_CHANNEL);
        xQueueSend(adcQueue, &adc_reading, portMAX_DELAY);
        RTOS_delay(500);
    }
}

void taskProcessing(void *pvParameters)
{
    int received_value;
    while (1)
    {
        if (xQueueReceive(adcQueue, &received_value, portMAX_DELAY))
        {
            ESP_LOGI(TAG, "ADC value: %d", received_value);
        }
    }
}

void app_main(void)
{
    adc_init();
    adcQueue = xQueueCreate(10, sizeof(int));
    xTaskCreate(taskADC, "TaskADC", 2048, NULL, 2, NULL);
    xTaskCreate(taskProcessing, "TaskProcessing", 2048, NULL, 1, NULL);
}
