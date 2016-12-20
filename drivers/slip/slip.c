/* slip.c - SLIP driver using uart_pipe. This is meant for
 * network connectivity between host and qemu. The host will
 * need to run tunslip process.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_SLIP_DEBUG)
#define SYS_LOG_DOMAIN "slip"
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#include <logging/sys_log.h>
#include <stdio.h>
#endif

#include <kernel.h>

#include <stdbool.h>
#include <errno.h>
#include <stddef.h>
#include <misc/util.h>
#include <net/buf.h>
#include <net/nbuf.h>
#include <net/net_if.h>
#include <net/net_core.h>
#include <console/uart_pipe.h>

#define SLIP_END     0300
#define SLIP_ESC     0333
#define SLIP_ESC_END 0334
#define SLIP_ESC_ESC 0335

enum slip_state {
	STATE_GARBAGE,
	STATE_OK,
	STATE_ESC,
};

struct slip_context {
	bool init_done;
	uint8_t buf[1];		/* SLIP data is read into this buf */
	struct net_buf *rx;	/* and then placed into this net_buf */
	struct net_buf *last;	/* Pointer to last fragment in the list */
	uint8_t *ptr;		/* Where in net_buf to add data */
	uint8_t state;

	uint8_t mac_addr[6];
	struct net_linkaddr ll_addr;

#if defined(CONFIG_SLIP_STATISTICS)
#define SLIP_STATS(statement)
#else
	uint16_t garbage;
	uint16_t multi_packets;
	uint16_t overflows;
	uint16_t ip_drop;
#define SLIP_STATS(statement) statement
#endif
};

#if defined(CONFIG_SLIP_DEBUG)
#if defined(CONFIG_SYS_LOG_SHOW_COLOR)
#define COLOR_OFF     "\x1B[0m"
#define COLOR_YELLOW  "\x1B[0;33m"
#else
#define COLOR_OFF     ""
#define COLOR_YELLOW  ""
#endif

static void hexdump(const char *str, const uint8_t *packet,
		    size_t length, size_t ll_reserve)
{
	int n = 0;

	if (!length) {
		SYS_LOG_DBG("%s zero-length packet", str);
		return;
	}

	while (length--) {
		if (n % 16 == 0) {
			printf("%s %08X ", str, n);
		}

#if defined(CONFIG_SYS_LOG_SHOW_COLOR)
		if (n < ll_reserve) {
			printf(COLOR_YELLOW);
		} else {
			printf(COLOR_OFF);
		}
#endif
		printf("%02X ", *packet++);

#if defined(CONFIG_SYS_LOG_SHOW_COLOR)
		if (n < ll_reserve) {
			printf(COLOR_OFF);
		}
#endif
		n++;
		if (n % 8 == 0) {
			if (n % 16 == 0) {
				printf("\n");
			} else {
				printf(" ");
			}
		}
	}

	if (n % 16) {
		printf("\n");
	}
}
#else
#define hexdump(slip, str, packet, length, ll_reserve)
#endif

static inline void slip_writeb(unsigned char c)
{
	uint8_t buf[1] = { c };

	uart_pipe_send(&buf[0], 1);
}

static int slip_send(struct net_if *iface, struct net_buf *buf)
{
	struct net_buf *frag;
#if defined(CONFIG_SLIP_TAP)
	uint16_t ll_reserve = net_nbuf_ll_reserve(buf);
	bool send_header_once = false;
#endif
	uint8_t *ptr;
	uint16_t i;
	uint8_t c;

	if (!buf->frags) {
		/* No data? */
		return -ENODATA;
	}

	slip_writeb(SLIP_END);

	for (frag = buf->frags; frag; frag = frag->frags) {
#if defined(CONFIG_SLIP_DEBUG)
		int frag_count = 0;
#endif

#if defined(CONFIG_SLIP_TAP)
		ptr = frag->data - ll_reserve;

		/* This writes ethernet header */
		if (!send_header_once && ll_reserve) {
			for (i = 0; i < ll_reserve; i++) {
				slip_writeb(*ptr++);
			}
		}

		if (net_if_get_mtu(iface) > net_buf_headroom(frag)) {
			/* Do not add link layer header if the mtu is bigger
			 * than fragment size. The first packet needs the
			 * link layer header always.
			 */
			send_header_once = true;
			ll_reserve = 0;
			ptr = frag->data;
		}
#else
		/* There is no ll header in tun device */
		ptr = frag->data;
#endif

		for (i = 0; i < frag->len; ++i) {
			c = *ptr++;
			if (c == SLIP_END) {
				slip_writeb(SLIP_ESC);
				c = SLIP_ESC_END;
			} else if (c == SLIP_ESC) {
				slip_writeb(SLIP_ESC);
				c = SLIP_ESC_ESC;
			}

			slip_writeb(c);
		}

#if defined(CONFIG_SLIP_DEBUG)
		SYS_LOG_DBG("sent data %d bytes",
			    frag->len + net_nbuf_ll_reserve(buf));
		if (frag->len + ll_reserve) {
			char msg[8 + 1];

			snprintf(msg, sizeof(msg), "<slip %2d", frag_count++);

			hexdump(msg, net_nbuf_ll(buf),
				frag->len + net_nbuf_ll_reserve(buf),
				net_nbuf_ll_reserve(buf));
		}
#endif
	}

	net_nbuf_unref(buf);
	slip_writeb(SLIP_END);

	return 0;
}

static struct net_buf *slip_poll_handler(struct slip_context *slip)
{
	if (slip->last && slip->last->len) {
		return slip->rx;
	}

	return NULL;
}

static void process_msg(struct slip_context *slip)
{
	struct net_buf *buf;

	buf = slip_poll_handler(slip);
	if (!buf || !buf->frags) {
		return;
	}

	if (net_recv_data(net_if_get_by_link_addr(&slip->ll_addr), buf) < 0) {
		net_nbuf_unref(buf);
	}

	slip->rx = slip->last = NULL;
}

static inline int slip_input_byte(struct slip_context *slip,
				  unsigned char c)
{
	switch (slip->state) {
	case STATE_GARBAGE:
		if (c == SLIP_END) {
			slip->state = STATE_OK;
		}

		return 0;
	case STATE_ESC:
		if (c == SLIP_ESC_END) {
			c = SLIP_END;
		} else if (c == SLIP_ESC_ESC) {
			c = SLIP_ESC;
		} else {
			slip->state = STATE_GARBAGE;
			SLIP_STATS(slip->garbage++);
			return 0;
		}

		slip->state = STATE_OK;

		break;
	case STATE_OK:
		if (c == SLIP_ESC) {
			slip->state = STATE_ESC;
			return 0;
		} else if (c == SLIP_END) {
			slip->state = STATE_OK;
			return 1;
		}

		break;
	}

	if (!slip->rx) {
		slip->rx = net_nbuf_get_reserve_rx(0);
		if (!slip->rx) {
			return 0;
		}

		slip->last = net_nbuf_get_reserve_data(0);
		if (!slip->last) {
			net_nbuf_unref(slip->rx);
			slip->rx = NULL;
			return 0;
		}

		net_buf_frag_add(slip->rx, slip->last);
		slip->ptr = net_nbuf_ip_data(slip->rx);
	}

	if (!net_buf_tailroom(slip->last)) {
		/* We need to allocate a new fragment */
		struct net_buf *frag;

		frag = net_nbuf_get_reserve_data(0);
		if (!frag) {
			SYS_LOG_ERR("[%p] cannot allocate data fragment",
				    slip);
			net_nbuf_unref(slip->rx);
			slip->rx = NULL;
			slip->last = NULL;

			return 0;
		}

		net_buf_frag_insert(slip->last, frag);
		slip->last = frag;
		slip->ptr = slip->last->data;
	}

	/* The net_buf_add_u8() cannot add data to ll header so we need
	 * a way to do it.
	 */
	if (slip->ptr < slip->last->data) {
		*slip->ptr = c;
	} else {
		slip->ptr = net_buf_add_u8(slip->last, c);
	}

	slip->ptr++;

	return 0;
}

static uint8_t *recv_cb(uint8_t *buf, size_t *off)
{
	struct slip_context *slip =
		CONTAINER_OF(buf, struct slip_context, buf);
	size_t i;

	if (!slip->init_done) {
		*off = 0;
		return buf;
	}

	for (i = 0; i < *off; i++) {
		if (slip_input_byte(slip, buf[i])) {
#if defined(CONFIG_SLIP_DEBUG)
			struct net_buf *frag = slip->rx->frags;
			int bytes = net_buf_frags_len(frag);
			int count = 0;

			while (bytes && frag) {
				char msg[8 + 1];

				snprintf(msg, sizeof(msg), ">slip %2d", count);

				hexdump(msg, frag->data, frag->len, 0);

				frag = frag->frags;
				count++;
			}

			SYS_LOG_DBG("[%p] received data %d bytes", slip, bytes);
#endif
			process_msg(slip);
			break;
		}
	}

	*off = 0;

	return buf;
}

static int slip_init(struct device *dev)
{
	struct slip_context *slip = dev->driver_data;

	SYS_LOG_DBG("[%p] dev %p", slip, dev);

	slip->state = STATE_OK;
	slip->rx = NULL;

#if defined(CONFIG_SLIP_TAP) && defined(CONFIG_NET_IPV4)
	SYS_LOG_DBG("ARP enabled");
#endif

	uart_pipe_register(slip->buf, sizeof(slip->buf), recv_cb);

	return 0;
}

static inline struct net_linkaddr *slip_get_mac(struct slip_context *slip)
{
	if (slip->mac_addr[0] == 0x00) {
		/* 10-00-00-00-00 to 10-00-00-00-FF Documentation RFC7042 */
		slip->mac_addr[0] = 0x10;
		slip->mac_addr[1] = 0x00;
		slip->mac_addr[2] = 0x00;

		slip->mac_addr[3] = 0x00;
		slip->mac_addr[4] = 0x00;
		slip->mac_addr[5] = sys_rand32_get();
	}

	slip->ll_addr.addr = slip->mac_addr;
	slip->ll_addr.len = sizeof(slip->mac_addr);

	return &slip->ll_addr;
}

static void slip_iface_init(struct net_if *iface)
{
	struct slip_context *slip = net_if_get_device(iface)->driver_data;
	struct net_linkaddr *ll_addr = slip_get_mac(slip);

	slip->init_done = true;

	net_if_set_link_addr(iface, ll_addr->addr, ll_addr->len);
}

static struct net_if_api slip_if_api = {
	.init = slip_iface_init,
	.send = slip_send,
};

static struct slip_context slip_context_data;

#if defined(CONFIG_SLIP_TAP) && defined(CONFIG_NET_L2_ETHERNET)
#define _SLIP_L2_LAYER ETHERNET_L2
#define _SLIP_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(ETHERNET_L2)
#define _SLIP_MTU 1500
#else
#define _SLIP_L2_LAYER DUMMY_L2
#define _SLIP_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)
#define _SLIP_MTU 576
#endif

NET_DEVICE_INIT(slip, CONFIG_SLIP_DRV_NAME, slip_init, &slip_context_data,
		NULL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &slip_if_api,
		_SLIP_L2_LAYER, _SLIP_L2_CTX_TYPE, _SLIP_MTU);
