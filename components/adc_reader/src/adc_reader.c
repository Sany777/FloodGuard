#include "adc_reader.h"

#include "driver/adc.h"
#include "freertos/FreeRTOS.h"



#define ADC_CHANNEL ADC1_CHANNEL_1  // GPIO13 (ESP32)
#define ADC_ATTEN ADC_ATTEN_DB_12     


int adc_init(void)
{
    int res = adc1_config_width(ADC_WIDTH_BIT_12);
    if(res == ESP_OK){
        res = adc2_config_channel_atten(ADC_CHANNEL, ADC_ATTEN);
    }
    return res;
}

int get_voltage_mv(bat_data_t * bat_data)
{
    int adc_value = 0;
    if(bat_data->volt_koef == 0) return -1;
    int res = adc2_get_raw(ADC_CHANNEL, ADC_WIDTH_BIT_12, &adc_value);
    bat_data->voltage_mv = (adc_value * bat_data->volt_koef) / 1000 ;
    return res;
}

void set_coef_val(bat_data_t * bat_data, unsigned real_volt_mv)
{
    bat_data->volt_koef = (real_volt_mv * 1000) / bat_data->voltage_mv ;
}

int get_bat_percentage(bat_data_t * bat_data)
{
    if (bat_data->voltage_mv >= bat_data->max_mVolt) return 100;
    if (bat_data->voltage_mv <= bat_data->min_mVolt) return 0;
    return ((bat_data->voltage_mv - bat_data->min_mVolt)*100) / (bat_data->max_mVolt - bat_data->min_mVolt);
}