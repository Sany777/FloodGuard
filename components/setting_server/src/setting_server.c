#include "setting_server.h"

#include <sys/stat.h>
#include <dirent.h>
#include "cJSON.h"
#include "stdbool.h"

#include "esp_http_server.h"
#include "esp_chip_info.h"
#include "string.h"
#include "portmacro.h"
#include "clock_module.h"
#include "string.h"
#include "device_macro.h"
#include "wifi_service.h"
#include "device_common.h"
#include "adc_reader.h"


static httpd_handle_t server;

static const char *MES_DATA_NOT_READ = "Data not read";
static const char *MES_DATA_TOO_LONG = "Data too long";
static const char *MES_NO_MEMORY = "No memory";
static const char *MES_BAD_DATA_FOMAT = "wrong data format";
static const char *MES_SUCCESSFUL = "Successful";



#define SEND_REQ_ERR(_req_, _str_) \
    do{ httpd_resp_send_err((_req_), HTTPD_400_BAD_REQUEST, (_str_));}while(0)

#define SEND_SERVER_ERR(_req_, _str_) \
    do{ httpd_resp_send_err((_req_), HTTPD_500_INTERNAL_SERVER_ERROR, (_str_));}while(0)


static cJSON * get_json_data(httpd_req_t *req)
{
    int received,
        total_len = req->content_len;
    char * server_buf = (char *)req->user_ctx;
    if(total_len >= NET_BUF_LEN){
        SEND_REQ_ERR(req, MES_DATA_TOO_LONG);
        return NULL;
    }
    received = httpd_req_recv(req, server_buf, total_len);
    if (received != total_len) {
        SEND_SERVER_ERR(req, MES_DATA_NOT_READ);
        return NULL;
    }
    server_buf[received] = 0;
    return cJSON_Parse(server_buf);
}

static int get_num_data(httpd_req_t *req, int *val)
{
    int received,
        total_len = req->content_len;
    char * server_buf = (char *)req->user_ctx;
    if(total_len >= NET_BUF_LEN){
        SEND_REQ_ERR(req, MES_DATA_TOO_LONG);
        return ESP_FAIL;
    }
    received = httpd_req_recv(req, server_buf, total_len);
    if (received != total_len) {
       SEND_SERVER_ERR(req, MES_DATA_NOT_READ);
       return ESP_FAIL;
    }
    server_buf[total_len] = 0;
    *val = atoi(server_buf);
    return ESP_OK;
}


void server_stop()
{
    device_clear_state(BIT_SERVER_RUN);
}

static esp_err_t index_redirect_handler(httpd_req_t *req)
{
    httpd_resp_set_status(req, "307 Temporary Redirect");
    httpd_resp_set_hdr(req, "Location", "/index.html");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t get_index_handler(httpd_req_t *req)
{
    extern const unsigned char index_html_start[] asm( "_binary_index_html_start" );
    extern const unsigned char index_html_end[] asm( "_binary_index_html_end" );
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start); 
    return ESP_OK;
}

static esp_err_t get_css_handler(httpd_req_t *req)
{
    extern const unsigned char style_css_start[] asm( "_binary_style_css_start" );
    extern const unsigned char style_css_end[] asm( "_binary_style_css_end" );
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, (const char *)style_css_start, style_css_end - style_css_start ); 
    return ESP_OK;
}

static esp_err_t get_js_handler(httpd_req_t *req)
{
    extern const unsigned char script_js_start[] asm( "_binary_script_js_start" );
    extern const unsigned char script_js_end[] asm( "_binary_script_js_end" );
    httpd_resp_set_type(req, "text/javascript");
    httpd_resp_send(req, (const char *)script_js_start, script_js_end - script_js_start ); 
    return ESP_OK;
}


static esp_err_t handler_close(httpd_req_t *req)
{
    httpd_resp_sendstr(req, "Goodbay!");
    vTaskDelay(500/portTICK_PERIOD_MS);
    device_clear_state(BIT_SERVER_RUN);
    return ESP_OK;
}


static esp_err_t set_wifi_conf_handler(httpd_req_t *req)
{
    cJSON *root, *ssid_name_j, *pwd_wifi_j;
    root = get_json_data(req);
    if(!root){
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, MES_NO_MEMORY);
        return ESP_FAIL;
    }

    ssid_name_j = cJSON_GetObjectItemCaseSensitive(root, "SSID");
    pwd_wifi_j = cJSON_GetObjectItemCaseSensitive(root, "Password");
    
    if(cJSON_IsString(ssid_name_j) && (ssid_name_j->valuestring != NULL)){
        device_set_ssid(ssid_name_j->valuestring);
    }
    if(cJSON_IsString(pwd_wifi_j) && (pwd_wifi_j->valuestring != NULL)){
        device_set_pwd(pwd_wifi_j->valuestring);
    }
    cJSON_Delete(root);
    httpd_resp_sendstr(req, MES_SUCCESSFUL);
    return ESP_OK;
}



static esp_err_t set_baterry_conf_handler(httpd_req_t *req)
{
    cJSON *root, *min_mv_j, *max_mv_j, *real_volt_j;
    root = get_json_data(req);
    if(!root){
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, MES_NO_MEMORY);
        return ESP_FAIL;
    }

    min_mv_j = cJSON_GetObjectItemCaseSensitive(root, "Min voltage, mV");
    max_mv_j = cJSON_GetObjectItemCaseSensitive(root, "Max voltage, mV");
    real_volt_j = cJSON_GetObjectItemCaseSensitive(root, "Real voltage, mV");

    if(cJSON_IsNumber(min_mv_j) && cJSON_IsNumber(max_mv_j) && cJSON_IsNumber(real_volt_j)
    && device_set_bat_conf(min_mv_j->valueint, max_mv_j->valueint, real_volt_j->valueint)){
        httpd_resp_sendstr(req, MES_SUCCESSFUL);
    } else {
        SEND_REQ_ERR(req, MES_BAD_DATA_FOMAT);
    }
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t set_telegram_data_handler(httpd_req_t *req)
{
    cJSON *root, *chat_id_j, *token_j;
    root = get_json_data(req);
    if(!root){
        SEND_SERVER_ERR(req, MES_NO_MEMORY);
    }
    chat_id_j = cJSON_GetObjectItemCaseSensitive(root, "ChatID");
    token_j = cJSON_GetObjectItemCaseSensitive(root, "Token");
    if(cJSON_IsString(chat_id_j) && (chat_id_j->valuestring != NULL)){
        device_set_chat_id(chat_id_j->valuestring);
    }
    if(cJSON_IsString(token_j) && (token_j->valuestring != NULL)){
        device_set_token(token_j->valuestring);
    }
    cJSON_Delete(root);
    httpd_resp_sendstr(req, MES_SUCCESSFUL);
    return ESP_OK;
}

static const char *get_chip(int model_id)
{
    switch(model_id){
        case 1: return "ESP32";
        case 2: return "ESP32-S2";
        case 3: return "ESP32-S3";
        case 5: return "ESP32-C3";
        case 6: return "ESP32-H2";
        case 12: return "ESP32-C2";
        default: break;
    }
    return "uknown";
}

static esp_err_t get_info_handler(httpd_req_t *req)
{
    char * server_buf = (char *)req->user_ctx;
    bat_conf_t *bat_conf = device_get_bat_conf();
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    snprintf(server_buf, 200, "chip: %s\nvoltage: %u mV\nbaterry: %u%%\n%s",
                get_chip(chip_info.model), 
                get_voltage_mv(bat_conf),
                get_voltage_perc(bat_conf),
                snprintf_time("%c")
            );
    httpd_resp_sendstr(req, server_buf);
    return ESP_OK;
}


static esp_err_t get_settings_handler(httpd_req_t *req)
{
    cJSON * root = cJSON_CreateObject();
    bat_conf_t *bat_conf = device_get_bat_conf();
    if(root){
        cJSON_AddStringToObject(root, "SSID", device_get_ssid());
        cJSON_AddStringToObject(root, "Password", device_get_pwd());
        cJSON_AddStringToObject(root, "Token", device_get_token());
        cJSON_AddStringToObject(root, "Name ", device_get_placename());
        cJSON_AddStringToObject(root, "ChatID", device_get_chat_id());
        cJSON_AddNumberToObject(root, "Sec", device_get_delay());
        cJSON_AddNumberToObject(root, "Offset", device_get_offset());
        cJSON_AddNumberToObject(root, "Status", device_get_state());
        cJSON_AddNumberToObject(root, "Real voltage, mV", get_voltage_mv(bat_conf));
        cJSON_AddNumberToObject(root, "Min voltage, mV", bat_conf->min_mVolt);
        cJSON_AddNumberToObject(root, "Max voltage, mV", bat_conf->max_mVolt);
        char *data_to_send = cJSON_Print(root);
        cJSON_Delete(root);
        if(data_to_send){
            httpd_resp_set_type(req, "application/json");
            httpd_resp_sendstr(req, data_to_send);
            free(data_to_send);
            data_to_send = NULL;
            return ESP_OK;
        }
    }
    SEND_SERVER_ERR(req, MES_NO_MEMORY);
    return ESP_OK;
}

	

static esp_err_t set_flag_handler(httpd_req_t *req)
{
    int flags;
    if(get_num_data(req, &flags) != ESP_OK) return ESP_FAIL;
    device_set_state(flags&STORED_FLAGS);
    httpd_resp_sendstr(req, "Successful installation of config");
    return ESP_OK;
}

static esp_err_t set_delay_handler(httpd_req_t *req)
{
    int delay_to_alarm;
    cJSON *root, *delay_to_alarm_j;
    root = get_json_data(req);
    if(!root){
        SEND_SERVER_ERR(req, MES_NO_MEMORY);
        return ESP_FAIL;
    }

    delay_to_alarm_j = cJSON_GetObjectItemCaseSensitive(root, "Sec");

    if(cJSON_IsNumber(delay_to_alarm_j)){
        delay_to_alarm = delay_to_alarm_j->valueint;
        if(device_set_delay(delay_to_alarm)){
            device_set_delay(delay_to_alarm);
            httpd_resp_sendstr(req, MES_SUCCESSFUL);
            cJSON_Delete(root);
            return ESP_OK;
        }
    }
    cJSON_Delete(root);
    SEND_REQ_ERR(req, MES_BAD_DATA_FOMAT);
    return ESP_OK;
}

static esp_err_t set_offset_handler(httpd_req_t *req)
{
    int offset;
    cJSON *root, *offset_j;
    root = get_json_data(req);
    if(!root){
        SEND_SERVER_ERR(req, MES_NO_MEMORY);
        return ESP_FAIL;
    }

    offset_j = cJSON_GetObjectItemCaseSensitive(root, "Offset");
    
    if(cJSON_IsNumber(offset_j)){
        offset = offset_j->valueint;
        if(offset < 24 && offset > -24){
            device_set_offset(offset);
            httpd_resp_sendstr(req, MES_SUCCESSFUL);
            cJSON_Delete(root);
            return ESP_OK;
        }
    }
    cJSON_Delete(root);
    SEND_REQ_ERR(req, MES_BAD_DATA_FOMAT);
    return ESP_OK;
}



int deinit_server()
{
    esp_err_t err = ESP_FAIL;
    if(server != NULL){
        err = httpd_stop(server);
        vTaskDelay(1000/portTICK_PERIOD_MS);
        server = NULL;
    }
    return err;
}


int init_server(char *server_buf)
{
    if(server != NULL) return ESP_FAIL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 14;
    config.uri_match_fn = httpd_uri_match_wildcard;

    if(httpd_start(&server, &config) != ESP_OK){
        return ESP_FAIL;
    }
    
    httpd_uri_t set_flags = {
        .uri      = "/Status",
        .method   = HTTP_POST,
        .handler  = set_flag_handler,
        .user_ctx = server_buf
    };
    httpd_register_uri_handler(server, &set_flags);

    httpd_uri_t get_info = {
        .uri      = "/info?",
        .method   = HTTP_POST,
        .handler  = get_info_handler,
        .user_ctx = server_buf
    };
    httpd_register_uri_handler(server, &get_info);

    httpd_uri_t get_setting = {
        .uri      = "/settings?",
        .method   = HTTP_POST,
        .handler  = get_settings_handler,
        .user_ctx = server_buf
    };
    httpd_register_uri_handler(server, &get_setting);
    
     httpd_uri_t close_uri = {
        .uri      = "/close",
        .method   = HTTP_POST,
        .handler  = handler_close,
        .user_ctx = server_buf
    };
    httpd_register_uri_handler(server, &close_uri);

     httpd_uri_t wifi_uri = {
        .uri      = "/WiFi",
        .method   = HTTP_POST,
        .handler  = set_wifi_conf_handler,
        .user_ctx = server_buf
    };
    httpd_register_uri_handler(server, &wifi_uri);

    httpd_uri_t api_uri = {
        .uri      = "/Telegram",
        .method   = HTTP_POST,
        .handler  = set_telegram_data_handler,
        .user_ctx = server_buf
    };
    httpd_register_uri_handler(server, &api_uri);

     httpd_uri_t index_uri = {
        .uri      = "/index.html",
        .method   = HTTP_GET,
        .handler  = get_index_handler ,
        .user_ctx = server_buf
    };
    httpd_register_uri_handler(server, &index_uri);

     httpd_uri_t get_style = {
        .uri      = "/style.css",
        .method   = HTTP_GET,
        .handler  = get_css_handler,
        .user_ctx = server_buf
    };
    httpd_register_uri_handler(server, &get_style);

     httpd_uri_t get_script = {
        .uri      = "/script.js",
        .method   = HTTP_GET,
        .handler  = get_js_handler,
        .user_ctx = server_buf
    };
    httpd_register_uri_handler(server, &get_script);

     httpd_uri_t redir_uri = {
        .uri      = "/*",
        .method   = HTTP_GET,
        .handler  = index_redirect_handler,
        .user_ctx = server_buf
    };
    httpd_register_uri_handler(server, &redir_uri);

    httpd_uri_t set_delay_uri = {
        .uri      = "/Delay start",
        .method   = HTTP_POST,
        .handler  = set_delay_handler,
        .user_ctx = server_buf
    };
    httpd_register_uri_handler(server, &set_delay_uri);

    httpd_uri_t set_baterry_uri = {
        .uri      = "/Battery settings",
        .method   = HTTP_POST,
        .handler  = set_baterry_conf_handler,
        .user_ctx = server_buf
    };
    httpd_register_uri_handler(server, &set_baterry_uri);

        httpd_uri_t set_telegram_uri = {
        .uri      = "/Telegram",
        .method   = HTTP_POST,
        .handler  = set_telegram_data_handler,
        .user_ctx = server_buf
    };
    httpd_register_uri_handler(server, &set_telegram_uri);
    httpd_uri_t set_offset_uri = {
        .uri      = "/Offset",
        .method   = HTTP_POST,
        .handler  = set_offset_handler,
        .user_ctx = server_buf
    };
    httpd_register_uri_handler(server, &set_offset_uri);

    return ESP_OK;
}


