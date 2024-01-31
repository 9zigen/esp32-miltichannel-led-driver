//
// Created by Aleksey Volkov on 25.11.2020.
//
#include <stdio.h>
#include <mbedtls/md.h>
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_random.h"

#ifdef CONFIG_IDF_TARGET_ESP32S2
#include "esp_hmac.h"
#endif

#include "auth.h"

static const char *TAG="AUTH";

void generateToken(char *token_str)
{
#ifdef CONFIG_IDF_TARGET_ESP32
  char payload[32];
  snprintf(payload, 32, "LED_%lu", esp_random());

  for(int i= 0; i< sizeof(payload); i++){
//    printf("%02x", (int)payload[i]);
    sprintf(&token_str[2 * i], "%02x", (int)payload[i]);
  }
  ESP_LOGD(TAG, "%s",token_str);
#else
  uint8_t hmac[32];

  const char *message = "Hello, HMAC!";
  const size_t msg_len = 12;

  esp_err_t result = esp_hmac_calculate(HMAC_KEY4, message, msg_len, hmac);

  if (result == ESP_OK) {
    // HMAC written to hmac now
  } else {
    // failure calculating HMAC
  }

  char *key = "mySecretKey!";
  char payload[32];
  sprintf(payload, "LED_%d", esp_random());

  uint8_t hmac[32];

  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

  const size_t payload_length = strlen(payload);
  const size_t key_length = strlen(key);

  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
  mbedtls_md_hmac_starts(&ctx, (const unsigned char *) key, key_length);
  mbedtls_md_hmac_update(&ctx, (const unsigned char *) payload, payload_length);
  mbedtls_md_hmac_finish(&ctx, hmac);
  mbedtls_md_free(&ctx);

  ESP_LOGD(TAG, "Hash:");
  for(int i= 0; i< sizeof(hmac); i++){
    printf("%02x", (int)hmac[i]);
    sprintf(&token_str[2 * i], "%02x", (int)hmac[i]);
  }
  ESP_LOGD(TAG, "%s",token_str);
#endif
}