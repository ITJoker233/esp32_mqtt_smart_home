#include "wifi_event.h"
#include "protocol.h"
#include "utils.h"

SemaphoreHandle_t ap_sem;
#define SCRATCH_BUFSIZE 8192
const char *WIFI_TAG = "WIFI";
#define EXAMPLE_ESP_MAXIMUM_RETRY 10

static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
esp_netif_t *ap_netif;
httpd_handle_t server = NULL;
void wifi_init_sta();

extern const char root_start[] asm("_binary_root_html_start");
extern const char root_end[] asm("_binary_root_html_end");
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(WIFI_TAG, "station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(WIFI_TAG, "station " MACSTR " leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

void wifi_station_task(void *pvParameters)
{
    uint32_t result = 0;
    while (1)
    {
        result = xSemaphoreTake(ap_sem, portMAX_DELAY);
        if (result == pdPASS)
        {
            esp_restart();
            esp_wifi_stop();
            esp_event_handler_unregister(WIFI_EVENT,
                                         ESP_EVENT_ANY_ID,
                                         &wifi_event_handler);
            esp_netif_destroy_default_wifi(ap_netif);
            esp_event_loop_delete_default();
            esp_wifi_deinit();
            esp_netif_deinit();
            httpd_stop(server);
            wifi_init_sta();
            ESP_LOGI(WIFI_TAG, "wifi station inited...");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

esp_err_t http_get_nvs_wifi_config(wifi_config_t *wifi_config)
{
    char str[64] = "";
    size_t str_size = sizeof(str);

    esp_err_t err = from_nvs_get_value("wifi_ssid", str, &str_size);
    if (err == ESP_OK)
    {
        strncpy(&wifi_config->sta.ssid, str, str_size);
    }
    str[64] = "";
    err = from_nvs_get_value("wifi_pwd", str, &str_size);
    if (err == ESP_OK)
    {
        strncpy(&wifi_config->sta.password, str, sizeof(wifi_config->sta.password));
    }

    ESP_LOGI(WIFI_TAG, "%s\r\n", wifi_config->sta.ssid);
    ESP_LOGI(WIFI_TAG, "%s\r\n", wifi_config->sta.password);
    return ESP_OK;
}

//---------------------wifi_sta-----------------------//
static int s_retry_num = 0;

static void sta_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        wifi_event_sta_disconnected_t *disconnected = (wifi_event_sta_disconnected_t *)event_data;
        ESP_LOGE(WIFI_TAG, "Disconnect reason : %d", disconnected->reason);
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(WIFI_TAG, "retry to connect to the AP");
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(WIFI_TAG, "connect to the AP fail");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_BEACON_TIMEOUT)
    {
        ESP_LOGI(WIFI_TAG, "WIFI_EVENT_STA_BEACON_TIMEOUT");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(WIFI_TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}
void wifi_init_sta()
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    // ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &sta_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &sta_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    http_get_nvs_wifi_config(&wifi_config);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(WIFI_TAG, "wifi_init_sta finished.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(WIFI_TAG, "connected to ap SSID:%s password:%s",
                 wifi_config.sta.ssid, wifi_config.sta.password);
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(WIFI_TAG, "Failed to connect to SSID:%s, password:%s",
                 wifi_config.sta.ssid, wifi_config.sta.password);
    }
    else
    {
        ESP_LOGE(WIFI_TAG, "UNEXPECTED Eprotocol_examples_common.h:ENT");
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
}

void wifi_init_softap(void)
{
    char wifi_ap_name[32];
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    sprintf(wifi_ap_name, "%s-%0.12s", DEVICE_NAME, device_data.mac);

    wifi_config_t wifi_config = {
        .ap = {
            // .ssid = wifi_ap_name,
            .ssid_len = strlen(wifi_ap_name),
            .password = "",
            .max_connection = 10,
            .authmode = WIFI_AUTH_OPEN},
    };
    memcpy(wifi_config.ap.ssid, wifi_ap_name, sizeof(wifi_config.ap.ssid));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA)); // AP+STA
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);

    char ip_addr[16];
    inet_ntoa_r(ip_info.ip.addr, ip_addr, 16);
    ESP_LOGI(WIFI_TAG, "Set up softAP with IP: %s", ip_addr);

    ESP_LOGI(WIFI_TAG, "wifi_init_softap finished. SSID:'%s' password:'%s'",
             wifi_ap_name, "");
}

// HTTP GET Handler
static esp_err_t root_get_handler(httpd_req_t *req)
{
    const uint32_t root_len = root_end - root_start;

    ESP_LOGI(WIFI_TAG, "Serve root");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, root_start, root_len);

    return ESP_OK;
}

static const httpd_uri_t root = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_get_handler};

// HTTP Error (404) Handler - Redirects all requests to the root page
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    // Set status
    httpd_resp_set_status(req, "302 Temporary Redirect");
    // Redirect to the "/" root directory
    httpd_resp_set_hdr(req, "Location", "/");
    // iOS requires content in the response to detect a captive portal, simply redirecting is not sufficient.
    httpd_resp_send(req, "Redirect to the captive portal", HTTPD_RESP_USE_STRLEN);

    ESP_LOGI(WIFI_TAG, "Redirecting to root");
    return ESP_OK;
}
// WiFi Scan Handler 热点扫描
#define DEFAULT_WIFI_SCAN_COUNT 20
static wifi_ap_record_t wifi_scan_list[DEFAULT_WIFI_SCAN_COUNT];
static uint16_t wifi_scan_count = DEFAULT_WIFI_SCAN_COUNT;

static void get_wifi_list(void)
{
    esp_wifi_scan_start(NULL, true);
    esp_wifi_scan_get_ap_num(&wifi_scan_count);
    printf("Number of access points found: %d\n", wifi_scan_count);
    if (wifi_scan_count > 0)
    {
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&wifi_scan_count, wifi_scan_list));
        printf("  SSID             | Channel | RSSI | Authmode\n");
        for (int i = 0; (i < wifi_scan_count) && (wifi_scan_count < DEFAULT_WIFI_SCAN_COUNT); i++)
        {
            printf("%32s | %7d | %4d | %s\n", (char *)wifi_scan_list[i].ssid, wifi_scan_list[i].primary, wifi_scan_list[i].rssi, (wifi_scan_list[i].authmode == WIFI_AUTH_OPEN) ? "open" : "unknown");
        }
    }
}
static esp_err_t wifi_scan_handler(httpd_req_t *req)
{
    cJSON *wifi_infos_obj = cJSON_CreateObject();
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    get_wifi_list();
    cJSON *wifi_infos = cJSON_CreateArray(); // 创建一个数组
    char *ssid;
    for (int i = 0; i < wifi_scan_count; i++)
    {
        cJSON *wifi_info = cJSON_CreateObject();
        cJSON_AddNumberToObject(wifi_info, "rssi", wifi_scan_list[i].rssi);
        cJSON_AddStringToObject(wifi_info, "ssid", (char *)(wifi_scan_list[i].ssid));
        cJSON_AddItemToArray(wifi_infos, wifi_info);
    }
    cJSON_AddItemToObject(wifi_infos_obj, "wifi_infos", wifi_infos); // 各模块数据
    char *wifi_infos_obj_json = cJSON_Print(wifi_infos_obj);         // JSON数据结构转换为JSON字符串
    httpd_resp_send(req, wifi_infos_obj_json, strlen(wifi_infos_obj_json));
    ESP_LOGI(WIFI_TAG, "wifi scan get successfully!");
    return ESP_OK;
}
static const httpd_uri_t wifi_scan = {
    .uri = "/wifi_scan", // Match all URIs of type /upload/path/to/file
    .method = HTTP_GET,
    .handler = wifi_scan_handler,
};
static esp_err_t send_wifi_handler(httpd_req_t *req)
{
    char user_id[32] = "";
    char user_code[64] = "";
    char qcp_code[10] = "";
    int total_len = req->content_len;
    int cur_len = 0;
    char buf[SCRATCH_BUFSIZE];
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE)
    {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len)
    {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0)
        {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }

    buf[total_len] = '\0';
    for (int i = 0; i < total_len; i++)
    {
        putchar(buf[i]);
    }
    cJSON *root = cJSON_Parse(buf);

    char *wifi_ssid = cJSON_GetObjectItem(root, "wifi_ssid")->valuestring;
    char *wifi_pwd = cJSON_GetObjectItem(root, "wifi_pwd")->valuestring;
    char *decive_code = cJSON_GetObjectItem(root, "decive_code")->valuestring;
    int len1 = strlen(wifi_ssid);
    int len2 = strlen(wifi_pwd);
    int len3 = strlen(decive_code);
    memcpy(user_id, wifi_ssid, strlen(wifi_ssid));
    memcpy(user_code, wifi_pwd, strlen(wifi_pwd));
    memcpy(qcp_code, decive_code, strlen(decive_code));
    user_id[len1] = '\0';
    user_code[len2] = '\0';
    qcp_code[len3] = '\0';
    cJSON_Delete(root);
    ESP_LOGI(WIFI_TAG, "json load  finished. SSID:%d wifi ssid:%d ", wifi_ssid, wifi_pwd);
    ESP_LOGI(WIFI_TAG, "json load  finished. SSID:%s wifi password:%s ", user_id, user_code);
    ESP_ERROR_CHECK(from_nvs_set_value("wifi_ssid", user_id));
    ESP_ERROR_CHECK(from_nvs_set_value("wifi_pwd", user_code));
    ESP_ERROR_CHECK(from_nvs_set_value("decive_code", qcp_code));
    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    if (strcmp(user_id, "\0") != 0 && strcmp(user_code, "\0") != 0)
    {
        xSemaphoreGive(ap_sem);
        ESP_LOGI(WIFI_TAG, "set wifi name and password successfully! goto station mode");
    }

    return ESP_OK;
}
static const httpd_uri_t wifi_data = {
    .uri = "/wifi_data", // Match all URIs of type /upload/path/to/file
    .method = HTTP_POST,
    .handler = send_wifi_handler,
};

httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192 * 4; // 32KB
    config.max_open_sockets = 13;
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(WIFI_TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
        // Set URI handlers
        ESP_LOGI(WIFI_TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &wifi_data);
        httpd_register_uri_handler(server, &wifi_scan);
        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_error_handler);
    }
    return server;
}