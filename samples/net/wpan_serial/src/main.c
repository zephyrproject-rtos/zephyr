/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief App implementing 802.15.4 "serial-radio" protocol
 *
 * Application implementing 802.15.4 "serial-radio" protocol compatible
 * with popular Contiki-based native border routers.
 */

#include <string.h>
#include <device.h>
#include <uart.h>
#include <zephyr.h>
#include <stdio.h>

#include <misc/printk.h>

#include <net/buf.h>

#define SYS_LOG_DOMAIN "wpan_serial"
#include <logging/sys_log.h>

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
static struct k_fifo rx_queue;
static K_THREAD_STACK_DEFINE(rx_stack, 1024);
static struct k_thread rx_thread_data;

/* TX queue */
static struct k_sem tx_sem;
static struct k_fifo tx_queue;
static K_THREAD_STACK_DEFINE(tx_stack, 1024);
static struct k_thread tx_thread_data;

/* Buffer for SLIP encoded data for the worst case */
static u8_t slip_buf[1 + 2 * CONFIG_NET_BUF_DATA_SIZE];

/* ieee802.15.4 device */
static struct ieee802154_radio_api *radio_api;
static struct device *ieee802154_dev;
u8_t mac_addr[8];

/* UART device */
static struct device *uart_dev;

/* SLIP state machine */
static u8_t slip_state = STATE_OK;

static struct net_pkt *pkt_curr;

/* General helpers */

#ifdef VERBOSE_DEBUG
static void hexdump(const char *str, const u8_t *packet, size_t length)
{
	int n = 0;

	if (!length) {
		printk("%s zero-length signal packet\n", str);
		return;
	}

	while (length--) {
		if (n % 16 == 0) {
			printk("%s %08X ", str, n);
		}

		printk("%02X ", *packet++);

		n++;
		if (n % 8 == 0) {
			if (n % 16 == 0) {
				printk("\n");
			} else {
				printk(" ");
			}
		}
	}

	if (n % 16) {
		printk("\n");
	}
}
#else
#define hexdump(...)
#endif

static int slip_process_byte(unsigned char c)
{
	struct net_buf *buf;
#ifdef VERBOSE_DEBUG
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

#ifdef VERBOSE_DEBUG
	SYS_LOG_DBG("processed: state %u byte %x", slip_state, c);
#endif

	if (!pkt_curr) {
		pkt_curr = net_pkt_get_reserve_rx(0, K_NO_WAIT);
		if (!pkt_curr) {
			SYS_LOG_ERR("No more buffers");
			return 0;
		}
		buf = net_pkt_get_frag(pkt_curr, K_NO_WAIT);
		if (!buf) {
			SYS_LOG_ERR("No more buffers");
			net_pkt_unref(pkt_curr);
			return 0;
		}
		net_pkt_frag_insert(pkt_curr, buf);
	} else {
		buf = net_buf_frag_last(pkt_curr->frags);
	}

	if (!net_buf_tailroom(buf)) {
		SYS_LOG_ERR("No more buf space: buf %p len %u", buf, buf->len);

		net_pkt_unref(pkt_curr);
		pkt_curr = NULL;
		return 0;
	}

	net_buf_add_u8(buf, c);

	return 0;
}

static void interrupt_handler(struct device *dev)
{
	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
#ifdef VERBOSE_DEBUG
		SYS_LOG_DBG("");
#endif
		if (uart_irq_tx_ready(dev)) {
#ifdef VERBOSE_DEBUG
			SYS_LOG_DBG("TX ready interrupt");
#endif

			k_sem_give(&tx_sem);
		}

		if (uart_irq_rx_ready(dev)) {
			unsigned char byte;

#ifdef VERBOSE_DEBUG
			SYS_LOG_DBG("RX ready interrupt");
#endif

			while (uart_fifo_read(dev, &byte, sizeof(byte))) {
				if (slip_process_byte(byte)) {
					/**
					 * slip_process_byte() returns 1 on
					 * SLIP_END, even after receiving full
					 * packet
					 */
					if (!pkt_curr) {
						SYS_LOG_DBG("Skip SLIP_END");
						continue;
					}

					SYS_LOG_DBG("Full packet %p", pkt_curr);

					k_fifo_put(&rx_queue, pkt_curr);
					pkt_curr = NULL;
				}
			}
		}
	}
}

/* Allocate and send data to USB Host */
static void send_data(u8_t *cfg, u8_t *data, size_t len)
{
	struct net_pkt *pkt;
	struct net_buf *buf;

	pkt = net_pkt_get_reserve_rx(0, K_NO_WAIT);
	if (!pkt) {
		SYS_LOG_DBG("No pkt available");
		return;
	}

	buf = net_pkt_get_frag(pkt, K_NO_WAIT);
	if (!buf) {
		SYS_LOG_DBG("No fragment available");
		net_pkt_unref(pkt);
		return;
	}

	net_pkt_frag_insert(pkt, buf);

	SYS_LOG_DBG("queue pkt %p buf %p len %u", pkt, buf, len);

	/* Add configuration id */
	memcpy(net_buf_add(buf, 2), cfg, 2);

	memcpy(net_buf_add(buf, len), data, len);

	/* simulate LQI */
	net_buf_add(buf, 1);
	/* simulate FCS */
	net_buf_add(buf, 2);

	k_fifo_put(&tx_queue, pkt);
}

static void get_ieee_addr(void)
{
	u8_t cfg[2] = { '!', 'M' };
	u8_t mac[8];

	SYS_LOG_DBG("");

	/* Send in BE */
	sys_memcpy_swap(mac, mac_addr, sizeof(mac));

	send_data(cfg, mac, sizeof(mac));
}

static void process_request(struct net_buf *buf)
{
	u8_t cmd = net_buf_pull_u8(buf);


	switch (cmd) {
	case 'M':
		get_ieee_addr();
		break;
	default:
		SYS_LOG_ERR("Not handled request %c", cmd);
		break;
	}
}

static void send_pkt_report(u8_t seq, u8_t status, u8_t num_tx)
{
	u8_t cfg[2] = { '!', 'R' };
	u8_t report[3];

	report[0] = seq;
	report[1] = status;
	report[2] = num_tx;

	send_data(cfg, report, sizeof(report));
}

static void process_data(struct net_pkt *pkt)
{
	struct net_buf *buf = net_buf_frag_last(pkt->frags);
	u8_t seq, num_attr;
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
	ret = radio_api->tx(ieee802154_dev, pkt, buf);
	if (ret) {
		SYS_LOG_ERR("Error transmit data");
	}

	/* TODO: Return correct status codes */
	/* TODO: Implement re-transmissions if needed */

	/* Send packet data report */
	send_pkt_report(seq, ret, 1);
}

static void set_channel(u8_t chan)
{
	SYS_LOG_DBG("Set channel %c", chan);

	radio_api->set_channel(ieee802154_dev, chan);
}

static void process_config(struct net_pkt *pkt)
{
	struct net_buf *buf = net_buf_frag_last(pkt->frags);
	u8_t cmd = net_buf_pull_u8(buf);

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

static void rx_thread(void)
{
	SYS_LOG_INF("RX thread started");

	while (1) {
		struct net_pkt *pkt;
		struct net_buf *buf;
		u8_t specifier;

		pkt = k_fifo_get(&rx_queue, K_FOREVER);
		buf = net_buf_frag_last(pkt->frags);

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

		net_pkt_unref(pkt);

		k_yield();
	}
}

static size_t slip_buffer(u8_t *sbuf, struct net_buf *buf)
{
	size_t len = buf->len;
	u8_t *sbuf_orig = sbuf;
	int i;

	/**
	 * This strange protocol does not require send START
	 * *sbuf++ = SLIP_END;
	 */

	for (i = 0; i < len; i++) {
		u8_t byte = net_buf_pull_u8(buf);

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
static void tx_thread(void)
{
	SYS_LOG_DBG("TX thread started");

	/* Allow to send one TX */
	k_sem_give(&tx_sem);

	while (1) {
		struct net_pkt *pkt;
		struct net_buf *buf;
		size_t len;

		k_sem_take(&tx_sem, K_FOREVER);

		pkt = k_fifo_get(&tx_queue, K_FOREVER);
		buf = net_buf_frag_last(pkt->frags);
		len = net_pkt_get_len(pkt);

		SYS_LOG_DBG("Send pkt %p buf %p len %d", pkt, buf, len);

		hexdump("SLIP <", buf->data, buf->len);

		/* remove FCS 2 bytes */
		buf->len -= 2;

		/* SLIP encode and send */
		len = slip_buffer(slip_buf, buf);
		uart_fifo_fill(uart_dev, slip_buf, len);

		net_pkt_unref(pkt);

#if 0
		k_yield();
#endif
	}
}

static void init_rx_queue(void)
{
	k_fifo_init(&rx_queue);

	k_thread_create(&rx_thread_data, rx_stack,
			K_THREAD_STACK_SIZEOF(rx_stack),
			(k_thread_entry_t)rx_thread,
			NULL, NULL, NULL, K_PRIO_COOP(8), 0, K_NO_WAIT);
}

static void init_tx_queue(void)
{
	k_sem_init(&tx_sem, 0, UINT_MAX);
	k_fifo_init(&tx_queue);

	k_thread_create(&tx_thread_data, tx_stack,
			K_THREAD_STACK_SIZEOF(tx_stack),
			(k_thread_entry_t)tx_thread,
			NULL, NULL, NULL, K_PRIO_COOP(8), 0, K_NO_WAIT);
}

/**
 * FIXME choose correct OUI, or add support in L2
 */
static u8_t *get_mac(struct device *dev)
{
	u32_t *ptr = (u32_t *)mac_addr;

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
	SYS_LOG_INF("Initialize ieee802.15.4");

	ieee802154_dev = device_get_binding(CONFIG_IEEE802154_CC2520_DRV_NAME);
	if (!ieee802154_dev) {
		SYS_LOG_ERR("Cannot get CC250 device");
		return false;
	}

	radio_api = (struct ieee802154_radio_api *)ieee802154_dev->driver_api;

	/**
	 * Do actual initialization of the chip
	 */
	get_mac(ieee802154_dev);

	if (IEEE802154_HW_FILTER &
	    radio_api->get_capabilities(ieee802154_dev)) {
		struct ieee802154_filter filter;
		u16_t short_addr;

		/* Set short address */
		short_addr = (mac_addr[0] << 8) + mac_addr[1];
		filter.short_addr = short_addr;

		radio_api->set_filter(ieee802154_dev,
				      IEEE802154_FILTER_TYPE_SHORT_ADDR,
				      &filter);

		/* Set ieee address */
		filter.ieee_addr = mac_addr;
		radio_api->set_filter(ieee802154_dev,
				      IEEE802154_FILTER_TYPE_IEEE_ADDR,
				      &filter);

#ifdef CONFIG_NET_APP_SETTINGS
		SYS_LOG_INF("Set panid %x", CONFIG_NET_APP_IEEE802154_PAN_ID);

		filter.pan_id = CONFIG_NET_APP_IEEE802154_PAN_ID;

		radio_api->set_filter(ieee802154_dev,
				      IEEE802154_FILTER_TYPE_PAN_ID,
				      &filter);
#endif /* CONFIG_NET_APP_SETTINGS */
	}

#ifdef CONFIG_NET_APP_SETTINGS
	SYS_LOG_INF("Set channel %x", CONFIG_NET_APP_IEEE802154_CHANNEL);
	radio_api->set_channel(ieee802154_dev,
			       CONFIG_NET_APP_IEEE802154_CHANNEL);
#endif /* CONFIG_NET_APP_SETTINGS */

	/* Start ieee802154 */
	radio_api->start(ieee802154_dev);

	return true;
}

int net_recv_data(struct net_if *iface, struct net_pkt *pkt)
{
	SYS_LOG_DBG("Got data, pkt %p, frags->len %d",
		    pkt, net_pkt_get_len(pkt));

	k_fifo_put(&tx_queue, pkt);

	return 0;
}

void main(void)
{
	struct device *dev;
	u32_t baudrate, dtr = 0;
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
		printk("Failed to set DCD, ret code %d\n", ret);

	ret = uart_line_ctrl_set(dev, LINE_CTRL_DSR, 1);
	if (ret)
		printk("Failed to set DSR, ret code %d\n", ret);

	/* Wait 1 sec for the host to do all settings */
	sys_thread_busy_wait(1000000);
#endif
	ret = uart_line_ctrl_get(dev, LINE_CTRL_BAUD_RATE, &baudrate);
	if (ret)
		printk("Failed to get baudrate, ret code %d\n", ret);
	else
		printk("Baudrate detected: %d\n", baudrate);

	SYS_LOG_INF("USB serial initialized");

	/* Initialize net_pkt */
	net_pkt_init();

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
