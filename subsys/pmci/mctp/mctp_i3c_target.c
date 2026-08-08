/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/pmci/mctp/mctp_i3c_common.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
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

int mctp_i3c_target_stop(struct i3c_target_config *config)
{
	struct mctp_binding_i3c_target *b =
		CONTAINER_OF(config, struct mctp_binding_i3c_target, i3c_target_cfg);

	if (b->tx_pkt != NULL) {
		b->tx_pkt = NULL;
		k_sem_give(b->tx_complete);
	}

	if (b->rx_pkt != NULL) {
		mctp_bus_rx(&b->binding, b->rx_pkt);
		mctp_pktbuf_free(b->rx_pkt);
		b->rx_pkt = NULL;
	}

	return 0;
}

const struct i3c_target_callbacks mctp_i3c_target_callbacks = {
	.buf_write_received_cb = mctp_i3c_target_buf_write,
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

	uint8_t payload = MCTP_I3C_MDB_PENDING_READ;
	struct i3c_ibi ibi_req = {
		.ibi_type = I3C_IBI_TARGET_INTR,
		.payload = &payload,
		.payload_len = 1,
	};
	int ret;

	k_sem_take(b->tx_lock, K_FOREVER);

	b->tx_pkt = pkt;

	/* Calculate packet size and prepare TX buffer with PEC */
	size_t pktsize = pkt->end - pkt->start;
	uint8_t tx_buf[MCTP_PACKET_SIZE(MCTP_I3C_MAX_PKT_SIZE) + I3C_PROTOCOL_PEC_SZ];

	if (pktsize > MCTP_PACKET_SIZE(MCTP_I3C_MAX_PKT_SIZE)) {
		LOG_ERR("Packet too large to send: %zu bytes", pktsize);
		ret = -EINVAL;
		goto out;
	}

	/* Copy data and calculate PEC (Packet Error Code) */
	memcpy(tx_buf, pkt->data + pkt->start, pktsize);
	uint8_t addr_byte = b->dynamic_addr << 1U;

	tx_buf[pktsize] = mctp_i3c_calculate_pec(tx_buf, pktsize, addr_byte);

	/* Some I3C IP need to have data at TX fifo before raising IBI */
	ret = i3c_target_tx_write(b->i3c, tx_buf, pktsize + I3C_PROTOCOL_PEC_SZ, 0);
	if (ret < 0) {
		LOG_ERR("i3c_target_tx_write failed: %d", ret);
		goto out;
	}

	ret = i3c_ibi_raise(b->i3c, &ibi_req);
	__ASSERT_NO_MSG(ret == 0);
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
