#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define TAG "LED_ISR"

#define LED_GPIO     2
#define BUTTON_GPIO  4

QueueHandle_t buttonQueue;

static void IRAM_ATTR button_isr_handler(void* arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    int event = 1;

    xQueueSendFromISR(buttonQueue, &event, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}

void gpio_init(void)
{
    // LED salida
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&led_conf);

    // Botón entrada con pull-up e interrupción
    gpio_config_t button_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE  // flanco descendente
    };
    gpio_config(&button_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO, button_isr_handler, NULL);
}

void taskLed(void *pvParameters)
{
    int received;
    int ledState = 0;

    while (1)
    {
        if (xQueueReceive(buttonQueue, &received, portMAX_DELAY))
        {
            ledState = !ledState;
            gpio_set_level(LED_GPIO, ledState);

            ESP_LOGI(TAG, "LED cambiado a: %d", ledState);

            // Debounce simple por software
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}

void app_main(void)
{
    gpio_init();

    buttonQueue = xQueueCreate(10, sizeof(int));

    if (buttonQueue == NULL)
    {
        ESP_LOGE(TAG, "Error creando la cola");
        return;
    }

    xTaskCreate(taskLed, "TaskLed", 2048, NULL, 2, NULL);
}
