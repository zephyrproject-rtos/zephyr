/*
 * Copyright (c) 2025, Qingsong Gou <gouqs@hotmail.com>
 * Copyright (c) 2025, Haoran Jiang <halfsweet@halfsweet.cn>
 * Copyright (c) 2025, SiFli Technologies(Nanjing) Co., Ltd
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_i2c

#include "i2c-priv.h"

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control/sf32lb.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/dma/sf32lb.h>

LOG_MODULE_REGISTER(i2c_sf32lb, CONFIG_I2C_LOG_LEVEL);

#include <register.h>

#define I2C_CR    offsetof(I2C_TypeDef, CR)
#define I2C_TCR   offsetof(I2C_TypeDef, TCR)
#define I2C_IER   offsetof(I2C_TypeDef, IER)
#define I2C_SR    offsetof(I2C_TypeDef, SR)
#define I2C_DBR   offsetof(I2C_TypeDef, DBR)
#define I2C_SAR   offsetof(I2C_TypeDef, SAR)
#define I2C_LCR   offsetof(I2C_TypeDef, LCR)
#define I2C_WCR   offsetof(I2C_TypeDef, WCR)
#define I2C_RCCR  offsetof(I2C_TypeDef, RCCR)
#define I2C_BMR   offsetof(I2C_TypeDef, BMR)
#define I2C_DNR   offsetof(I2C_TypeDef, DNR)
#define I2C_RSVD1 offsetof(I2C_TypeDef, RSVD1)
#define I2C_FIFO  offsetof(I2C_TypeDef, FIFO)

#define I2C_MODE_STD    (0x00U)
#define I2C_MODE_FS     (0x01U)
#define I2C_MODE_HS_STD (0x02U)
#define I2C_MODE_HS_FS  (0x03U)

#define SF32LB_I2C_TIMEOUT_MAX_US (30000)

#define SF32LB_I2C_DMA_MAX_LEN (512U)

struct i2c_sf32lb_config {
	uintptr_t base;
	const struct pinctrl_dev_config *pincfg;
	struct sf32lb_clock_dt_spec clock;
	uint32_t bitrate;
	void (*irq_cfg_func)(void);
	bool dma_used;
	struct sf32lb_dma_dt_spec dma_rx;
	struct sf32lb_dma_dt_spec dma_tx;
};

struct i2c_sf32lb_data {
	struct k_mutex lock;
	uint8_t rw_flags;
	struct k_sem i2c_compl;
	struct i2c_msg *current_msg;
	uint8_t *buf_ptr;
	uint32_t remaining;
	bool is_tx;
	int error;
};

static void i2c_sf32lb_tx_helper(const struct device *dev, uint32_t sr)
{
	const struct i2c_sf32lb_config *config = dev->config;
	struct i2c_sf32lb_data *data = dev->data;
	uint32_t tcr;

	if (IS_BIT_SET(sr, I2C_SR_TE_Pos)) {
		sys_set_bit(config->base + I2C_SR, I2C_SR_TE_Pos);
		if (IS_BIT_SET(sr, I2C_SR_NACK_Pos)) {
			data->error = -EIO;
			sys_write32(0, config->base + I2C_IER);
			data->current_msg = NULL;
			k_sem_give(&data->i2c_compl);
			return;
		}

		if (data->remaining > 0) {
			sys_write8(*data->buf_ptr, config->base + I2C_DBR);
			data->buf_ptr++;
			data->remaining--;

			tcr = I2C_TCR_TB;
			if (data->remaining == 0 && i2c_is_stop_op(data->current_msg)) {
				tcr |= I2C_TCR_STOP;
			}
			sys_write32(tcr, config->base + I2C_TCR);
		} else {
			sys_write32(0, config->base + I2C_IER);
			data->current_msg = NULL;
			k_sem_give(&data->i2c_compl);
		}
	}

	if (IS_BIT_SET(sr, I2C_SR_MSD_Pos) && (data->remaining == 0)) {
		sys_set_bit(config->base + I2C_SR, I2C_SR_MSD_Pos);
		sys_write32(0, config->base + I2C_IER);
		data->current_msg = NULL;
		k_sem_give(&data->i2c_compl);
	}
}

static void i2c_sf32lb_rx_helper(const struct device *dev, uint32_t sr)
{
	const struct i2c_sf32lb_config *config = dev->config;
	struct i2c_sf32lb_data *data = dev->data;
	uint32_t tcr;

	if (IS_BIT_SET(sr, I2C_SR_RF_Pos)) {
		sys_set_bit(config->base + I2C_SR, I2C_SR_RF_Pos);

		if (data->remaining > 0) {
			if (IS_BIT_SET(sr, I2C_SR_NACK_Pos)) {
				data->error = -EIO;
				data->current_msg = NULL;
				k_sem_give(&data->i2c_compl);
				return;
			}
			*data->buf_ptr = sys_read8(config->base + I2C_DBR);
			data->buf_ptr++;
			data->remaining--;

			tcr = I2C_TCR_TB;
			if (data->remaining == 0) {
				if (i2c_is_stop_op(data->current_msg)) {
					tcr |= I2C_TCR_STOP;
				}
				tcr |= I2C_TCR_NACK;
			}
			sys_write32(tcr, config->base + I2C_TCR);
		}
	}

	if (IS_BIT_SET(sr, I2C_SR_MSD_Pos) && (data->remaining == 0)) {
		sys_set_bit(config->base + I2C_SR, I2C_SR_MSD_Pos);
		sys_write32(0, config->base + I2C_IER);
		data->current_msg = NULL;
		k_sem_give(&data->i2c_compl);
	}
}

static void i2c_sf32lb_isr(const struct device *dev)
{
	const struct i2c_sf32lb_config *config = dev->config;
	struct i2c_sf32lb_data *data = dev->data;
	uint32_t sr = sys_read32(config->base + I2C_SR);

	if (IS_BIT_SET(sr, I2C_SR_BED_Pos)) {
		sys_set_bit(config->base + I2C_SR, I2C_SR_BED_Pos);
		data->error = -EIO;
		sys_write32(0, config->base + I2C_IER);
		data->current_msg = NULL;
		k_sem_give(&data->i2c_compl);
		return;
	}
	if (config->dma_used) {
		if (IS_BIT_SET(sr, I2C_SR_DMADONE_Pos)) {
			sys_set_bit(config->base + I2C_SR, I2C_SR_DMADONE_Pos);
			sys_clear_bit(config->base + I2C_CR, I2C_CR_DMAEN_Pos);
			k_sem_give(&data->i2c_compl);
		}
	} else {
		if (data->is_tx) {
			i2c_sf32lb_tx_helper(dev, sr);
		} else {
			i2c_sf32lb_rx_helper(dev, sr);
		}
	}
}

static int i2c_sf32lb_send_addr(const struct device *dev, uint16_t addr, struct i2c_msg *msg)
{
	int ret = 0;
	const struct i2c_sf32lb_config *cfg = dev->config;
	uint32_t tcr = 0;

	addr = addr << 1;
	if (i2c_is_read_op(msg)) {
		addr |= 1;
	}

	tcr |= I2C_TCR_START;
	tcr |= I2C_TCR_TB;

	if ((msg->len == 0) && i2c_is_stop_op(msg)) {
		tcr |= I2C_TCR_MA | I2C_TCR_STOP;
	}

	sys_write8(addr, cfg->base + I2C_DBR);
	sys_write32(tcr, cfg->base + I2C_TCR);

	if (!WAIT_FOR(sys_test_bit(cfg->base + I2C_SR, I2C_SR_TE_Pos), SF32LB_I2C_TIMEOUT_MAX_US,
		      NULL)) {
		LOG_ERR("Abort timed out(I2C_SR: 0x%08x)", sys_read32(cfg->base + I2C_SR));
		return -EIO;
	}

	sys_write32(sys_read32(cfg->base + I2C_SR), cfg->base + I2C_SR);

	if (sys_test_bit(cfg->base + I2C_SR, I2C_SR_NACK_Pos)) {
		/* Wait for MSD(Master Stop Detected) to set, it appears slower than NACK */
		WAIT_FOR(sys_test_bit(cfg->base + I2C_SR, I2C_SR_MSD_Pos),
			 SF32LB_I2C_TIMEOUT_MAX_US, NULL);
		ret = -EIO;
	}

	if ((msg->len == 0) && i2c_is_stop_op(msg)) {
		if (!WAIT_FOR(!sys_test_bit(cfg->base + I2C_SR, I2C_SR_UB_Pos),
			      SF32LB_I2C_TIMEOUT_MAX_US, NULL)) {
			LOG_ERR("Stop timed out (I2C_SR:0x%08x)", sys_read32(cfg->base + I2C_SR));
		}
	}

	return ret;
}

static int i2c_sf32lb_dma_tx_config(const struct device *dev, struct i2c_msg *msg)
{
	const struct i2c_sf32lb_config *config = dev->config;
	struct dma_config tx_dma_cfg = {0};
	struct dma_block_config dma_blk = {0};
	int err;

	sf32lb_dma_config_init_dt(&config->dma_tx, &tx_dma_cfg);

	tx_dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	tx_dma_cfg.block_count = 1U;
	tx_dma_cfg.source_data_size = 1U;
	tx_dma_cfg.dest_data_size = 1U;

	tx_dma_cfg.head_block = &dma_blk;
	dma_blk.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	dma_blk.source_address = (uint32_t)msg->buf;
	dma_blk.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	dma_blk.dest_address = (uint32_t)(config->base + I2C_FIFO);
	dma_blk.block_size = msg->len;
	err = sf32lb_dma_config_dt(&config->dma_tx, &tx_dma_cfg);
	if (err < 0) {
		LOG_ERR("Error configuring Tx DMA (%d)", err);
		return err;
	}

	return 0;
}

static int i2c_sf32lb_dma_rx_config(const struct device *dev, struct i2c_msg *msg)
{
	const struct i2c_sf32lb_config *config = dev->config;
	struct dma_config rx_dma_cfg = {0};
	struct dma_block_config dma_blk = {0};
	int err;

	sf32lb_dma_config_init_dt(&config->dma_rx, &rx_dma_cfg);

	rx_dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	rx_dma_cfg.block_count = 1U;
	rx_dma_cfg.source_data_size = 1U;
	rx_dma_cfg.dest_data_size = 1U;

	rx_dma_cfg.head_block = &dma_blk;
	dma_blk.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	dma_blk.source_address = (uint32_t)(config->base + I2C_FIFO);
	dma_blk.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	dma_blk.dest_address = (uint32_t)msg->buf;
	dma_blk.block_size = msg->len;

	err = sf32lb_dma_config_dt(&config->dma_rx, &rx_dma_cfg);
	if (err < 0) {
		LOG_ERR("Error configuring Rx DMA (%d)", err);
		return err;
	}

	return 0;
}

static int i2c_sf32lb_master_send_dma(const struct device *dev, uint16_t addr, struct i2c_msg *msg)
{
	int ret;
	const struct i2c_sf32lb_config *config = dev->config;
	struct i2c_sf32lb_data *data = dev->data;
	bool addr_sent = (data->rw_flags != (msg->flags & I2C_MSG_RW_MASK));
	bool stop_needed = i2c_is_stop_op(msg);

	data->rw_flags = msg->flags & I2C_MSG_RW_MASK;
	data->error = 0;

	if (msg->len > SF32LB_I2C_DMA_MAX_LEN) {
		LOG_ERR("DMA length %d exceeds max %d",
			msg->len, SF32LB_I2C_DMA_MAX_LEN);
		return -ENOTSUP;
	}

	if (addr_sent) {
		ret = i2c_sf32lb_send_addr(dev, addr, msg);
		if (ret < 0) {
			return ret;
		}
	}

	if (msg->len == 0) {
		/* Zero-length message already handled in send_addr */
		return ret;
	}

	if (stop_needed) {
		sys_set_bit(config->base + I2C_CR, I2C_CR_LASTSTOP_Pos);
	}

	sys_set_bit(config->base + I2C_SR, I2C_SR_DMADONE_Pos);
	sys_set_bits(config->base + I2C_IER, I2C_IER_DMADONEIE | I2C_IER_BEDIE);
	sys_set_bits(config->base + I2C_CR, I2C_CR_MSDE);

	ret = i2c_sf32lb_dma_tx_config(dev, msg);
	if (ret < 0) {
		return ret;
	}

	ret = sf32lb_dma_start_dt(&config->dma_tx);
	if (ret < 0) {
		return ret;
	}

	sys_set_bit(config->base + I2C_CR, I2C_CR_DMAEN_Pos);
	sys_write8(msg->len, config->base + I2C_DNR);
	sys_write32(I2C_TCR_TXREQ, config->base + I2C_TCR);

	ret = k_sem_take(&data->i2c_compl, K_MSEC(SF32LB_I2C_TIMEOUT_MAX_US / 1000));
	if (ret < 0) {
		LOG_ERR("master send timeout");
		sf32lb_dma_stop_dt(&config->dma_tx);
		sys_clear_bit(config->base + I2C_CR, I2C_CR_DMAEN_Pos);
		sys_clear_bits(config->base + I2C_IER, I2C_IER_DMADONEIE | I2C_IER_BEDIE);
		sys_clear_bits(config->base + I2C_CR, I2C_CR_LASTSTOP | I2C_CR_MSDE);
		return ret;
	}
	sys_clear_bit(config->base + I2C_CR, I2C_CR_DMAEN_Pos);
	sys_set_bit(config->base + I2C_SR, I2C_SR_DMADONE_Pos);
	sf32lb_dma_stop_dt(&config->dma_tx);

	/* Wait for bus idle if stop was issued */
	if (stop_needed) {
		if (!WAIT_FOR(!sys_test_bit(config->base + I2C_SR, I2C_SR_UB_Pos),
			      SF32LB_I2C_TIMEOUT_MAX_US, NULL)) {
			LOG_ERR("Wait for bus idle timeout");
			return -ETIMEDOUT;
		}
		sys_clear_bits(config->base + I2C_CR, I2C_CR_LASTSTOP | I2C_CR_MSDE);
	}

	if (data->error != 0) {
		ret = data->error;
		data->error = 0;
	}

	return ret;
}

static int i2c_sf32lb_master_recv_dma(const struct device *dev, uint16_t addr, struct i2c_msg *msg)
{
	const struct i2c_sf32lb_config *config = dev->config;
	struct i2c_sf32lb_data *data = dev->data;
	bool addr_sent = (data->rw_flags != (msg->flags & I2C_MSG_RW_MASK));
	bool stop_needed = i2c_is_stop_op(msg);
	int ret;

	data->rw_flags = msg->flags & I2C_MSG_RW_MASK;
	data->error = 0;

	if (msg->len > SF32LB_I2C_DMA_MAX_LEN) {
		return -ENOTSUP;
	}

	if (addr_sent) {
		ret = i2c_sf32lb_send_addr(dev, addr, msg);
		if (ret < 0) {
			return ret;
		}
	}

	if (msg->len == 0) {
		return 0;
	}

	if (stop_needed) {
		sys_set_bits(config->base + I2C_CR, I2C_CR_LASTNACK | I2C_CR_LASTSTOP);
	}

	sys_set_bit(config->base + I2C_SR, I2C_SR_DMADONE_Pos);
	sys_set_bits(config->base + I2C_IER, I2C_IER_DMADONEIE | I2C_IER_BEDIE);
	sys_set_bits(config->base + I2C_CR, I2C_CR_MSDE);

	ret = i2c_sf32lb_dma_rx_config(dev, msg);
	if (ret < 0) {
		return ret;
	}

	ret = sf32lb_dma_start_dt(&config->dma_rx);
	if (ret < 0) {
		return ret;
	}

	sys_set_bit(config->base + I2C_CR, I2C_CR_DMAEN_Pos);

	sys_write8(msg->len, config->base + I2C_DNR);

	sys_write32(I2C_TCR_RXREQ, config->base + I2C_TCR);

	ret = k_sem_take(&data->i2c_compl, K_MSEC(SF32LB_I2C_TIMEOUT_MAX_US / 1000));
	if (ret < 0) {
		LOG_ERR("master recv timeout");
		sys_clear_bit(config->base + I2C_CR, I2C_CR_DMAEN_Pos);
		sys_set_bit(config->base + I2C_SR, I2C_SR_DMADONE_Pos);
		sf32lb_dma_stop_dt(&config->dma_rx);
		sys_write32(0, config->base + I2C_IER);
		return ret;
	}

	sys_clear_bit(config->base + I2C_CR, I2C_CR_DMAEN_Pos);
	sys_set_bit(config->base + I2C_SR, I2C_SR_DMADONE_Pos);
	sf32lb_dma_stop_dt(&config->dma_rx);

	if (stop_needed) {
		if (!WAIT_FOR(!sys_test_bit(config->base + I2C_SR, I2C_SR_UB_Pos),
			      SF32LB_I2C_TIMEOUT_MAX_US, NULL)) {
			LOG_ERR("Stop timed out (I2C_SR:0x%08x)",
				sys_read32(config->base + I2C_SR));
		}
		sys_clear_bits(config->base + I2C_CR, I2C_CR_LASTNACK | I2C_CR_LASTSTOP);
	}

	if (data->error != 0) {
		ret = data->error;
		data->error = 0;
	}

	return ret;
}

static int i2c_sf32lb_master_send(const struct device *dev, uint16_t addr, struct i2c_msg *msg)
{
	int ret = 0;
	const struct i2c_sf32lb_config *cfg = dev->config;
	struct i2c_sf32lb_data *data = dev->data;
	uint32_t tcr = I2C_TCR_TB;
	bool stop_needed = i2c_is_stop_op(msg);
	bool addr_sent = (data->rw_flags != (msg->flags & I2C_MSG_RW_MASK));

	data->rw_flags = msg->flags & I2C_MSG_RW_MASK;

	if (addr_sent) {
		ret = i2c_sf32lb_send_addr(dev, addr, msg);
		if (ret < 0) {
			return ret;
		}
	}

	if (msg->len == 0) {
		/* Zero-length message already handled in send_addr */
		return ret;
	}

	data->current_msg = msg;
	data->buf_ptr = msg->buf;
	data->remaining = msg->len;
	data->is_tx = true;
	data->error = 0;

	sys_set_bit(cfg->base + I2C_SR, I2C_SR_TE_Pos);

	sys_write8(*data->buf_ptr, cfg->base + I2C_DBR);
	data->buf_ptr++;
	data->remaining--;

	if (data->remaining == 0 && stop_needed) {
		tcr |= I2C_TCR_STOP;
	}
	sys_write32(tcr, cfg->base + I2C_TCR);

	sys_set_bit(cfg->base + I2C_IER, I2C_IER_TEIE_Pos);
	sys_set_bit(cfg->base + I2C_IER, I2C_IER_MSDIE_Pos);
	sys_set_bit(cfg->base + I2C_IER, I2C_IER_BEDIE_Pos);

	if (k_sem_take(&data->i2c_compl, K_MSEC(SF32LB_I2C_TIMEOUT_MAX_US / 1000)) != 0) {
		LOG_ERR("master sent timeout");
		sys_write32(0, cfg->base + I2C_IER);
		data->current_msg = NULL;
		return -ETIMEDOUT;
	}

	sys_write32(0, cfg->base + I2C_IER);

	if (data->error != 0) {
		ret = data->error;
	}

	return ret;
}

static int i2c_sf32lb_master_recv(const struct device *dev, uint16_t addr, struct i2c_msg *msg)
{
	int ret;
	const struct i2c_sf32lb_config *cfg = dev->config;
	struct i2c_sf32lb_data *data = dev->data;
	uint32_t tcr = I2C_TCR_TB;
	bool stop_needed = i2c_is_stop_op(msg);

	data->rw_flags = msg->flags & I2C_MSG_RW_MASK;

	ret = i2c_sf32lb_send_addr(dev, addr, msg);
	if (ret < 0) {
		return ret;
	}

	if (msg->len == 0) {
		return ret;
	}

	data->current_msg = msg;
	data->buf_ptr = msg->buf;
	data->remaining = msg->len;
	data->is_tx = false;
	data->error = 0;

	sys_set_bit(cfg->base + I2C_SR, I2C_SR_RF_Pos);

	if (data->remaining == 1) {
		if (stop_needed) {
			tcr |= I2C_TCR_STOP;
		}
		tcr |= I2C_TCR_NACK;
	}
	sys_write32(tcr, cfg->base + I2C_TCR);

	sys_set_bit(cfg->base + I2C_CR, I2C_CR_MSDE_Pos);
	sys_set_bits(cfg->base + I2C_IER, I2C_IER_RFIE | I2C_IER_MSDIE | I2C_IER_BEDIE);

	if (k_sem_take(&data->i2c_compl, K_MSEC(SF32LB_I2C_TIMEOUT_MAX_US / 1000)) != 0) {
		LOG_ERR("master recv timeout");
		sys_write32(0, cfg->base + I2C_IER);
		data->current_msg = NULL;
		return -ETIMEDOUT;
	}

	sys_write32(0, cfg->base + I2C_IER);

	if (data->error != 0) {
		ret = data->error;
	}

	return ret;
}

static int i2c_sf32lb_configure(const struct device *dev, uint32_t dev_config)
{
	const struct i2c_sf32lb_config *cfg = dev->config;
	struct i2c_sf32lb_data *data = dev->data;
	uint32_t cr;
	uint32_t sar;

	if (!(I2C_MODE_CONTROLLER & dev_config)) {
		return -ENOTSUP;
	}

	cr = sys_read32(cfg->base + I2C_CR);
	cr &= ~I2C_CR_MODE_Msk;

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		cr |= I2C_MODE_STD;
		break;

	case I2C_SPEED_FAST:
		cr |= I2C_MODE_FS;
		break;

	case I2C_SPEED_FAST_PLUS:
		cr |= I2C_MODE_HS_STD;
		break;

	case I2C_SPEED_HIGH:
		cr |= I2C_MODE_HS_FS;
		break;

	default:
		LOG_ERR("Unsupported I2C speed requested:%d", I2C_SPEED_GET(dev_config));
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	sys_write32(cr, cfg->base + I2C_CR);
	sar = sys_read32(cfg->base + I2C_SAR);
	sar &= ~I2C_SAR_ADDR_Msk;
	/* Avoid sharing the same address with targets, 0x7c is a reserved address */
	sar |= FIELD_PREP(I2C_SAR_ADDR_Msk, 0x7CU);
	sys_write32(sar, cfg->base + I2C_SAR);
	k_mutex_unlock(&data->lock);

	return 0;
}

static int i2c_sf32lb_get_config(const struct device *dev, uint32_t *dev_config)
{
	const struct i2c_sf32lb_config *cfg = dev->config;
	uint32_t cr = sys_read32(cfg->base + I2C_CR);

	*dev_config = I2C_MODE_CONTROLLER;

	switch (FIELD_GET(I2C_CR_MODE_Msk, cr)) {
	case I2C_MODE_STD:
		*dev_config |= I2C_SPEED_SET(I2C_SPEED_STANDARD);
		break;
	case I2C_MODE_FS:
		*dev_config |= I2C_SPEED_SET(I2C_SPEED_FAST);
		break;
	case I2C_MODE_HS_STD:
		*dev_config |= I2C_SPEED_SET(I2C_SPEED_FAST_PLUS);
		break;
	case I2C_MODE_HS_FS:
		*dev_config |= I2C_SPEED_SET(I2C_SPEED_HIGH);
		break;
	default:
		return -ERANGE;
	}

	return 0;
}

static int i2c_sf32lb_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			       uint16_t addr)
{
	const struct i2c_sf32lb_config *cfg = dev->config;
	struct i2c_sf32lb_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	sys_set_bits(cfg->base + I2C_CR, I2C_CR_IUE | I2C_CR_MSDE);

	if (sys_test_bit(cfg->base + I2C_SR, I2C_SR_UB_Pos)) {
		k_mutex_unlock(&data->lock);
		LOG_ERR("Bus busy");
		return -EBUSY;
	};

	for (uint8_t i = 0U; i < num_msgs; i++) {
		if (I2C_MSG_ADDR_10_BITS & msgs->flags) {
			ret = -ENOTSUP;
			break;
		}

		if (msgs[i].flags & I2C_MSG_READ) {
			if (cfg->dma_used) {
				ret = i2c_sf32lb_master_recv_dma(dev, addr, &msgs[i]);
			} else {
				ret = i2c_sf32lb_master_recv(dev, addr, &msgs[i]);
			}
			if (ret < 0) {
				break;
			}
		} else {
			if (cfg->dma_used) {
				ret = i2c_sf32lb_master_send_dma(dev, addr, &msgs[i]);
			} else {
				ret = i2c_sf32lb_master_send(dev, addr, &msgs[i]);
			}
			if (ret < 0) {
				break;
			}
		}
	}

	sys_clear_bit(cfg->base + I2C_CR, I2C_CR_IUE_Pos);

	data->rw_flags = I2C_MSG_READ;

	k_mutex_unlock(&data->lock);

	return 0;
}

static int i2c_sf32lb_recover_bus(const struct device *dev)
{
	const struct i2c_sf32lb_config *config = dev->config;

	sys_set_bit(config->base + I2C_CR, I2C_CR_RSTREQ_Pos);
	while (sys_test_bit(config->base + I2C_CR, I2C_CR_RSTREQ_Pos)) {
	}

	return 0;
}

static DEVICE_API(i2c, i2c_sf32lb_driver_api) = {
	.configure = i2c_sf32lb_configure,
	.get_config = i2c_sf32lb_get_config,
	.transfer = i2c_sf32lb_transfer,
	.recover_bus = i2c_sf32lb_recover_bus,
};

static int i2c_sf32lb_init(const struct device *dev)
{
	const struct i2c_sf32lb_config *config = dev->config;
	struct i2c_sf32lb_data *data = dev->data;
	int ret;

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	if (!sf32lb_clock_is_ready_dt(&config->clock)) {
		return -ENODEV;
	}

	if (config->dma_used) {
		if (!sf32lb_dma_is_ready_dt(&config->dma_tx)) {
			LOG_ERR("Tx DMA channel not ready");
			return -ENODEV;
		}

		if (!sf32lb_dma_is_ready_dt(&config->dma_rx)) {
			LOG_ERR("Rx DMA channel not ready");
			return -ENODEV;
		}

		sys_set_bit(config->base + I2C_IER, I2C_IER_DMADONEIE_Pos);
	}
	ret = sf32lb_clock_control_on_dt(&config->clock);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_sf32lb_configure(dev, I2C_MODE_CONTROLLER | i2c_map_dt_bitrate(config->bitrate));
	if (ret < 0) {
		return ret;
	}

	sys_set_bits(config->base + I2C_CR, I2C_CR_IUE | I2C_CR_SCLE);

	data->rw_flags = I2C_MSG_READ;

	config->irq_cfg_func();

	return ret;
}

#define I2C_SF32LB_DEFINE(n)                                                                       \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static void i2c_sf32lb_irq_config_func_##n(void)                                           \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), i2c_sf32lb_isr,             \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	};                                                                                         \
	static struct i2c_sf32lb_data i2c_sf32lb_data_##n = {                                      \
		.lock = Z_MUTEX_INITIALIZER(i2c_sf32lb_data_##n.lock),                             \
		.i2c_compl = Z_SEM_INITIALIZER(i2c_sf32lb_data_##n.i2c_compl, 0, 1),               \
	};                                                                                         \
	static const struct i2c_sf32lb_config i2c_sf32lb_config_##n = {                            \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
		.clock = SF32LB_CLOCK_DT_INST_SPEC_GET(n),                                         \
		.bitrate = DT_INST_PROP_OR(n, clock_frequency, 100000),                            \
		.irq_cfg_func = i2c_sf32lb_irq_config_func_##n,                                    \
		.dma_used = DT_INST_NODE_HAS_PROP(n, dmas),                                        \
		.dma_tx = SF32LB_DMA_DT_INST_SPEC_GET_BY_NAME_OR(n, tx, {}),                       \
		.dma_rx = SF32LB_DMA_DT_INST_SPEC_GET_BY_NAME_OR(n, rx, {}),                       \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, i2c_sf32lb_init, NULL, &i2c_sf32lb_data_##n,                      \
			      &i2c_sf32lb_config_##n, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,       \
			      &i2c_sf32lb_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_SF32LB_DEFINE)
