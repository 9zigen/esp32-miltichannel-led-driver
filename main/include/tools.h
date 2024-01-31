/***
** Created by Aleksey Volkov on 11.02.2020.
***/

#ifndef HV_CC_LED_DRIVER_RTOS_TOOLS_H
#define HV_CC_LED_DRIVER_RTOS_TOOLS_H

#define CHECK(x) do { esp_err_t __; if ((__ = x) != ESP_OK) return __; } while (0)
#define CHECK_TIMER(x) do { BaseType_t __; if ((__ = x) != pdPASS) return __; } while (0)
//#define CHECK_QUEUE(x) do { QueueHandle_t __; __ = x; if (__ != NULL) return __; } while (0)
//#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0)
double exponential_filter(const double * in, double *last, const double *alpha);
//char *no_space(char *strin);

#endif //HV_CC_LED_DRIVER_RTOS_TOOLS_H
