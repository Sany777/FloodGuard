#include "device_common.h"







int get_button() 
{
    if(gpio_get_level(PIN_IN_BUT_RESET))return PIN_IN_BUT_RESET;
    if(gpio_get_level(PIN_IN_BUT_START_SERVER))return PIN_IN_BUT_START_SERVER;
    return NO_DATA;
}

void device_gpio_init() 
{
    gpio_config_t in_conf = {
        .pin_bit_mask = (1ULL << PIN_IN_BUT_RESET) | (1ULL << PIN_IN_BUT_START_SERVER),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&in_conf);

    gpio_config_t out_conf = {
        .pin_bit_mask = (1ULL << PIN_OUT_LED_ALARM) | (1ULL << PIN_IN_BUT_START_SERVER),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&out_conf);
}

int IRAM_ATTR device_set_pin(int pin, unsigned state) 
{
    gpio_set_direction((gpio_num_t)pin, GPIO_MODE_INPUT_OUTPUT);
    return gpio_set_level((gpio_num_t)pin, state);
}
