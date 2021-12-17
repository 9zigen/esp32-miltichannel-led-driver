//
// Created by Aleksey Volkov on 13.04.2021.
//

#include <string.h>
#include <sys/param.h>
#include <light.h>
#include <cJSON.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "arpa/inet.h"

#include "light.h"
#include "udp_multicast.h"

#define UDP_PORT 37801
#define MULTICAST_IPV4_ADDR "232.10.11.15" /* "232.10.11.12" */
#define MULTICAST_TTL 200

static const char *TAG = "multicast";
static const char *V4TAG = "mcast-ipv4";
extern esp_netif_t* wifi_netif;

QueueHandle_t xQueueUDP;

/* Add a socket, either IPV4-only or IPV6 dual mode, to the IPV4
   multicast group */
static int socket_add_ipv4_multicast_group(int sock, bool assign_source_if)
{
  struct ip_mreq imreq = { 0 };
  struct in_addr iaddr = { 0 };
  int err = 0;
  // Configure source interface
#if LISTEN_ALL_IF
  imreq.imr_interface.s_addr = IPADDR_ANY;
#else
  esp_netif_ip_info_t ip_info = { 0 };
  err = esp_netif_get_ip_info(wifi_netif, &ip_info);
  if (err != ESP_OK) {
    ESP_LOGE(V4TAG, "Failed to get IP address info. Error 0x%x", err);
    goto err;
  }
  inet_addr_from_ip4addr(&iaddr, &ip_info.ip);
#endif // LISTEN_ALL_IF
  // Configure multicast address to listen to
  err = inet_aton(MULTICAST_IPV4_ADDR, &imreq.imr_multiaddr.s_addr);
  if (err != 1) {
    ESP_LOGE(V4TAG, "Configured IPV4 multicast address '%s' is invalid.", MULTICAST_IPV4_ADDR);
    // Errors in the return value have to be negative
    err = -1;
    goto err;
  }
  ESP_LOGI(TAG, "Configured IPV4 Multicast address %s", inet_ntoa(imreq.imr_multiaddr.s_addr));
  if (!IP_MULTICAST(ntohl(imreq.imr_multiaddr.s_addr))) {
    ESP_LOGW(V4TAG, "Configured IPV4 multicast address '%s' is not a valid multicast address. This will probably not work.", MULTICAST_IPV4_ADDR);
  }

  if (assign_source_if) {
    // Assign the IPv4 multicast source interface, via its IP
    // (only necessary if this socket is IPV4 only)
    err = setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, &iaddr,
                     sizeof(struct in_addr));
    if (err < 0) {
      ESP_LOGE(V4TAG, "Failed to set IP_MULTICAST_IF. Error %d", errno);
      goto err;
    }
  }

  err = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                   &imreq, sizeof(struct ip_mreq));
  if (err < 0) {
    ESP_LOGE(V4TAG, "Failed to set IP_ADD_MEMBERSHIP. Error %d", errno);
    goto err;
  }

  err:
  return err;
}

static int create_multicast_ipv4_socket(void)
{
  struct sockaddr_in saddr = { 0 };
  int sock = -1;
  int err = 0;

  sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (sock < 0) {
    ESP_LOGE(V4TAG, "Failed to create socket. Error %d", errno);
    return -1;
  }

  // Bind the socket to any address
  saddr.sin_family = PF_INET;
  saddr.sin_port = htons(UDP_PORT);
  saddr.sin_addr.s_addr = htonl(INADDR_ANY);
  err = bind(sock, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
  if (err < 0) {
    ESP_LOGE(V4TAG, "Failed to bind socket. Error %d", errno);
    goto err;
  }

  // Assign multicast TTL (set separately from normal interface TTL)
  uint8_t ttl = MULTICAST_TTL;
  setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(uint8_t));
  if (err < 0) {
    ESP_LOGE(V4TAG, "Failed to set IP_MULTICAST_TTL. Error %d", errno);
    goto err;
  }

  // this is also a listening socket, so add it to the multicast
  // group for listening...
  err = socket_add_ipv4_multicast_group(sock, true);
  if (err < 0) {
    goto err;
  }

  // All set, socket is configured for sending and receiving
  return sock;

  err:
  close(sock);
  return -1;
}

/* sync lights */
void udp_set_light(const double * target_duty, double target_brightness, transition_mode_t mode)
{
  x_light_message_t txMessage;
  for (uint8_t i = 0; i < MAX_LED_CHANNELS; ++i)
  {
    /* prepare new queue message */
    txMessage.target_duty[i] = target_duty[i];
  }

  /* transition duration LEDC_FADE_TIME in ms. */
  txMessage.target_brightness = target_brightness;
  txMessage.transition_mode = mode;

  if (xQueueUDP != 0) {
    xQueueSendToBack(xQueueUDP, &txMessage, 100/portTICK_RATE_MS);
  }
}

void mcast_udp_task(void *pvParameters)
{
  xQueueUDP = xQueueCreate( 2, sizeof( x_light_message_t ) );

  while (1) {
    int sock;

    sock = create_multicast_ipv4_socket();
    if (sock < 0) {
        ESP_LOGE(TAG, "Failed to create IPv4 multicast socket");
    }

    if (sock < 0) {
      // Nothing to do!
      vTaskDelay(5 / portTICK_PERIOD_MS);
      continue;
    }

    // set destination multicast addresses for sending from these sockets
    struct sockaddr_in sdestv4 = {
        .sin_family = PF_INET,
        .sin_port = htons(UDP_PORT),
    };

    // We know this inet_aton will pass because we did it above already
    inet_aton(MULTICAST_IPV4_ADDR, &sdestv4.sin_addr.s_addr);

    // Loop waiting for UDP received, and sending UDP packets if we don't
    // see any.
    int err = 1;
    while (err > 0) {
      struct timeval tv = {
          .tv_sec = 2,
          .tv_usec = 0,
      };
      fd_set rfds;
      FD_ZERO(&rfds);
      FD_SET(sock, &rfds);

      int s = select(sock + 1, &rfds, NULL, NULL, &tv);
      if (s < 0) {
        ESP_LOGE(TAG, "Select failed: errno %d", errno);
        err = -1;
        break;
      }
      else if (s > 0) {
        if (FD_ISSET(sock, &rfds)) {
          // Incoming datagram received
          char recvbuf[256];
          char raddr_name[32] = { 0 };

          struct sockaddr_storage raddr; // Large enough for both IPv4 or IPv6
          socklen_t socklen = sizeof(raddr);
          int len = recvfrom(sock, recvbuf, sizeof(recvbuf)-1, 0,
                             (struct sockaddr *)&raddr, &socklen);
          if (len < 0) {
            ESP_LOGE(TAG, "multicast recvfrom failed: errno %d", errno);
            err = -1;
            break;
          }

          // Get the sender's address as a string
          if (raddr.ss_family == PF_INET) {
            inet_ntoa_r(((struct sockaddr_in *)&raddr)->sin_addr, raddr_name, sizeof(raddr_name)-1);
          }
          ESP_LOGD(TAG, "received %d bytes from %s:", len, raddr_name);

          recvbuf[len] = 0; // Null-terminate whatever we received and treat like a string...

          /* parse json --------------------------- */
          uint8_t channel_id = 0;
          double target_duty[MAX_LED_CHANNELS] = {0};
          double target_brightness = 0.0;
          uint8_t target_mode = 0;
          cJSON *root = cJSON_Parse(recvbuf);
          cJSON *channel_item;
          cJSON *channels = cJSON_GetObjectItem(root, "channels");
          if (cJSON_IsArray(channels)) {
            cJSON_ArrayForEach(channel_item, channels) {
              if (cJSON_IsNumber(channel_item)) {
                target_duty[channel_id] = channel_item->valuedouble;
                ESP_LOGD(TAG, "udp rx channel %d new duty %f", channel_id, channel_item->valuedouble);

              }
              channel_id++;
            }
          }
          cJSON *brightness = cJSON_GetObjectItem(root, "brightness");
          if (cJSON_IsNumber(brightness)) {
            target_brightness = brightness->valuedouble;
            ESP_LOGD(TAG, "udp rx new brightness %f", brightness->valuedouble);
          }
          cJSON *mode = cJSON_GetObjectItem(root, "mode");
          if (cJSON_IsNumber(brightness)) {
            target_mode = mode->valueint;
            ESP_LOGI(TAG, "udp rx new mode %d", mode->valueint);
          }

          set_light(target_duty, target_brightness, target_mode, 1);
          cJSON_Delete(root);

          ESP_LOGD(TAG, "%s", recvbuf);
        }
      }
      else { // s == 0
        /* Queue */
        x_light_message_t rxMessage;
        if (xQueueReceive(xQueueUDP, &rxMessage, 100 / portTICK_PERIOD_MS))
        {
          ESP_LOGD(TAG, "new udp queue --------- ");
          ESP_LOGD(TAG, "brightness: %f", rxMessage.target_brightness);

          cJSON * root = cJSON_CreateObject();
          cJSON * channels = cJSON_CreateArray();
          for (int i = 0; i < MAX_LED_CHANNELS; ++i) {
            cJSON_AddItemToArray(channels, cJSON_CreateNumber(rxMessage.target_duty[i]));
          }
          cJSON_AddItemToObject(root, "channels", channels);
          cJSON_AddItemToObject(root, "brightness", cJSON_CreateNumber(rxMessage.target_brightness));
          cJSON_AddItemToObject(root, "mode", cJSON_CreateNumber(rxMessage.transition_mode));

          char * string = cJSON_Print(root);
          cJSON_Delete(root);

          char sendbuf[1024];
          char addrbuf[32] = { 0 };
          int len = snprintf(sendbuf, sizeof(sendbuf), "%s", string);
          if (len > sizeof(sendbuf)) {
            ESP_LOGE(TAG, "Overflowed multicast sendfmt buffer!!");
            err = -1;
            break;
          }
          free(string);

          // Timeout passed with no incoming data, so send something!
//          static int send_count;
//          const char sendfmt[] = "Multicast #%d sent by ESP32\n";
//          char sendbuf[48];
//          char addrbuf[32] = { 0 };
//          int len = snprintf(sendbuf, sizeof(sendbuf), sendfmt, send_count++);
//          if (len > sizeof(sendbuf)) {
//            ESP_LOGE(TAG, "Overflowed multicast sendfmt buffer!!");
//            send_count = 0;
//            err = -1;
//            break;
//          }

          struct addrinfo hints = {
              .ai_flags = AI_PASSIVE,
              .ai_socktype = SOCK_DGRAM,
          };

          struct addrinfo *res;

          hints.ai_family = AF_INET; // For an IPv4 socket
          int err = getaddrinfo(MULTICAST_IPV4_ADDR,
                                NULL,
                                &hints,
                                &res);
          if (err < 0) {
            ESP_LOGE(TAG, "getaddrinfo() failed for IPV4 destination address. error: %d", err);
            break;
          }
          if (res == 0) {
            ESP_LOGE(TAG, "getaddrinfo() did not return any addresses");
            break;
          }

          ((struct sockaddr_in *)res->ai_addr)->sin_port = htons(UDP_PORT);
          inet_ntoa_r(((struct sockaddr_in *)res->ai_addr)->sin_addr, addrbuf, sizeof(addrbuf)-1);
          ESP_LOGI(TAG, "Sending to IPV4 multicast address %s:%d...",  addrbuf, UDP_PORT);

          err = sendto(sock, sendbuf, len, 0, res->ai_addr, res->ai_addrlen);
          freeaddrinfo(res);
          if (err < 0) {
            ESP_LOGE(TAG, "IPV4 sendto failed. errno: %d", errno);
            break;
          }
        }
      }
    }

    ESP_LOGE(TAG, "Shutting down socket and restarting...");
    shutdown(sock, 0);
    close(sock);
  }
}

