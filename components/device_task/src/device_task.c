#include "device_task.h"

#include "esp_sleep.h"
#include "stdbool.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "portmacro.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "telegram_client.h"
#include "sound_generator.h"
#include "periodic_task.h"
#include "device_common.h"
#include "setting_server.h"
#include "wifi_service.h"
#include "clock_module.h"
#include "adc_reader.h"



enum TimeoutMS{
    TIMEOUT_SEC                 = 1000,
    TIMEOUT_MOVING             = 10*TIMEOUT_SEC,
    TIMEOUT_MINUTE              = 60*TIMEOUT_SEC,
    TIMEOUT_24_HOUR             = 60*60*24*TIMEOUT_SEC,
    TIMEOUT_SLEEP               = 60*60*5*TIMEOUT_SEC,
    LONG_PRESS_TIME             = TIMEOUT_SEC,
    TIMEOUT_ALARM_SLEEP         = 5*TIMEOUT_SEC
};

enum TaskDelay{
    DELAY_SERV      = 100,
    DELAY_MAIN_TASK = 100,
};


static void change_delay();
static void end_moving_handler();
static void alarm_handler();

static void set_tap(bool close);



static void main_task(void *pv)
{
    float volt_val = 0;
    unsigned bits;
    int pin_val = NO_DATA;
    long long sleep_time, cur_time;
    int start_but_val, setting_but_val, sensor_val;
    set_offset(device_get_offset());
    inp_conf_t start_but, setting_but, sensor_in;
    inp_init(&start_but, PIN_BUT_RIGHT);
    inp_init(&setting_but, PIN_BUT_LEFT);
    inp_init(&sensor_in, PIN_IN_SENSOR);
    bool is_alarm, state_alarm = false;
    sleep_time = TIMEOUT_SLEEP;

    for(;;){

        device_start_timer();

        do{
            
            vTaskDelay(DELAY_MAIN_TASK/portTICK_PERIOD_MS);

            cur_time = esp_timer_get_time();

            start_but_val = get_but_state(&start_but, cur_time);
            setting_but_val = get_but_state(&setting_but, cur_time);

            bits = device_get_state();

            if(setting_but_val == ACT_MODE_1){
                if(bits&BIT_SERVER_RUN){
                    device_clear_state(BIT_SERVER_RUN);
                    start_single_signale(70);
                }else{
                    device_set_state(BIT_START_SERVER);
                    start_signale_series(35, 2);
                }
            }else if(setting_but_val == ACT_MODE_2){
                start_signale_series(30, 5);
                vTaskDelay(1000/portTICK_PERIOD_MS);
                change_delay();
            }else if(start_but_val == ACT_MODE_1){
                start_single_signale(70);
                if(bits&BIT_ALARM){
                    if(bits&BIT_ALARM_SOUND){
                        remove_task(alarm_handler);
                        device_clear_state(BIT_ALARM_SOUND);
                    } else {
                        device_clear_state(BIT_ALARM);
                    }
                } 
            } else if(start_but_val == ACT_MODE_2){
                if(bits&BIT_ALARM){
                    start_signale_series(30, 5);
                    device_clear_state(BIT_GUARD_DIS|BIT_ALARM);
                } else {
                    start_signale_series(70, 1);
                    device_set_state(BIT_GUARD_DIS|BIT_ALARM);
                }
            }

            if(bits&BIT_GUARD_DIS){
                gpio_set_level(PIN_OUT_LED_OK, 0);
            } else {
                gpio_set_level(PIN_OUT_LED_OK, 1);
                sensor_val = get_inp_state(&sensor_in, cur_time, device_get_delay());
                if(sensor_val != NO_DATA){
                    device_set_state(BIT_WET_SENSOR);
                    if(sensor_val == ACT_MODE_2 && !(bits&BIT_ALARM)){
                       device_set_state(BIT_ALARM); 
                    }
                } else {
                    device_clear_state(BIT_WET_SENSOR);
                }
            }

            is_alarm = bits&BIT_ALARM;

            if(state_alarm != is_alarm){
                state_alarm = is_alarm;
                if(is_alarm){
                    device_set_state(BIT_ALARM_SOUND);
                    create_periodic_task(alarm_handler, TIMEOUT_SEC, FOREVER);
                    sleep_time = TIMEOUT_MOVING;
                } else {
                    sleep_time = TIMEOUT_SLEEP;
                }
                set_tap(is_alarm);
                gpio_set_level(PIN_OUT_LED_ALARM, is_alarm);
            }
            

            if(bits&BIT_CHECK_BAT){
                
            }


        }while(bits&BITS_DENIED_SLEEP);

        esp_sleep_enable_timer_wakeup(sleep_time * 1000);
        esp_sleep_enable_gpio_wakeup();
        device_stop_timer();
        esp_light_sleep_start();

    }
}



static void service_task(void *pv)
{
    uint32_t bits;
    bool open_sesion;
    vTaskDelay(100/portTICK_PERIOD_MS);
    int esp_res, wait_client_timeout;
    bool fail_init_sntp = false;
    struct tm * tinfo;
    for(;;){
        bits = device_wait_bits_untile(BIT_START_SERVER|BIT_SEND_MESSAGE, 
                            portMAX_DELAY);
        if(bits & BIT_START_SERVER){
            if(start_ap() == ESP_OK ){
                bits = device_wait_bits(BIT_IS_AP_CONNECTION);
                if(bits & BIT_IS_AP_CONNECTION && init_server(network_buf) == ESP_OK){
                    wait_client_timeout = 0;
                    device_set_state(BIT_SERVER_RUN);
                    open_sesion = false;
                    while(bits = device_get_state(), bits&BIT_SERVER_RUN){
                        if(open_sesion){
                            if(!(bits&BIT_IS_AP_CLIENT) ){
                                device_clear_state(BIT_SERVER_RUN);
                            }
                        } else if(bits&BIT_IS_AP_CLIENT){
                            wait_client_timeout = 0;
                            open_sesion = true;
                        } else if(wait_client_timeout>TIMEOUT_MINUTE){
                            device_clear_state(BIT_SERVER_RUN);
                        } else {
                            wait_client_timeout += DELAY_SERV;
                        }

                        vTaskDelay(DELAY_SERV/portTICK_PERIOD_MS);

                    }
                    deinit_server();
                    device_commit_changes();
                }
                wifi_stop();
            }
            device_clear_state(BIT_START_SERVER);
        }

        if(bits&BIT_SEND_MESSAGE){
            esp_res = connect_sta(device_get_ssid(),device_get_pwd());
            if(esp_res == ESP_OK){
                device_clear_state(BIT_ERR_STA_CONF);
                if(! (bits&BIT_IS_TIME)){
                    init_sntp();
                    device_wait_bits(BIT_IS_TIME);
                    stop_sntp();
                }

            }
            if(esp_res == ESP_OK){

            } else {

            }

            wifi_stop();
            device_clear_state(BIT_SEND_MESSAGE);
        }
    }

}

static void end_moving_handler()
{
    gpio_set_level(PIN_OUT_POWER, 0);
}

static void set_tap(bool close)
{
    gpio_set_level(PIN_OUT_SERVO, close);
    gpio_set_level(PIN_OUT_POWER, 1);
    create_periodic_task(end_moving_handler, TIMEOUT_MOVING, 1);
}

static void alarm_handler()
{
    start_single_signale(100);
}

static void change_delay()
{
    unsigned work_time = 0;
    gpio_set_level(PIN_OUT_LED_ALARM, 0);
    gpio_set_level(PIN_OUT_LED_OK, 0);
    int delay_sec = 1;
    do{
        vTaskDelay(DELAY_MAIN_TASK/portTICK_PERIOD_MS);
        work_time += DELAY_MAIN_TASK;
        if(work_time > TIMEOUT_SEC){
            work_time = 0;
            device_set_delay(delay_sec++);
            gpio_set_level(PIN_OUT_LED_OK, 1);
            start_single_signale(20);
        } else {
            gpio_set_level(PIN_OUT_LED_OK, 0);
        }
    }while(gpio_get_level(PIN_BUT_RIGHT) && delay_sec < MAX_DELAY_START_SEC);
    device_commit_changes();
}


int task_init()
{
    if(xTaskCreate(
            service_task, 
            "service",
            20000, 
            NULL, 
            3,
            NULL) != pdTRUE
        || xTaskCreate(
            main_task, 
            "main",
            20000, 
            NULL, 
            4,
            NULL) != pdTRUE 
    ){
        ESP_LOGE("","task create failure");
        return ESP_FAIL;
    }
    return ESP_OK;   
}




