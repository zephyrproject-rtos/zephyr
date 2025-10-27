/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/sys/util.h>
#include <zephyr/drivers/i3c.h>
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
	/* TODO perhaps union target->dev with a generic void *
	 * to store the binding + index pair.
	 */
	struct mctp_i3c_endpoint *endpoint = (struct mctp_i3c_endpoint *)target->dev;
	struct mctp_binding_i3c_controller *binding = endpoint->binding;
	uint8_t endpoint_id = binding->endpoint_ids[endpoint->index];

	/* Callback already done *in a work queue* dedicated to i3c but is shared
	 * among all i3c buses. Likely only one per device anyways since its a
	 * beastly IP block so no need to requeue the request
	 */
	struct i3c_msg msg = {
		.buf = binding->rx_buf,
		.len = binding->rx_buf_len,
		.flags = I3C_MSG_READ | I3C_MSG_STOP,
	};

	/* TODO  Take rx sem */
	int rc = i3c_transfer(target, &msg, 1);

	if (rc != 0) {
		LOG_ERR("Error requesting read from endpoint %d", endpoint_id);
		return;
	}

	struct mctp_pktbuf *pkt = mctp_pktbuf_alloc(&binding->binding, msg.num_xfer);

	if (pkt == NULL) {
		LOG_ERR("Out of memory trying to allocate mctp pktbuf when receiving message from endpoint %d", endpoint_id);
		return;
	}

	memcpy(pkt->data, msg.buf, msg.num_xfer);

	/* TODO give rx sem */

	/* pkt is moved to mctp and no longer owned by the binding */
	mctp_bus_rx(&binding->binding, pkt);
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
	LOG_INF("sending message");
	int rc = i3c_transfer(b->endpoint_descs[endpoint_idx], &msg, 1);

	if (rc != 0) {
		LOG_WRN("Failed sending message to endpoint %d, result %d", hdr->dest, rc);
	}

	/* We must *always* return 0 despite errors, otherwise libmctp does not free the packet! */
	return 0;
}

#define MCTP_HDR_VER 0x01
#define MCTP_NULL_ADDR 0x00
#define MCTP_CONTROL_ID 0x00
#define MCTP_GET_VERSION_CODE 0xFE
const uint8_t MCTP_GET_VER_CMD[] = { MCTP_HDR_VER, MCTP_NULL_ADDR, MCTP_NULL_ADDR, 0x00, MCTP_CONTROL_ID, MCTP_GET_VERSION_CODE, 0xFF };


int mctp_i3c_controller_start(struct mctp_binding *binding)
{
	int rc = 0;

	struct mctp_binding_i3c_controller *b =
		CONTAINER_OF(binding, struct mctp_binding_i3c_controller, binding);

	if (rc != 0) {
		LOG_WRN("could not do dynamic address assignment");

	}

	for (int i = 0; i < b->num_endpoints; i++) {
		b->endpoint_descs[i] = i3c_device_find(b->i3c, &b->endpoint_pids[i]);

		if (b->endpoint_descs[i] == NULL) {
			LOG_WRN("No device found for i3c pid %llx",
			        (uint64_t)b->endpoint_pids[i].pid);
		}
	}

	mctp_binding_set_tx_enabled(binding, true);

	/* Initial flow of mctp over i3c wants us to request the MCTP version
	 * and then get back the version from an IBI with a payload.
	 */
	struct i3c_msg msg;
	msg.buf = (uint8_t *)MCTP_GET_VER_CMD;
	msg.len = sizeof(MCTP_GET_VER_CMD);

	/* Get the MCTP version of the i3c target by requesting it using physical bus addressing only and expecting the base
	 * version information returned (0xFF)
	 */
	for (int i = 0; i < b->num_endpoints; i++) {
		rc = i3c_ibi_enable(b->endpoint_descs[i]);
		if (rc != 0) {
			LOG_WRN("Could not enable IBI for I3C PID %llx",
			        (uint64_t)b->endpoint_pids[i].pid);
			continue;
		}
		rc = i3c_transfer(b->endpoint_descs[i], &msg, 1);
		if (rc != 0) {
			LOG_WRN("Could not transfer request of MCTP version to I3C PID %llx",
			        (uint64_t)b->endpoint_pids[i].pid);
		}
	}

	/* TODO maybe wait here for the IBI responses? Then assign EIDs using the Discovery Notify protocol */

	LOG_DBG("started");

	return 0;
}
