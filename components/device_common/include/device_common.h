#ifndef DEVICE_SYSTEM_H
#define DEVICE_SYSTEM_H


#ifdef __cplusplus
extern "C" {
#endif

#include "stdbool.h"
#include "time.h"
#include "driver/gpio.h"


enum BasicConst{
    NO_DATA = -1,
    WEEK_DAYS_NUM           = 7,
    MAX_STR_LEN             = 32,
    TOKEN_LEN               = 50,
    FORBIDDED_NOTIF_HOUR    = 6*60,
    DESCRIPTION_SIZE        = 20,
    FORECAST_LIST_SIZE      = 5,
    NET_BUF_LEN             = 5000,
};

enum Bits{
    BIT_OFF                     = (1<<0),
    BIT_STA_CONF_OK             = (1<<1),
    BIT_TELEGRAM_OK             = (1<<2),
    BIT_ALARM                   = (1<<3),
    BIT_ERR_SSID_NOT_FOUND      = (1<<4),
    
    BIT_START_SERVER            = (1<<11),
    BIT_SEND_MESSAGE            = (1<<12),
    BIT_IS_LOW_BAT              = (1<<13),
    BIT_SERVER_BUT_PRESSED      = (1<<14),
    BIT_WAIT_BUT_INPUT          = (1<<15),
    BIT_WAIT_SIGNALE            = (1<<18),
    BIT_BUT_SERVER              = (1<<19),
    BIT_BUT_START               = (1<<19),
    BIT_CHECK_BAT               = (1<<22),

    STORED_FLAGS                = (BIT_OFF),
    BITS_DENIED_SLEEP           = (BIT_WAIT_BUT_INPUT|BIT_START_SERVER),
    // BITS_NEW_BUT_DATA           = (BIT_BUT_PRESSED|BIT_BUT_LONG_PRESSED|BIT_ENCODER_ROTATE)
};

typedef struct {
    char ssid[MAX_STR_LEN+1];
    char pwd[MAX_STR_LEN+1];
    char chat_id[MAX_STR_LEN+1];
    char token[TOKEN_LEN+1];
    unsigned flags;
} settings_data_t;


// --------------------------------------- GPIO
void device_gpio_init(void);
int device_set_pin(int pin, unsigned state);
int get_but_state();

#define PIN_IN_BUT_RESET           GPIO_NUM_2
#define PIN_IN_BUT_START_SERVER    GPIO_NUM_3
#define PIN_OUT_LED_ALARM          GPIO_NUM_10
#define PIN_OUT_LED_OK             GPIO_NUM_6
#define PIN_IN_ADC_ADJUSTMENT      GPIO_NUM_7
#define PIN_IN_ADC_VOLTAGE         GPIO_NUM_11
#define PIN_OUT_SIG_OUT            GPIO_NUM_5 


// --------------------------------------- common

void device_set_pwd(const char *str);
void device_set_ssid(const char *str);
void device_set_chat_id(const char *str);
void device_set_token(const char *str);
char *device_get_ssid();
char *device_get_pwd();
char *device_get_token();
char *device_get_chat_id();
int device_commit_changes();
unsigned device_get_state();
unsigned device_wait_bits_untile(unsigned bits, unsigned time_ms);
unsigned device_clear_state(unsigned bits);
void device_set_state_isr(unsigned bits);
void device_clear_state_isr(unsigned bits);
unsigned device_set_state(unsigned bits);
void device_init();
int get_button();
int device_set_pin(int pin, unsigned state);


#define device_wait_bits(bits) \
    device_wait_bits_untile(bits, 12000/portTICK_PERIOD_MS)
    
#define get_notif_size(schema) \
    (get_notif_num(schema)*sizeof(unsigned))



extern char network_buf[];






#ifdef __cplusplus
}
#endif



#endif