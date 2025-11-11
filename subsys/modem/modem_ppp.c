/*
 * Copyright (c) 2022 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/ppp.h>
#include <zephyr/sys/crc.h>
#include <zephyr/modem/ppp.h>
#include <zephyr/pm/device_runtime.h>
#include <string.h>

#include "modem_workqueue.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modem_ppp, CONFIG_MODEM_MODULES_LOG_LEVEL);

#define MODEM_PPP_STATE_ATTACHED_BIT	(0)
#define MODEM_PPP_FRAME_TAIL_SIZE	(2)

#define MODEM_PPP_CODE_DELIMITER	(0x7E)
#define MODEM_PPP_CODE_ESCAPE		(0x7D)
#define MODEM_PPP_VALUE_ESCAPE		(0x20)

static uint16_t modem_ppp_fcs_init(uint8_t byte)
{
	return crc16_ccitt(0xFFFF, &byte, 1);
}

static uint16_t modem_ppp_fcs_update(uint16_t fcs, uint8_t byte)
{
	return crc16_ccitt(fcs, &byte, 1);
}

static uint16_t modem_ppp_fcs_final(uint16_t fcs)
{
	return fcs ^ 0xFFFF;
}

static uint16_t modem_ppp_ppp_protocol(struct net_pkt *pkt)
{
	if (net_pkt_family(pkt) == AF_INET) {
		return PPP_IP;
	}

	if (net_pkt_family(pkt) == AF_INET6) {
		return PPP_IPV6;
	}

	LOG_WRN("Unsupported protocol");
	return 0;
}

static bool modem_ppp_needs_escape(uint32_t async_map, uint8_t byte)
{
	uint32_t byte_bit;

	if ((byte == MODEM_PPP_CODE_DELIMITER) || (byte == MODEM_PPP_CODE_ESCAPE)) {
		/* Always escaped */
		return true;
	} else if (byte >= MODEM_PPP_VALUE_ESCAPE) {
		/* Never escaped */
		return false;
	}
	byte_bit = BIT(byte);
	/* Escaped if required by the async control character map */
	return byte_bit & async_map;
}

static uint32_t modem_ppp_wrap(struct modem_ppp *ppp, uint8_t *buffer, uint32_t available)
{
	uint32_t async_map = ppp_peer_async_control_character_map(ppp->iface);
	uint32_t offset = 0;
	uint32_t remaining;
	uint16_t protocol;
	uint8_t upper;
	uint8_t lower;
	uint8_t byte;

	while (offset < available) {
		remaining = available - offset;

		switch (ppp->transmit_state) {
		case MODEM_PPP_TRANSMIT_STATE_SOF:
			if (remaining < 4) {
				/* Insufficient space for constant header prefix */
				goto end;
			}
			/* Init cursor for later phases */
			net_pkt_cursor_init(ppp->tx_pkt);
			/* 3 byte common header */
			buffer[offset++] = MODEM_PPP_CODE_DELIMITER;
			buffer[offset++] = 0xFF;
			buffer[offset++] = MODEM_PPP_CODE_ESCAPE;
			buffer[offset++] = 0x23;
			/* Initialise the FCS.
			 * This value is always the same at this point, so use the constant value.
			 * Equivelent to:
			 *   ppp->tx_pkt_fcs = modem_ppp_fcs_init(0xFF);
			 *   ppp->tx_pkt_fcs = modem_ppp_fcs_update(ppp->tx_pkt_fcs, 0x03);
			 */
			ARG_UNUSED(modem_ppp_fcs_init);
			ppp->tx_pkt_fcs = 0x3DE3;
			/* Next state */
			if (net_pkt_is_ppp(ppp->tx_pkt)) {
				ppp->transmit_state = MODEM_PPP_TRANSMIT_STATE_DATA;
			} else {
				ppp->transmit_state = MODEM_PPP_TRANSMIT_STATE_PROTOCOL;
			}
			break;
		case MODEM_PPP_TRANSMIT_STATE_PROTOCOL:
			/* If both protocol bytes need escaping, it could be 4 bytes */
			if (remaining < 4) {
				/* Insufficient space for protocol bytes */
				goto end;
			}
			/* Extract protocol bytes */
			protocol = modem_ppp_ppp_protocol(ppp->tx_pkt);
			upper = (protocol >> 8) & 0xFF;
			lower = (protocol >> 0) & 0xFF;
			/* FCS is computed without the escape/modification */
			ppp->tx_pkt_fcs = modem_ppp_fcs_update(ppp->tx_pkt_fcs, upper);
			ppp->tx_pkt_fcs = modem_ppp_fcs_update(ppp->tx_pkt_fcs, lower);
			/* Push protocol bytes (with required escaping) */
			if (modem_ppp_needs_escape(async_map, upper)) {
				buffer[offset++] = MODEM_PPP_CODE_ESCAPE;
				upper ^= MODEM_PPP_VALUE_ESCAPE;
			}
			buffer[offset++] = upper;
			if (modem_ppp_needs_escape(async_map, lower)) {
				buffer[offset++] = MODEM_PPP_CODE_ESCAPE;
				lower ^= MODEM_PPP_VALUE_ESCAPE;
			}
			buffer[offset++] = lower;
			/* Next state */
			ppp->transmit_state = MODEM_PPP_TRANSMIT_STATE_DATA;
			break;
		case MODEM_PPP_TRANSMIT_STATE_DATA:
			/* Push all data bytes into the buffer */
			while (net_pkt_remaining_data(ppp->tx_pkt) > 0) {
				/* Space available, taking into account possible escapes */
				if (remaining < 2) {
					goto end;
				}
				/* Pull next byte we're sending */
				(void)net_pkt_read_u8(ppp->tx_pkt, &byte);
				/* FCS is computed without the escape/modification */
				ppp->tx_pkt_fcs = modem_ppp_fcs_update(ppp->tx_pkt_fcs, byte);
				/* Push encoded bytes into buffer*/
				if (modem_ppp_needs_escape(async_map, byte)) {
					buffer[offset++] = MODEM_PPP_CODE_ESCAPE;
					byte ^= MODEM_PPP_VALUE_ESCAPE;
					remaining--;
				}
				buffer[offset++] = byte;
				remaining--;
			}
			/* Data phase finished */
			ppp->transmit_state = MODEM_PPP_TRANSMIT_STATE_EOF;
			break;
		case MODEM_PPP_TRANSMIT_STATE_EOF:
			/* If both FCS bytes need escaping, it could be 5 bytes */
			if (remaining < 5) {
				/* Insufficient space for protocol bytes */
				goto end;
			}
			/* Push FCS (order is [lower, upper] unlike the protocol) */
			ppp->tx_pkt_fcs = modem_ppp_fcs_final(ppp->tx_pkt_fcs);
			lower = (ppp->tx_pkt_fcs >> 0) & 0xFF;
			upper = (ppp->tx_pkt_fcs >> 8) & 0xFF;
			if (modem_ppp_needs_escape(async_map, lower)) {
				buffer[offset++] = MODEM_PPP_CODE_ESCAPE;
				lower ^= MODEM_PPP_VALUE_ESCAPE;
			}
			buffer[offset++] = lower;
			if (modem_ppp_needs_escape(async_map, upper)) {
				buffer[offset++] = MODEM_PPP_CODE_ESCAPE;
				upper ^= MODEM_PPP_VALUE_ESCAPE;
			}
			buffer[offset++] = upper;
			buffer[offset++] = MODEM_PPP_CODE_DELIMITER;

			/* Packet has finished */
			ppp->transmit_state = MODEM_PPP_TRANSMIT_STATE_IDLE;
			goto end;
		default:
			LOG_DBG("Invalid transmit state (%d)", ppp->transmit_state);
			goto end;
		}
	}
end:
	return offset;
}

static bool modem_ppp_is_byte_expected(uint8_t byte, uint8_t expected_byte)
{
	if (byte == expected_byte) {
		return true;
	}
	LOG_DBG("Dropping byte 0x%02hhx because 0x%02hhx was expected.", byte, expected_byte);
	return false;
}

static void modem_ppp_process_received_byte(struct modem_ppp *ppp, uint8_t byte)
{
	switch (ppp->receive_state) {
	case MODEM_PPP_RECEIVE_STATE_HDR_SOF:
		if (modem_ppp_is_byte_expected(byte, MODEM_PPP_CODE_DELIMITER)) {
			ppp->receive_state = MODEM_PPP_RECEIVE_STATE_HDR_FF;
		}
		break;

	case MODEM_PPP_RECEIVE_STATE_HDR_FF:
		if (byte == MODEM_PPP_CODE_DELIMITER) {
			break;
		}
		if (modem_ppp_is_byte_expected(byte, 0xFF)) {
			ppp->receive_state = MODEM_PPP_RECEIVE_STATE_HDR_7D;
		} else {
			ppp->receive_state = MODEM_PPP_RECEIVE_STATE_HDR_SOF;
		}
		break;

	case MODEM_PPP_RECEIVE_STATE_HDR_7D:
		if (modem_ppp_is_byte_expected(byte, MODEM_PPP_CODE_ESCAPE)) {
			ppp->receive_state = MODEM_PPP_RECEIVE_STATE_HDR_23;
		} else {
			ppp->receive_state = MODEM_PPP_RECEIVE_STATE_HDR_SOF;
		}
		break;

	case MODEM_PPP_RECEIVE_STATE_HDR_23:
		if (modem_ppp_is_byte_expected(byte, 0x23)) {
			ppp->rx_pkt = net_pkt_rx_alloc_with_buffer(ppp->iface,
				CONFIG_MODEM_PPP_NET_BUF_FRAG_SIZE, AF_UNSPEC, 0, K_NO_WAIT);

			if (ppp->rx_pkt == NULL) {
				LOG_WRN("Dropped frame, no net_pkt available");
				ppp->receive_state = MODEM_PPP_RECEIVE_STATE_HDR_SOF;
				break;
			}

			LOG_DBG("Receiving PPP frame");
			ppp->receive_state = MODEM_PPP_RECEIVE_STATE_WRITING;
			net_pkt_cursor_init(ppp->rx_pkt);
		} else {
			ppp->receive_state = MODEM_PPP_RECEIVE_STATE_HDR_SOF;
		}

		break;

	case MODEM_PPP_RECEIVE_STATE_WRITING:
		if (byte == MODEM_PPP_CODE_DELIMITER) {
			LOG_DBG("Received PPP frame (len %zu)", net_pkt_get_len(ppp->rx_pkt));

			/* Remove FCS */
			net_pkt_remove_tail(ppp->rx_pkt, MODEM_PPP_FRAME_TAIL_SIZE);
			net_pkt_set_ppp(ppp->rx_pkt, true);

			if (net_recv_data(ppp->iface, ppp->rx_pkt) < 0) {
				LOG_WRN("Net pkt could not be processed");
				net_pkt_unref(ppp->rx_pkt);
			}

			ppp->rx_pkt = NULL;
			/* Skip SOF because the delimiter may be omitted for successive frames. */
			ppp->receive_state = MODEM_PPP_RECEIVE_STATE_HDR_FF;
			break;
		}

		if (net_pkt_available_buffer(ppp->rx_pkt) == 1) {
			if (net_pkt_alloc_buffer(ppp->rx_pkt, CONFIG_MODEM_PPP_NET_BUF_FRAG_SIZE,
						 AF_INET, K_NO_WAIT) < 0) {
				LOG_WRN("Failed to alloc buffer");
				net_pkt_unref(ppp->rx_pkt);
				ppp->rx_pkt = NULL;
				ppp->receive_state = MODEM_PPP_RECEIVE_STATE_HDR_SOF;
				break;
			}
		}

		if (byte == MODEM_PPP_CODE_ESCAPE) {
			ppp->receive_state = MODEM_PPP_RECEIVE_STATE_UNESCAPING;
			break;
		}

		if (net_pkt_write_u8(ppp->rx_pkt, byte) < 0) {
			LOG_WRN("Dropped PPP frame");
			net_pkt_unref(ppp->rx_pkt);
			ppp->rx_pkt = NULL;
			ppp->receive_state = MODEM_PPP_RECEIVE_STATE_HDR_SOF;
#if defined(CONFIG_NET_STATISTICS_PPP)
			ppp->stats.drop++;
#endif
		}

		break;

	case MODEM_PPP_RECEIVE_STATE_UNESCAPING:
		if (net_pkt_write_u8(ppp->rx_pkt, (byte ^ MODEM_PPP_VALUE_ESCAPE)) < 0) {
			LOG_WRN("Dropped PPP frame");
			net_pkt_unref(ppp->rx_pkt);
			ppp->rx_pkt = NULL;
			ppp->receive_state = MODEM_PPP_RECEIVE_STATE_HDR_SOF;
#if defined(CONFIG_NET_STATISTICS_PPP)
			ppp->stats.drop++;
#endif
			break;
		}

		ppp->receive_state = MODEM_PPP_RECEIVE_STATE_WRITING;
		break;
	}
}

#if CONFIG_MODEM_STATS
static uint32_t get_transmit_buf_length(struct modem_ppp *ppp)
{
	return ring_buf_size_get(&ppp->transmit_rb);
}

static void advertise_transmit_buf_stats(struct modem_ppp *ppp)
{
	uint32_t length;

	length = get_transmit_buf_length(ppp);
	modem_stats_buffer_advertise_length(&ppp->transmit_buf_stats, length);
}

static void advertise_receive_buf_stats(struct modem_ppp *ppp, uint32_t length)
{
	modem_stats_buffer_advertise_length(&ppp->receive_buf_stats, length);
}
#endif

static void modem_ppp_pipe_callback(struct modem_pipe *pipe, enum modem_pipe_event event,
				    void *user_data)
{
	struct modem_ppp *ppp = (struct modem_ppp *)user_data;

	switch (event) {
	case MODEM_PIPE_EVENT_RECEIVE_READY:
		modem_work_submit(&ppp->process_work);
		break;

	case MODEM_PIPE_EVENT_OPENED:
	case MODEM_PIPE_EVENT_TRANSMIT_IDLE:
		modem_work_submit(&ppp->send_work);
		break;

	default:
		break;
	}
}

static void modem_ppp_send_handler(struct k_work *item)
{
	struct modem_ppp *ppp = CONTAINER_OF(item, struct modem_ppp, send_work);
	uint8_t *reserved;
	uint32_t reserved_size;
	uint32_t pushed;
	int ret;

	if (ppp->tx_pkt == NULL) {
		ppp->tx_pkt = k_fifo_get(&ppp->tx_pkt_fifo, K_NO_WAIT);
	}

	if (ring_buf_is_empty(&ppp->transmit_rb)) {
		/* Reset to initial state to maximise contiguous claim */
		ring_buf_reset(&ppp->transmit_rb);
	}

	if (ppp->tx_pkt != NULL) {
		/* Initialize wrap */
		if (ppp->transmit_state == MODEM_PPP_TRANSMIT_STATE_IDLE) {
			ppp->transmit_state = MODEM_PPP_TRANSMIT_STATE_SOF;
		}

		/* Claim as much space as possible */
		reserved_size = ring_buf_put_claim(&ppp->transmit_rb, &reserved, UINT32_MAX);
		/* Push wrapped data into claimed buffer */
		pushed = modem_ppp_wrap(ppp, reserved, reserved_size);
		/* Limit claimed data to what was actually pushed */
		ring_buf_put_finish(&ppp->transmit_rb, pushed);

		if (ppp->transmit_state == MODEM_PPP_TRANSMIT_STATE_IDLE) {
			net_pkt_unref(ppp->tx_pkt);
			ppp->tx_pkt = k_fifo_get(&ppp->tx_pkt_fifo, K_NO_WAIT);
		}
	}

#if CONFIG_MODEM_STATS
	advertise_transmit_buf_stats(ppp);
#endif

	while (!ring_buf_is_empty(&ppp->transmit_rb)) {
		reserved_size = ring_buf_get_claim(&ppp->transmit_rb, &reserved, UINT32_MAX);

		ret = modem_pipe_transmit(ppp->pipe, reserved, reserved_size);
		if (ret < 0) {
			ring_buf_get_finish(&ppp->transmit_rb, 0);
			break;
		}

		ring_buf_get_finish(&ppp->transmit_rb, (uint32_t)ret);

		if (ret < reserved_size) {
			break;
		}
	}
}

static void modem_ppp_process_handler(struct k_work *item)
{
	struct modem_ppp *ppp = CONTAINER_OF(item, struct modem_ppp, process_work);
	int ret;

	ret = modem_pipe_receive(ppp->pipe, ppp->receive_buf, ppp->buf_size);
	if (ret < 1) {
		return;
	}

#if CONFIG_MODEM_STATS
	advertise_receive_buf_stats(ppp, ret);
#endif

	for (int i = 0; i < ret; i++) {
		modem_ppp_process_received_byte(ppp, ppp->receive_buf[i]);
	}

	modem_work_submit(&ppp->process_work);
}

static void modem_ppp_ppp_api_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct modem_ppp *ppp = (struct modem_ppp *)dev->data;

	net_ppp_init(iface);
	net_if_flag_set(iface, NET_IF_NO_AUTO_START);
	net_if_carrier_off(iface);

	if (ppp->init_iface != NULL) {
		ppp->init_iface(iface);
	}

	ppp->iface = iface;
}

static int modem_ppp_ppp_api_start(const struct device *dev)
{
	const struct modem_ppp_config *config = (const struct modem_ppp_config *)dev->config;

	if (config == NULL || config->dev == NULL) {
		return 0;
	}

	return pm_device_runtime_get(config->dev);
}

static int modem_ppp_ppp_api_stop(const struct device *dev)
{
	const struct modem_ppp_config *config = (const struct modem_ppp_config *)dev->config;

	if (config == NULL || config->dev == NULL) {
		return 0;
	}

	return pm_device_runtime_put_async(config->dev, K_NO_WAIT);
}

static int modem_ppp_ppp_api_send(const struct device *dev, struct net_pkt *pkt)
{
	struct modem_ppp *ppp = (struct modem_ppp *)dev->data;

	if (atomic_test_bit(&ppp->state, MODEM_PPP_STATE_ATTACHED_BIT) == false) {
		return -EPERM;
	}

	/* Validate packet protocol */
	if ((net_pkt_is_ppp(pkt) == false) && (net_pkt_family(pkt) != AF_INET) &&
	    (net_pkt_family(pkt) != AF_INET6)) {
		return -EPROTONOSUPPORT;
	}

	/* Validate packet data length */
	if (((net_pkt_get_len(pkt) < 2) && (net_pkt_is_ppp(pkt) == true)) ||
	    ((net_pkt_get_len(pkt) < 1))) {
		return -ENODATA;
	}

	net_pkt_ref(pkt);
	k_fifo_put(&ppp->tx_pkt_fifo, pkt);
	modem_work_submit(&ppp->send_work);
	return 0;
}

#if defined(CONFIG_NET_STATISTICS_PPP)
static struct net_stats_ppp *modem_ppp_ppp_get_stats(const struct device *dev)
{
	struct modem_ppp *ppp = (struct modem_ppp *)dev->data;

	return &ppp->stats;
}
#endif

#if CONFIG_MODEM_STATS
static uint32_t get_buf_size(struct modem_ppp *ppp)
{
	return ppp->buf_size;
}

static void init_buf_stats(struct modem_ppp *ppp)
{
	char iface_name[CONFIG_MODEM_STATS_BUFFER_NAME_SIZE - sizeof("_xx")];
	char name[CONFIG_MODEM_STATS_BUFFER_NAME_SIZE];
	int ret;
	uint32_t size;

	ret = net_if_get_name(ppp->iface, iface_name, sizeof(iface_name));
	if (ret < 0) {
		snprintk(iface_name, sizeof(iface_name), "ppp");
	}

	size = get_buf_size(ppp);
	snprintk(name, sizeof(name), "%s_rx", iface_name);
	modem_stats_buffer_init(&ppp->receive_buf_stats, name, size);
	snprintk(name, sizeof(name), "%s_tx", iface_name);
	modem_stats_buffer_init(&ppp->transmit_buf_stats, name, size);
}
#endif

const struct ppp_api modem_ppp_ppp_api = {
	.iface_api.init = modem_ppp_ppp_api_init,
	.start = modem_ppp_ppp_api_start,
	.stop = modem_ppp_ppp_api_stop,
	.send = modem_ppp_ppp_api_send,
#if defined(CONFIG_NET_STATISTICS_PPP)
	.get_stats = modem_ppp_ppp_get_stats,
#endif
};

int modem_ppp_attach(struct modem_ppp *ppp, struct modem_pipe *pipe)
{
	if (atomic_test_bit(&ppp->state, MODEM_PPP_STATE_ATTACHED_BIT) == true) {
		return 0;
	}

	ppp->pipe = pipe;
	modem_pipe_attach(pipe, modem_ppp_pipe_callback, ppp);

	atomic_set_bit(&ppp->state, MODEM_PPP_STATE_ATTACHED_BIT);
	return 0;
}

struct net_if *modem_ppp_get_iface(struct modem_ppp *ppp)
{
	return ppp->iface;
}

void modem_ppp_release(struct modem_ppp *ppp)
{
	struct k_work_sync sync;
	struct net_pkt *pkt;

	if (atomic_test_and_clear_bit(&ppp->state, MODEM_PPP_STATE_ATTACHED_BIT) == false) {
		return;
	}

	modem_pipe_release(ppp->pipe);
	k_work_cancel_sync(&ppp->send_work, &sync);
	k_work_cancel_sync(&ppp->process_work, &sync);
	ppp->pipe = NULL;
	ppp->receive_state = MODEM_PPP_RECEIVE_STATE_HDR_SOF;

	if (ppp->rx_pkt != NULL) {
		net_pkt_unref(ppp->rx_pkt);
		ppp->rx_pkt = NULL;
	}

	ppp->transmit_state = MODEM_PPP_TRANSMIT_STATE_IDLE;

	if (ppp->tx_pkt != NULL) {
		net_pkt_unref(ppp->tx_pkt);
		ppp->tx_pkt = NULL;
	}

	while (1) {
		pkt = k_fifo_get(&ppp->tx_pkt_fifo, K_NO_WAIT);
		if (pkt == NULL) {
			break;
		}

		net_pkt_unref(pkt);
	}
}

int modem_ppp_init_internal(const struct device *dev)
{
	struct modem_ppp *ppp = (struct modem_ppp *)dev->data;

	atomic_set(&ppp->state, 0);
	ring_buf_init(&ppp->transmit_rb, ppp->buf_size, ppp->transmit_buf);
	k_work_init(&ppp->send_work, modem_ppp_send_handler);
	k_work_init(&ppp->process_work, modem_ppp_process_handler);
	k_fifo_init(&ppp->tx_pkt_fifo);

#if CONFIG_MODEM_STATS
	init_buf_stats(ppp);
#endif
	return 0;
}
