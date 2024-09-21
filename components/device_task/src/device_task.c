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
    TIMEOUT_10_SEC              = 10*TIMEOUT_SEC,
    TIMEOUT_UPDATE_SCREEN       = 19*TIMEOUT_SEC,
    TIMEOUT_MINUTE              = 60*TIMEOUT_SEC,
    TIMEOUT_FOUR_MINUTE         = 4*TIMEOUT_MINUTE,
    TIMEOUT_HOUR                = 60*TIMEOUT_MINUTE,
    TIMEOUT_4_HOUR              = 4*TIMEOUT_HOUR,
    DELAY_UPDATE_FORECAST       = 32*TIMEOUT_MINUTE,
    LONG_PRESS_TIME             = TIMEOUT_SEC,
};

enum TaskDelay{
    DELAY_SERV      = 100,
    DELAY_MAIN_TASK = 100,
};

static int delay_update_forecast = 2*TIMEOUT_MINUTE;



static void sig_end_but_inp_handler();




static void main_task(void *pv)
{
    float volt_val = 0;
    unsigned bits;
    int pin_val = NO_DATA;
    unsigned timeout = TIMEOUT_10_SEC;
    long long counter, work_time = 0;
    int volt_fail = 0;
    set_offset(device_get_offset());

    for(;;){

        device_start_timer();
        counter = esp_timer_get_time();


        while(1){
            
            vTaskDelay(DELAY_MAIN_TASK/portTICK_PERIOD_MS);

            bits = device_get_state();

            if(bits&BIT_BUT_START_PRESED){
                create_periodic_task(sig_end_but_inp_handler, TIMEOUT_10_SEC, 1);
                if(bits&BIT_SERVER_RUN){
                    device_clear_state(BIT_SERVER_RUN);
                } else {
                    device_set_state(BIT_START_SERVER);
                }
            }
            

            if(bits&BIT_CHECK_BAT){
                
            }


        }

        esp_sleep_enable_timer_wakeup(delay_update_forecast * 1000);
        esp_sleep_enable_gpio_wakeup();
        device_stop_timer();
        esp_light_sleep_start();
        if(esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER){
            timeout = 1;
        } else {
            timeout = TIMEOUT_10_SEC;
        }
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








static void sig_end_but_inp_handler()
{
    device_clear_state_isr(BIT_WAIT_BUT_INPUT);
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




