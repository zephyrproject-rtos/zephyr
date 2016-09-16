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

#include <zephyr.h>
#include <tc_util.h>

#include <net/net_ip.h>
#include <net/nbuf.h>

#include <ieee802154_frame.h>
#include <ipv6.h>

struct ieee802154_pkt_test {
	char *name;
	struct in6_addr src;
	struct in6_addr dst;
	uint8_t *pkt;
	uint8_t length;
	struct {
		struct ieee802154_fcf_seq *fc_seq;
		struct ieee802154_address_field *dst_addr;
		struct ieee802154_address_field *src_addr;
	} mhr_check;
};

uint8_t ns_pkt[] = {
	0x41, 0xd8, 0x3e, 0xcd, 0xab, 0xff, 0xff, 0xc2, 0xa3, 0x9e, 0x00,
	0x00, 0x4b, 0x12, 0x00, 0x7b, 0x09, 0x3a, 0x20, 0x01, 0x0d, 0xb8,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x02, 0x01, 0xff, 0x00, 0x00, 0x01, 0x87, 0x00, 0x2e, 0xad,
	0x00, 0x00, 0x00, 0x00, 0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x02,
	0x00, 0x12, 0x4b, 0x00, 0x00, 0x9e, 0xa3, 0xc2, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x3d, 0x74
};

struct ieee802154_pkt_test test_ns_pkt = {
	.name = "NS frame",
	.src =  { { { 0x20, 0x01, 0xdb, 0x08, 0x00, 0x00, 0x00, 0x00,
		      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 } } },
	.dst =  { { { 0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		      0x00, 0x00, 0x00, 0x01, 0xff, 0x00, 0x00, 0x01 } } },
	.pkt = ns_pkt,
	.length = sizeof(ns_pkt),
	.mhr_check.fc_seq = (struct ieee802154_fcf_seq *)ns_pkt,
	.mhr_check.dst_addr = (struct ieee802154_address_field *)(ns_pkt + 3),
	.mhr_check.src_addr = (struct ieee802154_address_field *)(ns_pkt + 7),
};

uint8_t ack_pkt[] = { 0x02, 0x10, 0x16, 0xa2, 0x97 };

struct ieee802154_pkt_test test_ack_pkt = {
	.name = "ACK frame",
	.pkt = ack_pkt,
	.length = sizeof(ack_pkt),
	.mhr_check.fc_seq = (struct ieee802154_fcf_seq *)ack_pkt,
	.mhr_check.dst_addr = NULL,
	.mhr_check.src_addr = NULL,
};

uint8_t beacon_pkt[] = {
	0x00, 0xd0, 0x11, 0xcd, 0xab, 0xc2, 0xa3, 0x9e, 0x00, 0x00, 0x4b,
	0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

struct ieee802154_pkt_test test_beacon_pkt = {
	.name = "Empty beacon frame",
	.pkt = beacon_pkt,
	.length = sizeof(beacon_pkt),
	.mhr_check.fc_seq = (struct ieee802154_fcf_seq *)beacon_pkt,
	.mhr_check.dst_addr = NULL,
	.mhr_check.src_addr =
	(struct ieee802154_address_field *) (beacon_pkt + 3),
};

struct net_buf *current_buf;
struct nano_sem driver_lock;
struct net_if *iface;

static void pkt_hexdump(uint8_t *pkt, uint8_t length)
{
	int i;

	TC_PRINT(" -> Packet content:\n");

	for (i = 0; i < length;) {
		int j;

		TC_PRINT("\t");

		for (j = 0; j < 10 && i < length; j++, i++) {
			TC_PRINT("%02x ", *pkt);
			pkt++;
		}

		TC_PRINT("\n");
	}
}

static void ieee_addr_hexdump(uint8_t *addr, uint8_t length)
{
	int i;

	TC_PRINT(" -> IEEE 802.15.4 Address: ");

	for (i = 0; i < length-1; i++) {
		TC_PRINT("%02x:", *addr);
		addr++;
	}

	TC_PRINT("%02x\n", *addr);
}

static inline int test_packet_parsing(struct ieee802154_pkt_test *t)
{
	struct ieee802154_mpdu mpdu;

	TC_PRINT("- Parsing packet 0x%p of frame %s\n", t->pkt, t->name);

	if (!ieee802154_validate_frame(t->pkt, t->length, &mpdu)) {
		TC_ERROR("*** Could not validate frame %s\n", t->name);
		return TC_FAIL;
	}

	if (mpdu.mhr.fs != t->mhr_check.fc_seq ||
	    mpdu.mhr.dst_addr != t->mhr_check.dst_addr ||
	    mpdu.mhr.src_addr != t->mhr_check.src_addr) {
		TC_PRINT("d: %p vs %p -- s: %p vs %p\n",
			 mpdu.mhr.dst_addr, t->mhr_check.dst_addr,
			 mpdu.mhr.src_addr, t->mhr_check.src_addr);
		TC_ERROR("*** Wrong MPDU information on frame %s\n",
			 t->name);

		return TC_FAIL;
	}

	return TC_PASS;
}

static inline int test_ns_sending(struct ieee802154_pkt_test *t)
{
	struct ieee802154_mpdu mpdu;

	TC_PRINT("- Sending NS packet\n");

	if (net_ipv6_send_ns(iface, NULL, &t->src, &t->dst, &t->dst, false)) {
		TC_ERROR("*** Could not create IPv6 NS packet\n");
		return TC_FAIL;
	}

	nano_sem_take(&driver_lock, MSEC(10));

	if (!current_buf) {
		TC_ERROR("*** Could not send IPv6 NS packet\n");
		return TC_FAIL;
	}

	pkt_hexdump(net_nbuf_ll(current_buf), net_buf_frags_len(current_buf));

	if (!ieee802154_validate_frame(net_nbuf_ll(current_buf),
				       net_buf_frags_len(current_buf), &mpdu)) {
		TC_ERROR("*** Sent packet is not valid\n");
		net_buf_unref(current_buf);

		return TC_FAIL;
	}

	net_buf_unref(current_buf);
	current_buf = NULL;

	return TC_PASS;
}

static inline int test_ack_reply(struct ieee802154_pkt_test *t)
{
	static uint8_t data_pkt[] = {
		0x61, 0xdc, 0x16, 0xcd, 0xab, 0x26, 0x11, 0x32, 0x00, 0x00, 0x4b,
		0x12, 0x00, 0x26, 0x18, 0x32, 0x00, 0x00, 0x4b, 0x12, 0x00, 0x7b,
		0x00, 0x3a, 0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x20, 0x01, 0x0d, 0xb8,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x02, 0x87, 0x00, 0x8b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x01,
		0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
		0x16, 0xf0, 0x02, 0xff, 0x16, 0xf0, 0x12, 0xff, 0x16, 0xf0, 0x32,
		0xff, 0x16, 0xf0, 0x00, 0xff, 0x16, 0xf0, 0x00, 0xff, 0x16
	};
	struct ieee802154_mpdu mpdu;
	struct net_buf *buf, *frag;

	TC_PRINT("- Sending ACK reply to a data packet\n");

	buf = net_nbuf_get_reserve_rx(0);
	frag = net_nbuf_get_reserve_rx(0);

	memcpy(frag->data, data_pkt, sizeof(data_pkt));
	frag->len = sizeof(data_pkt);

	net_buf_frag_add(buf, frag);

	net_recv_data(iface, buf);

	nano_sem_take(&driver_lock, MSEC(20));

	/* an ACK packet should be in current_buf */
	if (!current_buf) {
		TC_ERROR("*** No ACK reply sent\n");
		return TC_FAIL;
	}

	pkt_hexdump(net_nbuf_ll(current_buf), net_buf_frags_len(current_buf));

	if (!ieee802154_validate_frame(net_nbuf_ll(current_buf),
				       net_buf_frags_len(current_buf), &mpdu)) {
		TC_ERROR("*** ACK Reply is invalid\n");
		return TC_FAIL;
	}

	if (memcmp(mpdu.mhr.fs, t->mhr_check.fc_seq,
		   sizeof(struct ieee802154_fcf_seq))) {
		TC_ERROR("*** ACK Reply does not compare\n");
		return TC_FAIL;
	}

	return TC_PASS;
}

static inline int initialize_test_environment(void)
{
	struct device *dev;

	nano_sem_init(&driver_lock);

	current_buf = NULL;

	dev = device_get_binding("fake_ieee802154");
	if (!dev) {
		TC_ERROR("*** Could not get fake device\n");
		return TC_FAIL;
	}

	iface = net_if_lookup_by_dev(dev);
	if (!iface) {
		TC_ERROR("*** Could not get fake iface\n");
		return TC_FAIL;
	}

	TC_PRINT("Fake IEEE 802.15.4 network interface ready\n");

	ieee_addr_hexdump(iface->link_addr.addr, 8);

	return TC_PASS;
}

void main(void)
{
	int status = TC_FAIL;

	TC_PRINT("Starting ieee802154 stack test\n");

	if (initialize_test_environment() != TC_PASS) {
		goto end;
	}

	if (test_packet_parsing(&test_ns_pkt) != TC_PASS) {
		goto end;
	}

	if (test_ns_sending(&test_ns_pkt) != TC_PASS) {
		goto end;
	}

	if (test_packet_parsing(&test_ack_pkt) != TC_PASS) {
		goto end;
	}

	if (test_ack_reply(&test_ack_pkt) != TC_PASS) {
		goto end;
	}

	if (test_packet_parsing(&test_beacon_pkt) != TC_PASS) {
		goto end;
	}

	status = TC_PASS;

end:
	TC_END_RESULT(status);
	TC_END_REPORT(status);
}
