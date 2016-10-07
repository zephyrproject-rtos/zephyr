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

/**
 * @file
 * @brief Sample app for CDC ACM class driver
 *
 * Sample app for USB CDC ACM class driver. The received data is echoed back
 * to the serial port.
 */

#include <stdio.h>
#include <string.h>
#include <device.h>
#include <uart.h>
#include <zephyr.h>
#include <stdio.h>

#include <net/buf.h>

#define SYS_LOG_DOMAIN "main"
#include <misc/sys_log.h>

#include <net_private.h>

#include <net/ieee802154_radio.h>

#define SLIP_END     0300
#define SLIP_ESC     0333
#define SLIP_ESC_END 0334
#define SLIP_ESC_ESC 0335

enum slip_state {
	STATE_GARBAGE,
	STATE_OK,
	STATE_ESC,
};

/* RX queue */
static struct nano_fifo rx_queue;
static char __noinit __stack rx_fiber_stack[1024];
static nano_thread_id_t rx_fiber_id;

/* TX queue */
static struct nano_sem tx_sem;
static struct nano_fifo tx_queue;
static char __noinit __stack tx_fiber_stack[1024];
static nano_thread_id_t tx_fiber_id;

/* Buffer for SLIP encoded data */
/* FIXME size */
static uint8_t slip_buf[1000];

/* ieee802.15.4 device */
static struct ieee802154_radio_api *radio_api;
static struct device *ieee802154_dev;
uint8_t mac_addr[8];

/* UART device */
static struct device *uart_dev;

/* SLIP state machine */
static uint8_t slip_state = STATE_OK;

static struct net_buf *pkt_curr;

/* General helpers */

static void hexdump(const char *str, const uint8_t *packet, size_t length)
{
	int n = 0;

	if (!length) {
		printf("%s zero-length signal packet\n", str);
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

static int slip_process_byte(unsigned char c)
{
	struct net_buf *buf;
#if VERBOSE_DEBUG
	SYS_LOG_DBG("recv: state %u byte %x", slip_state, c);
#endif
	switch (slip_state) {
	case STATE_GARBAGE:
		if (c == SLIP_END) {
			slip_state = STATE_OK;
		}
		SYS_LOG_DBG("garbage: discard byte %x", c);
		return 0;

	case STATE_ESC:
		if (c == SLIP_ESC_END) {
			c = SLIP_END;
		} else if (c == SLIP_ESC_ESC) {
			c = SLIP_ESC;
		} else {
			slip_state = STATE_GARBAGE;
			return 0;
		}
		slip_state = STATE_OK;
		break;

	case STATE_OK:
		if (c == SLIP_ESC) {
			slip_state = STATE_ESC;
			return 0;
		} else if (c == SLIP_END) {
			return 1;
		}
		break;
	}

#if VERBOSE_DEBUG
	SYS_LOG_DBG("processed: state %u byte %x", slip_state, c);
#endif

	if (!pkt_curr) {
		pkt_curr = net_nbuf_get_reserve_rx(0);
		if (!pkt_curr) {
			SYS_LOG_ERR("No more buffers");
			return 0;
		}
		buf = net_nbuf_get_reserve_data(0);
		if (!buf) {
			SYS_LOG_ERR("No more buffers");
			net_nbuf_unref(pkt_curr);
			return 0;
		}
		net_buf_frag_insert(pkt_curr, buf);
	} else {
		buf = net_buf_frag_last(pkt_curr);
	}

	if (!net_buf_tailroom(buf)) {
		SYS_LOG_ERR("No more buf space: buf %p len %u", buf, buf->len);

		net_nbuf_unref(pkt_curr);
		pkt_curr = NULL;
		return 0;
	}

	net_buf_add_u8(buf, c);

	return 0;
}

static void interrupt_handler(struct device *dev)
{
	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
#if VERBOSE_DEBUG
		SYS_LOG_DBG("");
#endif
		if (uart_irq_tx_ready(dev)) {
			SYS_LOG_DBG("TX ready interrupt");

			nano_sem_give(&tx_sem);
		}

		if (uart_irq_rx_ready(dev)) {
			unsigned char byte;

			SYS_LOG_DBG("RX ready interrupt");

			while (uart_fifo_read(dev, &byte, sizeof(byte))) {
				if (slip_process_byte(byte)) {
					SYS_LOG_DBG("Full packet %p", pkt_curr);

					net_buf_put(&rx_queue, pkt_curr);

					pkt_curr = NULL;
				}
			}
		}
	}
}

/* Allocate and send data to USB Host */
static void send_data(uint8_t *cfg, uint8_t *data, size_t len)
{
	struct net_buf *buf, *pkt;

	pkt = net_nbuf_get_reserve_rx(0);
	if (!pkt) {
		SYS_LOG_DBG("No ctrl buf available");
		return;
	}

	buf = net_nbuf_get_reserve_data(0);
	if (!buf) {
		SYS_LOG_DBG("No buf available");
	}

	net_buf_frag_insert(pkt, buf);

	SYS_LOG_DBG("queue pkt %p buf %p len %u", pkt, buf, len);

	/* Add configuration id */
	memcpy(net_buf_add(buf, 2), cfg, 2);

	memcpy(net_buf_add(buf, len), data, len);

	/* simulate LQI */
	net_buf_add(buf, 1);
	/* simulate FCS */
	net_buf_add(buf, 2);

	net_buf_put(&tx_queue, pkt);
}

static void get_ieee_addr(void)
{
	uint8_t cfg[2] = { '!', 'M' };
	uint8_t mac[8];

	SYS_LOG_DBG("");

	/* Send in BE */
	sys_memcpy_swap(mac, mac_addr, sizeof(mac));

	send_data(cfg, mac, sizeof(mac));
}

static void process_request(struct net_buf *buf)
{
	uint8_t cmd = net_buf_pull_u8(buf);


	switch (cmd) {
	case 'M':
		get_ieee_addr();
		break;
	default:
		SYS_LOG_ERR("Not handled request %c", cmd);
		break;
	}
}

static void send_pkt_report(uint8_t seq, uint8_t status, uint8_t num_tx)
{
	uint8_t cfg[2] = { '!', 'R' };
	uint8_t report[3];

	report[0] = seq;
	report[1] = status;
	report[2] = num_tx;

	send_data(cfg, report, sizeof(report));
}

static void process_data(struct net_buf *pkt)
{
	struct net_buf *buf = net_buf_frag_last(pkt);
	uint8_t seq, num_attr;
	int ret, i;

	seq = net_buf_pull_u8(buf);
	num_attr = net_buf_pull_u8(buf);

	SYS_LOG_DBG("seq %u num_attr %u", seq, num_attr);

	/**
	 * There are some attributes sent over this protocol
	 * discard them and return packet data report.
	 */

	for (i = 0; i < num_attr; i++) {
		/* attr */
		net_buf_pull_u8(buf);
		/* value */
		net_buf_pull_be16(buf);
	}

	/* Transmit data through radio */
	ret = radio_api->tx(ieee802154_dev, pkt);
	if (ret) {
		SYS_LOG_ERR("Error transmit data");
	}

	/* TODO: Return correct status codes */
	/* TODO: Implement re-transmissions if needed */

	/* Send packet data report */
	send_pkt_report(seq, ret, 1);
}

static void set_channel(uint8_t chan)
{
	SYS_LOG_DBG("Set channel %c", chan);

	radio_api->set_channel(ieee802154_dev, chan);
}

static void process_config(struct net_buf *pkt)
{
	struct net_buf *buf = net_buf_frag_last(pkt);
	uint8_t cmd = net_buf_pull_u8(buf);

	SYS_LOG_DBG("Process config %c", cmd);

	switch (cmd) {
	case 'S':
		process_data(pkt);
		break;
	case 'C':
		set_channel(net_buf_pull_u8(buf));
		break;
	default:
		SYS_LOG_ERR("Unhandled cmd %u", cmd);
	}
}

static void rx_fiber(void)
{
	SYS_LOG_INF("RX fiber started");

	while (1) {
		struct net_buf *pkt, *buf;
		uint8_t specifier;

		pkt = net_buf_get_timeout(&rx_queue, 0, TICKS_UNLIMITED);
		buf = net_buf_frag_last(pkt);

		SYS_LOG_DBG("Got pkt %p buf %p", pkt, buf);

		hexdump("SLIP >", buf->data, buf->len);

		/* TODO: process */
		specifier = net_buf_pull_u8(buf);
		switch (specifier) {
		case '?':
			process_request(buf);
			break;
		case '!':
			process_config(pkt);
			break;
		default:
			SYS_LOG_ERR("Unknown message specifier %c", specifier);
			break;
		}

		net_nbuf_unref(pkt);

		fiber_yield();
	}
}

static size_t slip_buffer(uint8_t *sbuf, struct net_buf *buf)
{
	size_t len = buf->len;
	uint8_t *sbuf_orig = sbuf;
	int i;

	/**
	 * This strange protocol does not require send START
	 * *sbuf++ = SLIP_END;
	 */

	for (i = 0; i < len; i++) {
		uint8_t byte = net_buf_pull_u8(buf);

		switch (byte) {
		case SLIP_END:
			*sbuf++ = SLIP_ESC;
			*sbuf++ = SLIP_ESC_END;
			break;
		case SLIP_ESC:
			*sbuf++ = SLIP_ESC;
			*sbuf++ = SLIP_ESC_ESC;
			break;
		default:
			*sbuf++ = byte;
		}
	}

	*sbuf++ = SLIP_END;

	return sbuf - sbuf_orig;
}

/**
 * TX - transmit to SLIP interface
 */
static void tx_fiber(void)
{
	SYS_LOG_INF("TX fiber started");

	/* Allow to send one TX */
	nano_sem_give(&tx_sem);

	while (1) {
		struct net_buf *pkt, *buf;
		size_t len;

		nano_sem_take(&tx_sem, TICKS_UNLIMITED);

		pkt = net_buf_get_timeout(&tx_queue, 0, TICKS_UNLIMITED);
		buf = net_buf_frag_last(pkt);
		len = net_buf_frags_len(pkt);

		SYS_LOG_DBG("Send pkt %p buf %p len %d", pkt, buf, len);

		hexdump("SLIP <", buf->data, buf->len);

		/* Remove LQI */
		/* TODO: Reuse get_lqi() */
		buf->len -= 1;

		/* remove FCS 2 bytes */
		buf->len -= 2;

		/* SLIP encode and send */
		len = slip_buffer(slip_buf, buf);
		uart_fifo_fill(uart_dev, slip_buf, len);

		net_nbuf_unref(pkt);

#if 0
		fiber_yield();
#endif
	}
}

static void init_rx_queue(void)
{
	nano_fifo_init(&rx_queue);

	rx_fiber_id = fiber_start(rx_fiber_stack, sizeof(rx_fiber_stack),
				  (nano_fiber_entry_t)rx_fiber,
				  0, 0, 8, 0);
}

static void init_tx_queue(void)
{
	nano_sem_init(&tx_sem);
	nano_fifo_init(&tx_queue);

	tx_fiber_id = fiber_start(tx_fiber_stack, sizeof(tx_fiber_stack),
				  (nano_fiber_entry_t)tx_fiber,
				  0, 0, 8, 0);
}

/**
 * FIXME choose correct OUI, or add support in L2
 */
static uint8_t *get_mac(struct device *dev)
{
	uint32_t *ptr = (uint32_t *)mac_addr;

	mac_addr[7] = 0x00;
	mac_addr[6] = 0x12;
	mac_addr[5] = 0x4b;

	mac_addr[4] = 0x00;
	UNALIGNED_PUT(sys_rand32_get(), ptr);

	mac_addr[0] = (mac_addr[0] & ~0x01) | 0x02;

	return mac_addr;
}

static bool init_ieee802154(void)
{
	uint16_t short_addr;

	SYS_LOG_INF("Initialize ieee802.15.4");

	ieee802154_dev = device_get_binding(CONFIG_TI_CC2520_DRV_NAME);
	if (!ieee802154_dev) {
		SYS_LOG_ERR("Cannot get CC250 device");
		return false;
	}

	radio_api = (struct ieee802154_radio_api *)ieee802154_dev->driver_api;

	/**
	 * Do actual initialization of the chip
	 */
	get_mac(ieee802154_dev);

#ifdef CONFIG_NET_L2_IEEE802154_ORFD
	SYS_LOG_INF("Set panid %x channel %d",
		    CONFIG_NET_L2_IEEE802154_ORFD_PAN_ID,
		    CONFIG_NET_L2_IEEE802154_ORFD_CHANNEL);

	radio_api->set_pan_id(ieee802154_dev,
			      CONFIG_NET_L2_IEEE802154_ORFD_PAN_ID);
	radio_api->set_channel(ieee802154_dev,
			       CONFIG_NET_L2_IEEE802154_ORFD_CHANNEL);
#endif /* CONFIG_NET_L2_IEEE802154_ORFD */

	/* Set short address */
	short_addr = (mac_addr[0] << 8) + mac_addr[1];
	radio_api->set_short_addr(ieee802154_dev, short_addr);

	/* Set ieee address */
	radio_api->set_ieee_addr(ieee802154_dev, mac_addr);

	/* Start ieee802154 */
	radio_api->start(ieee802154_dev);

	return true;
}

void main(void)
{
	struct device *dev;
	uint32_t baudrate, dtr = 0;
	int ret;

	dev = device_get_binding(CONFIG_CDC_ACM_PORT_NAME);
	if (!dev) {
		SYS_LOG_ERR("CDC ACM device not found");
		return;
	}

	SYS_LOG_DBG("Wait for DTR");

	while (1) {
		uart_line_ctrl_get(dev, LINE_CTRL_DTR, &dtr);
		if (dtr)
			break;
	}

	uart_dev = dev;

	SYS_LOG_DBG("DTR set, continue");

#if CONFIG_DCD_DSR
	/* They are optional, we use them to test the interrupt endpoint */
	ret = uart_line_ctrl_set(dev, LINE_CTRL_DCD, 1);
	if (ret)
		printf("Failed to set DCD, ret code %d\n", ret);

	ret = uart_line_ctrl_set(dev, LINE_CTRL_DSR, 1);
	if (ret)
		printf("Failed to set DSR, ret code %d\n", ret);

	/* Wait 1 sec for the host to do all settings */
	sys_thread_busy_wait(1000000);
#endif
	ret = uart_line_ctrl_get(dev, LINE_CTRL_BAUD_RATE, &baudrate);
	if (ret)
		printf("Failed to get baudrate, ret code %d\n", ret);
	else
		printf("Baudrate detected: %d\n", baudrate);

	SYS_LOG_INF("USB serial initialized");

	/* Initialize nbufs */
	net_nbuf_init();

	/* Initialize RX queue */
	init_rx_queue();

	/* Initialize TX queue */
	init_tx_queue();

	/* Initialize ieee802154 device */
	if (!init_ieee802154()) {
		SYS_LOG_ERR("Unable to initialize ieee802154");
		return;
	};

	uart_irq_callback_set(dev, interrupt_handler);

	/* Enable rx interrupts */
	uart_irq_rx_enable(dev);

	/* Enable tx interrupts */
	uart_irq_tx_enable(dev);
}

void ieee802154_init(struct net_if *iface)
{
	SYS_LOG_DBG("");
}

int net_recv_data(struct net_if *iface, struct net_buf *pkt)
{
	SYS_LOG_DBG("Got data, buf %p, len %d frags->len %d",
		    pkt, pkt->len, net_buf_frags_len(pkt));

	net_buf_put(&tx_queue, pkt);

	return 0;
}

extern enum net_verdict ieee802154_radio_handle_ack(struct net_if *iface,
						    struct net_buf *buf)
{
	SYS_LOG_DBG("");

	/* parse on higher layer */
	return NET_CONTINUE;
}

int ieee802154_radio_send(struct net_if *iface, struct net_buf *buf)
{
	SYS_LOG_DBG("");

	return -ENOTSUP;
}
