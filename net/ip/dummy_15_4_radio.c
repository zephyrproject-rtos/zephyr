/* dummy_15_4_radio.c - 802.15.4 radio driver loopbacks Tx frames back to us */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "contiki.h"

#include <net/net_buf.h>

#include "net/packetbuf.h"
#include "net/netstack.h"
#include "dummy_15_4_radio.h"
#include "net_driver_15_4.h"

#include <string.h>

#if UIP_CONF_LOGGING
#define DEBUG DEBUG_FULL
#else
#define DEBUG DEBUG_NONE
#endif
#include "net/ip/uip-debug.h"

#if UIP_LOGGING
#include <stdio.h>
void uip_log(char *msg);
#define UIP_LOG(m) uip_log(m)
#else
#define UIP_LOG(m)
#endif

#define FOOTER_LEN 2
#define NETWORK_TEST_MAX_PACKET_LEN      PACKETBUF_SIZE

static volatile uint16_t last_packet_timestamp;
static uint8_t loopback[NETWORK_TEST_MAX_PACKET_LEN];

/*---------------------------------------------------------------------------*/
static int
init(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
prepare(const void *payload, unsigned short payload_len)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
transmit(struct net_mbuf *buf, unsigned short transmit_len)
{
  return RADIO_TX_OK;
}

static void route_buf(struct net_mbuf *buf)
{
	int len;
	struct net_mbuf *mbuf;

	len = packetbuf_copyto(buf, loopback);
	/* Receiver buffer that is passed to 15.4 Rx fiber */
	PRINTF("dummy154radio: got %d bytes\n", len);

	mbuf = net_mbuf_get_reserve(0);
	if (mbuf) {
		packetbuf_copyfrom(mbuf, loopback, len);
		packetbuf_set_datalen(mbuf, len);
		packetbuf_set_attr(mbuf, PACKETBUF_ATTR_TIMESTAMP,
						last_packet_timestamp);
		PRINTF("dummy154radio: 15.4 Rx input %d bytes\n", len);

		if (net_driver_15_4_recv_from_hw(mbuf) < 0) {
			PRINTF("dummy154radio: rdc input failed, "
							"packet discarded\n");
			net_mbuf_put(mbuf);
		}
	}
}

/*---------------------------------------------------------------------------*/
static int
send(struct net_mbuf *buf, const void *payload, unsigned short payload_len)
{
  route_buf(buf);
  return transmit(buf, payload_len);
}
/*---------------------------------------------------------------------------*/
static int
radio_read(void *buf, unsigned short buf_len)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
channel_clear(void)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
receiving_packet(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
pending_packet(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
on(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
off(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static radio_result_t
get_value(radio_param_t param, radio_value_t *value)
{
  return RADIO_RESULT_NOT_SUPPORTED;
}
/*---------------------------------------------------------------------------*/
static radio_result_t
set_value(radio_param_t param, radio_value_t value)
{
  return RADIO_RESULT_NOT_SUPPORTED;
}
/*---------------------------------------------------------------------------*/
static radio_result_t
get_object(radio_param_t param, void *dest, size_t size)
{
  return RADIO_RESULT_NOT_SUPPORTED;
}
/*---------------------------------------------------------------------------*/
static radio_result_t
set_object(radio_param_t param, const void *src, size_t size)
{
  return RADIO_RESULT_NOT_SUPPORTED;
}
/*---------------------------------------------------------------------------*/
const struct radio_driver dummy154radio_driver =
  {
    .init = init,
    .prepare = prepare,
    .transmit = transmit,
    .send = send,
    .read = radio_read,
    .channel_clear = channel_clear,
    .receiving_packet = receiving_packet,
    .pending_packet = pending_packet,
    .on = on,
    .off = off,
    .get_value = get_value,
    .set_value = set_value,
    .get_object = get_object,
    .set_object = set_object
  };
/*---------------------------------------------------------------------------*/
