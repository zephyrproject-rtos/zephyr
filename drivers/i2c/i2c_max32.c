/*
 * Copyright (c) 2024 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_max32_i2c

#include <errno.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/adi_max32_clock_control.h>
#include <zephyr/irq.h>

#if defined(CONFIG_I2C_MAX32_DMA)
#include <zephyr/drivers/dma.h>
#endif /* CONFIG_I2C_MAX32_DMA */

#include <wrap_max32_i2c.h>

#define ADI_MAX32_I2C_INT_FL0_MASK 0x00FFFFFF
#define ADI_MAX32_I2C_INT_FL1_MASK 0x7

#define ADI_MAX32_I2C_STATUS_MASTER_BUSY BIT(5)

#define I2C_RECOVER_MAX_RETRIES 3

#ifdef CONFIG_I2C_MAX32_DMA
struct max32_i2c_dma_config {
	const struct device *dev;
	const uint32_t channel;
	const uint32_t slot;
};
#endif /* CONFIG_I2C_MAX32_DMA */

/* Driver config */
struct max32_i2c_config {
	mxc_i2c_regs_t *regs;
	const struct pinctrl_dev_config *pctrl;
	const struct device *clock;
	struct max32_perclk perclk;
	uint32_t bitrate;
#if defined(CONFIG_I2C_TARGET) || defined(CONFIG_I2C_MAX32_INTERRUPT)
	uint8_t irqn;
	void (*irq_config_func)(const struct device *dev);
#endif
#ifdef CONFIG_I2C_MAX32_DMA
	struct max32_i2c_dma_config tx_dma;
	struct max32_i2c_dma_config rx_dma;
#endif /* CONFIG_I2C_MAX32_DMA */
};

struct max32_i2c_data {
	mxc_i2c_req_t req;
	const struct device *dev;
	struct k_sem lock;
	uint8_t target_mode;
	uint8_t flags;
#ifdef CONFIG_I2C_TARGET
	struct i2c_target_config *target_cfg;
	bool first_write;
#endif /* CONFIG_I2C_TARGET */
	uint32_t readb;
	uint32_t written;
#if defined(CONFIG_I2C_MAX32_INTERRUPT) || defined(CONFIG_I2C_MAX32_DMA)
	struct k_sem xfer;
	int err;
#endif
};

static int api_configure(const struct device *dev, uint32_t dev_cfg)
{
	int ret = 0;
	const struct max32_i2c_config *const cfg = dev->config;
	mxc_i2c_regs_t *i2c = cfg->regs;

	switch (I2C_SPEED_GET(dev_cfg)) {
	case I2C_SPEED_STANDARD: /** I2C Standard Speed: 100 kHz */
		ret = MXC_I2C_SetFrequency(i2c, MXC_I2C_STD_MODE);
		break;

	case I2C_SPEED_FAST: /** I2C Fast Speed: 400 kHz */
		ret = MXC_I2C_SetFrequency(i2c, MXC_I2C_FAST_SPEED);
		break;

#if defined(MXC_I2C_FASTPLUS_SPEED)
	case I2C_SPEED_FAST_PLUS: /** I2C Fast Plus Speed: 1 MHz */
		ret = MXC_I2C_SetFrequency(i2c, MXC_I2C_FASTPLUS_SPEED);
		break;
#endif

#if defined(MXC_I2C_HIGH_SPEED)
	case I2C_SPEED_HIGH: /** I2C High Speed: 3.4 MHz */
		ret = MXC_I2C_SetFrequency(i2c, MXC_I2C_HIGH_SPEED);
		break;
#endif

	default:
		/* Speed not supported */
		return -ENOTSUP;
	}

	return ((ret > 0) ? 0 : -EIO);
}

#ifdef CONFIG_I2C_TARGET
static int api_target_register(const struct device *dev, struct i2c_target_config *cfg)
{
	const struct max32_i2c_config *config = dev->config;
	struct max32_i2c_data *data = dev->data;
	mxc_i2c_regs_t *i2c = config->regs;
	int ret;

	data->target_cfg = cfg;

	ret = MXC_I2C_Init(i2c, 0, cfg->address);
	if (ret == E_NO_ERROR) {
		data->target_mode = 1;
		irq_enable(config->irqn);
		MXC_I2C_SlaveTransactionAsync(i2c, NULL);
	}

	return ret == E_NO_ERROR ? 0 : E_FAIL;
}

static int api_target_unregister(const struct device *dev, struct i2c_target_config *cfg)
{
	const struct max32_i2c_config *config = dev->config;
	struct max32_i2c_data *data = dev->data;
	mxc_i2c_regs_t *i2c = config->regs;

	data->target_cfg = NULL;
	data->target_mode = 0;

#ifndef CONFIG_I2C_MAX32_INTERRUPT
	irq_disable(config->irqn);
#endif

	return MXC_I2C_Init(i2c, 1, 0);
}

static int i2c_max32_target_callback(const struct device *dev, mxc_i2c_regs_t *i2c,
				     mxc_i2c_slave_event_t event)
{
	struct max32_i2c_data *data = dev->data;
	const struct i2c_target_callbacks *target_cb = data->target_cfg->callbacks;
	static uint8_t rxval, txval, rxcnt;

	switch (event) {
	case MXC_I2C_EVT_MASTER_WR:
		if (data->first_write && target_cb->write_requested) {
			target_cb->write_requested(data->target_cfg);
			data->first_write = false;
		}
		break;
	case MXC_I2C_EVT_MASTER_RD:
		break;
	case MXC_I2C_EVT_RX_THRESH:
	case MXC_I2C_EVT_OVERFLOW:
		rxcnt = MXC_I2C_GetRXFIFOAvailable(i2c);
		if (target_cb->write_received) {
			while (rxcnt--) {
				MXC_I2C_ReadRXFIFO(i2c, &rxval, 1);
				target_cb->write_received(data->target_cfg, rxval);
			}
		} else {
			MXC_I2C_ClearRXFIFO(i2c);
		}
		break;
	case MXC_I2C_EVT_TX_THRESH:
	case MXC_I2C_EVT_UNDERFLOW:
		if (target_cb->read_requested) {
			target_cb->read_requested(data->target_cfg, &txval);
			MXC_I2C_WriteTXFIFO(i2c, &txval, 1);
		}
		if (target_cb->read_processed) {
			target_cb->read_processed(data->target_cfg, &txval);
		}
		break;
	case MXC_I2C_EVT_TRANS_COMP:
		if (target_cb->stop) {
			target_cb->stop(data->target_cfg);
		}
		data->first_write = true;
		break;
	}

	return 0;
}
#endif /* CONFIG_I2C_TARGET */

static int api_recover_bus(const struct device *dev)
{
	int ret;
	const struct max32_i2c_config *const cfg = dev->config;
	mxc_i2c_regs_t *i2c = cfg->regs;

	ret = MXC_I2C_Recover(i2c, I2C_RECOVER_MAX_RETRIES);

	return ret;
}

#ifndef CONFIG_I2C_MAX32_INTERRUPT
static int i2c_max32_transfer_sync(mxc_i2c_regs_t *i2c, struct max32_i2c_data *data)
{
	uint32_t int_fl0, int_fl1;
	uint32_t readb = 0;
	mxc_i2c_req_t *req = &data->req;

	/* Wait for acknowledge */
	if (data->flags & (I2C_MSG_RESTART | I2C_MSG_READ)) {
		do {
			MXC_I2C_GetFlags(i2c, &int_fl0, &int_fl1);
		} while (!(int_fl0 & ADI_MAX32_I2C_INT_FL0_ADDR_ACK) &&
			 !(int_fl0 & ADI_MAX32_I2C_INT_FL0_ERR));
	}

	if (int_fl0 & ADI_MAX32_I2C_INT_FL0_ERR) {
		return -EIO;
	}

	while (req->tx_len > data->written) {
		MXC_I2C_GetFlags(i2c, &int_fl0, &int_fl1);
		if (int_fl0 & ADI_MAX32_I2C_INT_FL0_TX_THD) {
			data->written += MXC_I2C_WriteTXFIFO(i2c, &req->tx_buf[data->written],
							     req->tx_len - data->written);
			MXC_I2C_ClearFlags(i2c, ADI_MAX32_I2C_INT_FL0_TX_THD, 0);
		}

		if (int_fl0 & ADI_MAX32_I2C_INT_FL0_ERR) {
			return -EIO;
		}
	}

	MXC_I2C_ClearFlags(i2c, ADI_MAX32_I2C_INT_FL0_DONE, 0);
	Wrap_MXC_I2C_SetRxCount(i2c, req->rx_len);
	while (req->rx_len > readb) {
		MXC_I2C_GetFlags(i2c, &int_fl0, &int_fl1);
		if (int_fl0 & (ADI_MAX32_I2C_INT_FL0_RX_THD | ADI_MAX32_I2C_INT_FL0_DONE)) {
			readb += MXC_I2C_ReadRXFIFO(i2c, &req->rx_buf[readb], req->rx_len - readb);
			MXC_I2C_ClearFlags(i2c, ADI_MAX32_I2C_INT_FL0_RX_THD, 0);
		}

		if (int_fl0 & ADI_MAX32_I2C_INT_FL0_ERR) {
			return -EIO;
		}

		MXC_I2C_GetFlags(i2c, &int_fl0, &int_fl1);
		if ((int_fl0 & ADI_MAX32_I2C_INT_FL0_DONE) && (req->rx_len > readb) &&
		    MXC_I2C_GetRXFIFOAvailable(i2c) == 0) {
			Wrap_MXC_I2C_SetRxCount(i2c, req->rx_len - readb);
			Wrap_MXC_I2C_Restart(i2c);
			MXC_I2C_ClearFlags(i2c, ADI_MAX32_I2C_INT_FL0_DONE, 0);
			i2c->fifo = (req->addr << 1) | 0x1;
		}
	}

	MXC_I2C_GetFlags(i2c, &int_fl0, &int_fl1);
	if (int_fl0 & ADI_MAX32_I2C_INT_FL0_ERR) {
		return -EIO;
	}

	if (data->flags & I2C_MSG_STOP) {
		MXC_I2C_Stop(i2c);
		do {
			MXC_I2C_GetFlags(i2c, &int_fl0, &int_fl1);
		} while (!(int_fl0 & ADI_MAX32_I2C_INT_FL0_STOP));
	}

	if (req->rx_len) {
		do {
			MXC_I2C_GetFlags(i2c, &int_fl0, &int_fl1);
		} while (!(int_fl0 & ADI_MAX32_I2C_INT_FL0_DONE));
	} else {
		while (Wrap_MXC_I2C_GetTxFIFOLevel(i2c) > 0) {
			MXC_I2C_GetFlags(i2c, &int_fl0, &int_fl1);
		}
	}
	MXC_I2C_ClearFlags(i2c, ADI_MAX32_I2C_INT_FL0_MASK, ADI_MAX32_I2C_INT_FL1_MASK);

	return 0;
}
#endif /* CONFIG_I2C_MAX32_INTERRUPT */

#if defined(CONFIG_I2C_MAX32_DMA)
static void i2c_max32_dma_callback(const struct device *dev, void *arg, uint32_t channel,
				   int status)
{
	struct max32_i2c_data *data = arg;
	const struct device *i2c_dev = data->dev;
	const struct max32_i2c_config *const cfg = i2c_dev->config;

	if (status < 0) {
		data->err = -EIO;
		Wrap_MXC_I2C_SetIntEn(cfg->regs, 0, 0);
		k_sem_give(&data->xfer);
	} else {
		if (data->flags & I2C_MSG_STOP) {
			Wrap_MXC_I2C_Stop(cfg->regs);
		} else if (!(data->flags & I2C_MSG_READ)) {
			MXC_I2C_EnableInt(cfg->regs, ADI_MAX32_I2C_INT_EN0_TX_THD, 0);
		}
	}
}

static int i2c_max32_tx_dma_load(const struct device *dev, struct i2c_msg *msg)
{
	int ret;
	const struct max32_i2c_config *config = dev->config;
	struct max32_i2c_data *data = dev->data;
	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_blk = {0};

	dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	dma_cfg.dma_callback = i2c_max32_dma_callback;
	dma_cfg.user_data = (void *)data;
	dma_cfg.dma_slot = config->tx_dma.slot;
	dma_cfg.block_count = 1;
	dma_cfg.source_data_size = 1U;
	dma_cfg.source_burst_length = 1U;
	dma_cfg.dest_data_size = 1U;
	dma_cfg.head_block = &dma_blk;
	dma_blk.block_size = msg->len;
	dma_blk.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	dma_blk.source_address = (uint32_t)msg->buf;

	ret = dma_config(config->tx_dma.dev, config->tx_dma.channel, &dma_cfg);
	if (ret < 0) {
		return ret;
	}

	return dma_start(config->tx_dma.dev, config->tx_dma.channel);
}

static int i2c_max32_rx_dma_load(const struct device *dev, struct i2c_msg *msg)
{
	int ret;
	const struct max32_i2c_config *config = dev->config;
	struct max32_i2c_data *data = dev->data;
	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_blk = {0};

	dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	dma_cfg.dma_callback = i2c_max32_dma_callback;
	dma_cfg.user_data = (void *)data;
	dma_cfg.dma_slot = config->rx_dma.slot;
	dma_cfg.block_count = 1;
	dma_cfg.source_data_size = 1U;
	dma_cfg.source_burst_length = 1U;
	dma_cfg.dest_data_size = 1U;
	dma_cfg.head_block = &dma_blk;
	dma_blk.block_size = msg->len;
	dma_blk.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	dma_blk.dest_address = (uint32_t)msg->buf;

	ret = dma_config(config->rx_dma.dev, config->rx_dma.channel, &dma_cfg);
	if (ret < 0) {
		return ret;
	}

	return dma_start(config->rx_dma.dev, config->rx_dma.channel);
}

static int i2c_max32_transfer_dma(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
				  uint16_t target_address)
{
	int ret = 0;
	const struct max32_i2c_config *const cfg = dev->config;
	struct max32_i2c_data *data = dev->data;
	mxc_i2c_regs_t *i2c = cfg->regs;
	mxc_i2c_req_t *req = &data->req;
	uint8_t target_rw;
	unsigned int i = 0;

	k_sem_take(&data->lock, K_FOREVER);

	req->addr = target_address;
	req->i2c = i2c;

	MXC_I2C_SetRXThreshold(i2c, 1);
	MXC_I2C_SetTXThreshold(i2c, 2);
	MXC_I2C_ClearTXFIFO(i2c);
	MXC_I2C_ClearRXFIFO(i2c);

	/* First message should always begin with a START condition */
	msgs[0].flags |= I2C_MSG_RESTART;

	for (i = 0; i < num_msgs; i++) {
		if (msgs[i].flags & I2C_MSG_READ) {
			req->rx_len = msgs[i].len;
			req->tx_len = 0;
			target_rw = (target_address << 1) | 0x1;
			ret = i2c_max32_rx_dma_load(dev, &msgs[i]);
			if (ret < 0) {
				break;
			}
		} else {
			req->tx_len = msgs[i].len;
			req->rx_len = 0;
			target_rw = (target_address << 1) & ~0x1;
			ret = i2c_max32_tx_dma_load(dev, &msgs[i]);
			if (ret < 0) {
				break;
			}
		}

		/*
		 *  If previous message ends with a STOP condition, this message
		 *  should begin with a START
		 */
		if (i > 0) {
			if ((msgs[i - 1].flags & (I2C_MSG_STOP | I2C_MSG_READ))) {
				msgs[i].flags |= I2C_MSG_RESTART;
			}
		}

		data->flags = msgs[i].flags;
		data->readb = 0;
		data->written = 0;
		data->err = 0;

		MXC_I2C_ClearFlags(i2c, ADI_MAX32_I2C_INT_FL0_MASK, ADI_MAX32_I2C_INT_FL1_MASK);
		MXC_I2C_EnableInt(i2c, ADI_MAX32_I2C_INT_EN0_ERR, 0);
		Wrap_MXC_I2C_SetRxCount(i2c, req->rx_len);

		if ((data->flags & I2C_MSG_RESTART)) {
			MXC_I2C_EnableInt(i2c, ADI_MAX32_I2C_INT_EN0_ADDR_ACK, 0);
			MXC_I2C_Start(i2c);
			Wrap_MXC_I2C_WaitForRestart(i2c);
			MXC_I2C_WriteTXFIFO(i2c, &target_rw, 1);
		} else if (req->tx_len) {
			MXC_I2C_EnableInt(i2c, ADI_MAX32_I2C_INT_EN0_DONE, 0);
			i2c->dma |= ADI_MAX32_I2C_DMA_TX_EN;
		}

		ret = k_sem_take(&data->xfer, K_FOREVER);

		i2c->dma &= ~(ADI_MAX32_I2C_DMA_TX_EN);
		i2c->dma &= ~(ADI_MAX32_I2C_DMA_RX_EN);

		if (data->err) {
			ret = data->err;
		}
		if (ret) {
			MXC_I2C_Stop(i2c);
			dma_stop(cfg->tx_dma.dev, cfg->tx_dma.channel);
			dma_stop(cfg->rx_dma.dev, cfg->rx_dma.channel);
			break;
		}
	}

	k_sem_give(&data->lock);

	return ret;
}
#endif /* CONFIG_I2C_MAX32_DMA */

#ifdef CONFIG_I2C_MAX32_INTERRUPT
static int i2c_max32_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			      uint16_t target_address)
{
	int ret = 0;
	const struct max32_i2c_config *const cfg = dev->config;
	struct max32_i2c_data *data = dev->data;
	mxc_i2c_regs_t *i2c = cfg->regs;
	mxc_i2c_req_t *req = &data->req;
	uint8_t target_rw;
	unsigned int i = 0;

	req->i2c = i2c;
	req->addr = target_address;

	k_sem_take(&data->lock, K_FOREVER);

	MXC_I2C_ClearRXFIFO(i2c);
	MXC_I2C_ClearTXFIFO(i2c);
	MXC_I2C_SetRXThreshold(i2c, 1);

	/* First message should always begin with a START condition */
	msgs[0].flags |= I2C_MSG_RESTART;

	for (i = 0; i < num_msgs; i++) {
		if (msgs[i].flags & I2C_MSG_READ) {
			req->rx_buf = (unsigned char *)msgs[i].buf;
			req->rx_len = msgs[i].len;
			req->tx_buf = NULL;
			req->tx_len = 0;
			target_rw = (target_address << 1) | 0x1;
		} else {
			req->tx_buf = (unsigned char *)msgs[i].buf;
			req->tx_len = msgs[i].len;
			req->rx_buf = NULL;
			req->rx_len = 0;
			target_rw = (target_address << 1) & ~0x1;
		}

		/*
		 *  If previous message ends with a STOP condition, this message
		 *  should begin with a START
		 */
		if (i > 0) {
			if ((msgs[i - 1].flags & (I2C_MSG_STOP | I2C_MSG_READ))) {
				msgs[i].flags |= I2C_MSG_RESTART;
			}
		}

		data->flags = msgs[i].flags;
		data->readb = 0;
		data->written = 0;
		data->err = 0;

		MXC_I2C_ClearFlags(i2c, ADI_MAX32_I2C_INT_FL0_MASK, ADI_MAX32_I2C_INT_FL1_MASK);
		MXC_I2C_EnableInt(i2c, ADI_MAX32_I2C_INT_EN0_ERR, 0);
		Wrap_MXC_I2C_SetRxCount(i2c, req->rx_len);
		if ((data->flags & I2C_MSG_RESTART)) {
			MXC_I2C_EnableInt(i2c, ADI_MAX32_I2C_INT_EN0_ADDR_ACK, 0);
			MXC_I2C_Start(i2c);
			Wrap_MXC_I2C_WaitForRestart(i2c);
			MXC_I2C_WriteTXFIFO(i2c, &target_rw, 1);
		} else {
			if (req->tx_len) {
				data->written = MXC_I2C_WriteTXFIFO(i2c, req->tx_buf, 1);
				MXC_I2C_EnableInt(i2c, ADI_MAX32_I2C_INT_EN0_TX_THD, 0);
			} else {
				MXC_I2C_EnableInt(i2c, ADI_MAX32_I2C_INT_EN0_RX_THD, 0);
			}
		}

		ret = k_sem_take(&data->xfer, K_FOREVER);
		if (data->err) {
			MXC_I2C_Stop(i2c);
			ret = data->err;
		} else {
			if (data->flags & I2C_MSG_STOP) {
				/* 0 length transactions are needed for I2C SCANs */
				if ((req->tx_len == req->rx_len) && (req->tx_len == 0)) {
					MXC_I2C_ClearFlags(i2c, ADI_MAX32_I2C_INT_FL0_MASK,
							   ADI_MAX32_I2C_INT_FL1_MASK);
				} else {
					/* Wait for busy flag to be cleared for clock stetching
					 * use-cases
					 */
					while (i2c->status & ADI_MAX32_I2C_STATUS_MASTER_BUSY) {
					}
					MXC_I2C_ClearFlags(i2c, ADI_MAX32_I2C_INT_FL0_MASK,
							   ADI_MAX32_I2C_INT_FL1_MASK);
				}
			}
		}
		if (ret) {
			break;
		}
	}

	k_sem_give(&data->lock);

	return ret;
}
#else
static int i2c_max32_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			      uint16_t target_address)
{
	int ret = 0;
	const struct max32_i2c_config *const cfg = dev->config;
	struct max32_i2c_data *data = dev->data;
	mxc_i2c_regs_t *i2c = cfg->regs;
	mxc_i2c_req_t *req = &data->req;
	uint8_t target_rw;
	unsigned int i = 0;

	req->i2c = i2c;
	req->addr = target_address;

	k_sem_take(&data->lock, K_FOREVER);

	MXC_I2C_ClearRXFIFO(i2c);

	/* First message should always begin with a START condition */
	msgs[0].flags |= I2C_MSG_RESTART;

	for (i = 0; i < num_msgs; i++) {
		if (msgs[i].flags & I2C_MSG_READ) {
			req->rx_buf = (unsigned char *)msgs[i].buf;
			req->rx_len = msgs[i].len;
			req->tx_buf = NULL;
			req->tx_len = 0;
			target_rw = (target_address << 1) | 0x1;
		} else {
			req->tx_buf = (unsigned char *)msgs[i].buf;
			req->tx_len = msgs[i].len;
			req->rx_buf = NULL;
			req->rx_len = 0;
			target_rw = (target_address << 1) & ~0x1;
		}

		/*
		 *  If previous message ends with a STOP condition, this message
		 *  should begin with a START
		 */
		if (i > 0) {
			if ((msgs[i - 1].flags & (I2C_MSG_STOP | I2C_MSG_READ))) {
				msgs[i].flags |= I2C_MSG_RESTART;
			}
		}

		data->flags = msgs[i].flags;
		data->readb = 0;
		data->written = 0;

		MXC_I2C_ClearFlags(i2c, ADI_MAX32_I2C_INT_FL0_MASK, ADI_MAX32_I2C_INT_FL1_MASK);

		Wrap_MXC_I2C_SetIntEn(i2c, 0, 0);
		if (data->flags & I2C_MSG_RESTART) {
			MXC_I2C_Start(i2c);
			Wrap_MXC_I2C_WaitForRestart(i2c);
			MXC_I2C_WriteTXFIFO(i2c, &target_rw, 1);
		}
		ret = i2c_max32_transfer_sync(i2c, data);
		if (ret) {
			MXC_I2C_Stop(i2c);
			break;
		}
	}

	k_sem_give(&data->lock);

	return ret;
}
#endif /* CONFIG_I2C_MAX32_INTERRUPT */

static int api_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			uint16_t target_address)
{
#if CONFIG_I2C_MAX32_DMA
	const struct max32_i2c_config *cfg = dev->config;

	if ((cfg->tx_dma.channel != 0xFF) && (cfg->rx_dma.channel != 0xFF)) {
		return i2c_max32_transfer_dma(dev, msgs, num_msgs, target_address);
	}
#endif
	return i2c_max32_transfer(dev, msgs, num_msgs, target_address);
}

#ifdef CONFIG_I2C_TARGET
static void i2c_max32_isr_target(const struct device *dev, mxc_i2c_regs_t *i2c)
{
	uint32_t ctrl;
	uint32_t int_fl0;
	uint32_t int_fl1;
	uint32_t int_en0;
	uint32_t int_en1;

	ctrl = i2c->ctrl;
	Wrap_MXC_I2C_GetIntEn(i2c, &int_en0, &int_en1);
	MXC_I2C_GetFlags(i2c, &int_fl0, &int_fl1);
	MXC_I2C_ClearFlags(i2c, ADI_MAX32_I2C_INT_FL0_MASK, ADI_MAX32_I2C_INT_FL1_MASK);

	if (int_fl0 & ADI_MAX32_I2C_INT_FL0_ERR) {
		/* Error occurred, notify callback function and end transaction */
		i2c_max32_target_callback(dev, i2c, MXC_I2C_EVT_TRANS_COMP);

		MXC_I2C_ClearFlags(i2c, ADI_MAX32_I2C_INT_FL0_MASK, ADI_MAX32_I2C_INT_FL1_MASK);
		MXC_I2C_ClearTXFIFO(i2c);
		MXC_I2C_ClearRXFIFO(i2c);
	}

	/* Check whether data is available if we received an interrupt occurred while receiving */
	if (int_en0 & ADI_MAX32_I2C_INT_EN0_RX_THD || int_en1 & ADI_MAX32_I2C_INT_EN1_RX_OVERFLOW) {
		if (int_fl0 & ADI_MAX32_I2C_INT_FL0_RX_THD) {
			i2c_max32_target_callback(dev, i2c, MXC_I2C_EVT_RX_THRESH);
		}

		if (int_fl1 & ADI_MAX32_I2C_INT_FL1_RX_OVERFLOW) {
			i2c_max32_target_callback(dev, i2c, MXC_I2C_EVT_OVERFLOW);
		}
	}

	/* Check whether TX FIFO needs to be refilled if interrupt ocurred while transmitting */
	if (int_en0 & (ADI_MAX32_I2C_INT_EN0_TX_THD | ADI_MAX32_I2C_INT_EN0_TX_LOCK_OUT) ||
	    int_en1 & ADI_MAX32_I2C_INT_EN1_TX_UNDERFLOW) {
		if (int_fl0 & ADI_MAX32_I2C_INT_FL0_TX_THD) {
			i2c_max32_target_callback(dev, i2c, MXC_I2C_EVT_TX_THRESH);
		}

		if (int_fl1 & ADI_MAX32_I2C_INT_EN1_TX_UNDERFLOW) {
			i2c_max32_target_callback(dev, i2c, MXC_I2C_EVT_UNDERFLOW);
		}

		if (int_fl0 & ADI_MAX32_I2C_INT_FL0_TX_LOCK_OUT) {
			int_en0 = ADI_MAX32_I2C_INT_EN0_ADDR_MATCH;
			int_en1 = 0;
			i2c_max32_target_callback(dev, i2c, MXC_I2C_EVT_TRANS_COMP);
		}
	}

	/* Check if transaction completed or restart occurred */
	if (int_en0 & ADI_MAX32_I2C_INT_EN0_DONE) {
		if (int_fl0 & ADI_MAX32_I2C_INT_FL0_STOP) {
			/* Stop/NACK condition occurred, transaction complete */
			i2c_max32_target_callback(dev, i2c, MXC_I2C_EVT_TRANS_COMP);
			int_en0 = ADI_MAX32_I2C_INT_EN0_ADDR_MATCH;
		} else if (int_fl0 & ADI_MAX32_I2C_INT_FL0_DONE) {
			/* Restart detected, re-arm address match interrupt */
			int_en0 = ADI_MAX32_I2C_INT_EN0_ADDR_MATCH;
		}
		int_en1 = 0;
	}

	/* Check for address match interrupt */
	if (int_en0 & ADI_MAX32_I2C_INT_EN0_ADDR_MATCH) {
		if (int_fl0 & ADI_MAX32_I2C_INT_FL0_ADDR_MATCH) {
			/* Address match occurred, prepare for transaction */
			if (i2c->ctrl & MXC_F_I2C_CTRL_READ) {
				/* Read request received from the master */
				i2c_max32_target_callback(dev, i2c, MXC_I2C_EVT_MASTER_RD);
				int_en0 = ADI_MAX32_I2C_INT_EN0_TX_THD |
					  ADI_MAX32_I2C_INT_EN0_TX_LOCK_OUT |
					  ADI_MAX32_I2C_INT_EN0_DONE | ADI_MAX32_I2C_INT_EN0_ERR;
				int_en1 = ADI_MAX32_I2C_INT_EN1_TX_UNDERFLOW;
			} else {
				/* Write request received from the master */
				i2c_max32_target_callback(dev, i2c, MXC_I2C_EVT_MASTER_WR);
				int_en0 = ADI_MAX32_I2C_INT_EN0_RX_THD |
					  ADI_MAX32_I2C_INT_EN0_DONE | ADI_MAX32_I2C_INT_EN0_ERR;
				int_en1 = ADI_MAX32_I2C_INT_EN1_RX_OVERFLOW;
			}
		}
	}
	Wrap_MXC_I2C_SetIntEn(i2c, int_en0, int_en1);
}
#endif /* CONFIG_I2C_TARGET */

#ifdef CONFIG_I2C_MAX32_INTERRUPT
static void i2c_max32_isr_controller(const struct device *dev, mxc_i2c_regs_t *i2c)
{
	struct max32_i2c_data *data = dev->data;
	mxc_i2c_req_t *req = &data->req;
	uint32_t written, readb;
	uint32_t txfifolevel;
	uint32_t int_fl0, int_fl1;
	uint32_t int_en0, int_en1;

	written = data->written;
	readb = data->readb;

	Wrap_MXC_I2C_GetIntEn(i2c, &int_en0, &int_en1);
	MXC_I2C_GetFlags(i2c, &int_fl0, &int_fl1);
	MXC_I2C_ClearFlags(i2c, ADI_MAX32_I2C_INT_FL0_MASK, ADI_MAX32_I2C_INT_FL1_MASK);
	txfifolevel = Wrap_MXC_I2C_GetTxFIFOLevel(i2c);

	if (int_fl0 & ADI_MAX32_I2C_INT_FL0_ERR) {
		data->err = -EIO;
		Wrap_MXC_I2C_SetIntEn(i2c, 0, 0);
		k_sem_give(&data->xfer);
		return;
	}

	if (int_fl0 & ADI_MAX32_I2C_INT_FL0_ADDR_ACK) {
		MXC_I2C_DisableInt(i2c, ADI_MAX32_I2C_INT_EN0_ADDR_ACK, 0);
		if (written < req->tx_len) {
			MXC_I2C_EnableInt(i2c, ADI_MAX32_I2C_INT_EN0_TX_THD, 0);
		} else if (readb < req->rx_len) {
			MXC_I2C_EnableInt(
				i2c, ADI_MAX32_I2C_INT_EN0_RX_THD | ADI_MAX32_I2C_INT_EN0_DONE, 0);
		}
		/* 0-length transactions are needed for I2C scans.
		 * In these cases, just give up the semaphore.
		 */
		else if ((req->tx_len == req->rx_len) && (req->tx_len == 0)) {
			k_sem_give(&data->xfer);
		}
	}

	if (req->tx_len &&
	    (int_fl0 & (ADI_MAX32_I2C_INT_FL0_TX_THD | ADI_MAX32_I2C_INT_FL0_DONE))) {
		if (written < req->tx_len) {
			written += MXC_I2C_WriteTXFIFO(i2c, &req->tx_buf[written],
						       req->tx_len - written);
		} else {
			if (!(int_en0 & ADI_MAX32_I2C_INT_EN0_DONE)) {
				/* We are done, stop sending more data */
				MXC_I2C_DisableInt(i2c, ADI_MAX32_I2C_INT_EN0_TX_THD, 0);
				if (data->flags & I2C_MSG_STOP) {
					MXC_I2C_EnableInt(i2c, ADI_MAX32_I2C_INT_EN0_DONE, 0);
					/* Done flag is not set if stop/restart is not set */
					Wrap_MXC_I2C_Stop(i2c);
				} else {
					k_sem_give(&data->xfer);
				}
			}

			if ((int_fl0 & ADI_MAX32_I2C_INT_FL0_DONE)) {
				MXC_I2C_DisableInt(i2c, ADI_MAX32_I2C_INT_EN0_DONE, 0);
				k_sem_give(&data->xfer);
			}
		}
	} else if ((int_fl0 & (ADI_MAX32_I2C_INT_FL0_RX_THD | ADI_MAX32_I2C_INT_FL0_DONE))) {
		readb += MXC_I2C_ReadRXFIFO(i2c, &req->rx_buf[readb], req->rx_len - readb);
		if (readb == req->rx_len) {
			MXC_I2C_DisableInt(i2c, ADI_MAX32_I2C_INT_EN0_RX_THD, 0);
			if (data->flags & I2C_MSG_STOP) {
				MXC_I2C_DisableInt(i2c, ADI_MAX32_I2C_INT_EN0_DONE, 0);
				Wrap_MXC_I2C_Stop(i2c);
				k_sem_give(&data->xfer);
			} else {
				if (int_fl0 & ADI_MAX32_I2C_INT_FL0_DONE) {
					MXC_I2C_DisableInt(i2c, ADI_MAX32_I2C_INT_EN0_DONE, 0);
					k_sem_give(&data->xfer);
				}
			}
		} else if ((int_en0 & ADI_MAX32_I2C_INT_EN0_DONE) &&
			   (int_fl0 & ADI_MAX32_I2C_INT_FL0_DONE)) {
			MXC_I2C_DisableInt(
				i2c, (ADI_MAX32_I2C_INT_EN0_RX_THD | ADI_MAX32_I2C_INT_EN0_DONE),
				0);
			Wrap_MXC_I2C_SetRxCount(i2c, req->rx_len - readb);
			MXC_I2C_EnableInt(i2c, ADI_MAX32_I2C_INT_EN0_ADDR_ACK, 0);
			i2c->fifo = (req->addr << 1) | 0x1;
			Wrap_MXC_I2C_Restart(i2c);
		}
	}

	data->written = written;
	data->readb = readb;
}
#endif /* CONFIG_I2C_MAX32_INTERRUPT */

#ifdef CONFIG_I2C_MAX32_DMA
static void i2c_max32_isr_controller_dma(const struct device *dev, mxc_i2c_regs_t *i2c)
{
	struct max32_i2c_data *data = dev->data;
	const struct max32_i2c_config *cfg = dev->config;
	struct dma_status dma_stat;
	uint32_t int_fl0, int_fl1;
	uint32_t int_en0, int_en1;

	Wrap_MXC_I2C_GetIntEn(i2c, &int_en0, &int_en1);
	MXC_I2C_GetFlags(i2c, &int_fl0, &int_fl1);
	MXC_I2C_ClearFlags(i2c, ADI_MAX32_I2C_INT_FL0_MASK, ADI_MAX32_I2C_INT_FL1_MASK);

	if (int_fl0 & ADI_MAX32_I2C_INT_FL0_ERR) {
		data->err = -EIO;
		Wrap_MXC_I2C_SetIntEn(i2c, 0, 0);
		k_sem_give(&data->xfer);
	} else {
		/* Run DMA once address is acknowledged */
		if ((int_fl0 & ADI_MAX32_I2C_INT_FL0_ADDR_ACK)) {
			MXC_I2C_DisableInt(i2c, ADI_MAX32_I2C_INT_EN0_ADDR_ACK, 0);
			MXC_I2C_EnableInt(i2c, ADI_MAX32_I2C_INT_EN0_DONE, 0);
			if (data->flags & I2C_MSG_READ) {
				i2c->dma |= ADI_MAX32_I2C_DMA_RX_EN;
			} else {
				i2c->dma |= ADI_MAX32_I2C_DMA_TX_EN;
			}
		} else if ((int_fl0 & ADI_MAX32_I2C_INT_FL0_DONE)) {
			MXC_I2C_DisableInt(i2c, ADI_MAX32_I2C_INT_EN0_DONE, 0);
			if ((data->flags & I2C_MSG_READ)) {
				dma_get_status(cfg->rx_dma.dev, cfg->rx_dma.channel, &dma_stat);
				/* Send RESTART if more data is expected */
				if (dma_stat.pending_length > 0) {
					Wrap_MXC_I2C_SetRxCount(i2c, dma_stat.pending_length);
					MXC_I2C_EnableInt(i2c, ADI_MAX32_I2C_INT_EN0_ADDR_ACK, 0);
					i2c->fifo = (data->req.addr << 1) | 0x1;
					Wrap_MXC_I2C_Restart(i2c);
				} else {
					k_sem_give(&data->xfer);
				}
			} else {
				k_sem_give(&data->xfer);
			}
		} else if ((int_fl0 & ADI_MAX32_I2C_INT_FL0_TX_THD)) {
			MXC_I2C_DisableInt(
				i2c, ADI_MAX32_I2C_INT_EN0_DONE | ADI_MAX32_I2C_INT_EN0_TX_THD, 0);
			k_sem_give(&data->xfer);
		}
	}
}
#endif /* CONFIG_I2C_MAX32_DMA */

#if defined(CONFIG_I2C_TARGET) || defined(CONFIG_I2C_MAX32_INTERRUPT)
static void i2c_max32_isr(const struct device *dev)
{
	const struct max32_i2c_config *cfg = dev->config;
	struct max32_i2c_data *data = dev->data;
	mxc_i2c_regs_t *i2c = cfg->regs;

#ifdef CONFIG_I2C_MAX32_INTERRUPT
	if (data->target_mode == 0) {
#ifdef CONFIG_I2C_MAX32_DMA
		if ((cfg->tx_dma.channel != 0xFF) && (cfg->rx_dma.channel != 0xFF)) {
			i2c_max32_isr_controller_dma(dev, i2c);
			return;
		}
#endif
		i2c_max32_isr_controller(dev, i2c);
		return;
	}
#endif /* CONFIG_I2C_MAX32_INTERRUPT */

#ifdef CONFIG_I2C_TARGET
	if (data->target_mode == 1) {
		i2c_max32_isr_target(dev, i2c);
	}
#endif
}
#endif /* CONFIG_I2C_TARGET || CONFIG_I2C_MAX32_INTERRUPT */

static DEVICE_API(i2c, api) = {
	.configure = api_configure,
	.transfer = api_transfer,
#ifdef CONFIG_I2C_TARGET
	.target_register = api_target_register,
	.target_unregister = api_target_unregister,
#endif
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
	.recover_bus = api_recover_bus,
};

static int i2c_max32_init(const struct device *dev)
{
	const struct max32_i2c_config *const cfg = dev->config;
	struct max32_i2c_data *data = dev->data;
	mxc_i2c_regs_t *i2c = cfg->regs;
	int ret = 0;

	if (!device_is_ready(cfg->clock)) {
		return -ENODEV;
	}

	MXC_I2C_Shutdown(i2c); /* Clear everything out */

	ret = clock_control_on(cfg->clock, (clock_control_subsys_t)&cfg->perclk);
	if (ret) {
		return ret;
	}

	ret = pinctrl_apply_state(cfg->pctrl, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}

	ret = MXC_I2C_Init(i2c, 1, 0); /* Configure as master */
	if (ret) {
		return ret;
	}

	MXC_I2C_SetFrequency(i2c, cfg->bitrate);

	k_sem_init(&data->lock, 1, 1);

#if defined(CONFIG_I2C_TARGET) || defined(CONFIG_I2C_MAX32_INTERRUPT)
	cfg->irq_config_func(dev);
#endif

#ifdef CONFIG_I2C_MAX32_INTERRUPT
	irq_enable(cfg->irqn);

	k_sem_init(&data->xfer, 0, 1);
#endif

#if defined(CONFIG_I2C_TARGET)
	data->first_write = true;
	data->target_mode = 0;
#endif
	data->dev = dev;

	return ret;
}

#if defined(CONFIG_I2C_TARGET) || defined(CONFIG_I2C_MAX32_INTERRUPT)
#define I2C_MAX32_CONFIG_IRQ_FUNC(n)                                                               \
	.irq_config_func = i2c_max32_irq_config_func_##n, .irqn = DT_INST_IRQN(n),

#define I2C_MAX32_IRQ_CONFIG_FUNC(n)                                                               \
	static void i2c_max32_irq_config_func_##n(const struct device *dev)                        \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), i2c_max32_isr,              \
			    DEVICE_DT_INST_GET(n), 0);                                             \
	}
#else
#define I2C_MAX32_CONFIG_IRQ_FUNC(n)
#define I2C_MAX32_IRQ_CONFIG_FUNC(n)
#endif

#if CONFIG_I2C_MAX32_DMA
#define MAX32_DT_INST_DMA_CTLR(n, name)                                                            \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, dmas),                                                \
		    (DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(n, name))), (NULL))

#define MAX32_DT_INST_DMA_CELL(n, name, cell)                                                      \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, dmas), (DT_INST_DMAS_CELL_BY_NAME(n, name, cell)),    \
		    (0xff))

#define MAX32_I2C_TX_DMA_INIT(n)                                                                   \
	.tx_dma.dev = MAX32_DT_INST_DMA_CTLR(n, tx),                                               \
	.tx_dma.channel = MAX32_DT_INST_DMA_CELL(n, tx, channel),                                  \
	.tx_dma.slot = MAX32_DT_INST_DMA_CELL(n, tx, slot),
#define MAX32_I2C_RX_DMA_INIT(n)                                                                   \
	.rx_dma.dev = MAX32_DT_INST_DMA_CTLR(n, rx),                                               \
	.rx_dma.channel = MAX32_DT_INST_DMA_CELL(n, rx, channel),                                  \
	.rx_dma.slot = MAX32_DT_INST_DMA_CELL(n, rx, slot),
#else
#define MAX32_I2C_TX_DMA_INIT(n)
#define MAX32_I2C_RX_DMA_INIT(n)
#endif

#define DEFINE_I2C_MAX32(_num)                                                                     \
	PINCTRL_DT_INST_DEFINE(_num);                                                              \
	I2C_MAX32_IRQ_CONFIG_FUNC(_num)                                                            \
	static const struct max32_i2c_config max32_i2c_dev_cfg_##_num = {                          \
		.regs = (mxc_i2c_regs_t *)DT_INST_REG_ADDR(_num),                                  \
		.pctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(_num),                                     \
		.clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(_num)),                                 \
		.perclk.bus = DT_INST_CLOCKS_CELL(_num, offset),                                   \
		.perclk.bit = DT_INST_CLOCKS_CELL(_num, bit),                                      \
		.bitrate = DT_INST_PROP(_num, clock_frequency),                                    \
		I2C_MAX32_CONFIG_IRQ_FUNC(_num) MAX32_I2C_TX_DMA_INIT(_num)                        \
			MAX32_I2C_RX_DMA_INIT(_num)};                                              \
	static struct max32_i2c_data max32_i2c_data_##_num;                                        \
	I2C_DEVICE_DT_INST_DEFINE(_num, i2c_max32_init, NULL, &max32_i2c_data_##_num,              \
				  &max32_i2c_dev_cfg_##_num, PRE_KERNEL_2,                         \
				  CONFIG_I2C_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_I2C_MAX32)
