typedef enum {
    DATA_INT,
    DATA_STRING
} data_type_t;

typedef struct {
    data_type_t type;
    union {
        int intValue;
        char strValue[32];
    } data;
} queue_message_t;

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#define TAG "QUEUE_EXAMPLE"

#define QUEUE_LENGTH 10

QueueHandle_t dataQueue;

void producerTask1(void *pvParameters)
{
    queue_message_t msg;
    int counter = 0;

    while (1)
    {
        msg.type = DATA_INT;
        msg.data.intValue = counter++;

        if (xQueueSend(dataQueue, &msg, portMAX_DELAY) == pdPASS)
        {
            ESP_LOGI(TAG, "Task1 envió entero: %d", msg.data.intValue);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void producerTask2(void *pvParameters)
{
    queue_message_t msg;
    int counter = 0;

    while (1)
    {
        msg.type = DATA_STRING;
        snprintf(msg.data.strValue, sizeof(msg.data.strValue),
                 "Mensaje %d", counter++);

        if (xQueueSend(dataQueue, &msg, portMAX_DELAY) == pdPASS)
        {
            ESP_LOGI(TAG, "Task2 envió string: %s", msg.data.strValue);
        }

        vTaskDelay(pdMS_TO_TICKS(1500));
    }
}

void consumerTask(void *pvParameters)
{
    queue_message_t receivedMsg;

    while (1)
    {
        if (xQueueReceive(dataQueue, &receivedMsg, portMAX_DELAY) == pdPASS)
        {
            switch (receivedMsg.type)
            {
                case DATA_INT:
                    ESP_LOGI(TAG, "Consumer recibió entero: %d",
                             receivedMsg.data.intValue);
                    break;

                case DATA_STRING:
                    ESP_LOGI(TAG, "Consumer recibió string: %s",
                             receivedMsg.data.strValue);
                    break;
            }
        }
    }
}

void app_main(void)
{
    dataQueue = xQueueCreate(QUEUE_LENGTH, sizeof(queue_message_t));

    if (dataQueue == NULL)
    {
        ESP_LOGE(TAG, "Error creando la cola");
        return;
    }

    xTaskCreate(producerTask1, "Producer1", 2048, NULL, 2, NULL);
    xTaskCreate(producerTask2, "Producer2", 2048, NULL, 2, NULL);
    xTaskCreate(consumerTask,  "Consumer",  2048, NULL, 1, NULL);
}
