#include "device_common.h"

#include "driver/gpio.h"
#include "esp_sleep.h"
#include "esp_timer.h"

#define DEBOUNCE_TIME_MS 200

static void IRAM_ATTR but_handler(void* arg) 
{
    static int last_pin = NO_DATA;
    static volatile uint32_t last_interrupt_time = 0;
    int cur_pin = (int) arg;
    uint32_t current_time = esp_timer_get_time() / 1000;  
    if(cur_pin == last_pin){
        if (current_time - last_interrupt_time > DEBOUNCE_TIME_MS) {
            last_interrupt_time = current_time; 
            if(last_pin == PIN_IN_BUT_START_SERVER){
                device_set_state_isr(BIT_BUT_START_PRESED);
            } else {
                device_set_state_isr(BIT_BUT_RESET_PRESSED);
            }
            last_pin = NO_DATA;
        }
    } else if(last_pin == NO_DATA){
        last_pin = cur_pin;
        last_interrupt_time = current_time;
    }
}

static void IRAM_ATTR sensor_handler(void* arg) 
{
    device_set_state_isr(BIT_WET_SENSOR);
}

void device_gpio_init() 
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_NEGEDGE;  
    io_conf.pin_bit_mask = (1ULL << PIN_IN_SENSOR);
    io_conf.mode = GPIO_MODE_INPUT;           
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;  
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf);

    gpio_isr_handler_add(PIN_IN_SENSOR, sensor_handler, NULL);

    gpio_config_t but_conf;
    io_conf.intr_type = GPIO_INTR_ANYEDGE;  
    io_conf.pin_bit_mask = (1ULL << PIN_IN_BUT_RESET) | (1ULL << PIN_IN_BUT_START_SERVER);
    io_conf.mode = GPIO_MODE_INPUT;           
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;  
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&but_conf);

    gpio_isr_handler_add(PIN_IN_BUT_RESET, but_handler, (void*)PIN_IN_BUT_RESET);
    gpio_isr_handler_add(PIN_IN_BUT_START_SERVER, but_handler, (void*)PIN_IN_BUT_START_SERVER);

    gpio_config_t out_conf = {
        .pin_bit_mask = (1ULL << PIN_OUT_LED_ALARM)|(1ULL << PIN_OUT_LED_OK)|(1<<PIN_OUT_SIG),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLUP_ENABLE
    };
    gpio_config(&out_conf);
}


