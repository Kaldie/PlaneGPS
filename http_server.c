/*
 * HTTP server example.
 *
 * This sample code is in the public domain.
 */
#include <espressif/esp_common.h>
#include <esp8266.h>
#include <esp/uart.h>
#include <string.h>
#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include <ssid_config.h>
#include <httpd/httpd.h>

#include "gyneo6/gyneo6.h"

enum
{
  SSI_GPS_FIX_TIME,
  SSI_LOCATION,
  SSI_SPEED,
  SSI_MAX_SPEED
};

bool sendJSON = false;

int32_t ssi_handler(int32_t iIndex, char *pcInsert, int32_t iInsertLen)
{
  switch (iIndex)
  {
  case SSI_GPS_FIX_TIME:
    gyneo6_currentTimeFix(pcInsert, iInsertLen);
    break;
  case SSI_LOCATION:
    gyneo6_currentLocation(pcInsert, iInsertLen);
    break;
  case SSI_SPEED:
    gyneo6_currentSpeed(pcInsert, iInsertLen);
    break;
  case SSI_MAX_SPEED:
    gyneo6_maxSpeed(pcInsert, iInsertLen);
    break;
  default:
    snprintf(pcInsert, iInsertLen, "N/A");
    break;
  }

  /* Tell the server how many characters to insert */
  return (strlen(pcInsert));
}

char *gpio_index_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
  for (int i = 0; i < iNumParams; i++)
  {
    if (strcmp(pcParam[i], "on") == 0)
    {
      uint8_t gpio_num = atoi(pcValue[i]);
      gpio_enable(gpio_num, GPIO_OUTPUT);
      gpio_write(gpio_num, true);
    }
    else if (strcmp(pcParam[i], "off") == 0)
    {
      uint8_t gpio_num = atoi(pcValue[i]);
      gpio_enable(gpio_num, GPIO_OUTPUT);
      gpio_write(gpio_num, false);
    }
    else if (strcmp(pcParam[i], "toggle") == 0)
    {
      uint8_t gpio_num = atoi(pcValue[i]);
      gpio_enable(gpio_num, GPIO_OUTPUT);
      gpio_toggle(gpio_num);
    }
  }
  return "/index.ssi";
}

char *about_cgi_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
  return "/about.html";
}

char *logging_cgi_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
  return "/logging.html";
}

void websocket_task(void *pvParameter)
{
  struct tcp_pcb *pcb = (struct tcp_pcb *)pvParameter;

  for (;;)
  {
    if (pcb == NULL || pcb->state != ESTABLISHED)
    {
      printf("Connection closed, deleting task\n");
      break;
    }

    if (sendJSON)
    {
      /* Generate response in JSON format */
      char response[85];
      gyneo_info *info = gyneo6_info();
      int len = snprintf(response, sizeof(response),
                         "{\"loc\" : {\"long\":\"%.4f\" , \"lat\":\"%.4f\"},"
                         " \"max\" : \"%.2f\","
                         " \"cur\" : \"%.2f\"}\n",
                         info->longitude.degrees + info->longitude.minutes / 60.0,
                         info->latitude.degrees + info->latitude.minutes / 60.0, // first row, loc
                         info->maxSpeed_km, info->speed_km);

      if (len < sizeof(response))
      {
        websocket_write(pcb, (unsigned char *)response, len, WS_TEXT_MODE);
      }
      else
      {
        printf("Len of output is too big namely: %i", len);
      }
    }

    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

/**
 * This function is called when websocket frame is received.
 *
 * Note: this function is executed on TCP thread and should return as soon
 * as possible.
 */
void websocket_cb(struct tcp_pcb *pcb, uint8_t *data, u16_t data_len, uint8_t mode)
{
  // printf("[websocket_callback]:\n%.*s\n", (int)data_len, (char *)data);

  uint8_t response[3];
  uint16_t val;

  switch (data[0])
  {
  case 'A': // ADC
    /* This should be done on a separate thread in 'real' applications */
    val = (uint16_t)gyneo6_info()->speed_km;
    break;
  case 'B':
    val = (uint16_t)gyneo6_info()->maxSpeed_km;
    break;
  case 'D': // Disable Record
    val = 0xDEAD;
    sendJSON = false;
    break;
  case 'E': // Enable Record
    val = 0xBEEF;
    sendJSON = true;
    break;
  default:
    printf("Unknown command\n");
    val = 0;
    break;
  }

  response[2] = (uint8_t)val;
  response[1] = val >> 8;
  response[0] = data[0];

  websocket_write(pcb, response, 3, WS_BIN_MODE);
}

/**
 * This function is called when new websocket is open and
 * creates a new websocket_task if requested URI equals '/stream'.
 */
void websocket_open_cb(struct tcp_pcb *pcb, const char *uri)
{
  printf("WS URI: %s\n", uri);
  if (!strcmp(uri, "/stream"))
  {
    printf("request for streaming\n");
    xTaskCreate(&websocket_task, "websocket_task", 512, (void *)pcb, 2, NULL);
  }
}

void httpd_task(void *pvParameters)
{
  tCGI pCGIs[] = {
      {"/index", (tCGIHandler)gpio_index_handler},
      {"/about", (tCGIHandler)about_cgi_handler},
      {"/logging", (tCGIHandler)logging_cgi_handler}};

  const char *pcConfigSSITags[] = {
      "gpsFix",   // GPS fix time
      "location", // Current location
      "speed",    // SSI_FREE_HEAP
      "maxSpeed"  // Max speed recorded in this session
  };

  /* register handlers and start the server */
  http_set_cgi_handlers(pCGIs, sizeof(pCGIs) / sizeof(pCGIs[0]));
  http_set_ssi_handler((tSSIHandler)ssi_handler, pcConfigSSITags,
                       sizeof(pcConfigSSITags) / sizeof(pcConfigSSITags[0]));
  websocket_register_callbacks((tWsOpenHandler)websocket_open_cb,
                               (tWsHandler)websocket_cb);
  httpd_init();

  for (;;)
    ;
}
