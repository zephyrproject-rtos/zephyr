/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "zephyr/drivers/i3c.h"
#include <zephyr/sys/__assert.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/pmci/mctp/mctp_i3c_controller.h>
#include <crc-16-ccitt.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mctp_i3c_controller, CONFIG_MCTP_LOG_LEVEL);

void mctp_i3c_ibi_cb(struct i3c_device_desc *target,
                     struct i3c_ibi_payload *payload)
{
	struct mctp_i3c_endpoint *endpoint =
		CONTAINER_OF(target, struct mctp_i3c_endpoint, i3c_device);
	int endpoint_idx = ARRAY_INDEX(endpoint->binding->endpoints, endpoint);
	uint8_t endpoint_id = endpoint->binding->endpoint_ids[endpoint_idx];

	/* Callback already done *in a work queue* dedicated to i3c but is shared
	 * among all i3c buses. Likely only one per device anyways since its a
	 * beastly IP block so no need to requeue the request
	 */
	struct i3c_msg msg = {
		.buf = endpoint->binding->rx_buf,
		.len = endpoint->binding->rx_buf_len,
		.flags = I3C_MSG_READ | I3C_MSG_STOP,
	};

	/* TODO  Take rx sem */
	int rc = i3c_transfer(target, &msg, 1);

	if (rc != 0) {
		LOG_ERR("Error requesting read from endpoint %d", endpoint_id);
		return;
	}

	struct mctp_pktbuf *pkt = mctp_pktbuf_alloc(&endpoint->binding->binding, msg.num_xfer);

	if (pkt == NULL) {
		LOG_ERR("Out of memory trying to allocate mctp pktbuf when receiving message from endpoint %d", endpoint_id);
		return;
	}

	memcpy(pkt->data, msg.buf, msg.num_xfer);

	/* TODO give rx sem */

	/* pkt is moved to mctp and no longer owned by the binding */
	mctp_bus_rx(&endpoint->binding->binding, pkt);
}

int mctp_i3c_controller_tx(struct mctp_binding *binding, struct mctp_pktbuf *pkt)
{
	/* Which i2c device am I sending this to? */
	struct mctp_hdr *hdr = mctp_pktbuf_hdr(pkt);
	struct mctp_binding_i3c_controller *b =
		CONTAINER_OF(binding, struct mctp_binding_i3c_controller, binding);
	uint8_t pktsize = pkt->end - pkt->start;
	int endpoint_idx = -1;
	struct mctp_i3c_endpoint *endpoint;

	for (int i = 0; i < b->num_endpoints; i++) {
		if (b->endpoint_ids[i] == hdr->dest) {
			endpoint_idx = i;
			break;
		}
	}

	if (endpoint_idx == -1) {
		LOG_ERR("Invalid endpoint id %d", hdr->dest);
		return 0;
	}

	endpoint= &b->endpoints[endpoint_idx];

	struct i3c_msg msg = {
		.buf = &pkt->data[pkt->start],
		.len = pktsize,
		.flags = I3C_MSG_WRITE | I3C_MSG_STOP,
	};
	int rc = i3c_transfer(&endpoint->i3c_device, &msg, 1);

	if (rc != 0) {
		LOG_WRN("Failed sending message to endpoint %d", hdr->dest);
	}

	/* We must *always* return 0 despite errors, otherwise libmctp does not free the packet! */
	return 0;
}

int mctp_i3c_controller_start(struct mctp_binding *binding)
{
	struct mctp_binding_i3c_controller *b =
		CONTAINER_OF(binding, struct mctp_binding_i3c_controller, binding);

	for (int i = 0; i < b->num_endpoints; i++) {
		b->endpoints[i].binding = b;
		b->endpoints[i].i3c_device.bus = b->i3c;
		/* TODO sort out how the address is assigned for the i3c device, statically? */
		/* b->endpoints[i].i3c_device.static_addr = b->i3c_addresses[i]; */
	}

	mctp_binding_set_tx_enabled(binding, true);

	for (int i = 0; i < b->num_endpoints; i++) {
		i3c_ibi_enable(&b->endpoints[i].i3c_device);
	}

	LOG_DBG("started");

	return 0;
}
