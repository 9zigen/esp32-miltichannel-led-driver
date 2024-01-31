//
// Created by Aleksey Volkov on 19.04.2022.
//

#include <string.h>
#include <sys/param.h>
#include <esp_mac.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "captive_dns.h"

#define FLAG_QR (1<<7)
#define FLAG_AA (1<<2)
#define FLAG_TC (1<<1)
#define FLAG_RD (1<<0)

#define QTYPE_A  1
#define QTYPE_NS 2
#define QTYPE_CNAME 5
#define QTYPE_SOA 6
#define QTYPE_WKS 11
#define QTYPE_PTR 12
#define QTYPE_HINFO 13
#define QTYPE_MINFO 14
#define QTYPE_MX 15
#define QTYPE_TXT 16
#define QTYPE_URI 256

#define QCLASS_IN 1
#define QCLASS_ANY 255
#define QCLASS_URI 256

static const char *TAG = "CAPTIVE";

static captive_dns_handle_t captive_dns_handle;

static esp_err_t dns_srv_cleanup();

static void captive_dns_task(void *pvParameters) {
  uint8_t rx_buffer[128];

  for(;;) {
    struct sockaddr_in dest_addr;

    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(53);

    int sock = captive_dns_handle.sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

    if(sock < 0) {
      ESP_LOGE(TAG, "Unable to create socket");
      break;
    }

    ESP_LOGI(TAG, "Socket created");

    int err = bind(sock, (struct sockaddr*) &dest_addr, sizeof(dest_addr));

    if(err < 0) {
      ESP_LOGE(TAG, "Socket unable to bind");
      break;
    }

    ESP_LOGI(TAG, "Listening...");

    for(;;) {
      struct sockaddr_in source_addr;
      socklen_t socklen = sizeof(source_addr);

      memset(rx_buffer, 0x00, sizeof(rx_buffer));
      int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer), 0, (struct sockaddr*) &source_addr, &socklen);

      if(len < 0) {
        ESP_LOGE(TAG, "revfrom failed");
        break;
      }

      if(len > CAPTIVE_DNS_HEADER_SIZE + CAPTIVE_DNS_QUESTION_MAX_LENGTH) {
        ESP_LOGW(TAG, "Received more data [%d] than expected. Ignoring.", len);
        continue;
      }

      // Nul termination. To prevent pointer escape
      rx_buffer[sizeof(rx_buffer) - 1] = '\0';

      dns_header_t *header = (dns_header_t*) rx_buffer;

      header->QR      = 1;
      header->OPCODE  = 0;
      header->AA      = 1;
      header->RCODE   = 0;
      header->TC      = 0;
      header->Z       = 0;
      header->RA      = 0;
      header->ANCOUNT = header->QDCOUNT;
      header->NSCOUNT = 0;
      header->ARCOUNT = 0;

      // ptr points to the beginning of the QUESTION
      uint8_t *ptr = rx_buffer + sizeof(dns_header_t);

      // Jump over QNAME
      while(*ptr++);

      // Jump over QTYPE
      ptr += 2;

      // Jump over QCLASS
      ptr += 2;

      captive_dns_answer_t *answer = (captive_dns_answer_t*) ptr;

      /* This is a pointer to the beginning of the question.
       * As per DNS standard, first two bits must be set to 11 for some odd reason hence 0xC0 */
      answer->NAME     = ntohs(0xC00C);
      answer->TYPE     = ntohs(QTYPE_A);
      answer->CLASS    = ntohs(QCLASS_IN);
      answer->TTL      = 0;
      answer->RDLENGTH = ntohs(4);
      answer->RDATA    = captive_dns_handle.resolve_ip.addr;

      // Jump over ANSWER
      ptr += sizeof(captive_dns_answer_t);

      int err = sendto(sock, rx_buffer, ptr - rx_buffer, 0, (struct sockaddr *)&source_addr, sizeof(source_addr));

      if (err < 0) {
        ESP_LOGE(TAG, "Error occurred during sending");
        break;
      }

      taskYIELD();
    }

    if(sock != -1) {
      dns_srv_cleanup();
      ESP_LOGI(TAG, "Restarting...");
    }
  }

  captive_dns_stop();
}

static esp_err_t dns_srv_cleanup() {
  ESP_LOGI(TAG, "Cleanup");

  if(captive_dns_handle.sockfd != -1) {
    shutdown(captive_dns_handle.sockfd, 0);
    close(captive_dns_handle.sockfd);

    captive_dns_handle.sockfd = -1;
  }

  return ESP_OK;
}

esp_err_t captive_dns_start() {
  if(captive_dns_handle.task_handle != NULL) {
    return ESP_OK;
  }

  ip4_addr_t resolve_ip;
  inet_pton(AF_INET, CONFIG_CAPTIVE_REDIRECT_IP, &resolve_ip);

  captive_dns_handle.resolve_ip = resolve_ip;
  captive_dns_handle.sockfd = -1;

//  ESP_LOGI(TAG, "Resolve IP: "IPSTR,
//           esp_ip4_addr1(&resolve_ip),
//           esp_ip4_addr2(&resolve_ip),
//           esp_ip4_addr3(&resolve_ip),
//           esp_ip4_addr4(&resolve_ip));

  xTaskCreate(captive_dns_task, "captive_dns_task", 4096, NULL, 5, &captive_dns_handle.task_handle);
  return ESP_OK;
}

esp_err_t captive_dns_stop() {
  vTaskDelete(captive_dns_handle.task_handle);
  captive_dns_handle.task_handle = NULL;

  dns_srv_cleanup();
  ESP_LOGI(TAG, "Server stopped.");

  return ESP_OK;
}