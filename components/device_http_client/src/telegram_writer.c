#include "device_http_client.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "stdarg.h"

#include "device_common.h"
#include "stdio.h"
#include "string.h"
#include "clock_module.h"

extern QueueHandle_t telegram_queue;

#define MAX_MESSAGE_DATA_SIZE 500
static char send_buf[MAX_MESSAGE_DATA_SIZE];
static const char* data_scope_end = send_buf + MAX_MESSAGE_DATA_SIZE;
static char* send_buf_ptr = send_buf;

static void push_message();
static void message_printf(const char *format, ...);


static void message_printf(const char *format, ...)
{
    if(send_buf_ptr >= data_scope_end) return;
    va_list args;
    va_start (args, format);
    send_buf_ptr += vsnprintf(send_buf_ptr, data_scope_end-send_buf_ptr, format, args);
    va_end (args);
}

static void push_message()
{
    const size_t message_size = strnlen(send_buf, sizeof(send_buf)) + 20;
    char *message = malloc(message_size);
    if(message){
        sprintf(message, "FloodGuard %s %s", snprintf_time("%x%X"), send_buf);
        xQueueSendToBack(telegram_queue, &message, 100);
    }
    send_buf_ptr = send_buf;
}

void create_telegram_message(unsigned device_state)
{
    if(device_state&BIT_GUARD_DIS){
        message_printf("--test mode--\n");
    }
    if(is_valid_bat_data(device_get_bat_data())){
        message_printf("bat:%d%%", get_bat_percentage(device_get_bat_data()));
    }
    if(device_state&BIT_ALARM){
        message_printf(" Alarm! Flood detected!");
    }
    if(device_state&BIT_WET_SENSOR){
        message_printf(" water detected");  
    }
    push_message();
    device_set_state(BIT_SEND_MESSAGE);                                          
}