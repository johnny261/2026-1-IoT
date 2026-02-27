#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define TAG "LED_BUTTON"

#define LED_GPIO     2
#define BUTTON_GPIO  4

QueueHandle_t buttonQueue;

void gpio_init(void)
{
    // LED como salida
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&led_conf);

    // Botón como entrada con pull-up interno
    gpio_config_t button_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&button_conf);
}

void taskButton(void *pvParameters)
{
    int lastState = 1;  // Pull-up 1 = no presionado
    int currentState;

    while (1)
    {
        currentState = gpio_get_level(BUTTON_GPIO);

        // Detectar flanco de bajada
        if (lastState == 1 && currentState == 0)
        {
            ESP_LOGI(TAG, "Botón presionado");
            xQueueSend(buttonQueue, &currentState, portMAX_DELAY);

            vTaskDelay(pdMS_TO_TICKS(200)); // debounce simple
        }

        lastState = currentState;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
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
        }
    }
}

void app_main(void)
{
    gpio_init();

    buttonQueue = xQueueCreate(5, sizeof(int));

    xTaskCreate(taskButton, "TaskButton", 2048, NULL, 2, NULL);
    xTaskCreate(taskLed,    "TaskLed",    2048, NULL, 2, NULL);
}
