#include <limits.h>
#include "unity.h"
#include "esp_err.h"
#include <time.h>
#include <string.h>
#include <errno.h>
#include "esp_log.h"
#include "cmock.h"


#include "adc_reader.h"


const char *TAG = "errno ";



TEST_CASE("initialisation adc", "[adc]")
{
    TEST_ESP_OK(adc_init());
}

TEST_CASE("set battery config", "[adc]")
{
    bat_data_t conf = {
        .max_mVolt = 4200,
        .min_mVolt = 3200,
        .volt_koef = 1000,
        .voltage_mv = 4200
    };
    set_coef_val(&conf, 4200);
    TEST_ASSERT_EQUAL(1000, conf.volt_koef);
}


TEST_CASE("get percentage", "[adc]")
{
    int MAX_MV = 4200, MIN_MV = 3200;
    int AVR_MV = MAX_MV - (MAX_MV - MIN_MV) / 2;
    bat_data_t conf = {
        .max_mVolt = MAX_MV,
        .min_mVolt = MIN_MV,
        .volt_koef = 1000,
        .voltage_mv = AVR_MV
    };

    TEST_ASSERT_EQUAL(50, get_bat_percentage(&conf));
}