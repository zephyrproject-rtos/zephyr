/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "zephyr/pmci/mctp/mctp_i2c_gpio_common.h"
#include <zephyr/sys/__assert.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/pmci/mctp/mctp_i2c_gpio_target.h>
#include <crc-16-ccitt.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mctp_i2c_gpio_target, CONFIG_MCTP_LOG_LEVEL);

int mctp_i2c_gpio_target_write_requested(struct i2c_target_config *config)
{

	struct mctp_binding_i2c_gpio_target *b =
		CONTAINER_OF(config, struct mctp_binding_i2c_gpio_target, i2c_target_cfg);

	if (b->rxtx || b->reg_addr == MCTP_I2C_GPIO_INVALID_ADDR) {
		/* Reset our state */
		b->reg_addr = MCTP_I2C_GPIO_INVALID_ADDR;
		b->rxtx = false;
		b->rx_idx = 0;
	}

	return 0;
}

int mctp_i2c_gpio_target_write_received(struct i2c_target_config *config, uint8_t val)
{
	struct mctp_binding_i2c_gpio_target *b =
		CONTAINER_OF(config, struct mctp_binding_i2c_gpio_target, i2c_target_cfg);
	int ret = 0;

	switch (b->reg_addr) {
	case MCTP_I2C_GPIO_INVALID_ADDR:
		b->rxtx = false;
		b->reg_addr = val;
		break;
	case MCTP_I2C_GPIO_RX_MSG_LEN_ADDR:
		b->rxtx = true;
		b->rx_pkt = mctp_pktbuf_alloc(&b->binding, (size_t)val);
		break;
	case MCTP_I2C_GPIO_RX_MSG_ADDR:
		b->rxtx = true;
		b->rx_pkt->data[b->rx_idx] = val;
		b->rx_idx += 1;

		/* buffer full */
		if (b->rx_idx >= b->rx_pkt->size) {
			ret = -ENOMEM;
		}
		break;
	default:
		LOG_ERR("Write when reg_addr is %d", b->reg_addr);
		ret = -EIO;
		break;
	}

	return ret;
}

int mctp_i2c_gpio_target_read_requested(struct i2c_target_config *config, uint8_t *val)
{
	struct mctp_binding_i2c_gpio_target *b =
		CONTAINER_OF(config, struct mctp_binding_i2c_gpio_target, i2c_target_cfg);
	uint8_t pkt_len;
	int ret = 0;

	switch (b->reg_addr) {
	case MCTP_I2C_GPIO_TX_MSG_LEN_ADDR:
		b->rxtx = true;
		if (b->tx_pkt == NULL) {
			LOG_WRN("empty packet?");
			pkt_len = 0;
		} else {
			pkt_len = (uint8_t)(b->tx_pkt->end - b->tx_pkt->start);
		}
		*val = pkt_len;
		break;
	case MCTP_I2C_GPIO_TX_MSG_ADDR:
		b->rxtx = true;
		*val = b->tx_pkt->data[b->tx_pkt->start];
		b->tx_idx = b->tx_pkt->start;
		break;
	default:
		LOG_WRN("invalid rre reg %d", b->reg_addr);
		ret = -EINVAL;
	}

	return ret;
}

int mctp_i2c_gpio_target_read_processed(struct i2c_target_config *config, uint8_t *val)
{
	struct mctp_binding_i2c_gpio_target *b =
		CONTAINER_OF(config, struct mctp_binding_i2c_gpio_target, i2c_target_cfg);

	b->tx_idx += 1;

	if (b->reg_addr != MCTP_I2C_GPIO_TX_MSG_ADDR) {
		goto out;
	}

	if (b->tx_idx > b->tx_pkt->end) {
		LOG_WRN("rrp past end reg %d", b->reg_addr);
		return -EIO;
	}

	*val = b->tx_pkt->data[b->tx_idx];

out:
	return 0;
}

int mctp_i2c_gpio_target_stop(struct i2c_target_config *config)
{
	struct mctp_binding_i2c_gpio_target *b =
		CONTAINER_OF(config, struct mctp_binding_i2c_gpio_target, i2c_target_cfg);
	uint8_t pkt_len;

	if (b->rxtx) {
		switch (b->reg_addr) {
		case MCTP_I2C_GPIO_TX_MSG_ADDR:
			pkt_len = (b->tx_pkt->end - b->tx_pkt->start);
			if (b->tx_idx < pkt_len - 1) {
				LOG_WRN("Only %d of %d bytes of the transmit packet were read",
					b->tx_idx, pkt_len);
			}
			b->tx_pkt = NULL;
			k_sem_give(b->tx_complete);
			break;

		case MCTP_I2C_GPIO_RX_MSG_ADDR:
			LOG_DBG("stop rx msg, give pkt");
			/* Give message to mctp to process */
			mctp_bus_rx(&b->binding, b->rx_pkt);
			b->rx_pkt = NULL;
			break;
		case MCTP_I2C_GPIO_RX_MSG_LEN_ADDR:
		case MCTP_I2C_GPIO_TX_MSG_LEN_ADDR:
			break;
		default:
			LOG_WRN("unexpected stop for reg %d", b->reg_addr);
			break;
		}
	}

	return 0;
}

const struct i2c_target_callbacks mctp_i2c_gpio_target_callbacks = {
	.write_requested = mctp_i2c_gpio_target_write_requested,
	.read_requested = mctp_i2c_gpio_target_read_requested,
	.write_received = mctp_i2c_gpio_target_write_received,
	.read_processed = mctp_i2c_gpio_target_read_processed,
	.stop = mctp_i2c_gpio_target_stop,
};

/*
 * libmctp wants us to return once the packet is sent not before
 * so the entire process of flagging the tx with gpio, waiting on the read,
 * needs to complete before we can move on.
 *
 * this is called for each packet in the packet queue libmctp provides
 */
int mctp_i2c_gpio_target_tx(struct mctp_binding *binding, struct mctp_pktbuf *pkt)
{
	struct mctp_binding_i2c_gpio_target *b =
		CONTAINER_OF(binding, struct mctp_binding_i2c_gpio_target, binding);
	int rc;

	k_sem_take(b->tx_lock, K_FOREVER);

	b->tx_pkt = pkt;

	rc = gpio_pin_set_dt(&b->endpoint_gpio, 1);
	if (rc != 0) {
		LOG_ERR("failed to set gpio pin");
		b->tx_pkt = NULL;
		goto out;
	}

	k_sem_take(b->tx_complete, K_FOREVER);

	rc = gpio_pin_set_dt(&b->endpoint_gpio, 0);
	if (rc != 0) {
		LOG_ERR("failed to clear gpio pin");
	}

out:
	k_sem_give(b->tx_lock);
	return rc;
}

int mctp_i2c_gpio_target_start(struct mctp_binding *binding)
{
	struct mctp_binding_i2c_gpio_target *b =
		CONTAINER_OF(binding, struct mctp_binding_i2c_gpio_target, binding);
	int rc;

	/* Register i2c target */
	rc = i2c_target_register(b->i2c, &b->i2c_target_cfg);
	if (rc != 0) {
		LOG_ERR("failed to register i2c target");
		goto out;
	}

	/* Configure pin to use as data ready signaling */
	rc = gpio_pin_configure_dt(&b->endpoint_gpio, GPIO_OUTPUT_INACTIVE);
	if (rc != 0) {
		LOG_ERR("failed to configure gpio");
		goto out;
	}

	mctp_binding_set_tx_enabled(binding, true);

out:
	return 0;
}
