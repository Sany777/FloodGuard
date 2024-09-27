#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "device_common.h"
#include "device_task.h"
#include "periodic_task.h"

#include "adc_reader.h"
#include "esp_log.h"


void app_main(void) 
{
    adc_init();
    device_init();
    device_init_timer();
    task_init();
}
