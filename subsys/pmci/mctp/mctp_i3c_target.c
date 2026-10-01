/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/pmci/mctp/mctp_i3c_common.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/kernel.h>
#include <zephyr/pmci/mctp/mctp_i3c_target.h>
#include <zephyr/pmci/mctp/mctp_i3c_pec.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mctp_i3c_target, CONFIG_MCTP_LOG_LEVEL);

void mctp_i3c_target_buf_write(struct i3c_target_config *config, uint8_t *val, uint32_t len)
{
	struct mctp_binding_i3c_target *b =
		CONTAINER_OF(config, struct mctp_binding_i3c_target, i3c_target_cfg);
	struct i3c_config_target cfg;

	LOG_DBG("I3C Target buffer write received, len=%d", len);

	/* Get dynamic address if not yet retrieved */
	if (b->dynamic_addr == 0) {
		if (i3c_config_get_target(b->i3c, &cfg) == 0) {
			b->dynamic_addr = cfg.dynamic_addr;
			LOG_DBG("Target dynamic address assigned: 0x%02x", b->dynamic_addr);
		}
	}

	/* PEC verification as per DSP0233 1.0.0: CRC-8 seeded with the address byte
	 * (dynamic_addr << 1 | W), computed over all received bytes except PEC.
	 */
	uint8_t addr_byte = b->dynamic_addr << 1U;

	LOG_DBG("PEC verification: addr=0x%02x, data_len=%u, received_pec=0x%02x",
		addr_byte, len, val[len - 1]);

	if (mctp_i3c_verify_pec(val, len, addr_byte) != 0) {
		LOG_WRN("PEC verification failed (addr: 0x%02x)", addr_byte);
		return;
	}

	/* Strip the trailing PEC byte before allocating pktbuf */
	size_t payload_len = len - I3C_PROTOCOL_PEC_SZ;

	b->rx_pkt = mctp_pktbuf_alloc(&b->binding, payload_len);

	if (b->rx_pkt == NULL) {
		LOG_WRN("Could not allocate pktbuf of len %zu to receive I3C message", payload_len);
		return;
	}

	LOG_DBG("Read %zu bytes from controller (PEC ok)", payload_len);
	memcpy(b->rx_pkt->data, val, payload_len);
}

/*
 * mctp_bus_rx() can synchronously call the binding tx (e.g. a control-message
 * response), which blocks on a semaphore. stop_cb runs in the driver ISR, so
 * deliver received packets from thread context here instead.
 *
 * Packets are pulled from rx_msgq, which stop_cb fills: draining the whole
 * queue keeps every fragment of a multi-packet message in order.
 */
static void mctp_i3c_target_rx_work(struct k_work *work)
{
	struct mctp_binding_i3c_target *b =
		CONTAINER_OF(work, struct mctp_binding_i3c_target, rx_work);
	struct mctp_pktbuf *pkt;

	while (k_msgq_get(&b->rx_msgq, &pkt, K_NO_WAIT) == 0) {
		struct i3c_config_target cfg;
		size_t len = pkt->end - pkt->start;
		uint8_t addr_byte;

		/* Get dynamic address if not yet retrieved */
		if (b->dynamic_addr == 0) {
			if (i3c_config_get_target(b->i3c, &cfg) == 0) {
				b->dynamic_addr = cfg.dynamic_addr;
			}
		}

		/* PEC as per DSP0233 1.0.0: seeded with the address byte (dynamic_addr
		 * << 1 | W), computed over all received bytes except the trailing PEC.
		 */
		addr_byte = b->dynamic_addr << 1U;

		if (len < I3C_PROTOCOL_PEC_SZ) {
			LOG_WRN("I3C RX packet too short to contain a PEC (%zu bytes)", len);
		} else if (mctp_i3c_verify_pec(&pkt->data[pkt->start], len, addr_byte) != 0) {
			LOG_WRN("PEC verification failed (addr: 0x%02x)", addr_byte);
		} else {
			/* Strip the trailing PEC before handing up to libmctp */
			pkt->end -= I3C_PROTOCOL_PEC_SZ;
			mctp_bus_rx(&b->binding, pkt);
		}

		mctp_pktbuf_free(pkt);
	}
}

int mctp_i3c_target_stop(struct i3c_target_config *config)
{
	struct mctp_binding_i3c_target *b =
		CONTAINER_OF(config, struct mctp_binding_i3c_target, i3c_target_cfg);

	if (b->tx_pkt != NULL) {
		b->tx_pkt = NULL;
		k_sem_give(b->tx_complete);
	}

	if (b->rx_pkt != NULL) {
		/*
		 * PEC verification uses the hardware CRC (which takes a mutex) and
		 * mctp_bus_rx() may block, but stop_cb runs in the driver ISR. Queue
		 * the packet and handle it from thread context.
		 */
		if (k_msgq_put(&b->rx_msgq, &b->rx_pkt, K_NO_WAIT) != 0) {
			LOG_WRN("I3C RX queue full, dropping packet");
			mctp_pktbuf_free(b->rx_pkt);
		}
		b->rx_pkt = NULL;
		k_work_submit(&b->rx_work);
	}

	return 0;
}

int mctp_i3c_target_write_received(struct i3c_target_config *config, uint8_t val)
{
	struct mctp_binding_i3c_target *b =
		CONTAINER_OF(config, struct mctp_binding_i3c_target, i3c_target_cfg);

	if (b->rx_pkt == NULL) {
		/*
		 * First byte of the transaction, so allocate
		 * Do it here since not all i3c drivers might call write_requested
		 *
		 * mctp_pktbuf_alloc implementation always allocates a fixed amount per binding.
		 * The 0 len passed in indicates that we start with 0 bytes valid in the allocated
		 * buffer. The bytes are then pushed 1-by-1 via mctp_pktbuf_push.
		 */
		b->rx_pkt = mctp_pktbuf_alloc(&b->binding, 0);
		if (b->rx_pkt == NULL) {
			LOG_ERR("Could not allocate pktbuf for I3C RX");
			return -ENOMEM;
		}
	}

	if (mctp_pktbuf_push(b->rx_pkt, &val, 1) < 0) {
		LOG_WRN("I3C RX packet exceeded max size %zu", b->binding.pkt_size);
		return -EMSGSIZE;
	}

	return 0;
}

int mctp_i3c_target_read_processed(struct i3c_target_config *config, uint8_t *val)
{
	struct mctp_binding_i3c_target *b =
		CONTAINER_OF(config, struct mctp_binding_i3c_target, i3c_target_cfg);
	uint16_t pkt_len;

	if (b->tx_pkt == NULL) {
		LOG_ERR("I3C read with no TX packet staged");
		return -EIO;
	}

	/* pkt_len includes the PEC appended by mctp_i3c_target_tx() */
	pkt_len = b->tx_pkt->end - b->tx_pkt->start;

	if (b->tx_ptr >= pkt_len) {
		/* Controller read past the payload and PEC */
		LOG_WRN("I3C read past TX packet end: tx_ptr=%u, len=%u", b->tx_ptr, pkt_len);
		return -EIO;
	}

	/* Serve the next byte (payload, then the trailing PEC) */
	*val = b->tx_pkt->data[b->tx_pkt->start + b->tx_ptr++];

	return 0;
}

const struct i3c_target_callbacks mctp_i3c_target_callbacks = {
	.write_received_cb = mctp_i3c_target_write_received,
	.read_processed_cb = mctp_i3c_target_read_processed,
#ifdef CONFIG_I3C_TARGET_BUFFER_MODE
	.buf_write_received_cb = mctp_i3c_target_buf_write,
#endif
	.stop_cb = mctp_i3c_target_stop,
};

/*
 * libmctp wants us to return once the packet is sent not before
 * so the entire process of raising IBI and writing the data
 * needs to complete before we can move on.
 *
 * this is called for each packet in the packet queue libmctp provides
 */
int mctp_i3c_target_tx(struct mctp_binding *binding, struct mctp_pktbuf *pkt)
{
	struct mctp_binding_i3c_target *b =
		CONTAINER_OF(binding, struct mctp_binding_i3c_target, binding);
	int ret;

	k_sem_take(b->tx_lock, K_FOREVER);

	b->tx_pkt = pkt;
	b->tx_ptr = 0U;

	size_t pktsize = pkt->end - pkt->start;

	if (pktsize > MCTP_PACKET_SIZE(MCTP_I3C_MAX_PKT_SIZE)) {
		LOG_ERR("I3C TX packet too large to send: %zu bytes", pktsize);
		ret = -EINVAL;
		goto out;
	}

	/*
	 * Append the PEC to the packet so it is sent as the trailing byte. The
	 * tx_storage buffer reserves pkt_trailer (I3C_PROTOCOL_PEC_SZ) bytes for
	 * this. read_processed() then serves it from tx_pkt on byte-by-byte
	 * drivers, and the buffer passed below carries it for buffer-mode drivers.
	 * PEC per DSP0233 1.0.0: CRC-8 seeded with the address byte
	 * (dynamic_addr << 1 | R), computed over the payload.
	 */
	uint8_t addr_byte = (b->dynamic_addr << 1U) | 1U;
	uint8_t pec = mctp_i3c_calculate_pec(&pkt->data[pkt->start], pktsize, addr_byte);

	if (mctp_pktbuf_push(pkt, &pec, I3C_PROTOCOL_PEC_SZ) < 0) {
		LOG_ERR("No room to append PEC to I3C TX packet");
		ret = -ENOMEM;
		goto out;
	}

	/* Some I3C IP need the data in the TX FIFO before the IBI is raised */
	ret = i3c_target_tx_write(b->i3c, &pkt->data[pkt->start], pktsize + I3C_PROTOCOL_PEC_SZ, 0);
	if (ret < 0) {
		LOG_ERR("i3c_target_tx_write failed: %d", ret);
		goto out;
	}

#ifdef CONFIG_I3C_USE_IBI
	uint8_t payload = MCTP_I3C_MDB_PENDING_READ;
	struct i3c_ibi ibi_req = {
		.ibi_type = I3C_IBI_TARGET_INTR,
		.payload = &payload,
		.payload_len = 1,
	};

	ret = i3c_ibi_raise(b->i3c, &ibi_req);
	__ASSERT_NO_MSG(ret == 0);
#else
	ret = 0;
#endif
	k_sem_take(b->tx_complete, K_FOREVER);

out:
	k_sem_give(b->tx_lock);

	return ret;
}

int mctp_i3c_target_start(struct mctp_binding *binding)
{
	struct mctp_binding_i3c_target *b =
		CONTAINER_OF(binding, struct mctp_binding_i3c_target, binding);
	struct i3c_config_target config;
	int rc;

	k_msgq_init(&b->rx_msgq, (char *)b->rx_msgq_slots, sizeof(struct mctp_pktbuf *),
		    MCTP_I3C_RX_QUEUE_DEPTH);
	k_work_init(&b->rx_work, mctp_i3c_target_rx_work);

	/* Get target device configuration to retrieve dynamic address */
	rc = i3c_config_get_target(b->i3c, &config);
	if (rc == 0) {
		b->dynamic_addr = config.dynamic_addr;
		if (b->dynamic_addr != 0) {
			LOG_DBG("Target dynamic address: 0x%02x", b->dynamic_addr);
		} else {
			LOG_WRN("Target dynamic address not yet assigned");
		}
	} else {
		LOG_ERR("Failed to get target configuration: %d", rc);
	}

	/* Register i3c target */
	rc = i3c_target_register(b->i3c, &b->i3c_target_cfg);
	if (rc != 0) {
		LOG_ERR("Failed to register i3c target");
		goto out;
	}
	mctp_binding_set_tx_enabled(binding, true);

out:
	return 0;
}
