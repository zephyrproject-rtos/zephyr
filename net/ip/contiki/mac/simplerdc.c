/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <net/l2_buf.h>

#include "contiki/mac/mac-sequence.h"
#include "contiki/packetbuf.h"
#include "contiki/queuebuf.h"
#include "contiki/netstack.h"
#include "contiki/llsec/anti-replay.h"
#include <string.h>

#ifdef CONFIG_NETWORK_IP_STACK_DEBUG_15_4_MAC
#define DEBUG 1
#endif
#include "contiki/ip/uip-debug.h"

#ifndef SIMPLERDC_MAX_RETRANSMISSIONS
#define SIMPLERDC_MAX_RETRANSMISSIONS	3
#endif

#define ACK_LEN 3

static struct nano_sem ack_lock;
static bool ack_received;

static inline bool handle_ack_packet(struct net_buf *buf)
{
	if (packetbuf_datalen(buf) == ACK_LEN) {
		ack_received = true;
		nano_sem_give(&ack_lock);
#ifdef SIMPLERDC_802154_AUTOACK
		PRINTF("simplerdc: ignore ACK packet\n");
		return true;
#endif
	}

	return false;
}

#ifdef SIMPLERDC_802154_AUTOACK
static inline bool check_duplicate(struct net_buf *buf)
{
	if (mac_sequence_is_duplicate(buf) != 0) {
		PRINTF("simplerdc: duplicated packet seq %u\n",
		       packetbuf_attr(buf, PACKETBUF_ATTR_MAC_SEQNO));
		return true;
	}

	mac_sequence_register_seqno(buf);

	return false;
}
#else
#define check_duplicate(...)	(false)
#endif

#ifdef SIMPLERDC_802154_SEND_ACK
#include "contiki/mac/frame802154.h"

static inline bool send_ack_packet(struct net_buf *buf, uint8_t *status)
{
	struct net_buf *ack_buf;
	frame802154_t frame;

	if (packetbuf_attr(buf, PACKETBUF_ATTR_FRAME_TYPE) !=
	    FRAME802154_DATAFRAME) {
		return false;
	}

	frame802154_parse(packetbuf_dataptr(buf),
			  packetbuf_datalen(buf),
			  &frame);

	if (frame.fcf.ack_required == 0 ||
	    !linkaddr_cmp((linkaddr_t *)&frame.dest_addr,
			  &linkaddr_node_addr)) {
		return false;
	}

	ack_buf = l2_buf_get_reserve(0);
	if (!ack_buf) {
		*status = MAC_TX_ERR;
		return false;
	}

	uip_pkt_packetbuf(ack_buf)[0] = FRAME802154_ACKFRAME;
	uip_pkt_packetbuf(ack_buf)[1] = 0;
	uip_pkt_packetbuf(ack_buf)[2] = frame.seq;

	*status = NETSTACK_RADIO.send(ack_buf, NULL, ACK_LEN);

	l2_buf_unref(ack_buf);

	PRINTF("simplerdc: Send ACK to packet %u\n",
	       packetbuf_attr(buf, PACKETBUF_ATTR_MAC_SEQNO));

	return true;
}
#else
#define send_ack_packet(...)	(false)
#endif

static inline bool prepare_for_ack(struct net_buf *buf)
{
	if (packetbuf_attr(buf, PACKETBUF_ATTR_MAC_ACK) != 0) {
		PRINTF("simplerdc: ACK requested\n");

		nano_sem_init(&ack_lock);
		ack_received = false;

		return true;
	}

	return false;
}

static inline uint8_t wait_for_ack(bool broadcast, bool ack_required)
{
	if (broadcast || !ack_required) {
		return MAC_TX_OK;
	}

	if (nano_sem_take(&ack_lock, MSEC(10)) == 0) {
		nano_sem_init(&ack_lock);
	}

	if (!ack_received) {
		return MAC_TX_NOACK;
	}

	return MAC_TX_OK;
}

static inline uint8_t prepare_packet(struct net_buf *buf)
{
	uint8_t retries;

	retries = packetbuf_attr(buf, PACKETBUF_ATTR_MAX_MAC_TRANSMISSIONS);
	if (retries <= 0) {
		retries = SIMPLERDC_MAX_RETRANSMISSIONS + 1;
	}

	packetbuf_set_addr(buf, PACKETBUF_ADDR_SENDER, &linkaddr_node_addr);

	if (NETSTACK_FRAMER.create_and_secure(buf) < 0) {
		return 0;
	}

	return retries;
}

static void init(void)
{
	NETSTACK_RADIO.on();
}

static uint8_t send_packet(struct net_buf *buf,
			   mac_callback_t sent_callback, void *ptr)
{
	int ret = MAC_TX_OK;
	bool is_broadcast, ack_required;
	uint8_t attempts;
	uint8_t retries;

#ifdef SIMPLERDC_802154_ACK_REQ
	packetbuf_set_attr(buf, PACKETBUF_ATTR_MAC_ACK, 1);
#endif

	retries = prepare_packet(buf);
	if (!retries) {
		return MAC_TX_ERR_FATAL;
	}

	ack_required = prepare_for_ack(buf);
	is_broadcast = !!packetbuf_holds_broadcast(buf);
	attempts = 0;

	while (retries) {
		attempts++;
		retries--;

		ret = NETSTACK_RADIO.transmit(buf, packetbuf_totlen(buf));
		if (ret == RADIO_TX_COLLISION) {
			continue;
		}

		ret = wait_for_ack(is_broadcast, ack_required);
		if (ret == MAC_TX_OK) {
			break;
		}
	}

	mac_call_sent_callback(buf, sent_callback, ptr, ret, attempts);

	if (ret == MAC_TX_OK) {
		return 1;
	}

	return 0;
}

static uint8_t send_packet_list(struct net_buf *buf,
				mac_callback_t sent_callback,
				void *ptr, struct rdc_buf_list *list)
{
	while (list) {
		struct rdc_buf_list *next = list->next;
		int last_sent_ok;

		queuebuf_to_packetbuf(buf, list->buf);

		last_sent_ok = send_packet(buf, sent_callback, ptr);
		if (!last_sent_ok) {
			return 0;
		}

		list = next;
	}

	return 1;
}

static uint8_t input_packet(struct net_buf *buf)
{
	uint8_t ret = 0;
	bool duplicate;

	if (handle_ack_packet(buf)) {
		return 0;
	}

	if (NETSTACK_FRAMER.parse(buf) < 0) {
		PRINTF("simpledc: parsing failed msg len %u\n",
		       packetbuf_datalen(buf));
		return 0;
	}

	duplicate = check_duplicate(buf);

	if (send_ack_packet(buf, &ret)) {
		return ret;
	}

	if (duplicate) {
		return 0;
	}

	return NETSTACK_MAC.input(buf);
}

static int on(void)
{
	return NETSTACK_RADIO.on();
}

static int off(int keep_radio_on)
{
	return NETSTACK_RADIO.off();
}

static unsigned short channel_check_interval(void)
{
	return 0;
}

const struct rdc_driver simplerdc_driver = {
	"simplerdc",
	init,
	send_packet,
	send_packet_list,
	input_packet,
	on,
	off,
	channel_check_interval,
};
