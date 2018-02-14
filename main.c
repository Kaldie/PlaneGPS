/*
 * HTTP server example.
 *
 * This sample code is in the public domain.
 */
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <espressif/esp_common.h>
#include <esp8266.h>
#include <esp/uart.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include "updServer.h"
#include "http_server.h"
#include "gyneo6/gyneo6.h"

#define AP_SSID "Cools"
#define AP_PSK "bernard23"

#define STATIONMODE

#define CALLBACK_DEBUG

void user_init(void)
{
    printf("SDK version:%s\n", sdk_system_get_sdk_version());

    uart_set_baud(0, 115200);
    sdk_os_delay_us(500); // Wait UART

#ifdef STATIONMODE

    struct sdk_station_config config = {
        .ssid = AP_SSID,
        .password = AP_PSK,
    };

    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_config(&config);
    sdk_wifi_station_connect();
#else
    struct ip_info ap_ip;
    IP4_ADDR(&ap_ip.ip, 172, 16, 0, 1);
    IP4_ADDR(&ap_ip.gw, 0, 0, 0, 0);
    IP4_ADDR(&ap_ip.netmask, 255, 255, 0, 0);
    sdk_wifi_set_ip_info(1, &ap_ip);

    struct sdk_softap_config ap_config = {
        .ssid = AP_SSID,
        .ssid_hidden = 0,
        .channel = 3,
        .ssid_len = strlen(AP_SSID),
        .authmode = AUTH_WPA_WPA2_PSK,
        .password = AP_PSK,
        .max_connection = 3,
        .beacon_interval = 100,
    };
    sdk_wifi_softap_set_config(&ap_config);

    ip_addr_t first_client_ip;
    IP4_ADDR(&first_client_ip, 172, 16, 0, 2);
    // dhcpserver_start(&first_client_ip, 4);
    printf("DHCP started\n");
#endif

    // create a upd endpoint that will return its ip adres when called
    // initUPD();

    // create the stuff needed for the gps stuff
    gyneo6_init();

    // create a http server
    // xTaskCreate(&httpd_task, "HTTP Daemon", 256, NULL, 2, NULL);
}