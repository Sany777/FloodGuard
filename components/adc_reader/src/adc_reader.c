#include "adc_reader.h"

#include "driver/adc.h"
#include "freertos/FreeRTOS.h"



#define ADC_CHANNEL ADC1_CHANNEL_1  // GPIO13 (ESP32)
#define ADC_ATTEN ADC_ATTEN_DB_0     




void adc_init(void)
{
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc2_config_channel_atten(ADC_CHANNEL, ADC_ATTEN);
}

// max adc_value = 4095 for 12 bit
int get_voltage_mv(bat_conf_t * bat_conf)
{
    int adc_value = 0;
    if(bat_conf->volt_koef == 0) return -1;
    adc2_get_raw(ADC_CHANNEL, ADC_WIDTH_BIT_12, &adc_value);
    return (adc_value * bat_conf->volt_koef) / 1000 ;
}

void set_coef_val(bat_conf_t * bat_conf, unsigned real_volt_mv)
{
    unsigned val = 0;
    bat_conf->volt_koef = 1000;
    while(val = get_voltage_mv(bat_conf), val < 1000)vTaskDelay(10);
    bat_conf->volt_koef = (real_volt_mv * 1000) / val ;
}

int get_voltage_perc(bat_conf_t * bat_conf)
{
    unsigned volt = 0;
    if(bat_conf->max_mVolt == 0 || bat_conf->min_mVolt == 0) return 0;
    while(volt = get_voltage_mv(bat_conf), volt < 1000)vTaskDelay(10);
    if (volt >= bat_conf->max_mVolt) return 100;
    if (volt <= bat_conf->min_mVolt) return 0;
    return ((get_voltage_mv(bat_conf) - bat_conf->min_mVolt)*100) / (bat_conf->max_mVolt - bat_conf->min_mVolt);
}