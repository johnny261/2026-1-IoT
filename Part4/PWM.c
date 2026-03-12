#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/ledc.h"
#include "esp_log.h"

#define PWM_GPIO 18
#define PWM_FREQ 5000
#define PWM_RES  LEDC_TIMER_10_BIT
#define PWM_MAX  1023
#define SPEED_MODE LEDC_LOW_SPEED_MODE // LEDC_HIGH_SPEED_MODE

#define RTOS_delay(x) vTaskDelay(pdMS_TO_TICKS(x))
#define TAG "PWM_TAG"

QueueHandle_t pwmQueue;

void pwm_init()
{
    ledc_timer_config_t timer = {
        .speed_mode       = SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = PWM_RES,
        .freq_hz          = PWM_FREQ,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);
    ledc_channel_config_t channel = {
        .gpio_num   = PWM_GPIO,
        .speed_mode = SPEED_MODE,
        .channel    = LEDC_CHANNEL_0,
        .timer_sel  = LEDC_TIMER_0,
        .duty       = 0,
        .hpoint     = 0
    };
    ledc_channel_config(&channel);
}

void pwm_task(void *pvParameters)
{
    int duty;
    while(1)
    {
        if(xQueueReceive(pwmQueue, &duty, portMAX_DELAY))
        {
            ledc_set_duty(SPEED_MODE, LEDC_CHANNEL_0, duty);
            ledc_update_duty(SPEED_MODE, LEDC_CHANNEL_0);
            ESP_LOGI(TAG, "Duty recibido: %d\n", duty);
        }
    }
}

void generator_task(void *pvParameters)
{
    int duty = 0;
    while(1)
    {
        xQueueSend(pwmQueue, &duty, portMAX_DELAY);
        ESP_LOGI(TAG, "Duty enviado: %d\n", duty);
        duty += 50;
        if(duty > PWM_MAX)
            duty = 0;
        RTOS_delay(500);
    }
}

void app_main()
{
    pwm_init();
    pwmQueue = xQueueCreate(5, sizeof(int));
    xTaskCreate(pwm_task, "PWM_Task", 2048, NULL, 5, NULL);
    xTaskCreate(generator_task, "Generator_Task", 2048, NULL, 4, NULL);
}
