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


#include "esp_err.h"
#include "esp_log.h"


static settings_data_t main_data = {0};
char network_buf[NET_BUF_LEN];

static bool changed_main_data;
static EventGroupHandle_t device_event_gr;
static const char *MAIN_DATA_NAME = "main_data";

static int read_data();

int device_get_offset()
{
    return main_data.offset;
}

int device_get_delay()
{
    return main_data.delay_to_alarm_sec;
}
void device_set_offset(int offset)
{
    set_offset(offset);
    main_data.offset = offset;
    changed_main_data = true;
}

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

void device_set_placename(const char *str)
{
    const int len = strnlen(str, MAX_STR_LEN);
    memcpy(main_data.place_name, str, len);
    main_data.place_name[len] = 0;
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
        main_data.token[TOKEN_LEN] = 0;
        changed_main_data = true;
    }
}

int device_commit_changes()
{
    if(changed_main_data){
        CHECK_AND_RET_ERR(write_flash(MAIN_DATA_NAME, (uint8_t *)&main_data, sizeof(main_data)));
        changed_main_data = false;
        return true;
    }
    return false;
}

unsigned device_get_state()
{
    return xEventGroupGetBits(device_event_gr);
} 

unsigned  device_set_state(unsigned bits)
{
    if(bits&STORED_FLAGS){
        main_data.config |= bits;
        changed_main_data = true;
    }
    return xEventGroupSetBits(device_event_gr, (EventBits_t) (bits));
}

unsigned  device_clear_state(unsigned bits)
{
    if(bits&STORED_FLAGS){
        main_data.config &= ~bits;
        changed_main_data = true;
    }
    return xEventGroupClearBits(device_event_gr, (EventBits_t) (bits));
}

unsigned device_wait_bits_untile(unsigned bits, unsigned time_ticks)
{
    return xEventGroupWaitBits(device_event_gr,
                                (EventBits_t) (bits),
                                pdFALSE,
                                pdFALSE,
                                time_ticks);
}

const char *  device_get_ssid()
{
    return main_data.ssid;
}
const char *  device_get_pwd()
{
    return main_data.pwd;
}
const char *  device_get_token()
{
    return main_data.token;
}
const char *  device_get_chat_id()
{
    return main_data.chat_id;
}

bat_conf_t * device_get_bat_conf()
{
    return &main_data.bat_conf;
}

int device_set_bat_conf(unsigned min_volt, unsigned max_volt, unsigned real_volt)
{
    if(min_volt > 2000 && max_volt < 28000){
        main_data.bat_conf.min_mVolt = min_volt;
        main_data.bat_conf.max_mVolt = max_volt;
        set_coef_val(&main_data.bat_conf, real_volt);
        changed_main_data = true;
        return ESP_OK;
    }
    return ESP_FAIL;
}

int device_set_delay(unsigned delay_to_alarm_sec)
{
    if(delay_to_alarm_sec < 180){
        main_data.delay_to_alarm_sec = delay_to_alarm_sec;
        return ESP_OK;
    }
    return ESP_FAIL;
}

const char * device_get_placename()
{
    return main_data.place_name;
}


static int read_data()
{
    CHECK_AND_RET_ERR(read_flash(MAIN_DATA_NAME, (unsigned char *)&main_data, sizeof(main_data)));
    device_set_state(main_data.config&STORED_FLAGS);
    return ESP_OK;
}

void device_init()
{
    device_event_gr = xEventGroupCreate();
    init_nvs();
    device_gpio_out_init();
    read_data();
    wifi_init();
}


void device_set_state_isr(unsigned bits)
{
    BaseType_t pxHigherPriorityTaskWoken;
    xEventGroupSetBitsFromISR(device_event_gr, (EventBits_t)bits, &pxHigherPriorityTaskWoken);
    portYIELD_FROM_ISR( pxHigherPriorityTaskWoken );
}

void  device_clear_state_isr(unsigned bits)
{
    xEventGroupClearBitsFromISR(device_event_gr, (EventBits_t)bits);
}
