/***
** Created by Aleksey Volkov on 19.12.2019.
***/

#ifndef HV_CC_LED_DRIVER_RTOS_RTC_H
#define HV_CC_LED_DRIVER_RTOS_RTC_H

void init_clock();
void print_time();
void get_time_string(char *time_string);
void det_time_string_since_boot(char * time_string);
uint8_t get_ntp_sync_status();
#endif //HV_CC_LED_DRIVER_RTOS_RTC_H
