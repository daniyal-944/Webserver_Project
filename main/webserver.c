#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_spi_flash.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "stdbool.h"
#include <esp_http_server.h>
#include "nvs_flash.h"
#include "esp_spiffs.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sys.h"

static const char *TAG_MAIN = "main";

//LED CODE
// Function for LED 
// This function will blink the LED connected to GPIO_NUM_2
// You can change the GPIO pin number as per your hardware configuration

#define LED_PIN GPIO_NUM_2 
static bool led_on = false;
void led_init()
{
    gpio_reset_pin(LED_PIN);
	gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level(LED_PIN, 0);
	
}
void led_set_state(bool state)
{
    if (state) {
        gpio_set_level(LED_PIN, 1);
        led_on = true;
    } else {
        gpio_set_level(LED_PIN, 0);
        led_on = false;
    }
}
bool led_get_state()
{
    return led_on;
}

/*``````````````````````````````````````````````````````````````````*/

//WIFI CODE
#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_WIFI_CHANNEL   CONFIG_ESP_WIFI_CHANNEL
#define EXAMPLE_MAX_STA_CONN       CONFIG_ESP_MAX_STA_CONN


#ifndef CONFIG_ESP_WIFI_CHANNEL
#define CONFIG_ESP_WIFI_CHANNEL 1
#endif

#ifndef CONFIG_ESP_MAX_STA_CONN
#define CONFIG_ESP_MAX_STA_CONN 4
#endif


static const char *TAG_WIFI = "wifi softAP";

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG_WIFI, "station %02x:%02x:%02x:%02x:%02x:%02x join, AID=%d",
         event->mac[0], event->mac[1], event->mac[2],
         event->mac[3], event->mac[4], event->mac[5], event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG_WIFI, "station %02x:%02x:%02x:%02x:%02x:%02x leave, AID=%d, reason=%d",
         event->mac[0], event->mac[1], event->mac[2],
         event->mac[3], event->mac[4], event->mac[5], event->aid, event->reason);
    }
}

void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
#ifdef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
            .authmode = WIFI_AUTH_WPA3_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
#else /* CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT */
            .authmode = WIFI_AUTH_WPA2_PSK,
#endif
            .pmf_cfg = {
                    .required = true,
            },
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG_WIFI, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
}

/*--------------------------------------------------------------------------------------*/

//Spiffs Initialization
void init_spiffs(void)
{
    ESP_LOGI(TAG_MAIN, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_MAIN, "Failed to mount SPIFFS (%s)", esp_err_to_name(ret));
    }
}

/*---------------------------------------------------------------------------------------*/


//WEBSERVER CODE
static const char *TAG_WS = "websocket";
static httpd_handle_t server = NULL;

#define INDEX_HTML_PATH "/spiffs/index.html"
char index_html[4096]; // Buffer to hold the HTML file content

// Serve the index.html file over HTTP GET at "/"
esp_err_t index_get_handler(httpd_req_t *req)
{
    FILE *file = fopen(INDEX_HTML_PATH, "r");
    if (!file) {
        ESP_LOGE(TAG_WS, "Failed to open %s", INDEX_HTML_PATH);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    size_t read_bytes = fread(index_html, 1, sizeof(index_html) - 1, file);
    fclose(file);

    if (read_bytes <= 0) {
    ESP_LOGE(TAG_WS, "Failed to read file");
    httpd_resp_send_500(req);
    return ESP_FAIL;
    }

    index_html[read_bytes] = '\0';
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, index_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// WebSocket handler
esp_err_t websocket_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG_WS, "WebSocket handshake done");
        return httpd_ws_upgrade(req);
    }

    httpd_ws_frame_t frame = {
        .final = true,
        .fragmented = false,
        .type = HTTPD_WS_TYPE_TEXT,
        .payload = NULL,
        .len = 0,
    };

    esp_err_t ret = httpd_ws_recv_frame(req, &frame, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_WS, "Failed to receive WS frame header");
        return ret;
    }

    if (frame.len > 4095) {
        ESP_LOGW(TAG_WS, "Frame too big");
        return ESP_FAIL;
    }

    frame.payload = malloc(frame.len + 1);
    if (!frame.payload) return ESP_ERR_NO_MEM;

    ret = httpd_ws_recv_frame(req, &frame, frame.len);
    if (ret != ESP_OK) {
        free(frame.payload);
        return ret;
    }
    ((char *)frame.payload)[frame.len] = 0;

    ESP_LOGI(TAG_WS, "Received: %s", (char *)frame.payload);

    // LED Control
    if (strcmp((char *)frame.payload, "ON") == 0) {
        led_set_state(true);
    } else if (strcmp((char *)frame.payload, "OFF") == 0) {
        led_set_state(false);
    }

    const char *resp = led_get_state() ? "ON" : "OFF";

    httpd_ws_frame_t ws_msg = {
        .type = HTTPD_WS_TYPE_TEXT,
        .payload = (uint8_t *)resp,
        .len = strlen(resp),
        .final = true
    };

    ret = httpd_ws_send_frame(req, &ws_msg);
    free(frame.payload);
    return ret;
}

httpd_handle_t setup_websocket_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    httpd_uri_t uri_get = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = index_get_handler,
        .user_ctx = NULL
    };

    httpd_uri_t websocket = {
        .uri = "/websocket",
        .method = HTTP_GET,
        .handler = websocket_handler,
        .user_ctx = NULL,
        .is_websocket = true
    };

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &websocket);
        ESP_LOGI(TAG_WS, "WebSocket server started");
    } else {
        ESP_LOGE(TAG_WS, "Failed to start WebSocket server");
    }

    return server;
}

// Main application entry point


void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG_MAIN, "ESP_WIFI_MODE_AP");
    init_spiffs();
    wifi_init_softap();
    led_init();
    setup_websocket_server();
}