#ifndef DEVICE_SYSTEM_H
#define DEVICE_SYSTEM_H


#ifdef __cplusplus
extern "C" {
#endif

#include "stdbool.h"
#include "time.h"
#include "adc_reader.h"

enum DataConst{
    NO_DATA = -1,
    ACT_MODE_1,
    ACT_MODE_2,
};

enum BasicConst{
    WEEK_DAYS_NUM           = 7,
    MAX_STR_LEN             = 32,
    TOKEN_LEN               = 46,
    CHAT_ID_LEN             = 9,
    MAX_ALARM_DELAY_MS      = 60*1000,
    DESCRIPTION_SIZE        = 20,
    FORECAST_LIST_SIZE      = 5,
    NET_BUF_LEN             = 5000,
    MAX_DELAY_START_SEC     = 240,
    
};

enum Bits{
    BIT_LIMESCALE_PREVENTION    = (1<<0),
    BIT_GUARD_DIS               = (1<<1), 
    BIT_INFO_NOTIFACTION_EN     = (1<<2),
    BIT_NOTIFICATION_DIS        = (1<<3),
    BIT_WET_SENSOR              = (1<<4),
    BIT_ALARM                   = (1<<5), 
    BIT_ERR_STA_CONF            = (1<<6),       
    BIT_ERR_TELEGRAM_SEND       = (1<<7), 
    BIT_ERR_SSID_NOT_FOUND      = (1<<8), 
    BIT_IS_AP_CONNECTION        = (1<<9),
    BIT_IS_STA_CONNECTION       = (1<<10),
    BIT_START_SERVER            = (1<<11),
    BIT_SEND_MESSAGE            = (1<<12),
    BIT_IS_LOW_BAT              = (1<<13),
    BIT_ALARM_SOUND             = (1<<14),
    BIT_WAIT_BUT_INPUT          = (1<<15),
    BIT_WAIT_SIGNALE            = (1<<18),
    BIT_SERVER_RUN              = (1<<16),
    BIT_UPDATE_TIME             = (1<<17),
    BIT_IS_TIME                 = (1<<18),
    BIT_IS_AP_CLIENT            = (1<<19),
    BIT_WAIT_SANDING            = (1<<20),
    BIT_CHANGE_SENSOR_STATE     = (1<<20),
    
    STORED_FLAGS                = (BIT_GUARD_DIS|BIT_INFO_NOTIFACTION_EN|BIT_NOTIFICATION_DIS|BIT_LIMESCALE_PREVENTION),
    BITS_DENIED_SLEEP           = (BIT_START_SERVER|BIT_WAIT_SIGNALE|BIT_SEND_MESSAGE|BIT_WAIT_SANDING),
};

typedef struct {
    char ssid[MAX_STR_LEN+1];
    char pwd[MAX_STR_LEN+1];
    char chat_id[MAX_STR_LEN+1];
    char place_name[MAX_STR_LEN+1];
    char token[TOKEN_LEN+1];
    unsigned config;
    int offset;
    unsigned delay_to_alarm_ms;
    bat_conf_t bat_conf;
} settings_data_t;


// --------------------------------------- GPIO
typedef struct{
    bool pressed;
    int pin_num;
    unsigned start_state_time;
}inp_conf_t;

void inp_init(inp_conf_t * conf, int pin_num);
int get_but_state(inp_conf_t * button, unsigned cur_time);
void device_gpio_out_init(void);
int get_inp_state(inp_conf_t * conf, unsigned cur_time, unsigned delay_time);

#define PIN_BUT_RIGHT              19
#define PIN_BUT_LEFT               18      
#define PIN_OUT_LED_ALARM          5
#define PIN_OUT_LED_OK             16
#define PIN_OUT_SIG                4
#define PIN_OUT_SERVO              2
#define PIN_OUT_POWER              17
#define PIN_IN_SENSOR              23

// --------------------------------------- common
int device_set_offset(int time_offset);
void device_set_pwd(const char *str);
void device_set_ssid(const char *str);
void device_set_placename(const char *str);
int device_set_chat_id(const char *str);
int device_set_token(const char *str);
bool device_commit_changes();
unsigned device_get_state();
unsigned  device_set_state(unsigned bits);
unsigned  device_clear_state(unsigned bits);
unsigned device_wait_bits_untile(unsigned bits, unsigned time_ticks);
const char  * device_get_ssid();
const char  * device_get_pwd();
const char  * device_get_token();
const char  * device_get_chat_id();
bat_conf_t * device_get_bat_conf();
int device_set_bat_conf(unsigned min_volt, unsigned max_volt, unsigned real_volt);
int device_set_delay(unsigned delay_to_alarm_ms);
const char * device_get_placename();
void device_init();
void device_set_state_isr(unsigned bits);
void  device_clear_state_isr(unsigned bits);
int device_get_offset();
int device_get_delay();
bool is_valid_bat_conf();

#define device_wait_bits(bits) \
    device_wait_bits_untile(bits, 12000/portTICK_PERIOD_MS)
    
#define get_notif_size(schema) \
    (get_notif_num(schema)*sizeof(unsigned))



extern char network_buf[];






#ifdef __cplusplus
}
#endif



#endif