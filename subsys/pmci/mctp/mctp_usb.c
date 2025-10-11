/*
 * Copyright (c) 2024 Intel Corporation
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/sys/__assert.h>
#include <zephyr/kernel.h>
#include <zephyr/pmci/mctp/mctp_usb.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mctp_usb, CONFIG_MCTP_LOG_LEVEL);

#include "libmctp-alloc.h"

#define MCTP_USB_DMTF_0 0x1A
#define MCTP_USB_DMTF_1 0xB4

static void mctp_usb_reset_rx_state(struct mctp_binding_usb *usb)
{
	if (usb->rx_pkt != NULL) {
		mctp_pktbuf_free(usb->rx_pkt);
	}

	usb->rx_data_idx = 0;
	usb->rx_state = STATE_WAIT_HDR_DMTF0;
}

void mctp_usb_data_recv_cb(const void *const buf, const uint16_t size, const void *const priv)
{
	struct mctp_binding_usb *usb = (struct mctp_binding_usb *)priv;
	uint8_t *rx_buf = (uint8_t *)buf;

	LOG_DBG("size=%d", size);
	LOG_HEXDUMP_DBG(buf, size, "buf = ");

	for (int i = 0; i < size; i++) {
		switch (usb->rx_state) {
		case STATE_WAIT_HDR_DMTF0: {
			if (rx_buf[i] == MCTP_USB_DMTF_0) {
				usb->rx_state = STATE_WAIT_HDR_DMTF1;
			} else {
				LOG_ERR("Invalid DMTF0 %02X", rx_buf[i]);
				goto recv_exit;
			}
			break;
		}
		case STATE_WAIT_HDR_DMTF1: {
			if (rx_buf[i] == MCTP_USB_DMTF_1) {
				usb->rx_state = STATE_WAIT_HDR_RSVD0;
			} else {
				LOG_ERR("Invalid DMTF1 %02X", rx_buf[i]);
				usb->rx_state = STATE_WAIT_HDR_DMTF0;
				goto recv_exit;
			}
			break;
		}
		case STATE_WAIT_HDR_RSVD0: {
			if (rx_buf[i] == 0) {
				usb->rx_state = STATE_WAIT_HDR_LEN;
			} else {
				LOG_ERR("Invalid RSVD0 %02X", rx_buf[i]);
				usb->rx_state = STATE_WAIT_HDR_DMTF0;
				goto recv_exit;
			}
			break;
		}
		case STATE_WAIT_HDR_LEN: {
			if (rx_buf[i] > MCTP_USB_MAX_PACKET_LENGTH || rx_buf[i] == 0) {
				LOG_ERR("Invalid LEN %02X", rx_buf[i]);
				usb->rx_state = STATE_WAIT_HDR_DMTF0;
				goto recv_exit;
			}

			usb->rx_data_idx = 0;
			usb->rx_pkt = mctp_pktbuf_alloc(&usb->binding, rx_buf[i]);
			if (usb->rx_pkt == NULL) {
				LOG_ERR("Could not allocate PKT buffer");
				mctp_usb_reset_rx_state(usb);
				goto recv_exit;
			} else {
				usb->rx_state = STATE_DATA;
			}

			LOG_DBG("Expecting LEN=%d", (int)rx_buf[i]);

			break;
		}
		case STATE_DATA: {
			usb->rx_pkt->data[usb->rx_data_idx++] = rx_buf[i];

			if (usb->rx_data_idx == usb->rx_pkt->end) {
				LOG_DBG("Packet complete");
				mctp_bus_rx(&usb->binding, usb->rx_pkt);
				mctp_usb_reset_rx_state(usb);
			}

			break;
		}
		}
	}

recv_exit:
}

void mctp_usb_error_cb(const int err, const uint8_t ep, const void *const priv)
{
	struct mctp_binding_usb *usb = (struct mctp_binding_usb *)priv;

	LOG_ERR("Encountered USB error %d, endpoint %d, on %s", err, ep, usb->binding.name);
}

int mctp_usb_tx(struct mctp_binding *binding, struct mctp_pktbuf *pkt)
{
	struct mctp_binding_usb *usb = CONTAINER_OF(binding, struct mctp_binding_usb, binding);
	size_t len = mctp_pktbuf_size(pkt);
	int err;

	if (len > MCTP_USB_MAX_PACKET_LENGTH) {
		return -E2BIG;
	}

	err = k_sem_take(&usb->tx_lock, K_MSEC(CONFIG_MCTP_USB_TX_TIMEOUT));
	if (err != 0) {
		LOG_ERR("Semaphore could not be obtained");
		return err;
	}

	usb->tx_buf[0] = MCTP_USB_DMTF_0;
	usb->tx_buf[1] = MCTP_USB_DMTF_1;
	usb->tx_buf[2] = 0;
	usb->tx_buf[3] = len;

	memcpy((void *)&usb->tx_buf[MCTP_USB_HEADER_SIZE], pkt->data, len);

	LOG_HEXDUMP_DBG(usb->tx_buf, len + 4, "buf = ");

	err = usbd_mctp_send(usb->usb_inst, (void *)usb->tx_buf, len + MCTP_USB_HEADER_SIZE);
	if (err != 0) {
		LOG_ERR("Failed to send MCTP data over USB");
		return err;
	}

	k_sem_give(&usb->tx_lock);

	return 0;
}

int mctp_usb_start(struct mctp_binding *binding)
{
	struct mctp_binding_usb *usb = CONTAINER_OF(binding, struct mctp_binding_usb, binding);

	k_sem_init(&usb->tx_lock, 1, 1);
	mctp_binding_set_tx_enabled(binding, true);

	return 0;
}
