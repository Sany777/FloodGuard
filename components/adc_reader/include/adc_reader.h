#ifndef ADC_READER_H
#define ADC_READER_H

#ifdef __cplusplus
extern "C" {
#endif


typedef struct{
    unsigned min_mVolt;
    unsigned max_mVolt;
    unsigned volt_koef;
}bat_conf_t;



void adc_init(void);
int get_voltage_mv(bat_conf_t * bat_conf);
int get_voltage_perc(bat_conf_t * bat_conf);
void set_coef_val(bat_conf_t * bat_conf, unsigned real_volt_mv);





#ifdef __cplusplus
}
#endif



#endif // ADC_READER_H
