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
#include "string.h"
#include "stdio.h"

#include "device_http_client.h"
#include "sound_generator.h"
#include "periodic_task.h"
#include "device_common.h"
#include "setting_server.h"
#include "wifi_service.h"
#include "clock_module.h"
#include "adc_reader.h"



enum TimeoutMS{
    TIMEOUT_SEC                 = 1000,
    TIMEOUT_MOVING              = 10*TIMEOUT_SEC,
    TIMEOUT_MINUTE              = 60*TIMEOUT_SEC,
    TIMEOUT_24_HOUR             = 60*60*24*TIMEOUT_SEC,
    TIMEOUT_SLEEP               = 60*60*5*TIMEOUT_SEC,
    LONG_PRESS_TIME             = TIMEOUT_SEC,
    TIMEOUT_ALARM_SLEEP         = 5*TIMEOUT_SEC,
    BUT_LONG_PRESS_LATENCY      = 2*TIMEOUT_SEC,
};

enum TaskDelay{
    DELAY_SERV      = 100,
    DELAY_MAIN_TASK = 100,
};

enum{
    SENSOR_WET_STATE        = ACT_MODE_0,
    SENSOR_ALARM_STATE      = ACT_MODE_2,
    BUT_PRESS_STATE         = ACT_MODE_1,
    BUT_LONG_PRESS_STATE    = ACT_MODE_2
};


static void change_delay();
static void end_moving_handler();
static void alarm_handler();
static void update_time_handler();

QueueHandle_t telegram_queue;

static void set_tap(bool close);
long long sleep_time = TIMEOUT_SLEEP;

void blink_alarm_handle()
{
    gpio_set_level(PIN_OUT_LED_ALARM, !gpio_get_level(PIN_OUT_LED_ALARM));
    start_single_signale(10);
}

static void main_task(void *pv)
{
    unsigned bits;
    long long cur_time;
    int start_but_val, setting_but_val, sensor_val;
    unsigned work_time, start_run, timeout = 5*TIMEOUT_SEC;
    set_offset(device_get_offset());
    inp_conf_t start_but, setting_but, sensor_in;
    inp_init(&start_but, PIN_BUT_RIGHT, 0);
    inp_init(&setting_but, PIN_BUT_LEFT, 0);
    inp_init(&sensor_in, PIN_IN_SENSOR, 0);
    bool send = true;
    device_set_state(BIT_UPDATE_TIME);
    create_periodic_task(update_time_handler, TIMEOUT_24_HOUR, FOREVER);
    vTaskDelay(1000/portTICK_PERIOD_MS);
    struct tm *tinfo = get_cur_time_tm();
    int day_send = 0;
    for(;;){
        device_start_timer();
        start_run = esp_timer_get_time() / 1000;
        while(1){
            vTaskDelay(DELAY_MAIN_TASK/portTICK_PERIOD_MS);
            bits = device_get_state();         
            cur_time = esp_timer_get_time() / 1000;
            start_but_val = get_inp_state(&start_but, cur_time, BUT_LONG_PRESS_LATENCY);
            setting_but_val = get_inp_state(&setting_but, cur_time, BUT_LONG_PRESS_LATENCY);

            if(setting_but_val == BUT_PRESS_STATE){
                ESP_LOGI("", "start but press!");
                if(bits&BIT_SERVER_RUN){
                    device_clear_state(BIT_SERVER_RUN);
                    start_single_signale(70);
                }else{
                    device_set_state(BIT_START_SERVER);
                    start_signale_series(35, 2);
                }
            }else if(setting_but_val == BUT_LONG_PRESS_STATE){
                ESP_LOGI("", "setting but long press!");
                start_signale_series(30, 5);
                vTaskDelay(1000/portTICK_PERIOD_MS);
                change_delay();
            }else if(start_but_val == BUT_PRESS_STATE){
                ESP_LOGI("", "start but press!");
                start_single_signale(70);
                if(bits&BIT_ALARM){
                    if(bits&BIT_ALARM_SOUND){
                        remove_task(alarm_handler);
                        device_clear_state(BIT_ALARM_SOUND);
                    } else {
                        device_clear_state(BIT_ALARM);
                    }
                } 
            } else if(start_but_val == BUT_LONG_PRESS_STATE){
                ESP_LOGI("", "start but long press!");
                if(bits&BIT_ALARM){
                    start_signale_series(30, 5);
                    device_clear_state(BIT_GUARD_DIS|BIT_ALARM);
                    set_tap(false);
                } else {
                    start_signale_series(70, 1);
                    device_set_state(BIT_GUARD_DIS|BIT_ALARM);
                }
            }

            sensor_val = get_inp_state(&sensor_in, cur_time, device_get_delay());

            if(sensor_val == NO_DATA){
                if(bits&BIT_WET_SENSOR){
                    ESP_LOGI("", "Dry sensor!");
                    device_clear_state(BIT_WET_SENSOR);
                    remove_task(blink_alarm_handle);
                    if(bits&BIT_ALARM && !(bits&BIT_GUARD_DIS) ){
                         send = true;
                    }
                }
            } else if(sensor_val == SENSOR_WET_STATE){
                if(!(bits&BIT_WET_SENSOR)){
                    ESP_LOGI("", "Wet sensor!");
                    device_set_state(BIT_WET_SENSOR);
                    create_periodic_task(blink_alarm_handle, TIMEOUT_SEC, FOREVER);
                }
            } else if(sensor_val == SENSOR_ALARM_STATE && !(bits&BIT_GUARD_DIS) ){
                if(!(bits&BIT_ALARM)){
                    ESP_LOGI("", "Alarm!");
                    device_set_state(BIT_ALARM); 
                    remove_task(blink_alarm_handle);
                    create_periodic_task(alarm_handler, 3*TIMEOUT_SEC, FOREVER);
                    sleep_time = 5*TIMEOUT_SEC;
                    send = true;
                    set_tap(true);
                }
            }

            if(bits&BIT_GUARD_DIS){
                gpio_set_level(PIN_OUT_LED_OK, 0);
            } else {
                gpio_set_level(PIN_OUT_LED_OK, 1);
            }

            if(bits&BIT_ALARM){
                gpio_set_level(PIN_OUT_LED_ALARM, 1);
            } else {
                gpio_set_level(PIN_OUT_LED_ALARM, 0);
            }

            if(send){
                create_telegram_message(bits);
                send = false;
                day_send = tinfo->tm_wday;
            }

            if(!(bits&BITS_DENIED_SLEEP)){
                work_time = cur_time - start_run;
            }

        }while(work_time < timeout);

        if(bits&BIT_ERR_TELEGRAM_SEND){
            if(sleep_time < TIMEOUT_MINUTE*30){
                sleep_time *= 2;
            }
        }

        esp_sleep_enable_timer_wakeup(sleep_time * 1000);
        esp_sleep_enable_ext0_wakeup(PIN_IN_SENSOR, 0);
        esp_sleep_enable_ext1_wakeup(PIN_BUT_RIGHT, 0);
        device_stop_timer();

        ESP_LOGI("", "sleep");
        vTaskDelay(100);

        esp_light_sleep_start();

        if(bits&BIT_ERR_TELEGRAM_SEND){
            device_set_state(BIT_SEND_MESSAGE);
        }
        if(esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER){
            if(day_send != tinfo->tm_wday){
                if( (is_valid_bat_conf() && get_voltage_perc(device_get_bat_conf()) < 20) 
                    || (bits&BIT_IS_TIME && bits&BIT_INFO_NOTIFACTION_EN && tinfo->tm_hour > 8) ){
                    send = true; 
                }
            } 
            ESP_LOGI("", "weakup timer");
            timeout = 0;
        } else {
             ESP_LOGI("", "weakup gpio");
             timeout = 5*TIMEOUT_SEC;
        }

    }
}



static void service_task(void *pv)
{
    uint32_t bits;
    bool open_sesion;
    telegram_queue = xQueueCreate(5, sizeof(char*));
    char *message;
    vTaskDelay(100/portTICK_PERIOD_MS);
    int esp_res, wait_client_timeout;


    for(;;){
        bits = device_wait_bits_untile(BIT_START_SERVER|BIT_SEND_MESSAGE|BIT_UPDATE_TIME, 
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

        if(bits&BIT_SEND_MESSAGE || bits&BIT_UPDATE_TIME){
            esp_res = connect_sta(device_get_ssid(),device_get_pwd());
            if(esp_res == ESP_OK){
                device_clear_state(BIT_ERR_STA_CONF);
                if(bits&BIT_UPDATE_TIME){
                    device_clear_state(BIT_UPDATE_TIME|BIT_IS_TIME);  
                    esp_res = device_update_time();
                    if(esp_res == ESP_OK){
                        device_set_state(BIT_IS_TIME);
                    }
                }
                if(esp_res == ESP_OK){
                    while(message != NULL || xQueueReceive(telegram_queue, &message, 0) == pdTRUE){
                        esp_res = send_telegram_message(device_get_token(), device_get_chat_id(), message);
                        if(esp_res != ESP_OK)break;
                        free(message);
                        message = NULL;
                    }
                }
            }
            if(bits&BIT_SEND_MESSAGE){
                if(esp_res != ESP_OK){
                    device_set_state(BIT_ERR_TELEGRAM_SEND);
                } else if(bits&BIT_ERR_TELEGRAM_SEND){
                    device_clear_state(BIT_ERR_TELEGRAM_SEND);
                }
            }
            wifi_stop();
            device_clear_state(BIT_SEND_MESSAGE|BIT_WAIT_SENDING);
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

static void update_time_handler()
{
    device_set_state_isr(BIT_UPDATE_TIME);
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
    }while(gpio_get_level(PIN_BUT_RIGHT) == 0 && delay_sec < MAX_DELAY_START_SEC);
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




