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
#ifdef CONFIG_I2C_SF32LB_DMA
#include <zephyr/drivers/dma/sf32lb.h>
#endif

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

struct i2c_sf32lb_config {
	uintptr_t base;
	const struct pinctrl_dev_config *pincfg;
	struct sf32lb_clock_dt_spec clock;
	uint32_t bitrate;
#if defined(CONFIG_I2C_SF32LB_INTERRUPT) || defined(CONFIG_I2C_SF32LB_DMA)
	void (*irq_cfg_func)(void);
#endif
#ifdef CONFIG_I2C_SF32LB_DMA
	struct sf32lb_dma_dt_spec dma_rx;
	struct sf32lb_dma_dt_spec dma_tx;
#endif
};

struct i2c_sf32lb_data {
	struct k_mutex lock;
	uint8_t rw_flags;
#if defined(CONFIG_I2C_SF32LB_INTERRUPT) || defined(CONFIG_I2C_SF32LB_DMA)
	struct k_sem i2c_compl;
#endif
#ifdef CONFIG_I2C_SF32LB_INTERRUPT
	struct i2c_msg *current_msg;
	uint8_t *buf_ptr;
	uint32_t remaining;
	bool is_tx;
	int error;
#endif
};

#if defined(CONFIG_I2C_SF32LB_INTERRUPT) || defined(CONFIG_I2C_SF32LB_DMA)
static void i2c_sf32lb_isr(const struct device *dev)
{
	const struct i2c_sf32lb_config *config = dev->config;
	struct i2c_sf32lb_data *data = dev->data;
	uint32_t sr = sys_read32(config->base + I2C_SR);
#ifdef CONFIG_I2C_SF32LB_INTERRUPT
	uint32_t tcr;
#endif

#ifdef CONFIG_I2C_SF32LB_DMA
	if (IS_BIT_SET(sr, I2C_SR_DMADONE_Pos)) {
		sys_set_bit(config->base + I2C_SR, I2C_SR_DMADONE_Pos);
		sys_clear_bit(config->base + I2C_CR, I2C_CR_DMAEN_Pos);
		printk("DMA transfer completed\n");
		k_sem_give(&data->i2c_compl);
	}
#endif
#ifdef CONFIG_I2C_SF32LB_INTERRUPT
	if (data->is_tx) {
		if (IS_BIT_SET(sr, I2C_SR_TE_Pos)) {
			if (IS_BIT_SET(sr, I2C_SR_NACK_Pos)) {
				data->error = -EIO;
				sys_write32(0, config->base + I2C_IER);
				data->current_msg = NULL;
				k_sem_give(&data->i2c_compl);
			}

			sys_set_bit(config->base + I2C_SR, I2C_SR_TE_Pos);

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
			sys_clear_bit(config->base + I2C_SR, I2C_SR_MSD_Pos);
			sys_write32(0, config->base + I2C_IER);
			data->current_msg = NULL;
			k_sem_give(&data->i2c_compl);
		}
	} else {
		if (IS_BIT_SET(sr, I2C_SR_RF_Pos)) {
			sys_set_bit(config->base + I2C_SR, I2C_SR_RF_Pos);

			if (data->remaining > 0) {
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
			sys_write32(BIT(I2C_SR_MSD_Pos), config->base + I2C_SR);
			sys_write32(0, config->base + I2C_IER);
			data->current_msg = NULL;
			k_sem_give(&data->i2c_compl);
		}
	}
#endif
}
#endif

__USED volatile int test_flag = 0;

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
		LOG_ERR("No ACK received for address 0x%02x", addr >> 1);
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

#ifdef CONFIG_I2C_SF32LB_INTERRUPT
static int i2c_sf32lb_master_send_it(const struct device *dev, uint16_t addr, struct i2c_msg *msg)
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

static int i2c_sf32lb_master_recv_it(const struct device *dev, uint16_t addr, struct i2c_msg *msg)
{
	int ret = 0;
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
#elif defined(CONFIG_I2C_SF32LB_DMA)
static void i2c_sf32lb_rx_done(const struct device *dev, void *arg, uint32_t channel, int status)
{
	const struct device *device = arg;
	const struct i2c_sf32lb_config *config = device->config;
	struct i2c_sf32lb_data *data = device->data;

	LOG_DBG("I2C RX DMA transfer completed\n");

	// sys_write32(0, config->base + I2C_IER);

	// k_sem_give(&data->i2c_compl);
}

static void i2c_sf32lb_tx_done(const struct device *dev, void *arg, uint32_t channel, int status)
{
	const struct device *device = arg;
	const struct i2c_sf32lb_config *config = device->config;
	struct i2c_sf32lb_data *data = device->data;

	LOG_DBG("I2C TX DMA transfer completed\n");

	// sys_write32(0, config->base + I2C_IER);

	// k_sem_give(&data->i2c_compl);
}

static int i2c_sf32lb_dma_tx_config(const struct device *dev, struct i2c_msg *msg)
{
	const struct i2c_sf32lb_config *config = dev->config;
	int err;

	err = sf32lb_dma_reload_dt(&config->dma_tx, (uint32_t)msg->buf,
				   (uint32_t)(config->base + I2C_FIFO), msg->len);
	if (err) {
		LOG_ERR("Error configuring Rx DMA (%d)", err);
	}

	return err;
}

static int i2c_sf32lb_dma_rx_config(const struct device *dev, struct i2c_msg *msg)
{
	const struct i2c_sf32lb_config *config = dev->config;
	int err;

	err = sf32lb_dma_reload_dt(&config->dma_rx, (uint32_t)(config->base + I2C_FIFO),
				   (uint32_t)msg->buf, msg->len);
	if (err) {
		LOG_ERR("Error configuring Rx DMA (%d)", err);
	}

	return err;
}

static int i2c_sf32lb_master_send_dma(const struct device *dev, uint16_t addr, struct i2c_msg *msg)
{
	int ret;
	const struct i2c_sf32lb_config *config = dev->config;
	struct i2c_sf32lb_data *data = dev->data;
	bool addr_sent = (data->rw_flags != (msg->flags & I2C_MSG_RW_MASK));
	bool stop_needed = i2c_is_stop_op(msg);

	data->rw_flags = msg->flags & I2C_MSG_RW_MASK;

	if (msg->len > 512U) {
		return -ENOTSUP;
	}

	LOG_DBG("I2C master send DMA len=%d", msg->len);

	if (addr_sent) {
		ret = i2c_sf32lb_send_addr(dev, addr, msg);
		if (ret < 0) {
			return ret;
		}
	}

	// __ASM("B .");

	if (stop_needed) {
		sys_set_bits(config->base + I2C_CR, I2C_CR_LASTSTOP);
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

	ret = k_sem_take(&data->i2c_compl, K_FOREVER);
	if (ret < 0) {
		LOG_ERR("master send timeout");
		return ret;
	}

	/* Cleanup - same as recv_dma */
	sys_clear_bit(config->base + I2C_CR, I2C_CR_DMAEN_Pos);
	sys_set_bit(config->base + I2C_SR, I2C_SR_DMADONE_Pos);

	/* Wait for bus idle if stop was issued */
	if (stop_needed) {
		while (sys_test_bit(config->base + I2C_SR, I2C_SR_UB_Pos)) {
			k_sleep(K_MSEC(1));
		}
		sys_clear_bits(config->base + I2C_CR, I2C_CR_LASTSTOP | I2C_CR_MSDE);
	}

	sf32lb_dma_stop_dt(&config->dma_tx);

	return 0;
}

static int i2c_sf32lb_master_recv_dma(const struct device *dev, uint16_t addr, struct i2c_msg *msg)
{
	const struct i2c_sf32lb_config *config = dev->config;
	struct i2c_sf32lb_data *data = dev->data;
	bool addr_sent = (data->rw_flags != (msg->flags & I2C_MSG_RW_MASK));
	bool stop_needed = i2c_is_stop_op(msg);
	int ret;

	data->rw_flags = msg->flags & I2C_MSG_RW_MASK;

	if (msg->len > 512U) {
		return -ENOTSUP;
	}

	LOG_DBG("I2C master recv DMA len=%d", msg->len);

	if (addr_sent) {
		ret = i2c_sf32lb_send_addr(dev, addr, msg);
		if (ret < 0) {
			return ret;
		}
	}

	if (stop_needed) {
		sys_set_bits(config->base + I2C_CR, I2C_CR_LASTNACK | I2C_CR_LASTSTOP);
	}

	/* Clear stale completion, program byte count, then arm IRQ */
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

	// __ASM("B .");
	printk("I2C_DNR: %d\n", msg->len);
	printk("I2C_TCR: 0x%08x\n", sys_read32(config->base + I2C_TCR));
	printk("I2C_CR: 0x%08x\n", sys_read32(config->base + I2C_CR));
	printk("I2C_IER: 0x%08x\n", sys_read32(config->base + I2C_IER));
	sys_set_bit(config->base + I2C_CR, I2C_CR_DMAEN_Pos);
	// if (stop_needed) {
	// 	sys_write32(I2C_CR_LASTNACK | I2C_CR_LASTSTOP | I2C_CR_DMAEN,
	// 		    config->base + I2C_CR);
	// } else {
	// 	sys_write32(I2C_CR_DMAEN, config->base + I2C_CR);
	// }
	sys_write8(msg->len, config->base + I2C_DNR);
	sys_write32(I2C_TCR_RXREQ, config->base + I2C_TCR);
	// sys_set_bit(config->base + I2C_TCR, I2C_TCR_RXREQ_Pos);

	// ret = k_sem_take(&data->i2c_compl, K_MSEC(SF32LB_I2C_TIMEOUT_MAX_US / 1000));
	// forever
	ret = k_sem_take(&data->i2c_compl, K_FOREVER);
	if (ret < 0) {
		LOG_ERR("master recv timeout");
		return ret;
	}

	sys_clear_bit(config->base + I2C_CR, I2C_CR_DMAEN_Pos);
	sys_set_bit(config->base + I2C_SR, I2C_SR_DMADONE_Pos);
	if (stop_needed) {
		while (sys_test_bit(config->base + I2C_SR, I2C_SR_UB_Pos)) {
			k_sleep(K_MSEC(1));
		}
		sys_clear_bits(config->base + I2C_CR, I2C_CR_LASTNACK | I2C_CR_LASTSTOP);
	}
	sf32lb_dma_stop_dt(&config->dma_rx);

	return ret;
}
#else
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

	for (uint32_t j = 0U; j < msg->len; j++) {
		bool last = ((msg->len - j) == 1U) ? true : false;

		if (last && stop_needed) {
			tcr |= I2C_TCR_STOP;
		}

		sys_write8(msg->buf[j], cfg->base + I2C_DBR);

		sys_write32(tcr, cfg->base + I2C_TCR);

		while (!sys_test_bit(cfg->base + I2C_SR, I2C_SR_TE_Pos)) {
		}

		sys_set_bit(cfg->base + I2C_SR, I2C_SR_TE_Pos);

		if (sys_test_bit(cfg->base + I2C_SR, I2C_SR_NACK_Pos)) {
			ret = -EIO;
			break;
		}
	}

	if (stop_needed) {
		while (sys_test_bit(cfg->base + I2C_SR, I2C_SR_UB_Pos)) {
		}
	}

	return ret;
}

static int i2c_sf32lb_master_recv(const struct device *dev, uint16_t addr, struct i2c_msg *msg)
{
	int ret = 0;
	const struct i2c_sf32lb_config *cfg = dev->config;
	struct i2c_sf32lb_data *data = dev->data;
	uint32_t tcr = I2C_TCR_TB;
	bool stop_needed = i2c_is_stop_op(msg);

	data->rw_flags = msg->flags & I2C_MSG_RW_MASK;

	ret = i2c_sf32lb_send_addr(dev, addr, msg);
	if (ret < 0) {
		return ret;
	}

	for (uint32_t j = 0U; j < msg->len; j++) {
		bool last = (j == (msg->len - 1U)) ? true : false;

		if (last && stop_needed) {
			tcr |= (I2C_TCR_STOP | I2C_TCR_NACK);
		}

		sys_write32(tcr, cfg->base + I2C_TCR);

		while (!sys_test_bit(cfg->base + I2C_SR, I2C_SR_RF_Pos)) {
		}

		sys_set_bit(cfg->base + I2C_SR, I2C_SR_RF_Pos);

		msg->buf[j] = sys_read8(cfg->base + I2C_DBR);
	}

	if (stop_needed) {
		while (sys_test_bit(cfg->base + I2C_SR, I2C_SR_UB_Pos)) {
		}
	}

	return ret;
}
#endif

// static int i2c_sf32lb_master_send(const struct device *dev, uint16_t addr, struct i2c_msg *msg)
// {
// 	int ret = 0;
// 	const struct i2c_sf32lb_config *cfg = dev->config;
// 	struct i2c_sf32lb_data *data = dev->data;
// 	uint32_t tcr = I2C_TCR_TB;
// 	bool stop_needed = i2c_is_stop_op(msg);
// 	bool addr_sent = (data->rw_flags != (msg->flags & I2C_MSG_RW_MASK));

// 	data->rw_flags = msg->flags & I2C_MSG_RW_MASK;

// 	if (addr_sent) {
// 		ret = i2c_sf32lb_send_addr(dev, addr, msg);
// 		if (ret < 0) {
// 			return ret;
// 		}
// 	}

// 	for (uint32_t j = 0U; j < msg->len; j++) {
// 		bool last = ((msg->len - j) == 1U) ? true : false;

// 		if (last && stop_needed) {
// 			tcr |= I2C_TCR_STOP;
// 		}

// 		sys_write8(msg->buf[j], cfg->base + I2C_DBR);

// 		sys_write32(tcr, cfg->base + I2C_TCR);

// 		while (!sys_test_bit(cfg->base + I2C_SR, I2C_SR_TE_Pos)) {
// 		}

// 		sys_set_bit(cfg->base + I2C_SR, I2C_SR_TE_Pos);

// 		if (sys_test_bit(cfg->base + I2C_SR, I2C_SR_NACK_Pos)) {
// 			ret = -EIO;
// 			break;
// 		}
// 	}

// 	if (stop_needed) {
// 		while (sys_test_bit(cfg->base + I2C_SR, I2C_SR_UB_Pos)) {
// 		}
// 	}

// 	return ret;
// }

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

static int i2c_sf32lb_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			       uint16_t addr)
{
	const struct i2c_sf32lb_config *cfg = dev->config;
	struct i2c_sf32lb_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);
	printk("i2c_sf32lb_transfer num_msgs=%d addr=0x%02x\n", num_msgs, addr);
	sys_set_bits(cfg->base + I2C_CR, I2C_CR_IUE | I2C_CR_MSDE);
	// __ASM("B .");

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
#if defined(CONFIG_I2C_SF32LB_DMA)
			ret = i2c_sf32lb_master_recv_dma(dev, addr, &msgs[i]);
#elif defined(CONFIG_I2C_SF32LB_INTERRUPT)
			ret = i2c_sf32lb_master_recv_it(dev, addr, &msgs[i]);
#else
			ret = i2c_sf32lb_master_recv(dev, addr, &msgs[i]);
#endif
			if (ret < 0) {
				break;
			}
		} else {
#if defined(CONFIG_I2C_SF32LB_DMA)
			ret = i2c_sf32lb_master_send_dma(dev, addr, &msgs[i]);
#elif defined(CONFIG_I2C_SF32LB_INTERRUPT)
			ret = i2c_sf32lb_master_send_it(dev, addr, &msgs[i]);
#else
			ret = i2c_sf32lb_master_send(dev, addr, &msgs[i]);
#endif
			if (ret < 0) {
				break;
			}
		}
	}

	sys_clear_bit(cfg->base + I2C_CR, I2C_CR_IUE_Pos);

	data->rw_flags = I2C_MSG_READ;

	k_mutex_unlock(&data->lock);

	return ret;
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
	.transfer = i2c_sf32lb_transfer,
	.recover_bus = i2c_sf32lb_recover_bus,
};

static int i2c_sf32lb_init(const struct device *dev)
{
	const struct i2c_sf32lb_config *config = dev->config;
	struct i2c_sf32lb_data *data = dev->data;
	int ret;
#ifdef CONFIG_I2C_SF32LB_DMA
	struct dma_config tx_dma_cfg = {0};
	struct dma_config rx_dma_cfg = {0};
	struct dma_block_config dma_blk = {0};
#endif

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	if (!sf32lb_clock_is_ready_dt(&config->clock)) {
		return -ENODEV;
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

#if defined(CONFIG_I2C_SF32LB_INTERRUPT) || defined(CONFIG_I2C_SF32LB_DMA)
	if (config->irq_cfg_func) {
		config->irq_cfg_func();
	}
#endif
#ifdef CONFIG_I2C_SF32LB_DMA
	if (!sf32lb_dma_is_ready_dt(&config->dma_tx)) {
		return -ENODEV;
	}

	if (!sf32lb_dma_is_ready_dt(&config->dma_rx)) {
		return -ENODEV;
	}

	sf32lb_dma_config_init_dt(&config->dma_tx, &tx_dma_cfg);
	sf32lb_dma_config_init_dt(&config->dma_rx, &rx_dma_cfg);

	// __ASM("B .");

	/* TX DMA configuration */
	tx_dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	tx_dma_cfg.block_count = 1U;
	tx_dma_cfg.source_data_size = 1U;
	tx_dma_cfg.dest_data_size = 1U;
	tx_dma_cfg.complete_callback_en = 1U;
	tx_dma_cfg.dma_callback = i2c_sf32lb_tx_done;
	tx_dma_cfg.user_data = (void *)dev;

	tx_dma_cfg.head_block = &dma_blk;
	dma_blk.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	dma_blk.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

	sf32lb_dma_config_dt(&config->dma_tx, &tx_dma_cfg);

	/* RX DMA configuration - reset dma_blk for RX settings */
	memset(&dma_blk, 0, sizeof(dma_blk));
	rx_dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	rx_dma_cfg.block_count = 1U;
	rx_dma_cfg.source_data_size = 1U;
	rx_dma_cfg.dest_data_size = 1U;
	rx_dma_cfg.complete_callback_en = 1U;
	rx_dma_cfg.dma_callback = i2c_sf32lb_rx_done;
	rx_dma_cfg.user_data = (void *)dev;

	rx_dma_cfg.head_block = &dma_blk;
	dma_blk.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	dma_blk.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;

	sf32lb_dma_config_dt(&config->dma_rx, &rx_dma_cfg);

	sys_set_bit(config->base + I2C_IER, I2C_IER_DMADONEIE_Pos);
#endif
	return ret;
}

#if defined(CONFIG_I2C_SF32LB_INTERRUPT) || defined(CONFIG_I2C_SF32LB_DMA)
#define I2C_SF32LB_IRQ_HANDLER_DECL(n) static void i2c_sf32lb_irq_config_func_##n(void);
#define I2C_SF32LB_IRQ_HANDLER_FUNC(n) .irq_cfg_func = i2c_sf32lb_irq_config_func_##n,
#define I2C_SF32LB_SEM_INIT(n)         .i2c_compl = Z_SEM_INITIALIZER(i2c_sf32lb_data_##n.i2c_compl, 0, 1),
#define I2C_SF32LB_IRQ_HANDLER(n)                                                                  \
	static void i2c_sf32lb_irq_config_func_##n(void)                                           \
	{                                                                                          \
                                                                                                   \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), i2c_sf32lb_isr,             \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	};
#else
#define I2C_SF32LB_IRQ_HANDLER_DECL
#define I2C_SF32LB_IRQ_HANDLER_FUNC(n)
#define I2C_SF32LB_IRQ_HANDLER(n)
#define I2C_SF32LB_SEM_INIT(n)
#endif

#define I2C_SF32LB_DEFINE(n)                                                                         \
	PINCTRL_DT_INST_DEFINE(n);                                                                   \
	I2C_SF32LB_IRQ_HANDLER_DECL(n)                                                               \
                                                                                                     \
	static struct i2c_sf32lb_data i2c_sf32lb_data_##n = {                                        \
		.lock = Z_MUTEX_INITIALIZER(i2c_sf32lb_data_##n.lock), I2C_SF32LB_SEM_INIT(n)};      \
	static const struct i2c_sf32lb_config i2c_sf32lb_config_##n = {                              \
		.base = DT_INST_REG_ADDR(n),                                                         \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.clock = SF32LB_CLOCK_DT_INST_SPEC_GET(n),                                           \
		.bitrate = DT_INST_PROP_OR(n, clock_frequency, 100000),                              \
		IF_ENABLED(CONFIG_I2C_SF32LB_DMA, (                                                \
			.dma_tx = SF32LB_DMA_DT_INST_SPEC_GET_BY_NAME(n, tx),                      \
			.dma_rx = SF32LB_DMA_DT_INST_SPEC_GET_BY_NAME(n, rx),                      \
		)) \
					    I2C_SF32LB_IRQ_HANDLER_FUNC(n)};                         \
	I2C_SF32LB_IRQ_HANDLER(n)                                                                    \
	DEVICE_DT_INST_DEFINE(n, i2c_sf32lb_init, NULL, &i2c_sf32lb_data_##n,                        \
			      &i2c_sf32lb_config_##n, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,         \
			      &i2c_sf32lb_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_SF32LB_DEFINE)
