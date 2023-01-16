/*
 * Copyright (c) 2016-2019 Intel Corporation
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

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wpan_serial, CONFIG_USB_DEVICE_LOG_LEVEL);

#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/random/rand32.h>

#include <zephyr/net/buf.h>
#include <net_private.h>
#include <zephyr/net/ieee802154_radio.h>

#if defined(CONFIG_NET_TC_THREAD_COOPERATIVE)
#define THREAD_PRIORITY K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)
#else
#define THREAD_PRIORITY K_PRIO_PREEMPT(8)
#endif

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
static struct k_fifo tx_queue;
static K_THREAD_STACK_DEFINE(tx_stack, 1024);
static struct k_thread tx_thread_data;

/* Buffer for SLIP encoded data for the worst case */
static uint8_t slip_buf[1 + 2 * CONFIG_NET_BUF_DATA_SIZE];

/* ieee802.15.4 device */
static struct ieee802154_radio_api *radio_api;
static const struct device *const ieee802154_dev =
	DEVICE_DT_GET(DT_CHOSEN(zephyr_ieee802154));
uint8_t mac_addr[8]; /* in little endian */

/* UART device */
static const struct device *const uart_dev =
	DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);

/* SLIP state machine */
static uint8_t slip_state = STATE_OK;

static struct net_pkt *pkt_curr;

/* General helpers */

static int slip_process_byte(unsigned char c)
{
	struct net_buf *buf;
#ifdef VERBOSE_DEBUG
	LOG_DBG("recv: state %u byte %x", slip_state, c);
#endif
	switch (slip_state) {
	case STATE_GARBAGE:
		if (c == SLIP_END) {
			slip_state = STATE_OK;
		}
		LOG_DBG("garbage: discard byte %x", c);
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
	LOG_DBG("processed: state %u byte %x", slip_state, c);
#endif

	if (!pkt_curr) {
		pkt_curr = net_pkt_rx_alloc_with_buffer(NULL, 256,
							AF_UNSPEC, 0,
							K_NO_WAIT);
		if (!pkt_curr) {
			LOG_ERR("No more buffers");
			return 0;
		}
	}

	buf = net_buf_frag_last(pkt_curr->buffer);
	if (!net_buf_tailroom(buf)) {
		LOG_ERR("No more buf space: buf %p len %u", buf, buf->len);

		net_pkt_unref(pkt_curr);
		pkt_curr = NULL;
		return 0;
	}

	net_buf_add_u8(buf, c);

	return 0;
}

static void interrupt_handler(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);

	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
		unsigned char byte;

		if (!uart_irq_rx_ready(dev)) {
			continue;
		}

		while (uart_fifo_read(dev, &byte, sizeof(byte))) {
			if (slip_process_byte(byte)) {
				/**
				 * slip_process_byte() returns 1 on
				 * SLIP_END, even after receiving full
				 * packet
				 */
				if (!pkt_curr) {
					LOG_DBG("Skip SLIP_END");
					continue;
				}

				LOG_DBG("Full packet %p, len %u", pkt_curr,
					net_pkt_get_len(pkt_curr));

				k_fifo_put(&rx_queue, pkt_curr);
				pkt_curr = NULL;
			}
		}
	}
}

/* Allocate and send data to USB Host */
static void send_data(uint8_t *cfg, uint8_t *data, size_t len)
{
	struct net_pkt *pkt;

	pkt = net_pkt_alloc_with_buffer(NULL, len + 5,
					AF_UNSPEC, 0, K_NO_WAIT);
	if (!pkt) {
		LOG_DBG("No pkt available");
		return;
	}

	LOG_DBG("queue pkt %p len %u", pkt, len);

	/* Add configuration id */
	net_pkt_write(pkt, cfg, 2);
	net_pkt_write(pkt, data, len);

	/* simulate LQI */
	net_pkt_skip(pkt, 1);
	/* simulate FCS */
	net_pkt_skip(pkt, 2);

	net_pkt_set_overwrite(pkt, true);

	k_fifo_put(&tx_queue, pkt);
}

static void get_ieee_addr(void)
{
	uint8_t cfg[2] = { '!', 'M' };
	uint8_t mac[8];

	LOG_DBG("");

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
		LOG_ERR("Not handled request %c", cmd);
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

static void process_data(struct net_pkt *pkt)
{
	struct net_buf *buf = net_buf_frag_last(pkt->buffer);
	uint8_t seq, num_attr;
	int ret, i;

	seq = net_buf_pull_u8(buf);
	num_attr = net_buf_pull_u8(buf);

	LOG_DBG("seq %u num_attr %u", seq, num_attr);

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
	ret = radio_api->tx(ieee802154_dev, IEEE802154_TX_MODE_DIRECT,
			    pkt, buf);
	if (ret) {
		LOG_ERR("Error transmit data");
	}

	/* TODO: Return correct status codes */
	/* TODO: Implement re-transmissions if needed */

	/* Send packet data report */
	send_pkt_report(seq, ret, 1);
}

static void set_channel(uint8_t chan)
{
	LOG_DBG("Set channel %u", chan);

	radio_api->set_channel(ieee802154_dev, chan);
}

static void process_config(struct net_pkt *pkt)
{
	struct net_buf *buf = net_buf_frag_last(pkt->buffer);
	uint8_t cmd = net_buf_pull_u8(buf);

	LOG_DBG("Process config %c", cmd);

	switch (cmd) {
	case 'S':
		process_data(pkt);
		break;
	case 'C':
		set_channel(net_buf_pull_u8(buf));
		break;
	default:
		LOG_ERR("Unhandled cmd %u", cmd);
	}
}

static void rx_thread(void)
{
	LOG_DBG("RX thread started");

	while (true) {
		struct net_pkt *pkt;
		struct net_buf *buf;
		uint8_t specifier;

		pkt = k_fifo_get(&rx_queue, K_FOREVER);
		buf = net_buf_frag_last(pkt->buffer);

		LOG_DBG("rx_queue pkt %p buf %p", pkt, buf);

		LOG_HEXDUMP_DBG(buf->data, buf->len, "SLIP >");

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
			LOG_ERR("Unknown message specifier %c", specifier);
			break;
		}

		net_pkt_unref(pkt);
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

static int try_write(uint8_t *data, uint16_t len)
{
	int wrote;

	while (len) {
		wrote = uart_fifo_fill(uart_dev, data, len);
		if (wrote <= 0) {
			return wrote;
		}

		len -= wrote;
		data += wrote;
	}

	return 0;
}

/**
 * TX - transmit to SLIP interface
 */
static void tx_thread(void)
{
	LOG_DBG("TX thread started");

	while (true) {
		struct net_pkt *pkt;
		struct net_buf *buf;
		size_t len;

		pkt = k_fifo_get(&tx_queue, K_FOREVER);
		buf = net_buf_frag_last(pkt->buffer);
		len = net_pkt_get_len(pkt);

		LOG_DBG("Send pkt %p buf %p len %d", pkt, buf, len);

		LOG_HEXDUMP_DBG(buf->data, buf->len, "SLIP <");

		/* remove FCS 2 bytes */
		buf->len -= 2U;

		/* SLIP encode and send */
		len = slip_buffer(slip_buf, buf);

		try_write(slip_buf, len);

		net_pkt_unref(pkt);
	}
}

static void init_rx_queue(void)
{
	k_fifo_init(&rx_queue);

	k_thread_create(&rx_thread_data, rx_stack,
			K_THREAD_STACK_SIZEOF(rx_stack),
			(k_thread_entry_t)rx_thread,
			NULL, NULL, NULL, THREAD_PRIORITY, 0, K_NO_WAIT);
}

static void init_tx_queue(void)
{
	k_fifo_init(&tx_queue);

	k_thread_create(&tx_thread_data, tx_stack,
			K_THREAD_STACK_SIZEOF(tx_stack),
			(k_thread_entry_t)tx_thread,
			NULL, NULL, NULL, THREAD_PRIORITY, 0, K_NO_WAIT);
}

/**
 * FIXME choose correct OUI, or add support in L2
 */
static uint8_t *get_mac(const struct device *dev)
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
	LOG_INF("Initialize ieee802.15.4");

	if (!device_is_ready(ieee802154_dev)) {
		LOG_ERR("IEEE 802.15.4 device not ready");
		return false;
	}

	radio_api = (struct ieee802154_radio_api *)ieee802154_dev->api;

	/**
	 * Do actual initialization of the chip
	 */
	get_mac(ieee802154_dev);

	if (IEEE802154_HW_FILTER &
	    radio_api->get_capabilities(ieee802154_dev)) {
		struct ieee802154_filter filter;
		uint16_t short_addr;

		/* Set short address */
		short_addr = (mac_addr[0] << 8) + mac_addr[1];
		filter.short_addr = short_addr;

		radio_api->filter(ieee802154_dev, true,
				  IEEE802154_FILTER_TYPE_SHORT_ADDR,
				  &filter);

		/* Set ieee address */
		filter.ieee_addr = mac_addr;
		radio_api->filter(ieee802154_dev, true,
				  IEEE802154_FILTER_TYPE_IEEE_ADDR,
				  &filter);

#ifdef CONFIG_NET_CONFIG_SETTINGS
		LOG_INF("Set panid %x", CONFIG_NET_CONFIG_IEEE802154_PAN_ID);

		filter.pan_id = CONFIG_NET_CONFIG_IEEE802154_PAN_ID;

		radio_api->filter(ieee802154_dev, true,
				  IEEE802154_FILTER_TYPE_PAN_ID,
				  &filter);
#endif /* CONFIG_NET_CONFIG_SETTINGS */
	}

#ifdef CONFIG_NET_CONFIG_SETTINGS
	LOG_INF("Set channel %u", CONFIG_NET_CONFIG_IEEE802154_CHANNEL);
	radio_api->set_channel(ieee802154_dev,
			       CONFIG_NET_CONFIG_IEEE802154_CHANNEL);
#endif /* CONFIG_NET_CONFIG_SETTINGS */

	/* Start ieee802154 */
	radio_api->start(ieee802154_dev);

	return true;
}

int net_recv_data(struct net_if *iface, struct net_pkt *pkt)
{
	LOG_DBG("Received pkt %p, len %d", pkt, net_pkt_get_len(pkt));

	k_fifo_put(&tx_queue, pkt);

	return 0;
}

enum net_verdict ieee802154_radio_handle_ack(struct net_if *iface, struct net_pkt *pkt)
{
	return NET_CONTINUE;
}

void main(void)
{
	uint32_t baudrate, dtr = 0U;
	int ret;

	LOG_INF("Starting wpan_serial application");

	if (!device_is_ready(uart_dev)) {
		LOG_ERR("CDC ACM device not ready");
		return;
	}

	ret = usb_enable(NULL);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return;
	}

	LOG_DBG("Wait for DTR");

	while (1) {
		uart_line_ctrl_get(uart_dev, UART_LINE_CTRL_DTR, &dtr);
		if (dtr) {
			break;
		} else {
			/* Give CPU resources to low priority threads. */
			k_sleep(K_MSEC(100));
		}
	}

	LOG_DBG("DTR set, continue");

	ret = uart_line_ctrl_get(uart_dev, UART_LINE_CTRL_BAUD_RATE, &baudrate);
	if (ret) {
		LOG_WRN("Failed to get baudrate, ret code %d", ret);
	} else {
		LOG_DBG("Baudrate detected: %d", baudrate);
	}

	LOG_INF("USB serial initialized");

	/* Initialize net_pkt */
	net_pkt_init();

	/* Initialize RX queue */
	init_rx_queue();

	/* Initialize TX queue */
	init_tx_queue();

	/* Initialize ieee802154 device */
	if (!init_ieee802154()) {
		LOG_ERR("Unable to initialize ieee802154");
		return;
	}

	uart_irq_callback_set(uart_dev, interrupt_handler);

	/* Enable rx interrupts */
	uart_irq_rx_enable(uart_dev);
}
