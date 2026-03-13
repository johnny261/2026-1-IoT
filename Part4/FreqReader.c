#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_timer.h"
#include "esp_log.h"

#define SIGNAL_GPIO 4
#define RTOS_delay(x) vTaskDelay(pdMS_TO_TICKS(x))
#define TAG "IC_TAG"

#define PWM_GPIO0 17
#define PWM_FREQ0 5000
#define PWM_GPIO1 18
#define PWM_FREQ1 8500
#define PWM_RES  LEDC_TIMER_10_BIT
#define PWM_MAX  1023
#define SPEED_MODE LEDC_LOW_SPEED_MODE // LEDC_HIGH_SPEED_MODE

QueueHandle_t time_queue;

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    int64_t time = esp_timer_get_time(); // tiempo en microsegundos
    xQueueSendFromISR(time_queue, &time, NULL);
}

void gpio_init()
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << SIGNAL_GPIO),
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_POSEDGE,
        .pull_up_en = 0,
        .pull_down_en = 1
    };
    gpio_config(&io_conf);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(SIGNAL_GPIO, gpio_isr_handler, NULL);
}

void pwm_init()
{
    // PWM 1: GPIO 17, Freq 5kHz
    ledc_timer_config_t timer0 = {
        .speed_mode       = SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = PWM_RES,
        .freq_hz          = PWM_FREQ0,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer0);
    ledc_channel_config_t ch0_t0 = {
        .gpio_num   = PWM_GPIO0,
        .speed_mode = SPEED_MODE,
        .channel    = LEDC_CHANNEL_0,
        .timer_sel  = LEDC_TIMER_0,
        .duty       = 512,
        .hpoint     = 0
    };
    ledc_channel_config(&ch0_t0);
    
    // PWM 2: GPIO 18, Freq 8.5kHz
    ledc_timer_config_t timer1 = {
        .speed_mode       = SPEED_MODE,
        .timer_num        = LEDC_TIMER_1,
        .duty_resolution  = PWM_RES,
        .freq_hz          = PWM_FREQ1,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer1);
    ledc_channel_config_t ch0_t1 = {
        .gpio_num   = PWM_GPIO1,
        .speed_mode = SPEED_MODE,
        .channel    = LEDC_CHANNEL_1,
        .timer_sel  = LEDC_TIMER_1,
        .duty       = 512,
        .hpoint     = 0
    };
    ledc_channel_config(&ch0_t1);
}

void period_task(void *arg)
{
    int64_t t1 = 0;
    int64_t t2 = 0;
    int i = 0;
    while (1)
    {
        if (xQueueReceive(time_queue, &t2, portMAX_DELAY))
        {
            if (t1 != 0)
            {
                int64_t period = t2 - t1;
                if(i++ % 200 == 0)
                    ESP_LOGI(TAG, "(T, F) = (%lld us, %.2f Hz)", period, 1000000.0 / period);
            }
            t1 = t2;
        }
    }
}

void app_main()
{
    time_queue = xQueueCreate(100, sizeof(int64_t));
    gpio_init();
    pwm_init();
    xTaskCreate(period_task, "period_task", 4096, NULL, 5, NULL);
}
