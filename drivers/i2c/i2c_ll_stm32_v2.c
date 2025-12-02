/*
 * Copyright (c) 2016 BayLibre, SAS
 * Copyright (c) 2017 Linaro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * I2C Driver for: STM32F0, STM32F3, STM32F7, STM32L0, STM32L4, STM32WB and
 * STM32WL
 *
 */

#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <stm32_bitops.h>
#include <stm32_cache.h>
#include <stm32_ll_i2c.h>
#include <errno.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <stm32_cache.h>
#include <zephyr/cache.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/mem_mgmt/mem_attr.h>
#include <zephyr/dt-bindings/memory-attr/memory-attr-arm.h>

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_ll_stm32_v2);

#include "i2c_ll_stm32.h"
#include "i2c-priv.h"

#ifdef CONFIG_I2C_STM32_V2_TIMING
/* Use the algorithm to calcuate the I2C timing */
#ifndef I2C_STM32_VALID_TIMING_NBR
#define I2C_STM32_VALID_TIMING_NBR                 128U
#endif
#define I2C_STM32_SPEED_FREQ_STANDARD                0U    /* 100 kHz */
#define I2C_STM32_SPEED_FREQ_FAST                    1U    /* 400 kHz */
#define I2C_STM32_SPEED_FREQ_FAST_PLUS               2U    /* 1 MHz */
#define I2C_STM32_ANALOG_FILTER_DELAY_MIN            50U   /* ns */
#define I2C_STM32_ANALOG_FILTER_DELAY_MAX            260U  /* ns */
#define I2C_STM32_USE_ANALOG_FILTER                  1U
#define I2C_STM32_DIGITAL_FILTER_COEF                0U
#define I2C_STM32_PRESC_MAX                          16U
#define I2C_STM32_SCLDEL_MAX                         16U
#define I2C_STM32_SDADEL_MAX                         16U
#define I2C_STM32_SCLH_MAX                           256U
#define I2C_STM32_SCLL_MAX                           256U

/* I2C_DEVICE_Private_Types */
struct i2c_stm32_charac_t {
	uint32_t freq;       /* Frequency in Hz */
	uint32_t freq_min;   /* Minimum frequency in Hz */
	uint32_t freq_max;   /* Maximum frequency in Hz */
	uint32_t hddat_min;  /* Minimum data hold time in ns */
	uint32_t vddat_max;  /* Maximum data valid time in ns */
	uint32_t sudat_min;  /* Minimum data setup time in ns */
	uint32_t lscl_min;   /* Minimum low period of the SCL clock in ns */
	uint32_t hscl_min;   /* Minimum high period of SCL clock in ns */
	uint32_t trise;      /* Rise time in ns */
	uint32_t tfall;      /* Fall time in ns */
	uint32_t dnf;        /* Digital noise filter coefficient */
};

struct i2c_stm32_timings_t {
	uint32_t presc;      /* Timing prescaler */
	uint32_t tscldel;    /* SCL delay */
	uint32_t tsdadel;    /* SDA delay */
	uint32_t sclh;       /* SCL high period */
	uint32_t scll;       /* SCL low period */
};

/* I2C_DEVICE Private Constants */
static const struct i2c_stm32_charac_t i2c_stm32_charac[] = {
	[I2C_STM32_SPEED_FREQ_STANDARD] = {
		.freq = 100000,
		.freq_min = 80000,
		.freq_max = 120000,
		.hddat_min = 0,
		.vddat_max = 3450,
		.sudat_min = 250,
		.lscl_min = 4700,
		.hscl_min = 4000,
		.trise = 640,
		.tfall = 20,
		.dnf = I2C_STM32_DIGITAL_FILTER_COEF,
	},
	[I2C_STM32_SPEED_FREQ_FAST] = {
		.freq = 400000,
		.freq_min = 320000,
		.freq_max = 480000,
		.hddat_min = 0,
		.vddat_max = 900,
		.sudat_min = 100,
		.lscl_min = 1300,
		.hscl_min = 600,
		.trise = 250,
		.tfall = 100,
		.dnf = I2C_STM32_DIGITAL_FILTER_COEF,
	},
	[I2C_STM32_SPEED_FREQ_FAST_PLUS] = {
		.freq = 1000000,
		.freq_min = 800000,
		.freq_max = 1200000,
		.hddat_min = 0,
		.vddat_max = 450,
		.sudat_min = 50,
		.lscl_min = 500,
		.hscl_min = 260,
		.trise = 60,
		.tfall = 100,
		.dnf = I2C_STM32_DIGITAL_FILTER_COEF,
	},
};

static struct i2c_stm32_timings_t i2c_valid_timing[I2C_STM32_VALID_TIMING_NBR];
static uint32_t i2c_valid_timing_nbr;
#endif /* CONFIG_I2C_STM32_V2_TIMING */

#ifdef CONFIG_I2C_STM32_V2_DMA
static int configure_dma(struct stream const *dma, struct dma_config *dma_cfg,
			 struct dma_block_config *blk_cfg)
{
	if (!device_is_ready(dma->dev_dma)) {
		LOG_ERR("DMA device not ready");
		return -ENODEV;
	}

	dma_cfg->head_block = blk_cfg;
	dma_cfg->block_count = 1;

	int ret = dma_config(dma->dev_dma, dma->dma_channel, dma_cfg);

	if (ret != 0) {
		LOG_ERR("Problem setting up DMA: %d", ret);
		return ret;
	}

	ret = dma_start(dma->dev_dma, dma->dma_channel);

	if (ret != 0) {
		LOG_ERR("Problem starting DMA: %d", ret);
		return ret;
	}

	return 0;
}

static int dma_xfer_start(const struct device *dev, struct i2c_msg *msg)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;
	int ret = 0;

	if ((msg->flags & I2C_MSG_READ) != 0U) {
		/* Configure RX DMA */
		data->dma_blk_cfg.source_address = LL_I2C_DMA_GetRegAddr(
			cfg->i2c, LL_I2C_DMA_REG_DATA_RECEIVE);
		data->dma_blk_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		data->dma_blk_cfg.dest_address = (uint32_t)data->current.buf;
		data->dma_blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		data->dma_blk_cfg.block_size = data->current.len;

		ret = configure_dma(&cfg->rx_dma, &data->dma_rx_cfg, &data->dma_blk_cfg);
		if (ret != 0) {
			return ret;
		}
		LL_I2C_EnableDMAReq_RX(i2c);
	} else {
		if (data->current.len != 0U) {
			/* Configure TX DMA */
			data->dma_blk_cfg.source_address = (uint32_t)data->current.buf;
			data->dma_blk_cfg.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
			data->dma_blk_cfg.dest_address = LL_I2C_DMA_GetRegAddr(
				cfg->i2c, LL_I2C_DMA_REG_DATA_TRANSMIT);
			data->dma_blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
			data->dma_blk_cfg.block_size = data->current.len;

			ret = configure_dma(&cfg->tx_dma, &data->dma_tx_cfg, &data->dma_blk_cfg);
			if (ret != 0) {
				return ret;
			}
			LL_I2C_EnableDMAReq_TX(i2c);
		}
	}
	return 0;
}

static void dma_finish(const struct device *dev, struct i2c_msg *msg)
{
	const struct i2c_stm32_config *cfg = dev->config;

	if ((msg->flags & I2C_MSG_READ) != 0U) {
		dma_stop(cfg->rx_dma.dev_dma, cfg->rx_dma.dma_channel);
		LL_I2C_DisableDMAReq_RX(cfg->i2c);
		if (!stm32_buf_in_nocache((uintptr_t)msg->buf, msg->len)) {
			sys_cache_data_invd_range(msg->buf, msg->len);
		}
	} else {
		dma_stop(cfg->tx_dma.dev_dma, cfg->tx_dma.dma_channel);
		LL_I2C_DisableDMAReq_TX(cfg->i2c);
	}
}

#endif /* CONFIG_I2C_STM32_V2_DMA */

#ifdef CONFIG_I2C_STM32_INTERRUPT

static void i2c_stm32_disable_transfer_interrupts(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

	LL_I2C_DisableIT_TX(i2c);
	LL_I2C_DisableIT_RX(i2c);
	LL_I2C_DisableIT_STOP(i2c);
	LL_I2C_DisableIT_NACK(i2c);
	LL_I2C_DisableIT_TC(i2c);

	if (!data->smbalert_active) {
		LL_I2C_DisableIT_ERR(i2c);
	}
}

#if defined(CONFIG_I2C_TARGET)
static void i2c_stm32_slave_event(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;
	const struct i2c_target_callbacks *slave_cb;
	struct i2c_target_config *slave_cfg;

	if (data->slave_cfg->flags != I2C_TARGET_FLAGS_ADDR_10_BITS) {
		uint8_t slave_address;

		/* Choose the right slave from the address match code */
		slave_address = LL_I2C_GetAddressMatchCode(i2c) >> 1;
		if (data->slave_cfg != NULL &&
				slave_address == data->slave_cfg->address) {
			slave_cfg = data->slave_cfg;
		} else if (data->slave2_cfg != NULL &&
				slave_address == data->slave2_cfg->address) {
			slave_cfg = data->slave2_cfg;
		} else {
			__ASSERT_NO_MSG(0);
			return;
		}
	} else {
		/* On STM32 the LL_I2C_GetAddressMatchCode & (ISR register) returns
		 * only 7bits of address match so 10 bit dual addressing is broken.
		 * Revert to assuming single address match.
		 */
		if (data->slave_cfg != NULL) {
			slave_cfg = data->slave_cfg;
		} else {
			__ASSERT_NO_MSG(0);
			return;
		}
	}

	slave_cb = slave_cfg->callbacks;

	if (LL_I2C_IsActiveFlag_TXIS(i2c)) {
		uint8_t val = 0x00;

		if (slave_cb->read_processed(slave_cfg, &val) < 0) {
			LOG_ERR("Error continuing reading");
		}

		LL_I2C_TransmitData8(i2c, val);

		return;
	}

	if (LL_I2C_IsActiveFlag_RXNE(i2c)) {
		uint8_t val = LL_I2C_ReceiveData8(i2c);

		if (slave_cb->write_received(slave_cfg, val)) {
			LL_I2C_AcknowledgeNextData(i2c, LL_I2C_NACK);
		}
		return;
	}

	if (LL_I2C_IsActiveFlag_NACK(i2c)) {
		LL_I2C_ClearFlag_NACK(i2c);
	}

	if (LL_I2C_IsActiveFlag_STOP(i2c)) {
		i2c_stm32_disable_transfer_interrupts(dev);

		/* Flush remaining TX byte before clearing Stop Flag */
		LL_I2C_ClearFlag_TXE(i2c);

		LL_I2C_ClearFlag_STOP(i2c);

		slave_cb->stop(slave_cfg);

		/* Prepare to ACK next transmissions address byte */
		LL_I2C_AcknowledgeNextData(i2c, LL_I2C_ACK);
	}

	if (LL_I2C_IsActiveFlag_ADDR(i2c)) {
		uint32_t dir;

		LL_I2C_ClearFlag_ADDR(i2c);

		dir = LL_I2C_GetTransferDirection(i2c);
		if (dir == LL_I2C_DIRECTION_WRITE) {
			if (slave_cb->write_requested(slave_cfg) < 0) {
				LOG_ERR("Error initiating writing");
			} else {
				LL_I2C_EnableIT_RX(i2c);
			}
		} else {
			uint8_t val;

			if (slave_cb->read_requested(slave_cfg, &val) < 0) {
				LOG_ERR("Error initiating reading");
			} else {
				LL_I2C_TransmitData8(i2c, val);
				LL_I2C_EnableIT_TX(i2c);
			}
		}

		LL_I2C_EnableIT_STOP(i2c);
		LL_I2C_EnableIT_NACK(i2c);
		LL_I2C_EnableIT_TC(i2c);
		LL_I2C_EnableIT_ERR(i2c);
	}
}

/* Attach and start I2C as target */
int i2c_stm32_target_register(const struct device *dev,
			      struct i2c_target_config *config)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;
	uint32_t bitrate_cfg;
	int ret;

	if (!config) {
		return -EINVAL;
	}

	if (data->slave_cfg && data->slave2_cfg) {
		return -EBUSY;
	}

	if (data->master_active) {
		return -EBUSY;
	}

	bitrate_cfg = i2c_map_dt_bitrate(cfg->bitrate);

	ret = i2c_stm32_runtime_configure(dev, bitrate_cfg);
	if (ret < 0) {
		LOG_ERR("i2c: failure initializing");
		return ret;
	}

	/* Mark device as active */
	(void)pm_device_runtime_get(dev);

#if !defined(CONFIG_SOC_SERIES_STM32F7X)
	if (pm_device_wakeup_is_capable(dev)) {
		/* Enable wake-up from stop */
		LOG_DBG("i2c: enabling wakeup from stop");
		LL_I2C_EnableWakeUpFromStop(cfg->i2c);
	}
#endif /* CONFIG_SOC_SERIES_STM32F7X */

	LL_I2C_Enable(i2c);

	if (!data->slave_cfg) {
		data->slave_cfg = config;
		if (data->slave_cfg->flags == I2C_TARGET_FLAGS_ADDR_10_BITS)	{
			LL_I2C_SetOwnAddress1(i2c, config->address, LL_I2C_OWNADDRESS1_10BIT);
			LOG_DBG("i2c: target #1 registered with 10-bit address");
		} else {
			LL_I2C_SetOwnAddress1(i2c, config->address << 1U, LL_I2C_OWNADDRESS1_7BIT);
			LOG_DBG("i2c: target #1 registered with 7-bit address");
		}

		LL_I2C_EnableOwnAddress1(i2c);

		LOG_DBG("i2c: target #1 registered");
	} else {
		data->slave2_cfg = config;

		if (data->slave2_cfg->flags == I2C_TARGET_FLAGS_ADDR_10_BITS)	{
			return -EINVAL;
		}
		LL_I2C_SetOwnAddress2(i2c, config->address << 1U,
				      LL_I2C_OWNADDRESS2_NOMASK);
		LL_I2C_EnableOwnAddress2(i2c);
		LOG_DBG("i2c: target #2 registered");
	}

	data->slave_attached = true;

	LL_I2C_EnableIT_ADDR(i2c);

	return 0;
}

int i2c_stm32_target_unregister(const struct device *dev,
				struct i2c_target_config *config)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

	if (!data->slave_attached) {
		return -EINVAL;
	}

	if (data->master_active) {
		return -EBUSY;
	}

	if (config == data->slave_cfg) {
		LL_I2C_DisableOwnAddress1(i2c);
		data->slave_cfg = NULL;

		LOG_DBG("i2c: slave #1 unregistered");
	} else if (config == data->slave2_cfg) {
		LL_I2C_DisableOwnAddress2(i2c);
		data->slave2_cfg = NULL;

		LOG_DBG("i2c: slave #2 unregistered");
	} else {
		return -EINVAL;
	}

	/* Return if there is a slave remaining */
	if (data->slave_cfg || data->slave2_cfg) {
		LOG_DBG("i2c: target#%c still registered", data->slave_cfg?'1':'2');
		return 0;
	}

	/* Otherwise disable I2C */
	LL_I2C_DisableIT_ADDR(i2c);
	i2c_stm32_disable_transfer_interrupts(dev);

	LL_I2C_ClearFlag_NACK(i2c);
	LL_I2C_ClearFlag_STOP(i2c);
	LL_I2C_ClearFlag_ADDR(i2c);

	if (!data->smbalert_active) {
		LL_I2C_Disable(i2c);
	}

#if !defined(CONFIG_SOC_SERIES_STM32F7X)
	if (pm_device_wakeup_is_capable(dev)) {
		/* Disable wake-up from STOP */
		LOG_DBG("i2c: disabling wakeup from stop");
		LL_I2C_DisableWakeUpFromStop(i2c);
	}
#endif /* CONFIG_SOC_SERIES_STM32F7X */

	/* Release the device */
	(void)pm_device_runtime_put(dev);

	data->slave_attached = false;

	return 0;
}
#endif /* defined(CONFIG_I2C_TARGET) */

void i2c_stm32_event(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *regs = cfg->i2c;
	uint32_t isr = stm32_reg_read(&regs->ISR);

#if defined(CONFIG_I2C_TARGET)
	if (data->slave_attached && !data->master_active) {
		i2c_stm32_slave_event(dev);
		return;
	}
#endif

	if ((isr & I2C_ISR_NACKF) != 0U) {
		/* NACK received, a STOP will automatically be sent */
		LL_I2C_ClearFlag_NACK(regs);
		data->current.is_nack = 1U;

	} else if ((isr & I2C_ISR_STOPF) != 0U) {
		/* STOP detected, either caused by automatic STOP after NACK or
		 * by request below in transfer complete
		 */
		/* Acknowledge stop condition */
		LL_I2C_ClearFlag_STOP(regs);
		/* Flush I2C controller TX buffer */
		LL_I2C_ClearFlag_TXE(regs);
		goto irq_xfer_completed;

	} else if ((isr & I2C_ISR_RXNE) != 0U) {
		__ASSERT_NO_MSG(data->current.len != 0U);
		*data->current.buf = LL_I2C_ReceiveData8(regs);
		data->current.len--;
		data->current.buf++;

	} else if ((isr & I2C_ISR_TCR) != 0U) {
		/* Transfer complete with reload flag set means more data shall be transferred
		 * in same direction (No RESTART or STOP)
		 */
		uint32_t cr2 = stm32_reg_read(&regs->CR2);
#ifdef CONFIG_I2C_STM32_V2_DMA
		/* Get number of bytes bytes transferred by DMA */
		uint32_t xfer_len = (cr2 & I2C_CR2_NBYTES_Msk) >> I2C_CR2_NBYTES_Pos;

		data->current.len -= xfer_len;
		data->current.buf += xfer_len;
#endif

		if (data->current.len == 0U) {
			/* In this state all data from current message is transferred
			 * and that reload was used indicates that next message will
			 * contain more data in the same direction
			 * So keep reload turned on and let thread continue with next message
			 */
			goto irq_xfer_completed;
		} else if (data->current.len > 255U) {
			/* More data exceeding I2C controllers maximum single transfer length
			 * remaining in current message
			 * Keep RELOAD mode and set NBYTES to 255 again
			 */
			stm32_reg_write(&regs->CR2, cr2);
		} else {
			/* Data for a single transfer remains in buffer, set its length and
			 * - If more messages follow and transfer direction for next message is
			 *   same, keep reload on
			 * - If direction changes or current message is the last,
			 *   end reload mode and wait for TC
			 */
			cr2 &= ~I2C_CR2_NBYTES_Msk;
			cr2 |= data->current.len << I2C_CR2_NBYTES_Pos;
			/* If no more message data remains to be sent in current direction */
			if (!data->current.continue_in_next) {
				/* Disable reload mode, expect I2C_ISR_TC next */
				cr2 &= ~I2C_CR2_RELOAD;
			}
			stm32_reg_write(&regs->CR2, cr2);
		}

	} else if ((isr & I2C_ISR_TXIS) != 0U) {
		__ASSERT_NO_MSG(data->current.len != 0U);
		LL_I2C_TransmitData8(regs, *data->current.buf);
		data->current.len--;
		data->current.buf++;

	} else if ((isr & I2C_ISR_TC) != 0U) {
		/* Transfer Complete, (I2C_ISR_TC is set) no reload this time so either do
		 * stop now or restart in thread
		 */

		/* Send stop if flag set in message */
		if ((data->current.msg->flags & I2C_MSG_STOP) != 0U) {
			/* Setting STOP here will clear TC, expect I2C_ISR_STOPF next */
			LL_I2C_GenerateStopCondition(regs);
		} else {
			/* Keep TC set and handover to thread for restart */
			goto irq_xfer_completed;
		}
	} else {
		/* Should not happen */
		__ASSERT_NO_MSG(0);
	}

	/* Make a dummy read from ISR to ensure we don't return before
	 * i2c controller had a chance to clear its interrupt flags due
	 * to bus delays
	 */
	(void)LL_I2C_ReadReg(regs, ISR);
	return;

irq_xfer_completed:
	/* Disable IRQ:s involved in data transfer */
	i2c_stm32_disable_transfer_interrupts(dev);
	/* Wakeup thread */
	k_sem_give(&data->device_sync_sem);
}

int i2c_stm32_error(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

#if defined(CONFIG_I2C_TARGET)
	i2c_target_error_cb_t error_cb = NULL;

	if (data->slave_attached && !data->master_active &&
	    data->slave_cfg != NULL && data->slave_cfg->callbacks != NULL) {
		error_cb = data->slave_cfg->callbacks->error;
	}
#endif

	if (LL_I2C_IsActiveFlag_ARLO(i2c)) {
		LL_I2C_ClearFlag_ARLO(i2c);
		data->current.is_arlo = 1U;
#if defined(CONFIG_I2C_TARGET)
		if (error_cb != NULL) {
			error_cb(data->slave_cfg, I2C_ERROR_ARBITRATION);
		}
#endif
		goto end;
	}

	/* Don't end a transaction on bus error in master mode
	 * as errata sheet says that spurious false detections
	 * of BERR can happen which shall be ignored.
	 * If a real Bus Error occurs, transaction will time out.
	 */
	if (LL_I2C_IsActiveFlag_BERR(i2c)) {
		LL_I2C_ClearFlag_BERR(i2c);
		data->current.is_err = 1U;
#if defined(CONFIG_I2C_TARGET)
		if (error_cb != NULL) {
			error_cb(data->slave_cfg, I2C_ERROR_GENERIC);
		}
#endif
		goto end;
	}

#if defined(CONFIG_SMBUS_STM32_SMBALERT)
	if (LL_I2C_IsActiveSMBusFlag_ALERT(i2c)) {
		LL_I2C_ClearSMBusFlag_ALERT(i2c);
		if (data->smbalert_cb_func != NULL) {
			data->smbalert_cb_func(data->smbalert_cb_dev);
		}
		goto end;
	}
#endif

	return 0;
end:
#if defined(CONFIG_I2C_TARGET)
	if (data->slave_attached && !data->master_active) {
		return -EIO;
	}
#endif
	i2c_stm32_disable_transfer_interrupts(dev);
	/* Wakeup thread */
	k_sem_give(&data->device_sync_sem);
	return -EIO;
}

static int stm32_i2c_irq_msg_finish(const struct device *dev, struct i2c_msg *msg)
{
	struct i2c_stm32_data *data = dev->data;
	const struct i2c_stm32_config *cfg = dev->config;
	bool keep_enabled = (msg->flags & I2C_MSG_STOP) == 0U;
	int ret;

	/* Wait for IRQ to complete or timeout */
	ret = k_sem_take(&data->device_sync_sem, K_MSEC(CONFIG_I2C_STM32_TRANSFER_TIMEOUT_MSEC));

#ifdef CONFIG_I2C_STM32_V2_DMA
	/* Stop DMA and invalidate cache if needed */
	dma_finish(dev, msg);
#endif

	/* Check for transfer errors or timeout */
	if (data->current.is_nack || data->current.is_arlo || (ret != 0)) {

		if (data->current.is_arlo) {
			LOG_DBG("ARLO");
		}

		if (data->current.is_nack) {
			LOG_DBG("NACK");
		}

		if (data->current.is_err) {
			LOG_DBG("ERR %d", data->current.is_err);
		}

		if (ret != 0) {
			LOG_DBG("TIMEOUT");
		}
		ret = -EIO;
	}

#if defined(CONFIG_I2C_TARGET)
	if (!keep_enabled || (ret != 0)) {
		data->master_active = false;
	}
	/* Don't disable I2C if a slave is attached */
	if (data->slave_attached) {
		keep_enabled = true;
	}
#endif

	/* Don't disable I2C if SMBus Alert is active */
	if (data->smbalert_active) {
		keep_enabled = true;
	}

	/* If I2C no longer need to be enabled or on error */
	if (!keep_enabled || (ret != 0)) {
		LL_I2C_Disable(cfg->i2c);
	}

	return ret;
}

static int stm32_i2c_irq_xfer(const struct device *dev, struct i2c_msg *msg,
			      uint8_t *next_msg_flags, uint16_t slave)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *regs = cfg->i2c;

	data->current.len = msg->len;
	data->current.buf = msg->buf;
	data->current.is_arlo = 0U;
	data->current.is_nack = 0U;
	data->current.is_err = 0U;
	data->current.msg = msg;

#if defined(CONFIG_I2C_TARGET)
	data->master_active = true;
#endif

#if defined(CONFIG_I2C_STM32_V2_DMA)
	if (!stm32_buf_in_nocache((uintptr_t)msg->buf, msg->len) &&
	    ((msg->flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE)) {
		sys_cache_data_flush_range(msg->buf, msg->len);
	}
#endif /* CONFIG_I2C_STM32_V2_DMA */

	/* Flush TX register */
	LL_I2C_ClearFlag_TXE(regs);

	/* Enable I2C peripheral if not already done */
	LL_I2C_Enable(regs);

	uint32_t cr2 = stm32_reg_read(&regs->CR2);
	uint32_t isr = stm32_reg_read(&regs->ISR);

	/* Clear fields in CR2 which will be filled in later in function */
	cr2 &= ~(I2C_CR2_RELOAD | I2C_CR2_AUTOEND | I2C_CR2_NBYTES_Msk | I2C_CR2_SADD_Msk |
		I2C_CR2_ADD10);

	if ((I2C_ADDR_10_BITS & data->dev_config) != 0U) {
		cr2 |= (uint32_t)slave | I2C_CR2_ADD10;
	} else {
		cr2 |= (uint32_t)slave << 1;
	}

	/* If this is not a stop message and more messages follow without change of direction,
	 * reload mode must be used during this transaction
	 * also a helper variable is set to inform IRQ handler about that it should
	 * keep reload mode turned on ready for next message
	 */
	if (((msg->flags & I2C_MSG_STOP) == 0U) && (next_msg_flags != NULL) &&
	    ((*next_msg_flags & I2C_MSG_RESTART) == 0U)) {
		cr2 |= I2C_CR2_RELOAD;
		data->current.continue_in_next = true;
	} else {
		data->current.continue_in_next = false;
	}

	/* For messages larger than 255 bytes, transactions must be split in chunks
	 * Use reload mode and let IRQ handler take care of jumping to next chunk
	 */
	if (msg->len > 255U) {
		cr2 |= (255U << I2C_CR2_NBYTES_Pos) | I2C_CR2_RELOAD;
	} else {
		/* Whole message can be sent in one I2C HW transaction */
		cr2 |= msg->len << I2C_CR2_NBYTES_Pos;
	}

	/* If a reload mode transfer is pending since last message then skip
	 * checking for transfer complete or restart flag in message
	 * Reload transfer will start right after writing new length
	 * to CR2 below
	 */
	if ((isr & I2C_ISR_TCR) == 0U) {

		/* As TCR is not set, expect TC to be set or that this is a (re)start message
		 * - msg->flags contains I2C_MSG_RESTART (for first start) or
		 * - TC in ISR register is set which happens when IRQ handler
		 *   has finalized its transfer and is waiting for restart
		 * For both cases, a new start condition shall be sent
		 */
		__ASSERT_NO_MSG(((isr & I2C_ISR_TC) != 0U) ||
				((msg->flags & I2C_MSG_RESTART) != 0U));

		if ((msg->flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
			cr2 &= ~I2C_CR2_RD_WRN;
#ifndef CONFIG_I2C_STM32_V2_DMA
			/* Prepare first byte in TX buffer before transfer start as a
			 * workaround for errata: "Transmission stalled after first byte transfer"
			 */
			if (data->current.len > 0U) {
				LL_I2C_TransmitData8(regs, *data->current.buf);
				data->current.len--;
				data->current.buf++;
			}
#endif

		} else {
			cr2 |= I2C_CR2_RD_WRN;
		}
		/* Issue (re)start condition */
		cr2 |= I2C_CR2_START;
	}

	/* Set common interrupt enable bits */
	uint32_t cr1 = I2C_CR1_ERRIE | I2C_CR1_STOPIE | I2C_CR1_TCIE | I2C_CR1_NACKIE;

#ifdef CONFIG_I2C_STM32_V2_DMA
	if (dma_xfer_start(dev, msg) != 0) {
		LL_I2C_Disable(regs);
#if defined(CONFIG_I2C_TARGET)
		data->master_active = false;
#endif
		return -EIO;
	}
#else
	/* If not using DMA, also enable RX and TX empty interrupts */
	cr1 |= I2C_CR1_TXIE | I2C_CR1_RXIE;
#endif /* CONFIG_I2C_STM32_V2_DMA */

	/* Commit configuration to I2C controller and start transfer */
	stm32_reg_write(&regs->CR2, cr2);

	/* Enable interrupts */
	stm32_reg_set_bits(&regs->CR1, cr1);

	/* Wait for transfer to finish */
	return stm32_i2c_irq_msg_finish(dev, msg);
}

#else /* !CONFIG_I2C_STM32_INTERRUPT */
static inline int check_errors(const struct device *dev, const char *funcname)
{
	const struct i2c_stm32_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->i2c;

	if (LL_I2C_IsActiveFlag_NACK(i2c)) {
		LL_I2C_ClearFlag_NACK(i2c);
		LOG_DBG("%s: NACK", funcname);
		goto error;
	}

	if (LL_I2C_IsActiveFlag_ARLO(i2c)) {
		LL_I2C_ClearFlag_ARLO(i2c);
		LOG_DBG("%s: ARLO", funcname);
		goto error;
	}

	if (LL_I2C_IsActiveFlag_OVR(i2c)) {
		LL_I2C_ClearFlag_OVR(i2c);
		LOG_DBG("%s: OVR", funcname);
		goto error;
	}

	if (LL_I2C_IsActiveFlag_BERR(i2c)) {
		LL_I2C_ClearFlag_BERR(i2c);
		LOG_DBG("%s: BERR", funcname);
		goto error;
	}

	return 0;
error:
	if (LL_I2C_IsEnabledReloadMode(i2c)) {
		LL_I2C_DisableReloadMode(i2c);
	}
	return -EIO;
}

static inline void msg_init(const struct device *dev, struct i2c_msg *msg,
			    uint8_t *next_msg_flags, uint16_t slave,
			    uint32_t transfer)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

	if (LL_I2C_IsEnabledReloadMode(i2c)) {
		LL_I2C_SetTransferSize(i2c, msg->len);
	} else {
		if (I2C_ADDR_10_BITS & data->dev_config) {
			LL_I2C_SetMasterAddressingMode(i2c,
					LL_I2C_ADDRESSING_MODE_10BIT);
			LL_I2C_SetSlaveAddr(i2c, (uint32_t) slave);
		} else {
			LL_I2C_SetMasterAddressingMode(i2c,
				LL_I2C_ADDRESSING_MODE_7BIT);
			LL_I2C_SetSlaveAddr(i2c, (uint32_t) slave << 1);
		}

		if (!(msg->flags & I2C_MSG_STOP) && next_msg_flags &&
		    !(*next_msg_flags & I2C_MSG_RESTART)) {
			LL_I2C_EnableReloadMode(i2c);
		} else {
			LL_I2C_DisableReloadMode(i2c);
		}
		LL_I2C_DisableAutoEndMode(i2c);
		LL_I2C_SetTransferRequest(i2c, transfer);
		LL_I2C_SetTransferSize(i2c, msg->len);

#if defined(CONFIG_I2C_TARGET)
		data->master_active = true;
#endif
		LL_I2C_Enable(i2c);

		LL_I2C_GenerateStartCondition(i2c);
	}
}

static inline int msg_done(const struct device *dev,
			   unsigned int current_msg_flags)
{
	const struct i2c_stm32_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->i2c;
	int64_t start_time = k_uptime_get();

	/* Wait for transfer to complete */
	while (!LL_I2C_IsActiveFlag_TC(i2c) && !LL_I2C_IsActiveFlag_TCR(i2c)) {
		if (check_errors(dev, __func__)) {
			return -EIO;
		}
		if ((k_uptime_get() - start_time) >
		    CONFIG_I2C_STM32_TRANSFER_TIMEOUT_MSEC) {
			return -ETIMEDOUT;
		}
	}
	/* Issue stop condition if necessary */
	if (current_msg_flags & I2C_MSG_STOP) {
		LL_I2C_GenerateStopCondition(i2c);
		while (!LL_I2C_IsActiveFlag_STOP(i2c)) {
			if ((k_uptime_get() - start_time) >
			    CONFIG_I2C_STM32_TRANSFER_TIMEOUT_MSEC) {
				return -ETIMEDOUT;
			}
		}

		LL_I2C_ClearFlag_STOP(i2c);
		LL_I2C_DisableReloadMode(i2c);
	}

	return 0;
}

static int i2c_stm32_msg_write(const struct device *dev, struct i2c_msg *msg,
			       uint8_t *next_msg_flags, uint16_t slave)
{
	const struct i2c_stm32_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->i2c;
	unsigned int len = 0U;
	uint8_t *buf = msg->buf;
	int64_t start_time = k_uptime_get();

	msg_init(dev, msg, next_msg_flags, slave, LL_I2C_REQUEST_WRITE);

	len = msg->len;
	while (len) {
		while (1) {
			if (LL_I2C_IsActiveFlag_TXIS(i2c)) {
				break;
			}

			if (check_errors(dev, __func__)) {
				return -EIO;
			}

			if ((k_uptime_get() - start_time) >
			    CONFIG_I2C_STM32_TRANSFER_TIMEOUT_MSEC) {
				return -ETIMEDOUT;
			}
		}

		LL_I2C_TransmitData8(i2c, *buf);
		buf++;
		len--;
	}

	return msg_done(dev, msg->flags);
}

static int i2c_stm32_msg_read(const struct device *dev, struct i2c_msg *msg,
			      uint8_t *next_msg_flags, uint16_t slave)
{
	const struct i2c_stm32_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->i2c;
	unsigned int len = 0U;
	uint8_t *buf = msg->buf;
	int64_t start_time = k_uptime_get();

	msg_init(dev, msg, next_msg_flags, slave, LL_I2C_REQUEST_READ);

	len = msg->len;
	while (len) {
		while (!LL_I2C_IsActiveFlag_RXNE(i2c)) {
			if (check_errors(dev, __func__)) {
				return -EIO;
			}
			if ((k_uptime_get() - start_time) >
			    CONFIG_I2C_STM32_TRANSFER_TIMEOUT_MSEC) {
				return -ETIMEDOUT;
			}
		}

		*buf = LL_I2C_ReceiveData8(i2c);
		buf++;
		len--;
	}

	return msg_done(dev, msg->flags);
}
#endif

#ifdef CONFIG_I2C_STM32_V2_TIMING
/*
 * Macro used to fix the compliance check warning :
 * "DEEP_INDENTATION: Too many leading tabs - consider code refactoring
 * in the i2c_compute_scll_sclh() function below
 */
#define I2C_LOOP_SCLH();						\
	if ((tscl >= clk_min) &&					\
		(tscl <= clk_max) &&					\
		(tscl_h >= i2c_stm32_charac[i2c_speed].hscl_min) &&	\
		(ti2cclk < tscl_h)) {					\
									\
		int32_t error = (int32_t)tscl - (int32_t)ti2cspeed;	\
									\
		if (error < 0) {					\
			error = -error;					\
		}							\
									\
		if ((uint32_t)error < prev_error) {			\
			prev_error = (uint32_t)error;			\
			i2c_valid_timing[count].scll = scll;		\
			i2c_valid_timing[count].sclh = sclh;		\
			ret = count;					\
		}							\
	}

/*
 * @brief  Calculate SCLL and SCLH and find best configuration.
 * @param  clock_src_freq I2C source clock in Hz.
 * @param  i2c_speed I2C frequency (index).
 * @retval config index (0 to I2C_VALID_TIMING_NBR], 0xFFFFFFFF for no valid config.
 */
uint32_t i2c_compute_scll_sclh(uint32_t clock_src_freq, uint32_t i2c_speed)
{
	uint32_t ret = 0xFFFFFFFFU;
	uint32_t ti2cclk;
	uint32_t ti2cspeed;
	uint32_t prev_error;
	uint32_t dnf_delay;
	uint32_t clk_min, clk_max;
	uint32_t scll, sclh;
	uint32_t tafdel_min;

	ti2cclk = (NSEC_PER_SEC + (clock_src_freq / 2U)) / clock_src_freq;
	ti2cspeed = (NSEC_PER_SEC + (i2c_stm32_charac[i2c_speed].freq / 2U)) /
		i2c_stm32_charac[i2c_speed].freq;

	tafdel_min = (I2C_STM32_USE_ANALOG_FILTER == 1U) ?
		I2C_STM32_ANALOG_FILTER_DELAY_MIN :
		0U;

	/* tDNF = DNF x tI2CCLK */
	dnf_delay = i2c_stm32_charac[i2c_speed].dnf * ti2cclk;

	clk_max = NSEC_PER_SEC / i2c_stm32_charac[i2c_speed].freq_min;
	clk_min = NSEC_PER_SEC / i2c_stm32_charac[i2c_speed].freq_max;

	prev_error = ti2cspeed;

	for (uint32_t count = 0; count < I2C_STM32_VALID_TIMING_NBR; count++) {
		/* tPRESC = (PRESC+1) x tI2CCLK*/
		uint32_t tpresc = (i2c_valid_timing[count].presc + 1U) * ti2cclk;

		for (scll = 0; scll < I2C_STM32_SCLL_MAX; scll++) {
			/* tLOW(min) <= tAF(min) + tDNF + 2 x tI2CCLK + [(SCLL+1) x tPRESC ] */
			uint32_t tscl_l = tafdel_min + dnf_delay +
				(2U * ti2cclk) + ((scll + 1U) * tpresc);

			/*
			 * The I2CCLK period tI2CCLK must respect the following conditions:
			 * tI2CCLK < (tLOW - tfilters) / 4 and tI2CCLK < tHIGH
			 */
			if ((tscl_l > i2c_stm32_charac[i2c_speed].lscl_min) &&
				(ti2cclk < ((tscl_l - tafdel_min - dnf_delay) / 4U))) {
				for (sclh = 0; sclh < I2C_STM32_SCLH_MAX; sclh++) {
					/*
					 * tHIGH(min) <= tAF(min) + tDNF +
					 * 2 x tI2CCLK + [(SCLH+1) x tPRESC]
					 */
					uint32_t tscl_h = tafdel_min + dnf_delay +
						(2U * ti2cclk) + ((sclh + 1U) * tpresc);

					/* tSCL = tf + tLOW + tr + tHIGH */
					uint32_t tscl = tscl_l +
						tscl_h + i2c_stm32_charac[i2c_speed].trise +
					i2c_stm32_charac[i2c_speed].tfall;

					/* get timings with the lowest clock error */
					I2C_LOOP_SCLH();
				}
			}
		}
	}

	return ret;
}

/*
 * Macro used to fix the compliance check warning :
 * "DEEP_INDENTATION: Too many leading tabs - consider code refactoring
 * in the i2c_compute_presc_scldel_sdadel() function below
 */
#define I2C_LOOP_SDADEL();								\
											\
	if ((tsdadel >= (uint32_t)tsdadel_min) &&					\
		(tsdadel <= (uint32_t)tsdadel_max)) {					\
		if (presc != prev_presc) {						\
			i2c_valid_timing[i2c_valid_timing_nbr].presc = presc;		\
			i2c_valid_timing[i2c_valid_timing_nbr].tscldel = scldel;	\
			i2c_valid_timing[i2c_valid_timing_nbr].tsdadel = sdadel;	\
			prev_presc = presc;						\
			i2c_valid_timing_nbr++;						\
											\
			if (i2c_valid_timing_nbr >= I2C_STM32_VALID_TIMING_NBR) {	\
				break;							\
			}								\
		}									\
	}

/*
 * @brief  Compute PRESC, SCLDEL and SDADEL.
 * @param  clock_src_freq I2C source clock in Hz.
 * @param  i2c_speed I2C frequency (index).
 * @retval None.
 */
void i2c_compute_presc_scldel_sdadel(uint32_t clock_src_freq, uint32_t i2c_speed)
{
	uint32_t prev_presc = I2C_STM32_PRESC_MAX;
	uint32_t ti2cclk;
	int32_t  tsdadel_min, tsdadel_max;
	int32_t  tscldel_min;
	uint32_t presc, scldel, sdadel;
	uint32_t tafdel_min, tafdel_max;

	ti2cclk   = (NSEC_PER_SEC + (clock_src_freq / 2U)) / clock_src_freq;

	tafdel_min = (I2C_STM32_USE_ANALOG_FILTER == 1U) ?
		I2C_STM32_ANALOG_FILTER_DELAY_MIN : 0U;
	tafdel_max = (I2C_STM32_USE_ANALOG_FILTER == 1U) ?
		I2C_STM32_ANALOG_FILTER_DELAY_MAX : 0U;
	/*
	 * tDNF = DNF x tI2CCLK
	 * tPRESC = (PRESC+1) x tI2CCLK
	 * SDADEL >= {tf +tHD;DAT(min) - tAF(min) - tDNF - [3 x tI2CCLK]} / {tPRESC}
	 * SDADEL <= {tVD;DAT(max) - tr - tAF(max) - tDNF- [4 x tI2CCLK]} / {tPRESC}
	 */
	tsdadel_min = (int32_t)i2c_stm32_charac[i2c_speed].tfall +
		(int32_t)i2c_stm32_charac[i2c_speed].hddat_min -
		(int32_t)tafdel_min -
		(int32_t)(((int32_t)i2c_stm32_charac[i2c_speed].dnf + 3) *
		(int32_t)ti2cclk);

	tsdadel_max = (int32_t)i2c_stm32_charac[i2c_speed].vddat_max -
		(int32_t)i2c_stm32_charac[i2c_speed].trise -
		(int32_t)tafdel_max -
		(int32_t)(((int32_t)i2c_stm32_charac[i2c_speed].dnf + 4) *
		(int32_t)ti2cclk);

	/* {[tr+ tSU;DAT(min)] / [tPRESC]} - 1 <= SCLDEL */
	tscldel_min = (int32_t)i2c_stm32_charac[i2c_speed].trise +
		(int32_t)i2c_stm32_charac[i2c_speed].sudat_min;

	if (tsdadel_min <= 0) {
		tsdadel_min = 0;
	}

	if (tsdadel_max <= 0) {
		tsdadel_max = 0;
	}

	for (presc = 0; presc < I2C_STM32_PRESC_MAX; presc++) {
		for (scldel = 0; scldel < I2C_STM32_SCLDEL_MAX; scldel++) {
			/* TSCLDEL = (SCLDEL+1) * (PRESC+1) * TI2CCLK */
			uint32_t tscldel = (scldel + 1U) * (presc + 1U) * ti2cclk;

			if (tscldel >= (uint32_t)tscldel_min) {
				for (sdadel = 0; sdadel < I2C_STM32_SDADEL_MAX; sdadel++) {
					/* TSDADEL = SDADEL * (PRESC+1) * TI2CCLK */
					uint32_t tsdadel = (sdadel * (presc + 1U)) * ti2cclk;

					I2C_LOOP_SDADEL();
				}

				if (i2c_valid_timing_nbr >= I2C_STM32_VALID_TIMING_NBR) {
					return;
				}
			}
		}
	}
}

int i2c_stm32_configure_timing(const struct device *dev, uint32_t clock)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;
	uint32_t timing = 0U;
	uint32_t idx;
	uint32_t speed = 0U;
	uint32_t i2c_freq = cfg->bitrate;

	/* Reset valid timing count at the beginning of each new computation */
	i2c_valid_timing_nbr = 0;

	if ((clock != 0U) && (i2c_freq != 0U)) {
		for (speed = 0 ; speed <= (uint32_t)I2C_STM32_SPEED_FREQ_FAST_PLUS ; speed++) {
			if ((i2c_freq >= i2c_stm32_charac[speed].freq_min) &&
				(i2c_freq <= i2c_stm32_charac[speed].freq_max)) {
				i2c_compute_presc_scldel_sdadel(clock, speed);
				idx = i2c_compute_scll_sclh(clock, speed);
				if (idx < I2C_STM32_VALID_TIMING_NBR) {
					timing = ((i2c_valid_timing[idx].presc  &
						0x0FU) << 28) |
					((i2c_valid_timing[idx].tscldel & 0x0FU) << 20) |
					((i2c_valid_timing[idx].tsdadel & 0x0FU) << 16) |
					((i2c_valid_timing[idx].sclh & 0xFFU) << 8) |
					((i2c_valid_timing[idx].scll & 0xFFU) << 0);
				}
				break;
			}
		}
	}

	/* Fill the current timing value in data structure at runtime */
	data->current_timing.periph_clock = clock;
	data->current_timing.i2c_speed = i2c_freq;
	data->current_timing.timing_setting = timing;

	LL_I2C_SetTiming(i2c, timing);

	return 0;
}
#else/* CONFIG_I2C_STM32_V2_TIMING */

int i2c_stm32_configure_timing(const struct device *dev, uint32_t clock)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;
	uint32_t i2c_hold_time_min, i2c_setup_time_min;
	uint32_t i2c_h_min_time, i2c_l_min_time;
	uint32_t presc = 1U;
	uint32_t timing = 0U;

	/*  Look for an adequate preset timing value */
	for (uint32_t i = 0; i < cfg->n_timings; i++) {
		const struct i2c_config_timing *preset = &cfg->timings[i];
		uint32_t speed = i2c_map_dt_bitrate(preset->i2c_speed);

		if ((I2C_SPEED_GET(speed) == I2C_SPEED_GET(data->dev_config))
		   && (preset->periph_clock == clock)) {
			/*  Found a matching periph clock and i2c speed */
			LL_I2C_SetTiming(i2c, preset->timing_setting);
			return 0;
		}
	}

	/* No preset timing was provided, let's dynamically configure */
	switch (I2C_SPEED_GET(data->dev_config)) {
	case I2C_SPEED_STANDARD:
		i2c_h_min_time = 4000U;
		i2c_l_min_time = 4700U;
		i2c_hold_time_min = 500U;
		i2c_setup_time_min = 1250U;
		break;
	case I2C_SPEED_FAST:
		i2c_h_min_time = 600U;
		i2c_l_min_time = 1300U;
		i2c_hold_time_min = 375U;
		i2c_setup_time_min = 500U;
		break;
	default:
		LOG_ERR("i2c: speed above \"fast\" requires manual timing configuration, "
			"see \"timings\" property of st,stm32-i2c-v2 devicetree binding");
		return -EINVAL;
	}

	/* Calculate period until prescaler matches */
	do {
		uint32_t t_presc = clock / presc;
		uint32_t ns_presc = NSEC_PER_SEC / t_presc;
		uint32_t sclh = i2c_h_min_time / ns_presc;
		uint32_t scll = i2c_l_min_time / ns_presc;
		uint32_t sdadel = i2c_hold_time_min / ns_presc;
		uint32_t scldel = i2c_setup_time_min / ns_presc;

		if ((sclh - 1) > 255 ||  (scll - 1) > 255) {
			++presc;
			continue;
		}

		if (sdadel > 15 || (scldel - 1) > 15) {
			++presc;
			continue;
		}

		timing = __LL_I2C_CONVERT_TIMINGS(presc - 1,
					scldel - 1, sdadel, sclh - 1, scll - 1);
		break;
	} while (presc < 16);

	if (presc >= 16U) {
		LOG_ERR("I2C:failed to find prescaler value");
		return -EINVAL;
	}

	LOG_DBG("I2C TIMING = 0x%x", timing);
	LL_I2C_SetTiming(i2c, timing);

	return 0;
}
#endif /* CONFIG_I2C_STM32_V2_TIMING */

int i2c_stm32_transaction(const struct device *dev,
			  struct i2c_msg msg, uint8_t *next_msg_flags,
			  uint16_t periph)
{
	int ret = 0;

#ifdef CONFIG_I2C_STM32_INTERRUPT
	ret = stm32_i2c_irq_xfer(dev, &msg, next_msg_flags, periph);
#else
	/*
	 * Perform a I2C transaction, while taking into account the STM32 I2C V2
	 * peripheral has a limited maximum chunk size. Take appropriate action
	 * if the message to send exceeds that limit.
	 *
	 * The last chunk of a transmission uses this function's next_msg_flags
	 * parameter for its backend calls (_write/_read). Any previous chunks
	 * use a copy of the current message's flags, with the STOP and RESTART
	 * bits turned off. This will cause the backend to use reload-mode,
	 * which will make the combination of all chunks to look like one big
	 * transaction on the wire.
	 */
	struct i2c_stm32_data *data = dev->data;
	const struct i2c_stm32_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->i2c;
	const uint32_t i2c_stm32_maxchunk = 255U;
	const uint8_t saved_flags = msg.flags;
	uint8_t combine_flags =
		saved_flags & ~(I2C_MSG_STOP | I2C_MSG_RESTART);
	uint8_t *flagsp = NULL;
	uint32_t rest = msg.len;

	do { /* do ... while to allow zero-length transactions */
		if (msg.len > i2c_stm32_maxchunk) {
			msg.len = i2c_stm32_maxchunk;
			msg.flags &= ~I2C_MSG_STOP;
			flagsp = &combine_flags;
		} else {
			msg.flags = saved_flags;
			flagsp = next_msg_flags;
		}
		if ((msg.flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
			ret = i2c_stm32_msg_write(dev, &msg, flagsp, periph);
		} else {
			ret = i2c_stm32_msg_read(dev, &msg, flagsp, periph);
		}
		if (ret < 0) {
			break;
		}
		rest -= msg.len;
		msg.buf += msg.len;
		msg.len = rest;
	} while (rest > 0U);

	if (ret == -ETIMEDOUT) {
		if (LL_I2C_IsEnabledReloadMode(i2c)) {
			LL_I2C_DisableReloadMode(i2c);
		}
#if defined(CONFIG_I2C_TARGET)
		data->master_active = false;
		if (!data->slave_attached && !data->smbalert_active) {
			LL_I2C_Disable(i2c);
		}
#else
		if (!data->smbalert_active) {
			LL_I2C_Disable(i2c);
		}
#endif
		return -EIO;
	}
#endif /* CONFIG_I2C_STM32_INTERRUPT */

	return ret;
}
