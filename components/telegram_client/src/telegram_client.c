#include "telegram_client.h"

#include "esp_http_client.h"
#include "device_macro.h"
#include "device_common.h"

#define SIZE_URL_BUF 500
#define MAX_KEY_NUM 10

extern char network_buf[];
static char url[SIZE_URL_BUF];



static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer; 
    static int output_len;      
    switch(evt->event_id) {
    case HTTP_EVENT_ON_DATA:
    {
        if(!esp_http_client_is_chunked_response(evt->client)){
            int copy_len = 0;
            if(evt->user_data){
                copy_len = MIN(evt->data_len, (NET_BUF_LEN - output_len));
                if (copy_len) {
                    memcpy(evt->user_data + output_len, evt->data, copy_len);
                }
            } else {
                const int buffer_len = esp_http_client_get_content_length(evt->client);
                if (output_buffer == NULL) {
                    output_buffer = (char *) malloc(buffer_len);
                    output_len = 0;
                    if (output_buffer == NULL) {
                        return ESP_FAIL;
                    }
                }
                copy_len = MIN(evt->data_len, (buffer_len - output_len));
                if (copy_len) {
                    memcpy(output_buffer + output_len, evt->data, copy_len);
                }
            }
            output_len += copy_len;
        }
        break;
    }
    case HTTP_EVENT_ON_FINISH:
    {
        if(output_buffer != NULL){
            free(output_buffer);
            output_buffer = NULL;
        }
        output_len = 0;
        break;
    }
    case HTTP_EVENT_DISCONNECTED:
    {
        if (output_buffer != NULL){
            free(output_buffer);
            output_buffer = NULL;
        }
        output_len = 0;
        break;
    }
    default:break;
    }
    return ESP_OK;
}



int send_telegram_message(const char *token, const char *chat_id, const char *message) 
{
    snprintf(url, sizeof(url), "https://api.telegram.org/bot%s/sendMessage?chat_id=%s&text=%s",
                token, chat_id, message);

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .user_data = (void*)network_buf,    
        .method = HTTP_METHOD_GET,
        .buffer_size = NET_BUF_LEN,
        .auth_type = HTTP_AUTH_TYPE_NONE,
        .skip_cert_common_name_check = true
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);
    esp_http_client_cleanup(client);
    return err;
}

