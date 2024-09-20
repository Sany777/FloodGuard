#include "device_common.h"


#include "stdlib.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "semaphore.h"
#include "string.h"
#include "portmacro.h"

#include "clock_module.h"
#include "device_macro.h"
#include "wifi_service.h"
#include "device_memory.h"



#include "esp_log.h"


static settings_data_t main_data = {0};
char network_buf[NET_BUF_LEN];

static bool changed_main_data;
static EventGroupHandle_t clock_event_group;
static const char *MAIN_DATA_NAME = "main_data";
static const char *NOTIFY_DATA_NAME = "notify_data";

static int read_data();








void device_set_pwd(const char *str)
{
    const int len = strnlen(str, MAX_STR_LEN);
    memcpy(main_data.pwd, str, len);
    main_data.pwd[len] = 0;
    changed_main_data = true;
}

void device_set_ssid(const char *str)
{
    const int len = strnlen(str, MAX_STR_LEN);
    memcpy(main_data.ssid, str, len);
    main_data.ssid[len] = 0;
    changed_main_data = true;
}

void device_set_chat_id(const char *str)
{
    const int len = strnlen(str, MAX_STR_LEN);
    memcpy(main_data.chat_id, str, len);
    main_data.chat_id[len] = 0;
    changed_main_data = true;
}

void device_set_token(const char *str)
{
    if(strnlen(str, TOKEN_LEN+1) == TOKEN_LEN){
        memcpy(main_data.token, str, TOKEN_LEN);
        changed_main_data = true;
        main_data.token[TOKEN_LEN] = 0;
    }
}

int device_commit_changes()
{
    if(changed_main_data){
        CHECK_AND_RET_ERR(write_flash(MAIN_DATA_NAME, (uint8_t *)&main_data, sizeof(main_data)));
        changed_main_data = false;
    }
    return ESP_OK;
}

unsigned device_get_state()
{
    return xEventGroupGetBits(clock_event_group);
} 

unsigned  device_set_state(unsigned bits)
{
    if(bits&STORED_FLAGS){
        main_data.flags |= bits;
        changed_main_data = true;
    }
    return xEventGroupSetBits(clock_event_group, (EventBits_t) (bits));
}

unsigned  device_clear_state(unsigned bits)
{
    if(bits&STORED_FLAGS){
        main_data.flags &= ~bits;
        changed_main_data = true;
    }
    return xEventGroupClearBits(clock_event_group, (EventBits_t) (bits));
}

unsigned device_wait_bits_untile(unsigned bits, unsigned time_ticks)
{
    return xEventGroupWaitBits(clock_event_group,
                                (EventBits_t) (bits),
                                pdFALSE,
                                pdFALSE,
                                time_ticks);
}



char *  device_get_ssid()
{
    return main_data.ssid;
}
char *  device_get_pwd()
{
    return main_data.pwd;
}
char *  device_get_token()
{
    return main_data.token;
}
char *  device_get_chat_id()
{
    return main_data.chat_id;
}



static int read_data()
{
    CHECK_AND_RET_ERR(read_flash(MAIN_DATA_NAME, (unsigned char *)&main_data, sizeof(main_data)));
    device_set_state(main_data.flags&STORED_FLAGS);
    return ESP_OK;
}



void device_init()
{
    clock_event_group = xEventGroupCreate();
    init_nvs();
    device_gpio_init();
    read_data();
    wifi_init();
}


void device_set_state_isr(unsigned bits)
{
    BaseType_t pxHigherPriorityTaskWoken;
    xEventGroupSetBitsFromISR(clock_event_group, (EventBits_t)bits, &pxHigherPriorityTaskWoken);
    portYIELD_FROM_ISR( pxHigherPriorityTaskWoken );
}

void  device_clear_state_isr(unsigned bits)
{
    xEventGroupClearBitsFromISR(clock_event_group, (EventBits_t)bits);
}
