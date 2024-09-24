#include "device_http_client.h"

#include "esp_http_client.h"
#include "device_macro.h"
#include "device_common.h"

#define SIZE_URL_BUF 500
#define MAX_KEY_NUM 10

extern char network_buf[];
static char url[SIZE_URL_BUF];

static size_t get_value_ptrs(char ***value_list, char *data_buf, const size_t buf_len, const char *key)
{
    char *buf_oper[MAX_KEY_NUM] = { 0 };
    if(data_buf == NULL) 
        return 0;
    const size_t key_size = strlen(key);
    char *data_ptr = data_buf;
    char **buf_ptr = buf_oper;
    const char *data_end = data_buf+buf_len;
    size_t value_list_size = 0;
    while(data_ptr = strstr(data_ptr, key), 
            data_ptr 
            && data_end > data_ptr 
            && MAX_KEY_NUM > value_list_size ){
        data_ptr += key_size;
        buf_ptr[value_list_size++] = data_ptr;
    }
    const size_t data_size = value_list_size * sizeof(char *);
    *value_list = (char **)malloc(data_size);
    if(value_list == NULL)
        return 0;
    memcpy(*value_list, buf_oper, data_size);
    return value_list_size;
}

static void split(char *data_buf, size_t data_size, const char *split_chars_str)
{
    char *ptr = data_buf;
    const char *end_data_buf = data_buf + data_size;
    const size_t symb_list_size = strlen(split_chars_str);
    char c;
    while(ptr != end_data_buf){
        c = *(ptr);
        for(int i=0; i<symb_list_size; ++i){
            if(split_chars_str[i] == c){
                *(ptr) = 0;
                break;
            }
        }
        ++ptr;
    }
}


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


int device_update_time()
{
    char ** unixtime = NULL;
    esp_http_client_config_t config = {
        .url = "http://worldtimeapi.org/api/timezone/Etc/UTC",  
        .event_handler = http_event_handler,
        .user_data = (void*)network_buf,    
        .method = HTTP_METHOD_GET,
        .buffer_size = NET_BUF_LEN,
        .auth_type = HTTP_AUTH_TYPE_NONE,
        .skip_cert_common_name_check = true                  
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);
    if(err == ESP_OK){
        const size_t data_size = esp_http_client_get_content_length(client);
        if(data_size){
            network_buf[data_size] = 0;
            get_value_ptrs(&unixtime, network_buf, data_size, "\"unixtime\":");
            if(unixtime){
                split(unixtime[0], data_size, ",");
                struct timeval tv = {
                    .tv_sec = atol(unixtime[0]) 
                };
                settimeofday(&tv, NULL);
                free(unixtime);
                unixtime = NULL;
            }
        }
    }
    esp_http_client_cleanup(client);
    return err;
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

