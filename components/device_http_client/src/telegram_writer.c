#include "device_http_client.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "stdarg.h"

#include "device_common.h"
#include "stdio.h"
#include "string.h"
#include "clock_module.h"

extern QueueHandle_t telegram_queue;

static char send_buf[500];
static char *send_buf_pos = send_buf;

static void push_message();
static void message_printf(const char *format, ...);


static void message_printf(const char *format, ...)
{
    char text_buf[100];
    va_list args;
    va_start (args, format);
    vsnprintf (text_buf, sizeof(text_buf), format, args);
    va_end (args);
    size_t len = strlen(text_buf);
    if(send_buf_pos + len < send_buf + sizeof(send_buf)){
        memcpy(send_buf_pos, text_buf, len);
        send_buf_pos += len;
    }
}

static void push_message()
{
    const size_t message_size = strlen(send_buf) + 20;
    char *message = malloc(message_size);
    snprintf(message, message_size, "FloodGuard\n%s\n%s\n", snprintf_time("c"), send_buf);
    xQueueSendToBack(telegram_queue, message, 100);
    send_buf_pos = send_buf;
}

void create_telegram_message(unsigned device_state)
{
    int perc = get_voltage_perc(device_get_bat_conf());
    message_printf("%s\n", device_state&BIT_GUARD_DIS ? "test mode" : "normal mode");
    if(perc != NO_DATA){
        message_printf("battery:%d%%", perc);
    } else {
        message_printf("need configuration battery!\n");
    }
    message_printf(device_state&BIT_ALARM ? "flood!" : "ok");  
    push_message();
    device_set_state(BIT_SEND_MESSAGE);                                          
}