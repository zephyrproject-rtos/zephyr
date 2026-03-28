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
#include <zephyr/pmci/mctp/mctp_i3c_controller.h>
#include <zephyr/pmci/mctp/mctp_i3c_endpoint.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mctp_i3c_controller, CONFIG_MCTP_LOG_LEVEL);

static inline void mctp_i3c_recv_msg(struct mctp_binding_i3c_controller *binding,
				     size_t endpoint_idx)
{
	uint8_t rx_buf[256];

	/* Callback already done *in a work queue* dedicated to i3c but is shared
	 * among all i3c buses. Likely only one per device anyways since its a
	 * beastly IP block so no need to requeue the request
	 */
	struct i3c_msg msg = {
		.buf = rx_buf,
		.len = sizeof(rx_buf),
		.flags = I3C_MSG_READ | I3C_MSG_STOP,
	};

	int rc = i3c_transfer(binding->endpoint_i3c_devs[endpoint_idx], &msg, 1);

	if (rc != 0) {
		LOG_ERR("Error requesting read from endpoint %d: %d", endpoint_idx, rc);
		return;
	}
	LOG_DBG("Read %d bytes from endpoint %d", msg.num_xfer, endpoint_idx);

	struct mctp_pktbuf *pkt = mctp_pktbuf_alloc(&binding->binding, msg.num_xfer);

	if (pkt == NULL) {
		LOG_ERR("Out of memory to allocate buffer when receiving message from endpoint %d",
			endpoint_idx);
		return;
	}

	memcpy(pkt->data, msg.buf, msg.num_xfer);

	/* pkt is moved to mctp and no longer owned by the binding */
	mctp_bus_rx(&binding->binding, pkt);
}


int mctp_i3c_ibi_cb(struct i3c_device_desc *target,
		    struct i3c_ibi_payload *payload)
{
	struct mctp_binding_i3c_controller *binding = mctp_i3c_endpoint_binding(target->dev);
	int endpoint_idx = -1;

	LOG_DBG("IBI received from target %p PID %llx BCR %x",
		target,
		(uint64_t)target->pid,
		target->bcr);

	for (int i = 0; i < binding->num_endpoints; i++) {
		if (binding->devices[i] == target->dev) {
			endpoint_idx = i;
			break;
		}
	}

	if (endpoint_idx == -1) {
		LOG_WRN("IBI from unknown I3C Device, maybe missing in devicetree? %p",
			target->dev);
		return -ENODEV;
	}

	if (payload->payload_len >= 1 && payload->payload[0] == MCTP_I3C_MDB_PENDING_READ) {
		LOG_DBG("Pending read IBI received from endpoint %d len: %u [0x%x]", endpoint_idx,
			payload->payload_len,
			payload->payload[0]);
		mctp_i3c_recv_msg(binding, endpoint_idx);
	} else {
		LOG_WRN("Expected a IBI payload with the mandatory pending read byte, something "
			"broke");
	}

	return 0;
}

int mctp_i3c_controller_tx(struct mctp_binding *binding, struct mctp_pktbuf *pkt)
{
	/* Which i2c device am I sending this to? */
	struct mctp_hdr *hdr = mctp_pktbuf_hdr(pkt);
	struct mctp_binding_i3c_controller *b =
		CONTAINER_OF(binding, struct mctp_binding_i3c_controller, binding);
	uint8_t pktsize = pkt->end - pkt->start;
	int endpoint_idx = -1;

	for (int i = 0; i < b->num_endpoints; i++) {
		if (b->endpoint_ids[i] == hdr->dest) {
			endpoint_idx = i;
			break;
		}
	}

	if (endpoint_idx == -1) {
		LOG_ERR("Invalid endpoint id %d when sending message", hdr->dest);
		return 0;
	}

	struct i3c_msg msg = {
		.buf = &pkt->data[pkt->start],
		.len = pktsize,
		.flags = I3C_MSG_WRITE | I3C_MSG_STOP,
	};

	int rc = i3c_transfer(b->endpoint_i3c_devs[endpoint_idx], &msg, 1);

	if (rc != 0) {
		LOG_WRN("Failed sending message to endpoint %d, result %d", hdr->dest, rc);
	}

	/* We must *always* return 0 despite errors, otherwise libmctp does not free the packet! */
	return 0;
}

int mctp_i3c_controller_start(struct mctp_binding *binding)
{
	int rc;

	struct mctp_binding_i3c_controller *b =
		CONTAINER_OF(binding, struct mctp_binding_i3c_controller, binding);

	for (int i = 0; i < b->num_endpoints; i++) {
		mctp_i3c_endpoint_bind(b->devices[i], b, &b->endpoint_i3c_devs[i]);
		LOG_INF("Enabling IBI for TARGET %p PID %llx BCR %x",
			b->endpoint_i3c_devs[i],
			b->endpoint_i3c_devs[i]->pid,
			b->endpoint_i3c_devs[i]->bcr);
		rc = i3c_ibi_enable(b->endpoint_i3c_devs[i]);
		if (rc != 0) {
			LOG_WRN("Could not enable IBI for I3C PID %llx",
				(uint64_t)b->endpoint_i3c_devs[i]->pid);
			continue;
		}
		b->endpoint_i3c_devs[i]->ibi_cb = mctp_i3c_ibi_cb;
	}

	mctp_binding_set_tx_enabled(binding, true);

	LOG_DBG("Started");

	return 0;
}
