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
static EventGroupHandle_t device_event_group;
static const char *MAIN_DATA_NAME = "main_data";

static int read_data();

int device_get_offset()
{
    return main_data.offset;
}

int device_get_delay()
{
    return main_data.delay_to_alarm_ms;
}

int device_set_offset(int offset)
{
    if(offset < -23 || offset > 23)return ESP_FAIL;
    set_offset(offset);
    main_data.offset = offset;
    changed_main_data = true;
    return ESP_OK;
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

int device_set_chat_id(const char *str)
{
    if(strnlen(str, CHAT_ID_LEN) != CHAT_ID_LEN) return ESP_FAIL;
    memcpy(main_data.chat_id, str, CHAT_ID_LEN);
    main_data.chat_id[CHAT_ID_LEN] = 0;
    changed_main_data = true;
    return ESP_OK;
}


int device_set_token(const char *str)
{
    if(strnlen(str, TOKEN_LEN+1) != TOKEN_LEN) return ESP_FAIL;
    memcpy(main_data.token, str, TOKEN_LEN);
    main_data.token[TOKEN_LEN] = 0;
    changed_main_data = true;
    return ESP_OK;
}

bool device_commit_changes()
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
    return xEventGroupGetBits(device_event_group);
} 

unsigned  device_set_state(unsigned bits)
{
    if(bits&STORED_FLAGS){
        main_data.config |= bits;
        changed_main_data = true;
    }
    return xEventGroupSetBits(device_event_group, (EventBits_t) (bits));
}

unsigned  device_clear_state(unsigned bits)
{
    if(bits&STORED_FLAGS){
        main_data.config &= ~bits;
        changed_main_data = true;
    }
    return xEventGroupClearBits(device_event_group, (EventBits_t) (bits));
}

unsigned device_wait_bits_untile(unsigned bits, unsigned time_ticks)
{
    return xEventGroupWaitBits(device_event_group,
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

bool is_valid_bat_data()
{
    return main_data.bat_data.max_mVolt < MAX_BAT_mV
            && main_data.bat_data.min_mVolt > MIN_BAT_mV
            && main_data.bat_data.volt_koef != 0;
}

bat_data_t * device_get_bat_data()
{
    return &main_data.bat_data;
}

int device_set_bat_data(int min_volt, int max_volt, int real_volt)
{
    if(min_volt > 2000 && max_volt < 28000 && real_volt > min_volt){
        main_data.bat_data.min_mVolt = min_volt;
        main_data.bat_data.max_mVolt = max_volt;
        set_coef_val(&main_data.bat_data, real_volt);
        changed_main_data = true;
        return ESP_OK;
    }
    return ESP_FAIL;
}

int device_set_delay(unsigned delay_to_alarm_ms)
{
    if(delay_to_alarm_ms <= MAX_ALARM_DELAY_MS){
        main_data.delay_to_alarm_ms = delay_to_alarm_ms;
        changed_main_data = true;
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
    device_event_group = xEventGroupCreate();
    init_nvs();
    device_gpio_out_init();
    read_data();
    wifi_init();
}


void device_set_state_isr(unsigned bits)
{
    BaseType_t pxHigherPriorityTaskWoken;
    xEventGroupSetBitsFromISR(device_event_group, (EventBits_t)bits, &pxHigherPriorityTaskWoken);
    portYIELD_FROM_ISR( pxHigherPriorityTaskWoken );
}

void  device_clear_state_isr(unsigned bits)
{
    xEventGroupClearBitsFromISR(device_event_group, (EventBits_t)bits);
}
