#ifndef ADC_READER_H
#define ADC_READER_H

#ifdef __cplusplus
extern "C" {
#endif


typedef struct{
    unsigned min_mVolt;
    unsigned max_mVolt;
    unsigned volt_koef;
    int voltage_mv;
}bat_data_t;



int adc_init(void);
int get_voltage_mv(bat_data_t * bat_data);
int get_bat_percentage(bat_data_t * bat_data);
void set_coef_val(bat_data_t * bat_data, unsigned real_volt_mv);





#ifdef __cplusplus
}
#endif



#endif // ADC_READER_H
