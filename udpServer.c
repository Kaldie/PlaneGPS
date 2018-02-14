#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <espressif/esp_common.h>

#include <esp8266.h>

#include <lwip/udp.h>
#include <lwip/pbuf.h>

#include "updServer.h"

#define ECHO_PORT_1 50432
#define CALLBACK_DEBUG
#ifdef CALLBACK_DEBUG
#define debug(s, ...) printf(__FILE__ ", %d : DEBUG : " s "\r\n", __LINE__, ##__VA_ARGS__)
#else
#define debug(s, ...)
#endif

#define REQUESTSTRING "Where is my PlaneGPS"
#define REQUESTSTRINGLENGTH 20
#define RESPONSESTRING "PlaneGPS is @ %s" // 15 char long

static char responseMessageBuffer[31];
static char inputBuffer[REQUESTSTRINGLENGTH + 1];

static const char *get_my_ip(void)
{
    static char ip[16] = "0.0.0.0";
    ip[0] = 0;
    struct ip_info ipinfo;
    (void)sdk_wifi_get_ip_info(STATION_IF, &ipinfo);
    snprintf(ip, sizeof(ip), IPSTR, IP2STR(&ipinfo.ip));
    return (char *)ip;
}

static void respondOnLocationRequest(struct udp_pcb *pcb)
{
    sprintf(responseMessageBuffer, RESPONSESTRING, get_my_ip());
    struct pbuf *responseBuffer = pbuf_alloc(PBUF_TRANSPORT, strlen(responseMessageBuffer), PBUF_RAM);
    if (!responseBuffer)
    {
        debug("Not able to alloc response buffer");
    }

    memcpy(responseBuffer->payload, responseMessageBuffer, strlen(responseMessageBuffer));
    err_t error = udp_sendto(pcb, responseBuffer, IP4_ADDR_BROADCAST, ECHO_PORT_1);

    if (error < 0)
    {
        debug("Error sending message: %s (%d)\n", lwip_strerr(error), error);
    }
    else
    {
        debug("Sent message '%s'\n", responseMessageBuffer);
    }

    pbuf_free(responseBuffer);
}

static void respondOnUPDMessage(void *a, struct udp_pcb *pcb, struct pbuf *buf, ip_addr_t *addr, uint16_t port)
{
    debug("Callback is called!");
    debug("buf->tot_len = %d, buf->len = %d", buf->tot_len, buf->len);
    debug("buf->len = %d, strlen(REQUESTSTRING) = %d", buf->len, REQUESTSTRINGLENGTH);

    if (buf->len == REQUESTSTRINGLENGTH)
    {
        memcpy(inputBuffer, buf->payload, REQUESTSTRINGLENGTH);
        if (strcmp(inputBuffer, REQUESTSTRING) == 0)
        {
            debug("String is correct");
            respondOnLocationRequest(pcb);
        }
    }
    pbuf_free(buf);
}

void initUPD()
{
    // initialising inputbuffer memory
    memset(inputBuffer, REQUESTSTRINGLENGTH + 1, 0);

    // initialise the lwip udp module
    udp_init();

    // create a udp pcb
    struct udp_pcb *udpConnection = udp_new();

    // bind it to all incoming ips and on echo port 1
    udp_bind(udpConnection, IP_ANY_TYPE, ECHO_PORT_1);

    // register a callback to it will respond to upd packages
    udp_recv(udpConnection, (udp_recv_fn)respondOnUPDMessage, NULL);
}
