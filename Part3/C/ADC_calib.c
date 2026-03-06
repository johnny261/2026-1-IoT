#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"

//#include "driver/adc_oneshot.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define TAG "ADC_CAL"

#define ADC_CHANNEL ADC_CHANNEL_6 // GPIO7
#define ADC_WIDTH ADC_BITWIDTH_12
#define ADC_ATTEN ADC_ATTEN_DB_12
//#define ADC_ATTEN ADC_ATTEN_DB_11

QueueHandle_t adcQueue;

/* handles ADC */

adc_oneshot_unit_handle_t adc_handle;
adc_cali_handle_t adc_cali_handle;

bool do_calibration = false;

void adc_init()
{
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };

    adc_oneshot_new_unit(&init_config, &adc_handle);

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_WIDTH,
    };

    adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &config);
}

bool adc_calibration_init()
{
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_WIDTH,
    };

    if (adc_cali_create_scheme_curve_fitting(&cali_config, &adc_cali_handle) == ESP_OK)
    {
        ESP_LOGI(TAG, "ADC Calibration enabled");
        return true;
    }

    ESP_LOGW(TAG, "ADC Calibration not supported");
    return false;
}

void taskADC(void *pvParameters)
{
    int adc_raw;

    while (1)
    {
        adc_oneshot_read(adc_handle, ADC_CHANNEL, &adc_raw);
        xQueueSend(adcQueue, &adc_raw, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void taskProcessing(void *pvParameters)
{
    int adc_raw;
    int voltage;

    while (1)
    {
        if (xQueueReceive(adcQueue, &adc_raw, portMAX_DELAY))
        {

            if (do_calibration)
            {
                adc_cali_raw_to_voltage(adc_cali_handle, adc_raw, &voltage);
                ESP_LOGI(TAG, "RAW: %d  Voltage: %d mV", adc_raw, voltage);
            }
            else
            {
                ESP_LOGI(TAG, "RAW: %d", adc_raw);
            }
        }
    }
}

void app_main(void)
{
    adc_init();

    do_calibration = adc_calibration_init();

    adcQueue = xQueueCreate(10, sizeof(int));

    xTaskCreate(taskADC, "TaskADC", 4096, NULL, 2, NULL);
    xTaskCreate(taskProcessing, "TaskProcessing", 4096, NULL, 1, NULL);
}
