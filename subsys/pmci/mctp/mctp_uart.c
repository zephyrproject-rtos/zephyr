/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/sys/__assert.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/pmci/mctp/mctp_uart.h>
#include <crc-16-ccitt.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mctp_uart, CONFIG_MCTP_LOG_LEVEL);

#define MCTP_UART_REVISION     0x01
#define MCTP_UART_FRAMING_FLAG 0x7e
#define MCTP_UART_ESCAPE       0x7d

const char *UART_EVENT_STRING[] = {
	"TX Done",     "TX Aborted", "RX Ready", "RX Buffer Request", "RX Buffer Released",
	"RX Disabled", "RX Stopped",
};

const char *MCTP_STATE_STRING[] = {
	"Wait: Sync Start", "Wait: Revision", "Wait: Len",  "Data",
	"Data: Escaped",    "Wait: FCS1",     "Wait: FCS2", "Wait: Sync End",
};

struct mctp_serial_header {
	uint8_t flag;
	uint8_t revision;
	uint8_t len;
};

struct mctp_serial_trailer {
	uint8_t fcs_msb;
	uint8_t fcs_lsb;
	uint8_t flag;
};

static inline struct mctp_binding_uart *binding_to_uart(struct mctp_binding *b)
{
	return (struct mctp_binding_uart *)b;
}

static void mctp_uart_finish_pkt(struct mctp_binding_uart *uart, bool valid)
{
	struct mctp_pktbuf *pkt = uart->rx_pkt;


	if (valid) {
		__ASSERT_NO_MSG(pkt);

		mctp_bus_rx(&uart->binding, pkt);
	}

	uart->rx_pkt = NULL;
}

static void mctp_uart_start_pkt(struct mctp_binding_uart *uart, uint8_t len)
{
	__ASSERT_NO_MSG(uart->rx_pkt == NULL);

	uart->rx_pkt = mctp_pktbuf_alloc(&uart->binding, len);

	__ASSERT_NO_MSG(uart->rx_pkt);
}

static size_t mctp_uart_pkt_escape(struct mctp_pktbuf *pkt, uint8_t *buf)
{
	uint8_t total_len;
	uint8_t *p;
	int i, j;

	total_len = pkt->end - pkt->mctp_hdr_off;

	p = (void *)mctp_pktbuf_hdr(pkt);

	for (i = 0, j = 0; i < total_len; i++, j++) {
		uint8_t c = p[i];

		if (c == MCTP_UART_FRAMING_FLAG || c == MCTP_UART_ESCAPE) {
			if (buf) {
				buf[j] = MCTP_UART_ESCAPE;
			}
			j++;
			c ^= 0x20;
		}
		if (buf) {
			buf[j] = c;
		}
	}

	return j;
}

/*
 * Each byte coming from the uart is run through this state machine which
 * does the MCTP packet decoding.
 *
 * The actual packet and buffer being read into is owned by the binding!
 */
static void mctp_uart_consume(struct mctp_binding_uart *uart, uint8_t c)
{
	struct mctp_pktbuf *pkt = uart->rx_pkt;
	bool valid = false;

	LOG_DBG("uart consume start state: %d:%s, char 0x%02x", uart->rx_state,
		MCTP_STATE_STRING[uart->rx_state], c);

	__ASSERT_NO_MSG(!pkt == (uart->rx_state == STATE_WAIT_SYNC_START ||
				 uart->rx_state == STATE_WAIT_REVISION ||
				 uart->rx_state == STATE_WAIT_LEN));

	switch (uart->rx_state) {
	case STATE_WAIT_SYNC_START:
		if (c != MCTP_UART_FRAMING_FLAG) {
			LOG_DBG("lost sync, dropping packet");
			if (pkt) {
				mctp_uart_finish_pkt(uart, false);
			}
		} else {
			uart->rx_state = STATE_WAIT_REVISION;
		}
		break;
	case STATE_WAIT_REVISION:
		if (c == MCTP_UART_REVISION) {
			uart->rx_state = STATE_WAIT_LEN;
			uart->rx_fcs_calc = crc_16_ccitt_byte(FCS_INIT_16, c);
		} else if (c == MCTP_UART_FRAMING_FLAG) {
			/* Handle the case where there are bytes dropped in request,
			 * and the state machine is out of sync. The failed request's
			 * trailing footer i.e. 0x7e would be interpreted as next
			 * request's framing footer. So if we are in STATE_WAIT_REVISION
			 * and receive 0x7e byte, then continue to stay in
			 * STATE_WAIT_REVISION
			 */
			LOG_DBG("Received serial framing flag 0x%02x while waiting"
				" for serial revision 0x%02x.",
				c, MCTP_UART_REVISION);
		} else {
			LOG_DBG("invalid revision 0x%02x", c);
			uart->rx_state = STATE_WAIT_SYNC_START;
		}
		break;
	case STATE_WAIT_LEN:
		if (c > uart->binding.pkt_size || c < sizeof(struct mctp_hdr)) {
			LOG_DBG("invalid size %d", c);
			uart->rx_state = STATE_WAIT_SYNC_START;
		} else {
			mctp_uart_start_pkt(uart, 0);
			pkt = uart->rx_pkt;
			uart->rx_exp_len = c;
			uart->rx_state = STATE_DATA;
			uart->rx_fcs_calc = crc_16_ccitt_byte(uart->rx_fcs_calc, c);
		}
		break;
	case STATE_DATA:
		if (c == MCTP_UART_ESCAPE) {
			uart->rx_state = STATE_DATA_ESCAPED;
		} else {
			mctp_pktbuf_push(pkt, &c, 1);
			uart->rx_fcs_calc = crc_16_ccitt_byte(uart->rx_fcs_calc, c);
			if (pkt->end - pkt->mctp_hdr_off == uart->rx_exp_len) {
				uart->rx_state = STATE_WAIT_FCS1;
			}
		}
		break;
	case STATE_DATA_ESCAPED:
		c ^= 0x20;
		mctp_pktbuf_push(pkt, &c, 1);
		uart->rx_fcs_calc = crc_16_ccitt_byte(uart->rx_fcs_calc, c);
		if (pkt->end - pkt->mctp_hdr_off == uart->rx_exp_len) {
			uart->rx_state = STATE_WAIT_FCS1;
		} else {
			uart->rx_state = STATE_DATA;
		}
		break;

	case STATE_WAIT_FCS1:
		uart->rx_fcs = c << 8;
		uart->rx_state = STATE_WAIT_FCS2;
		break;
	case STATE_WAIT_FCS2:
		uart->rx_fcs |= c;
		uart->rx_state = STATE_WAIT_SYNC_END;
		break;
	case STATE_WAIT_SYNC_END:
		if (uart->rx_fcs == uart->rx_fcs_calc) {
			if (c == MCTP_UART_FRAMING_FLAG) {
				valid = true;
			} else {
				valid = false;
				LOG_DBG("missing end frame marker");
			}
		} else {
			valid = false;
			LOG_DBG("invalid fcs : 0x%04x, expect 0x%04x", uart->rx_fcs,
				uart->rx_fcs_calc);
		}

		mctp_uart_finish_pkt(uart, valid);
		uart->rx_state = STATE_WAIT_SYNC_START;
		break;
	}

	LOG_DBG("uart consume end state: %d:%s, char 0x%02x", uart->rx_state,
		MCTP_STATE_STRING[uart->rx_state], c);
}

static void mctp_uart_callback(const struct device *dev, struct uart_event *evt, void *userdata)
{
	struct mctp_binding_uart *binding = userdata;

	switch (evt->type) {
	case UART_TX_DONE:
		binding->tx_res = 0;
		break;
	case UART_TX_ABORTED:
		binding->tx_res = -EIO;
		break;
	case UART_RX_RDY:
		/* buffer being read into is ready */
		binding->rx_res = evt->data.rx.len;
		/* parse the buffer */
		for (size_t i = 0; i < evt->data.rx.len; i++) {
			mctp_uart_consume(binding, evt->data.rx.buf[evt->data.rx.offset + i]);
		}
		break;
	case UART_RX_BUF_REQUEST:
		for (int i = 0; i < sizeof(binding->rx_buf_used); i++) {
			if (!binding->rx_buf_used[i]) {
				binding->rx_buf_used[i] = true;
				uart_rx_buf_rsp(dev, binding->rx_buf[i],
						sizeof(binding->rx_buf[i]));
				break;
			}
		}
		break;
	case UART_RX_BUF_RELEASED:
		for (int i = 0; i < sizeof(binding->rx_buf_used); i++) {
			if (binding->rx_buf[i] == evt->data.rx_buf.buf) {
				binding->rx_buf_used[i] = false;
				break;
			}
		}
		break;
	case UART_RX_STOPPED:
		break;
	case UART_RX_DISABLED:
		break;
	}
}

void mctp_uart_start_rx(struct mctp_binding_uart *uart)
{
	int res = uart_callback_set(uart->dev, mctp_uart_callback, uart);

	__ASSERT_NO_MSG(res == 0);
	uart->rx_buf_used[0] = true;
	res = uart_rx_enable(uart->dev, uart->rx_buf[0], sizeof(uart->rx_buf[0]), 1000);
	__ASSERT_NO_MSG(res == 0);
}

int mctp_uart_tx(struct mctp_binding *b, struct mctp_pktbuf *pkt)
{
	struct mctp_binding_uart *uart = binding_to_uart(b);
	struct mctp_serial_header *hdr;
	struct mctp_serial_trailer *tlr;
	uint8_t *buf;
	size_t len;
	uint16_t fcs;

	LOG_DBG("uart tx pkt %p", pkt);

	/* the length field in the header excludes serial framing
	 * and escape sequences
	 */
	len = mctp_pktbuf_size(pkt);

	hdr = (void *)uart->tx_buf;
	hdr->flag = MCTP_UART_FRAMING_FLAG;
	hdr->revision = MCTP_UART_REVISION;
	hdr->len = len;

	/* Calculate fcs */
	fcs = crc_16_ccitt(FCS_INIT_16, (const uint8_t *)hdr + 1, 2);
	fcs = crc_16_ccitt(fcs, (const uint8_t *)mctp_pktbuf_hdr(pkt), len);
	LOG_DBG("calculated crc %d", fcs);

	buf = (void *)(hdr + 1);

	len = mctp_uart_pkt_escape(pkt, NULL);
	if (len + sizeof(*hdr) + sizeof(*tlr) > sizeof(uart->tx_buf)) {
		return -EMSGSIZE;
	}

	mctp_uart_pkt_escape(pkt, buf);

	buf += len;

	tlr = (void *)buf;
	tlr->flag = MCTP_UART_FRAMING_FLAG;
	tlr->fcs_msb = fcs >> 8;
	tlr->fcs_lsb = fcs & 0xff;

	len += sizeof(*hdr) + sizeof(*tlr);

	int res = uart_tx(uart->dev, (const uint8_t *)uart->tx_buf, len, SYS_FOREVER_US);

	if (res != 0) {
		LOG_ERR("Failed sending data, %d", res);
		return res;
	}

	return uart->tx_res;
}

int mctp_uart_start(struct mctp_binding *binding)
{
	mctp_binding_set_tx_enabled(binding, true);
	return 0;
}
