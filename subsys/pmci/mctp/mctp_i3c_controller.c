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
#include <zephyr/pmci/mctp/mctp_i3c_pec.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mctp_i3c_controller, CONFIG_MCTP_LOG_LEVEL);

static inline void mctp_i3c_recv_msg(struct mctp_binding_i3c_controller *binding,
				     size_t endpoint_idx)
{
	/* +1 to accommodate the trailing PEC byte */
	uint8_t rx_buf[MCTP_I3C_MAX_PKT_SIZE + I3C_PROTOCOL_PEC_SZ];
	int rc;
	struct mctp_pktbuf *pkt;
	struct i3c_device_desc *dev = binding->endpoint_i3c_devs[endpoint_idx];

	if (dev == NULL) {
		LOG_ERR("No device descriptor for endpoint %d", endpoint_idx);
		return;
	}

	/* Callback already done *in a work queue* dedicated to i3c but is shared
	 * among all i3c buses. Likely only one per device anyways since its a
	 * beastly IP block so no need to requeue the request
	 */
	struct i3c_msg msg = {
		.buf = rx_buf,
		.len = sizeof(rx_buf),
		.flags = I3C_MSG_READ | I3C_MSG_STOP,
	};

	rc = i3c_transfer(dev, &msg, 1);
	if (rc != 0) {
		LOG_ERR("Error requesting read from endpoint %d: %d", endpoint_idx, rc);
		return;
	}

	/* PEC verification as per DSP0233 1.0.0: CRC-8 seeded with the address byte
	 * (dynamic_addr << 1 | R/W), computed over all received bytes except PEC.
	 */
	uint8_t addr_byte = (dev->dynamic_addr << 1U) | 1U;

	if (mctp_i3c_verify_pec(rx_buf, msg.num_xfer, addr_byte) != 0) {
		return;
	}

	/* Strip the trailing PEC byte before handing to libmctp */
	size_t payload_len = msg.num_xfer - I3C_PROTOCOL_PEC_SZ;

	pkt = mctp_pktbuf_alloc(&binding->binding, payload_len);
	if (pkt == NULL) {
		LOG_ERR("Out of memory to allocate buffer when receiving message from endpoint %d",
			endpoint_idx);
		return;
	}

	LOG_DBG("Read %zu bytes from endpoint %d (PEC ok)", payload_len, endpoint_idx);
	memcpy(pkt->data, rx_buf, payload_len);

	mctp_bus_rx(&binding->binding, pkt);
	mctp_pktbuf_free(pkt);
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
	/* Which i3c device am I sending this to? */
	struct mctp_hdr *hdr = mctp_pktbuf_hdr(pkt);
	struct mctp_binding_i3c_controller *b =
		CONTAINER_OF(binding, struct mctp_binding_i3c_controller, binding);
	uint8_t tx_buf[MCTP_PACKET_SIZE(MCTP_I3C_MAX_PKT_SIZE) + I3C_PROTOCOL_PEC_SZ];

	/* pkt->end - pkt->start spans the 4-byte MCTP header plus payload,
	 * so max size is MCTP_I3C_MAX_PKT_SIZE + sizeof(struct mctp_hdr).
	 * Use size_t to avoid uint8_t overflow at 259 bytes.
	 */
	size_t pktsize = pkt->end - pkt->start;
	int endpoint_idx = -1;

	for (int i = 0; i < b->num_endpoints; i++) {
		if (b->endpoint_ids[i] == hdr->dest) {
			endpoint_idx = i;
			break;
		}
	}

	/* MCTP_EID_NULL (0) is used for pre-assignment discovery commands
	 * (e.g. Get Endpoint ID). Route to the first available endpoint.
	 */
	if (endpoint_idx == -1 && hdr->dest == MCTP_EID_NULL && b->num_endpoints > 0) {
		endpoint_idx = 0;
	}

	if (endpoint_idx == -1) {
		LOG_ERR("Invalid endpoint id %d when sending message", hdr->dest);
		return 0;
	}

	/* PEC (DSP0233 1.0.0): CRC-8 seeded with the address byte
	 * (dynamic_addr << 1 | W), computed over the packet data.
	 * Copy data + PEC into a local buffer to send in a single transfer.
	 */
	if (pktsize > MCTP_PACKET_SIZE(MCTP_I3C_MAX_PKT_SIZE)) {
		LOG_ERR("Packet too large to send: %zu bytes", pktsize);
		return 0;
	}

	uint8_t addr_byte = b->endpoint_i3c_devs[endpoint_idx]->dynamic_addr << 1U;

	memcpy(tx_buf, &pkt->data[pkt->start], pktsize);
	tx_buf[pktsize] = mctp_i3c_calculate_pec(tx_buf, pktsize, addr_byte);

	struct i3c_msg msg = {
		.buf = tx_buf,
		.len = pktsize + I3C_PROTOCOL_PEC_SZ,
		.flags = I3C_MSG_WRITE | I3C_MSG_NBCH | I3C_MSG_STOP,
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
		/* Assign the callback before enabling IBI to avoid a race where
		 * the target raises an IBI between i3c_ibi_enable() and the
		 * callback assignment, which would cause a fault on a NULL or
		 * uninitialized function pointer.
		 */
		b->endpoint_i3c_devs[i]->ibi_cb = mctp_i3c_ibi_cb;
		rc = i3c_ibi_enable(b->endpoint_i3c_devs[i]);
		if (rc != 0) {
			/* Log but do not clear ibi_cb: IBI may already be
			 * enabled from a previous mctp_register_bus call on
			 * this binding (e.g. across test iterations), in which
			 * case the callback must remain set.
			 */
			LOG_WRN("Could not enable IBI for I3C PID %llx (rc=%d)",
				(uint64_t)b->endpoint_i3c_devs[i]->pid, rc);
		}
	}

	mctp_binding_set_tx_enabled(binding, true);

	LOG_DBG("Started");

	return 0;
}
