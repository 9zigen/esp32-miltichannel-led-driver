//
// Created by Aleksey Volkov on 04.04.2022.
//

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/xtensa_rtos.h"

#include <esp_log.h>

#include "esp_http_server.h"

#include "tools.h"
#include "server.h"
#include "web_socket.h"

static const char *TAG = "WS_SERVER";
extern httpd_handle_t server_handle;

struct async_resp_arg {
  httpd_handle_t hd;
  int fd;
};

void ws_async_send(void *arg)
{
  static const char * data = "Async data";
  struct async_resp_arg *resp_arg = arg;
  httpd_handle_t hd = resp_arg->hd;
  int fd = resp_arg->fd;
  httpd_ws_frame_t ws_pkt;
  memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
  ws_pkt.payload = (uint8_t*)data;
  ws_pkt.len = strlen(data);
  ws_pkt.type = HTTPD_WS_TYPE_TEXT;

  httpd_ws_send_frame_async(hd, fd, &ws_pkt);
  free(resp_arg);
}

//esp_err_t httpd_ws_send_frame_to_all_clients(httpd_ws_frame_t *ws_frame) {
//  static const size_t max_clients = CONFIG_LWIP_MAX_LISTENING_TCP;
//  size_t fds = max_clients;
//  int client_fds[max_clients] = {0};
//
//  esp_err_t ret = httpd_get_client_list(server_handle, &fds, client_fds);
//
//  if (ret != ESP_OK) {
//    return ret;
//  }
//
//  for (int i = 0; i < fds; i++) {
//    int client_info = httpd_ws_get_fd_info(server_handle, client_fds[i]);
//    if (client_info == HTTPD_WS_CLIENT_WEBSOCKET) {
//      httpd_ws_send_frame_async(server_handle, client_fds[i], ws_frame);
//    }
//  }
//
//  return ESP_OK;
//}

static esp_err_t trigger_async_send(httpd_handle_t handle, httpd_req_t *req)
{
  struct async_resp_arg *resp_arg = malloc(sizeof(struct async_resp_arg));
  resp_arg->hd = req->handle;
  resp_arg->fd = httpd_req_to_sockfd(req);
  return httpd_queue_work(handle, ws_async_send, resp_arg);
}

esp_err_t ws_handler(httpd_req_t *req)
{
  if (req->method == HTTP_GET) {
    ESP_LOGD(TAG, "Handshake done, the new connection was opened");
    return ESP_OK;
  }

  httpd_ws_frame_t ws_frame;
  uint8_t *buf = NULL;

  memset(&ws_frame, 0, sizeof(httpd_ws_frame_t));
  ws_frame.type = HTTPD_WS_TYPE_TEXT;

  /* Set max_len = 0 to get the frame len */
  esp_err_t ret = httpd_ws_recv_frame(req, &ws_frame, 0);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
    return ret;
  }

  ESP_LOGD(TAG, "frame len is %d", ws_frame.len);

  if (ws_frame.len) {
    /* ws_pkt.len + 1 is for NULL termination as we are expecting a string */
    buf = calloc(1, ws_frame.len + 1);
    if (buf == NULL) {
      ESP_LOGE(TAG, "Failed to calloc memory for buf");
      return ESP_ERR_NO_MEM;
    }
    ws_frame.payload = buf;
    /* Set max_len = ws_pkt.len to get the frame payload */
    ret = httpd_ws_recv_frame(req, &ws_frame, ws_frame.len);
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
      free(buf);
      return ret;
    }
    ESP_LOGD(TAG, "Got packet with message: %s", ws_frame.payload);
  }

  ESP_LOGD(TAG, "Packet type: %d", ws_frame.type);

//  if (ws_frame.type == HTTPD_WS_TYPE_TEXT && strcmp((char*)ws_frame.payload, "request-data") == 0) {
//    free(buf);
//    return trigger_async_send(req->handle, req);
//  }
//
  uint8_t * data = NULL;

  /* ToDo: channel status */
//  if (ws_frame.type == HTTPD_WS_TYPE_TEXT && strcmp((char*)ws_frame.payload, "get-sensors") == 0) {
//    data = (uint8_t*)get_sensors_json();
//    ws_frame.payload = data;
//    ws_frame.len = strlen((char *)data);
//  }
//
//  if (ws_frame.type == HTTPD_WS_TYPE_TEXT && strcmp((char*)ws_frame.payload, "get-battery") == 0) {
//    data = (uint8_t*)get_battery_json();
//    ws_frame.payload = data;
//    ws_frame.len = strlen((char *)data);
//  }

  ret = httpd_ws_send_frame(req, &ws_frame);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "httpd_ws_send_frame failed with %d", ret);
  }

  free(data);
  free(buf);
  return ret;
}