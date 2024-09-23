#include "device_common.h"

#include "driver/gpio.h"
#include "esp_sleep.h"
#include "esp_timer.h"

enum LatencyMS{
    BUT_DEBOUNCE_TIME  = 200,
    BUT_LONG_PRESS_TIME = 1000,
};



int get_but_state(inp_conf_t * button, unsigned cur_time)
{
    return get_inp_state(button, cur_time, BUT_LONG_PRESS_TIME);
}


int get_inp_state(inp_conf_t * conf, unsigned cur_time, unsigned other_mode_time)
{
    int cur_state = NO_DATA;
    const bool press = gpio_get_level((gpio_num_t)conf->pin_num);
    const unsigned state_time = cur_time - conf->start_state_time;
    if(press != conf->pressed || (press && state_time>other_mode_time)){
        if(press){
            if(state_time >= other_mode_time){
                cur_state = ACT_MODE_2;
            }
        } else if(state_time>BUT_DEBOUNCE_TIME){
            cur_state = state_time >= other_mode_time ? ACT_MODE_2 : ACT_MODE_1;
        }
        conf->pressed = press;
        conf->start_state_time = cur_time; 
    }
    return cur_state; 
}

void inp_init(inp_conf_t * conf, int pin_num)
{   
    conf->pressed = false;
    conf->pin_num = pin_num;
    conf->start_state_time = 0;
    gpio_set_direction(pin_num, GPIO_MODE_INPUT);
    gpio_set_level(pin_num, 0);
}


void device_gpio_out_init() 
{
    gpio_config_t out_conf = {
        .pin_bit_mask = (1ULL << PIN_OUT_LED_ALARM)|(1ULL << PIN_OUT_LED_OK)|(1<<PIN_OUT_SIG),
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE
    };
    gpio_config(&out_conf);
}


