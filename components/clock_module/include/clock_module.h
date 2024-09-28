#ifndef CLOCK_MODULE_H
#define CLOCK_MODULE_H


#ifdef __cplusplus
extern "C" {
#endif


#include <sys/time.h>



struct tm* get_cur_time_tm(void);
const char* snprintf_time(const char *format);
void set_offset(int offset_hours);




#ifdef __cplusplus
}
#endif





















#endif