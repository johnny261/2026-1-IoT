#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#include "esp_http_server.h"

#define WIFI_SSID "IWAN"
#define WIFI_PASS "1548Axdsl"

static const char *TAG = "APP";

// Evento WiFi
static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

static esp_netif_t *sta_netif = NULL;

// ============================
// EVENTOS WIFI
// ============================
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "Conectando al WiFi...");
            esp_wifi_connect();
            break;

        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGW(TAG, "Desconectado, reconectando...");
            xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
            esp_wifi_connect();
            break;

        default:
            break;
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

        ESP_LOGI(TAG, "IP obtenida: " IPSTR, IP2STR(&event->ip_info.ip));

        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// ============================
// WIFI INIT
// ============================
void wifi_init(void)
{
    wifi_event_group = xEventGroupCreate();
    esp_netif_init();
    esp_event_loop_create_default();
    sta_netif = esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    ESP_LOGI(TAG, "WiFi inicializado");
}

// ============================
// TAREA MONITOR IP
// ============================
void ip_monitor_task(void *pvParameters)
{
    esp_netif_ip_info_t ip_info;
    while (1)
    {
        xEventGroupWaitBits(wifi_event_group,
                            WIFI_CONNECTED_BIT,
                            pdFALSE,
                            pdFALSE,
                            portMAX_DELAY);
        if (sta_netif != NULL)
        {
            if (esp_netif_get_ip_info(sta_netif, &ip_info) == ESP_OK)
            {
                ESP_LOGI("NET",
                         "Servidor: http://" IPSTR " | MASK: " IPSTR " | GW: " IPSTR,
                         IP2STR(&ip_info.ip),
                         IP2STR(&ip_info.netmask),
                         IP2STR(&ip_info.gw));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// ============================
// HTTP HANDLERS
// ============================

// Página principal
esp_err_t root_get_handler(httpd_req_t *req)
{
    const char *resp =
        "<!DOCTYPE html>"
        "<html><body>"
        "<h1>ESP32 Server</h1>"
        "<p>Servidor funcionando</p>"
        "<a href=\"/status\">Estado</a><br>"
        "<a href=\"/led?state=on\">LED ON</a><br>"
        "<a href=\"/led?state=off\">LED OFF</a>"
        "</body></html>";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Estado en JSON
esp_err_t status_handler(httpd_req_t *req)
{
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(sta_netif, &ip_info);
    char response[200];
    sprintf(response,
            "{ \"ip\": \"" IPSTR "\", \"status\": \"ok\" }",
            IP2STR(&ip_info.ip));
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Control LED (simulado)
esp_err_t led_handler(httpd_req_t *req)
{
    char query[100];
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK)
    {
        char param[10];
        if (httpd_query_key_value(query, "state", param, sizeof(param)) == ESP_OK)
        {
            if (strcmp(param, "on") == 0)
            {
                ESP_LOGI(TAG, "LED ON");
            }
            else if (strcmp(param, "off") == 0)
            {
                ESP_LOGI(TAG, "LED OFF");
            }
        }
    }
    httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// ============================
// SERVIDOR HTTP
// ============================
httpd_handle_t start_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_uri_t root = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = root_get_handler};
        httpd_uri_t status = {
            .uri = "/status",
            .method = HTTP_GET,
            .handler = status_handler};
        httpd_uri_t led = {
            .uri = "/led",
            .method = HTTP_GET,
            .handler = led_handler};
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &status);
        httpd_register_uri_handler(server, &led);
        ESP_LOGI(TAG, "Servidor HTTP iniciado");
    }
    return server;
}

// ============================
// MAIN
// ============================
void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        nvs_flash_erase();
        nvs_flash_init();
    }
    wifi_init();
    // Tarea que imprime IP periódicamente
    xTaskCreate(ip_monitor_task, "ip_monitor", 4096, NULL, 4, NULL);
    // Esperar conexión WiFi
    xEventGroupWaitBits(wifi_event_group,
                        WIFI_CONNECTED_BIT,
                        pdFALSE,
                        pdFALSE,
                        portMAX_DELAY);
    ESP_LOGI(TAG, "Iniciando servidor HTTP...");
    start_server();
}
