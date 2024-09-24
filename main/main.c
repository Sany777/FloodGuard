#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "device_common.h"
#include "device_task.h"
#include "periodic_task.h"



void app_main(void) 
{
    device_init();
    device_init_timer();
    task_init();

}
