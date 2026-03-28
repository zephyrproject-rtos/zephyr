/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "zephyr/pmci/mctp/mctp_i3c_common.h"
#include <zephyr/sys/__assert.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/pmci/mctp/mctp_i3c_target.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mctp_i3c_target, CONFIG_MCTP_LOG_LEVEL);

void mctp_i3c_target_buf_write(struct i3c_target_config *config, uint8_t *val, uint32_t len)
{
	struct mctp_binding_i3c_target *b =
		CONTAINER_OF(config, struct mctp_binding_i3c_target, i3c_target_cfg);

	LOG_DBG("I3C Target buffer write received, len=%d", len);

	b->rx_pkt = mctp_pktbuf_alloc(&b->binding, len);

	if (b->rx_pkt == NULL) {
		LOG_WRN("Could not allocate pktbuf of len %d to receive I3C message", len);
		return;
	}

	memcpy(b->rx_pkt->data, val, len);
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

	/* Some I3C IP need to have data at TX fifo before raising IBI */
	ret = i3c_target_tx_write(b->i3c, pkt->data + pkt->start, pkt->end - pkt->start, 0);
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
	int rc;

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
