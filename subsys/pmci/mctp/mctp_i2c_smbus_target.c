/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * MCTP over SMBus/I2C target binding for Zephyr.
 *
 * RX path:
 *   SMBus Block Write with command code 0x0f, byte count, payload bytes,
 *   and PEC. The first payload byte is the source slave address, followed
 *   by MCTP bytes.
 *
 * TX path:
 *   Queue MCTP bytes and issue an SMBus Block Write back to the host by
 *   using the same controller in transient controller mode. The underlying
 *   I2C driver is responsible for restoring target mode afterwards.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

#include <errno.h>
#include <string.h>

#include "libmctp.h"
#include <zephyr/pmci/mctp/mctp_i2c_smbus_target.h>

LOG_MODULE_REGISTER(mctp_i2c_smbus_target, CONFIG_MCTP_LOG_LEVEL);

/* First payload byte is source slave address. */
#define MCTP_SMBUS_SRC_ADDR_LEN 1U

/* TX retry behavior */
#define MCTP_I2C_SMBUS_TX_RETRIES        5
#define MCTP_I2C_SMBUS_TX_RETRY_BASE_MS  2

enum mctp_i2c_smbus_rx_state {
	MCTP_I2C_SMBUS_RX_WAIT_CMD = 0,
	MCTP_I2C_SMBUS_RX_WAIT_COUNT,
	MCTP_I2C_SMBUS_RX_WAIT_DATA,
	MCTP_I2C_SMBUS_RX_WAIT_PEC,
	MCTP_I2C_SMBUS_RX_DROP,
};

/* SMBus PEC: CRC-8, polynomial 0x07, init 0x00. */
static uint8_t crc8_update(uint8_t crc, uint8_t data)
{
	crc ^= data;

	for (int i = 0; i < 8; i++) {
		crc = (crc & 0x80U) ? (uint8_t)((crc << 1) ^ 0x07U)
				    : (uint8_t)(crc << 1);
	}

	return crc;
}

/* SMBus Block Write PEC covers:
 *   (dest_addr << 1 | write), command, count, payload[count]
 */
static uint8_t smbus_pec_calc(uint8_t dest_addr_7bit, uint8_t cmd,
			      uint8_t count, const uint8_t *data)
{
	uint8_t crc = 0U;

	crc = crc8_update(crc, (uint8_t)((dest_addr_7bit << 1) | 0U));
	crc = crc8_update(crc, cmd);
	crc = crc8_update(crc, count);

	for (uint8_t i = 0; i < count; i++) {
		crc = crc8_update(crc, data[i]);
	}

	return crc;
}

static inline struct mctp_binding_i2c_smbus_target *
cfg_to_binding(struct i2c_target_config *cfg)
{
	return CONTAINER_OF(cfg, struct mctp_binding_i2c_smbus_target,
			    i2c_target_cfg);
}

static int tgt_write_requested(struct i2c_target_config *config)
{
	struct mctp_binding_i2c_smbus_target *b = cfg_to_binding(config);

	b->rx_state = MCTP_I2C_SMBUS_RX_WAIT_CMD;
	b->rx_cmd = 0U;
	b->rx_count = 0U;
	b->rx_idx = 0U;

	LOG_DBG("write_requested");
	return 0;
}

static int tgt_write_received(struct i2c_target_config *config, uint8_t val)
{
	struct mctp_binding_i2c_smbus_target *b = cfg_to_binding(config);

	switch (b->rx_state) {
	case MCTP_I2C_SMBUS_RX_WAIT_CMD:
		b->rx_cmd = val;
		b->rx_state = (b->rx_cmd == MCTP_SMBUS_CMD_CODE) ?
			      MCTP_I2C_SMBUS_RX_WAIT_COUNT : MCTP_I2C_SMBUS_RX_DROP;
		return 0;

	case MCTP_I2C_SMBUS_RX_WAIT_COUNT:
		b->rx_count = val;

		if ((b->rx_count == 0U) ||
		    (b->rx_count > CONFIG_MCTP_I2C_SMBUS_BLOCK_MAX)) {
			LOG_WRN("invalid SMBus count %u", b->rx_count);
			b->rx_state = MCTP_I2C_SMBUS_RX_DROP;
		} else {
			b->rx_idx = 0U;
			b->rx_state = MCTP_I2C_SMBUS_RX_WAIT_DATA;
		}

		return 0;

	case MCTP_I2C_SMBUS_RX_WAIT_DATA:
		if (b->rx_idx >= b->rx_count) {
			b->rx_state = MCTP_I2C_SMBUS_RX_DROP;
			return 0;
		}

		b->rx_buf[b->rx_idx++] = val;

		if (b->rx_idx == b->rx_count) {
			b->rx_state = MCTP_I2C_SMBUS_RX_WAIT_PEC;
		}

		return 0;

	case MCTP_I2C_SMBUS_RX_WAIT_PEC: {
		const uint8_t expected =
			smbus_pec_calc(b->ep_i2c_addr, b->rx_cmd,
				       b->rx_count, b->rx_buf);

		if (val != expected) {
			LOG_WRN("bad PEC: got 0x%02x expected 0x%02x",
				val, expected);
			b->rx_state = MCTP_I2C_SMBUS_RX_DROP;
			return 0;
		}

		if (b->rx_count <= MCTP_SMBUS_SRC_ADDR_LEN) {
			LOG_WRN("packet too short: count=%u", b->rx_count);
			b->rx_state = MCTP_I2C_SMBUS_RX_WAIT_CMD;
			return 0;
		}

		if ((b->rx_buf[0] & 0x01U) == 0U) {
			LOG_WRN("source address byte has write bit clear: 0x%02x",
				b->rx_buf[0]);
		}

		const uint8_t *mctp_bytes =
			&b->rx_buf[MCTP_SMBUS_SRC_ADDR_LEN];
		const size_t mctp_len =
			(size_t)b->rx_count - MCTP_SMBUS_SRC_ADDR_LEN;

		if (mctp_len < 4U) {
			LOG_WRN("MCTP packet too short: len=%u",
				(unsigned int)mctp_len);
			b->rx_state = MCTP_I2C_SMBUS_RX_WAIT_CMD;
			return 0;
		}

		if ((mctp_bytes[0] & 0x0FU) != 0x01U) {
			LOG_WRN("unexpected MCTP header version byte 0x%02x",
				mctp_bytes[0]);
		}

		struct mctp_pktbuf *pkt =
			mctp_pktbuf_alloc(&b->binding, mctp_len);
		if (!pkt) {
			LOG_WRN("failed to allocate MCTP packet buffer");
			b->rx_state = MCTP_I2C_SMBUS_RX_DROP;
			return 0;
		}

		memcpy(pkt->data + pkt->mctp_hdr_off, mctp_bytes, mctp_len);
		pkt->start = pkt->mctp_hdr_off;
		pkt->end = pkt->start + mctp_len;

		LOG_DBG("received MCTP packet len=%u",
			(unsigned int)mctp_len);
		LOG_HEXDUMP_DBG(mctp_bytes, MIN(mctp_len, 32U), "MCTP RX");

		mctp_bus_rx(&b->binding, pkt);

		b->rx_state = MCTP_I2C_SMBUS_RX_WAIT_CMD;
		return 0;
	}

	case MCTP_I2C_SMBUS_RX_DROP:
	default:
		return 0;
	}
}

static int tgt_stop(struct i2c_target_config *config)
{
	struct mctp_binding_i2c_smbus_target *b = cfg_to_binding(config);

	if (b->rx_state == MCTP_I2C_SMBUS_RX_WAIT_PEC) {
		LOG_WRN("STOP before PEC; dropping packet");
	}

	b->rx_state = MCTP_I2C_SMBUS_RX_WAIT_CMD;
	b->rx_cmd = 0U;
	b->rx_count = 0U;
	b->rx_idx = 0U;

	return 0;
}

const struct i2c_target_callbacks mctp_i2c_smbus_target_callbacks = {
	.write_requested = tgt_write_requested,
	.write_received = tgt_write_received,
	.stop = tgt_stop,
};

static void tx_work_fn(struct k_work *work)
{
	struct mctp_binding_i2c_smbus_target *b =
		CONTAINER_OF(work, struct mctp_binding_i2c_smbus_target,
			     tx_work);
	uint8_t mctp_bytes[CONFIG_MCTP_I2C_SMBUS_BLOCK_MAX];
	uint8_t mctp_len;
	uint8_t out[2 + CONFIG_MCTP_I2C_SMBUS_BLOCK_MAX + 1];
	size_t tx_sz;
	int rc = 0;

	k_sem_take(b->tx_lock, K_FOREVER);

	if (!b->tx_pending) {
		k_sem_give(b->tx_lock);
		LOG_DBG("no pending TX packet");
		return;
	}

	mctp_len = b->tx_len;
	memcpy(mctp_bytes, b->tx_buf, mctp_len);
	b->tx_pending = false;

	k_sem_give(b->tx_lock);

	if ((1U + mctp_len) > CONFIG_MCTP_I2C_SMBUS_BLOCK_MAX) {
		LOG_WRN("TX packet too large: mctp_len=%u",
			(unsigned int)mctp_len);
		return;
	}

	/* SMBus Block Write:
	 * [0] command
	 * [1] count
	 * [2] source slave address byte (R/W bit set)
	 * [3..] MCTP bytes
	 * [last] PEC
	 */
	out[0] = MCTP_SMBUS_CMD_CODE;
	out[1] = (uint8_t)(1U + mctp_len);
	out[2] = (uint8_t)((b->ep_i2c_addr << 1) | 0x01U);

	memcpy(&out[3], mctp_bytes, mctp_len);

	out[3 + mctp_len] =
		smbus_pec_calc(b->remote_i2c_addr, out[0], out[1], &out[2]);

	tx_sz = 2U + (size_t)out[1] + 1U;

	LOG_DBG("sending SMBus block write to 0x%02x, len=%u",
		b->remote_i2c_addr, (unsigned int)tx_sz);
	LOG_HEXDUMP_DBG(out, MIN(tx_sz, 32U), "SMBus TX");

	for (int attempt = 0; attempt < MCTP_I2C_SMBUS_TX_RETRIES; attempt++) {
		rc = i2c_write(b->i2c, out, tx_sz, b->remote_i2c_addr);
		if (rc == 0) {
			break;
		}

		LOG_DBG("TX attempt %d failed rc=%d", attempt + 1, rc);
		k_sleep(K_MSEC(MCTP_I2C_SMBUS_TX_RETRY_BASE_MS + attempt));
	}

	if (rc) {
		LOG_WRN("SMBus TX failed rc=%d", rc);
	}
}

int mctp_i2c_smbus_target_tx(struct mctp_binding *binding,
			     struct mctp_pktbuf *pkt)
{
	struct mctp_binding_i2c_smbus_target *b =
		CONTAINER_OF(binding, struct mctp_binding_i2c_smbus_target,
			     binding);
	const size_t mctp_len = mctp_pktbuf_size(pkt);

	if ((mctp_len == 0U) ||
	    (mctp_len > MCTP_I2C_SMBUS_MAX_MCTP_BYTES)) {
		return -EMSGSIZE;
	}

	k_sem_take(b->tx_lock, K_FOREVER);

	if (b->tx_pending) {
		k_sem_give(b->tx_lock);
		return -EBUSY;
	}

	memcpy(b->tx_buf, pkt->data + pkt->mctp_hdr_off, mctp_len);
	b->tx_len = (uint8_t)mctp_len;
	b->tx_pending = true;

	k_sem_give(b->tx_lock);

	k_work_submit(&b->tx_work);
	return 0;
}

int mctp_i2c_smbus_target_start(struct mctp_binding *binding)
{
	struct mctp_binding_i2c_smbus_target *b =
		CONTAINER_OF(binding, struct mctp_binding_i2c_smbus_target,
			     binding);
	int rc;

	if (!device_is_ready(b->i2c)) {
		LOG_ERR("I2C device is not ready");
		return -ENODEV;
	}

	k_work_init(&b->tx_work, tx_work_fn);

	rc = i2c_target_register(b->i2c, &b->i2c_target_cfg);
	if (rc) {
		LOG_ERR("failed to register I2C target at 0x%02x: %d",
			b->ep_i2c_addr, rc);
		return rc;
	}

	mctp_binding_set_tx_enabled(binding, true);
	return 0;
}
