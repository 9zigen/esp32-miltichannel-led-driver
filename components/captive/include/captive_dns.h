//
// Created by Aleksey Volkov on 19.04.2022.
//

#ifndef ESP32S2_LOW_POWER_SENSOR_CAPTIVE_DNS_H
#define ESP32S2_LOW_POWER_SENSOR_CAPTIVE_DNS_H

#define CAPTIVE_DNS_HEADER_SIZE           12
#define CAPTIVE_DNS_QUESTION_MAX_LENGTH   50

#include <lwip/ip4_addr.h>

typedef struct captive_dns_handle_t {
  TaskHandle_t task_handle;
  int sockfd;
  ip4_addr_t resolve_ip;
} captive_dns_handle_t;

typedef struct __attribute__((packed)) dns_header_t {
  uint16_t ID;
  uint8_t  RD       :1;
  uint8_t  TC       :1;
  uint8_t  AA       :1;
  uint8_t  OPCODE   :4;
  uint8_t  QR       :1;
  uint8_t  RCODE    :4;
  uint8_t  Z        :3;
  uint8_t  RA       :1;
  uint16_t QDCOUNT;
  uint16_t ANCOUNT;
  uint16_t NSCOUNT;
  uint16_t ARCOUNT;
} dns_header_t;

typedef struct __attribute__((packed)) captive_dns_answer_t {
  uint16_t NAME;
  uint16_t TYPE;
  uint16_t CLASS;
  uint32_t TTL;
  uint16_t RDLENGTH;
  uint32_t RDATA;
} captive_dns_answer_t;

esp_err_t captive_dns_start();
esp_err_t captive_dns_stop();

#endif //ESP32S2_LOW_POWER_SENSOR_CAPTIVE_DNS_H
