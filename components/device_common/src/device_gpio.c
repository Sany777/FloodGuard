#include "device_common.h"

#include "driver/gpio.h"
#include "esp_sleep.h"
#include "esp_timer.h"

enum LatencyMS{
    DEBOUNCE_TIME  = 500,
};



int get_inp_state(inp_conf_t * conf, unsigned cur_time, unsigned mode_2_latency)
{
    unsigned state_time;
    const bool active = gpio_get_level((gpio_num_t)conf->pin_num) == conf->active_level;
    int cur_state = active ? ACT_MODE_0 : NO_DATA;

    if(conf->start_state_time == 0){
        conf->start_state_time = cur_time;
    } else {
        state_time = cur_time - conf->start_state_time;
        if(state_time > DEBOUNCE_TIME){
            if(active != conf->active){
                conf->active = active;
                conf->start_state_time = cur_time; 
                if(active){
                    cur_state = ACT_MODE_1;
                }
            } else if(active && state_time > mode_2_latency){
                cur_state = ACT_MODE_2;
                conf->start_state_time = cur_time; 
            }
        }
    }
    return cur_state; 
}

void inp_init(inp_conf_t * conf, int pin_num, bool active_level)
{
    conf->active_level = active_level;
    conf->start_state_time = 0;
    conf->active = false;
    conf->pin_num = pin_num;
    gpio_config_t in_conf = {
        .pin_bit_mask = (1ULL << pin_num),
        .mode = GPIO_MODE_INPUT,
        in_conf.pull_up_en = active_level ? GPIO_PULLUP_DISABLE : GPIO_PULLUP_ENABLE,
        in_conf.pull_down_en = active_level ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&in_conf);
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


