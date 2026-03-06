#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"

#include "esp_adc/adc_continuous.h"

#define TAG "ADC_DMA"

/* configuración ADC */

#define ADC_CHANNEL ADC_CHANNEL_6
#define ADC_ATTEN ADC_ATTEN_DB_12

#define SAMPLE_FREQ_HZ 2000
#define FRAME_SIZE 256

QueueHandle_t adcQueue;

/* handle ADC */
adc_continuous_handle_t adc_handle;

/* estructura de datos */
typedef struct
{
    uint16_t value;
} adc_sample_t;

void adc_dma_setup()
{
    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = 1024,
        .conv_frame_size = FRAME_SIZE,
    };
    adc_continuous_new_handle(&adc_config, &adc_handle);

    adc_digi_pattern_config_t adc_pattern = {
        .atten = ADC_ATTEN,
        .channel = ADC_CHANNEL,
        .unit = ADC_UNIT_1,
        .bit_width = ADC_BITWIDTH_12
    };

    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = SAMPLE_FREQ_HZ,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
        .pattern_num = 1,
        .adc_pattern = &adc_pattern
    };

    adc_continuous_config(adc_handle, &dig_cfg);
    adc_continuous_start(adc_handle);
}

void taskADC(void *pvParameters)
{
    uint8_t buffer[FRAME_SIZE];
    uint32_t length = 0;
    adc_sample_t sample;
    while (1)
    {
        if (adc_continuous_read(adc_handle, buffer, FRAME_SIZE, &length, portMAX_DELAY) == ESP_OK)
        {
            for (int i = 0; i < length; i += sizeof(adc_digi_output_data_t))
            {
                adc_digi_output_data_t *p = (adc_digi_output_data_t *)&buffer[i];
                sample.value = p->type2.data;
                xQueueSend(adcQueue, &sample, portMAX_DELAY);
            }
        }
    }
}

#define FILTER_SIZE 10

void taskFilter(void *pvParameters)
{
    adc_sample_t sample;
    uint32_t sum = 0;
    uint16_t buffer[FILTER_SIZE];
    int index = 0;
    memset(buffer, 0, sizeof(buffer));
    while (1)
    {
        if (xQueueReceive(adcQueue, &sample, portMAX_DELAY))
        {
            sum -= buffer[index];
            buffer[index] = sample.value;
            sum += sample.value;
            index = (index + 1) % FILTER_SIZE;
            uint16_t avg = sum / FILTER_SIZE;
            ESP_LOGI(TAG, "RAW:%d  FILTER:%d", sample.value, avg);
        }
    }
}

void app_main()
{
    adc_dma_setup();
    adcQueue = xQueueCreate(50, sizeof(adc_sample_t));
    xTaskCreate(taskADC, "ADC_Task", 4096, NULL, 3, NULL);
    xTaskCreate(taskFilter, "Filter_Task", 4096, NULL, 2, NULL);
}
