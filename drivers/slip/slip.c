/* slip.c - SLIP driver using uart_pipe. This is meant for
 * network connectivity between host and qemu. The host will
 * need to run tunslip process.
 */

/*
 * Copyright (c) 2016 Intel Corporation
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

#if defined(CONFIG_SLIP_DEBUG)
#define SYS_LOG_DOMAIN "slip"
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#endif

#include <nanokernel.h>

#include <stdbool.h>
#include <errno.h>
#include <stddef.h>
#include <misc/sys_log.h>
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
	STATE_MULTI_PACKETS,
	STATE_GARBAGE,
	STATE_OK,
	STATE_ESC,
};

struct slip_context {
	uint8_t buf[1];		/* SLIP data is read into this buf */
	struct net_buf *rx;	/* and then placed into this net_buf */
	struct net_buf *last;	/* Pointer to last fragment in the list */
	uint8_t state;

	uint16_t ll_reserve;	/* Reserve any space for link layer headers */
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
static void hexdump(const char *str, const uint8_t *packet, size_t length)
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

		printf("%02X ", *packet++);

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
#define hexdump(str, packet, length)
#endif

static inline void slip_writeb(unsigned char c)
{
	uint8_t buf[1] = { c };

	uart_pipe_send(&buf[0], 1);
}

static int slip_send(struct net_if *iface, struct net_buf *buf)
{
	uint16_t i;
	uint8_t *ptr;
	uint8_t c;

	if (!buf->frags) {
		/* No data? */
		return -ENODATA;
	}

	slip_writeb(SLIP_END);

	while (buf->frags) {
		struct net_buf *frag = buf->frags;

#if defined(CONFIG_SLIP_DEBUG)
		int frag_count = 0;
#endif

		ptr = frag->data;

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
		SYS_LOG_DBG("[%p] sent data %d bytes", iface->dev->driver_data,
			    frag->len);
		if (frag->len) {
			char msg[7 + 1];
			snprintf(msg, sizeof(msg), "slip %d", frag_count++);
			msg[7] = '\0';
			hexdump(msg, frag->data, frag->len);
		}
#endif

		net_buf_frag_del(buf, frag);
		net_buf_unref(frag);
	}

	net_buf_unref(buf);

	slip_writeb(SLIP_END);

	return 0;
}

static struct net_buf *slip_poll_handler(struct slip_context *slip)
{
	if (slip->last && slip->last->len) {
		if (slip->state == STATE_MULTI_PACKETS) {
			/* Assume no bytes where lost */
			slip->state = STATE_OK;
		}

		return slip->rx;
	}

	return NULL;
}

static void process_msg(struct slip_context *slip)
{
	struct net_buf *buf;

	buf = slip_poll_handler(slip);
	if (!buf) {
		return;
	}

#if defined(CONFIG_NET_IPV4)
#error "FIXME - TBDL"
#endif /* IPv4 */

#if defined(CONFIG_NET_IPV6)
	if (buf->frags) {
		if (net_recv(net_if_get_by_link_addr(&slip->ll_addr),
			     buf) < 0) {
			net_nbuf_unref(buf);
		}
		slip->rx = slip->last = NULL;
	}
#endif /* CONFIG_NET_IPV6 */
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

	case STATE_MULTI_PACKETS:
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
			if (slip->last->len) {
				slip->state = STATE_MULTI_PACKETS;
				SLIP_STATS(slip->multi_packets++);
				return 1;
			}
			return 0;
		}
		break;
	}

	if (!slip->rx) {
		slip->rx = net_nbuf_get_reserve_rx(0);
		if (!slip->rx) {
			return 0;
		}
		slip->last = net_nbuf_get_reserve_data(slip->ll_reserve);
		if (!slip->last) {
			net_nbuf_unref(slip->rx);
			slip->rx = NULL;
			return 0;
		}
		net_buf_frag_add(slip->rx, slip->last);
	}

	if (!net_buf_tailroom(slip->last)) {
		/* We need to allocate a new fragment */
		struct net_buf *frag;

		frag = net_nbuf_get_reserve_data(slip->ll_reserve);
		if (!frag) {
			SYS_LOG_ERR("[%p] cannot allocate data fragment",
				    slip);
			net_nbuf_unref(frag);
			net_nbuf_unref(slip->rx);
			slip->rx = NULL;
			slip->last = NULL;
			return 0;
		}
		net_buf_frag_insert(slip->last, frag);
		slip->last = frag;
	}
	net_buf_add_u8(slip->last, c);

	return 0;
}

static uint8_t *recv_cb(uint8_t *buf, size_t *off)
{
	struct slip_context *slip =
		CONTAINER_OF(buf, struct slip_context, buf);
	int i;

	for (i = 0; i < *off; i++) {
		if (slip_input_byte(slip, buf[i])) {
#if defined(CONFIG_SLIP_DEBUG)
			struct net_buf *frag = slip->rx->frags;
			int count = 0;
			int bytes = net_buf_frags_len(slip->rx->frags);

			SYS_LOG_DBG("[%p] received data %d bytes", slip,
				    bytes);
			while (bytes && frag) {
				char msg[7 + 1];
				snprintf(msg, sizeof(msg), "slip %d", count);
				msg[7] = '\0';
				hexdump(msg, frag->data, frag->len);
				frag = frag->frags;
				count++;
			}
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

	dev->driver_api = NULL;

	slip->state = STATE_OK;
	slip->rx = NULL;
	slip->ll_reserve = 0;

	uart_pipe_register(slip->buf, sizeof(slip->buf), recv_cb);

	return 0;
}

static inline struct net_linkaddr *slip_get_mac(struct device *dev)
{
	struct slip_context *slip = dev->driver_data;

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
	struct net_linkaddr *ll_addr = slip_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, ll_addr->addr, ll_addr->len);
}

static struct net_if_api slip_if_api = {
	.init = slip_iface_init,
	.send = slip_send,
};

static struct slip_context slip_context_data;

NET_DEVICE_INIT(slip, CONFIG_SLIP_DRV_NAME, slip_init, &slip_context_data,
		NULL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &slip_if_api, 127);
