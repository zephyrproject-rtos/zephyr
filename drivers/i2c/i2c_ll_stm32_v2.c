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
#include <stm32_ll_i2c.h>
#include <errno.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include "i2c_ll_stm32.h"

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_ll_stm32_v2);

#include "i2c-priv.h"

/* Temporary macro to log any unexpected conditions during test
 * Remove this before merge
 */
#define stm32_i2c_assert(x)                                                  \
	do {                                                                     \
		if (!(x))                                                            \
			LOG_ERR("%s: assert failure in line: %d\n", __func__, __LINE__); \
	} while (0)

#ifdef CONFIG_I2C_STM32_V2_TIMING
/* Use the algorithm to calcuate the I2C timing */
#ifndef STM32_I2C_VALID_TIMING_NBR
#define STM32_I2C_VALID_TIMING_NBR 128U
#endif
#define STM32_I2C_SPEED_FREQ_STANDARD     0U   /* 100 kHz */
#define STM32_I2C_SPEED_FREQ_FAST         1U   /* 400 kHz */
#define STM32_I2C_SPEED_FREQ_FAST_PLUS    2U   /* 1 MHz */
#define STM32_I2C_ANALOG_FILTER_DELAY_MIN 50U  /* ns */
#define STM32_I2C_ANALOG_FILTER_DELAY_MAX 260U /* ns */
#define STM32_I2C_USE_ANALOG_FILTER       1U
#define STM32_I2C_DIGITAL_FILTER_COEF     0U
#define STM32_I2C_PRESC_MAX               16U
#define STM32_I2C_SCLDEL_MAX              16U
#define STM32_I2C_SDADEL_MAX              16U
#define STM32_I2C_SCLH_MAX                256U
#define STM32_I2C_SCLL_MAX                256U

/* I2C_DEVICE_Private_Types */
struct stm32_i2c_charac_t {
	uint32_t freq;      /* Frequency in Hz */
	uint32_t freq_min;  /* Minimum frequency in Hz */
	uint32_t freq_max;  /* Maximum frequency in Hz */
	uint32_t hddat_min; /* Minimum data hold time in ns */
	uint32_t vddat_max; /* Maximum data valid time in ns */
	uint32_t sudat_min; /* Minimum data setup time in ns */
	uint32_t lscl_min;  /* Minimum low period of the SCL clock in ns */
	uint32_t hscl_min;  /* Minimum high period of SCL clock in ns */
	uint32_t trise;     /* Rise time in ns */
	uint32_t tfall;     /* Fall time in ns */
	uint32_t dnf;       /* Digital noise filter coefficient */
};

struct stm32_i2c_timings_t {
	uint32_t presc;   /* Timing prescaler */
	uint32_t tscldel; /* SCL delay */
	uint32_t tsdadel; /* SDA delay */
	uint32_t sclh;    /* SCL high period */
	uint32_t scll;    /* SCL low period */
};

/* I2C_DEVICE Private Constants */
static const struct stm32_i2c_charac_t stm32_i2c_charac[] = {
	[STM32_I2C_SPEED_FREQ_STANDARD] =
		{
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
			.dnf = STM32_I2C_DIGITAL_FILTER_COEF,
		},
	[STM32_I2C_SPEED_FREQ_FAST] =
		{
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
			.dnf = STM32_I2C_DIGITAL_FILTER_COEF,
		},
	[STM32_I2C_SPEED_FREQ_FAST_PLUS] =
		{
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
			.dnf = STM32_I2C_DIGITAL_FILTER_COEF,
		},
};

static struct stm32_i2c_timings_t i2c_valid_timing[STM32_I2C_VALID_TIMING_NBR];
static uint32_t i2c_valid_timing_nbr;
#endif /* CONFIG_I2C_STM32_V2_TIMING */

#if !defined(CONFIG_I2C_STM32_INTERRUPT)
static inline void msg_init(const struct device *dev, struct i2c_msg *msg, uint8_t *next_msg_flags,
			    uint16_t slave, uint32_t transfer)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

	if (LL_I2C_IsEnabledReloadMode(i2c)) {
		LL_I2C_SetTransferSize(i2c, msg->len);
	} else {
		if (I2C_ADDR_10_BITS & data->dev_config) {
			LL_I2C_SetMasterAddressingMode(i2c, LL_I2C_ADDRESSING_MODE_10BIT);
			LL_I2C_SetSlaveAddr(i2c, (uint32_t)slave);
		} else {
			LL_I2C_SetMasterAddressingMode(i2c, LL_I2C_ADDRESSING_MODE_7BIT);
			LL_I2C_SetSlaveAddr(i2c, (uint32_t)slave << 1);
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
#endif /* defined (CONFIG_I2C_STM32_INTERRUPT)*/

#ifdef CONFIG_I2C_STM32_INTERRUPT

static void stm32_i2c_disable_transfer_interrupts(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;
	uint32_t flags =
		I2C_CR1_TXIE | I2C_CR1_RXIE | I2C_CR1_STOPIE | I2C_CR1_NACKIE | I2C_CR1_TCIE;
	if (!data->smbalert_active) {
		flags |= I2C_CR1_ERRIE;
	}
	i2c->CR1 &= ~flags;
}

static void stm32_i2c_master_mode_end(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

	stm32_i2c_disable_transfer_interrupts(dev);

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
	k_sem_give(&data->device_sync_sem);
}

#if defined(CONFIG_I2C_TARGET)
static void stm32_i2c_slave_event(const struct device *dev)
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
		if (data->slave_cfg != NULL && slave_address == data->slave_cfg->address) {
			slave_cfg = data->slave_cfg;
		} else if (data->slave2_cfg != NULL && slave_address == data->slave2_cfg->address) {
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
		uint8_t val;

		if (slave_cb->read_processed(slave_cfg, &val) < 0) {
			LOG_ERR("Error continuing reading");
		} else {
			LL_I2C_TransmitData8(i2c, val);
		}
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
		stm32_i2c_disable_transfer_interrupts(dev);

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
		/* Disable transfer interrupts */
		LL_I2C_EnableIT_STOP(i2c);
		LL_I2C_EnableIT_NACK(i2c);
		LL_I2C_EnableIT_TC(i2c);
		LL_I2C_EnableIT_ERR(i2c);
	}
}

/* Attach and start I2C as target */
int i2c_stm32_target_register(const struct device *dev, struct i2c_target_config *config)
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

#if defined(CONFIG_PM_DEVICE_RUNTIME)
	if (pm_device_wakeup_is_capable(dev)) {
		/* Mark device as active */
		(void)pm_device_runtime_get(dev);
		/* Enable wake-up from stop */
		LOG_DBG("i2c: enabling wakeup from stop");
		LL_I2C_EnableWakeUpFromStop(cfg->i2c);
	}
#endif /* defined(CONFIG_PM_DEVICE_RUNTIME) */

	LL_I2C_Enable(i2c);

	if (!data->slave_cfg) {
		data->slave_cfg = config;
		if (data->slave_cfg->flags == I2C_TARGET_FLAGS_ADDR_10_BITS) {
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

		if (data->slave2_cfg->flags == I2C_TARGET_FLAGS_ADDR_10_BITS) {
			return -EINVAL;
		}
		LL_I2C_SetOwnAddress2(i2c, config->address << 1U, LL_I2C_OWNADDRESS2_NOMASK);
		LL_I2C_EnableOwnAddress2(i2c);
		LOG_DBG("i2c: target #2 registered");
	}

	data->slave_attached = true;

	LL_I2C_EnableIT_ADDR(i2c);

	return 0;
}

int i2c_stm32_target_unregister(const struct device *dev, struct i2c_target_config *config)
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
		LOG_DBG("i2c: target#%c still registered", data->slave_cfg ? '1' : '2');
		return 0;
	}

	/* Otherwise disable I2C */
	LL_I2C_DisableIT_ADDR(i2c);
	stm32_i2c_disable_transfer_interrupts(dev);

	LL_I2C_ClearFlag_NACK(i2c);
	LL_I2C_ClearFlag_STOP(i2c);
	LL_I2C_ClearFlag_ADDR(i2c);

	if (!data->smbalert_active) {
		LL_I2C_Disable(i2c);
	}

#if defined(CONFIG_PM_DEVICE_RUNTIME)
	if (pm_device_wakeup_is_capable(dev)) {
		/* Disable wake-up from STOP */
		LOG_DBG("i2c: disabling wakeup from stop");
		LL_I2C_DisableWakeUpFromStop(i2c);
		/* Release the device */
		(void)pm_device_runtime_put(dev);
	}
#endif /* defined(CONFIG_PM_DEVICE_RUNTIME) */

	data->slave_attached = false;

	return 0;
}

#endif /* defined(CONFIG_I2C_TARGET) */

static void stm32_i2c_event(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *reg = cfg->i2c;
	uint32_t isr = reg->ISR;

#if defined(CONFIG_I2C_TARGET)
	if (data->slave_attached && !data->master_active) {
		stm32_i2c_slave_event(dev);
		return;
	}
#endif
	/* NACK received, a STOP will automatically be sent */
	if (isr & I2C_ISR_NACKF) {
		reg->ICR = I2C_ICR_NACKCF;
		data->current.is_nack = 1U;

	} else if (isr & I2C_ISR_STOPF) {
		/*
		 * STOP detected, either caused by automatic STOP after NACK or
		 * by request below in transfer complete
		 */
		/* Acknowledge stop condition */
		reg->ICR = I2C_ICR_STOPCF;
		/* Flush I2C controller TX buffer */
		reg->ISR = I2C_ISR_TXE;
		goto irq_done;

	} else if (isr & I2C_ISR_RXNE) {
		stm32_i2c_assert(data->current.len > 0);
		*data->current.buf = reg->RXDR;
		data->current.len--;
		data->current.buf++;

	} else if (isr & I2C_ISR_TCR) {
		/*
		 * Transfer complete with reload flag set means more data shall be transferred
		 * in same direction (No RESTART or STOP)
		 */
		uint32_t cr2 = reg->CR2;

		stm32_i2c_assert((isr & I2C_ISR_TC) == 0);
		if (data->current.len == 0) {
			/* In this state all data from current message is transferred
			 * and that reload was used indicates that next message will
			 * contain more data in the same direction
			 * So keep reload turned on and let thread continue with next message
			 */
			goto irq_done;
		} else if (data->current.len > 255) {
			/* More data exceeding I2C controller maximum single transfer length
			 * remaining in current message
			 * Keep RELOAD mode and set NBYTES to 255 again
			 */
			reg->CR2 = cr2;
		} else {
			/*
			 * Data for a single transfer remains in buffer, set its length and
			 * - If more messages follow and transfer direction for next message is
			 *   same, keep reload on
			 * - If direction change or current message is the last,
			 *   end reload mode and wait for TC
			 */
			cr2 &= ~I2C_CR2_NBYTES_Msk;
			cr2 |= (data->current.len << I2C_CR2_NBYTES_Pos);
			/* If no more message data remains to be sent in current direction */
			if (data->current.continue_in_next == 0) {
				/* Disable reload mode, expect I2C_ISR_TC next */
				cr2 &= ~I2C_CR2_RELOAD;
			}
			/*
			 * If reload was not disabled above, expect to arrive in this if/else
			 * statement with data->current.len == 0
			 */
			reg->CR2 = cr2;
		}
		/* Request for transmit data */
	} else if (isr & I2C_ISR_TXIS) {
		stm32_i2c_assert(data->current.len > 0);
		reg->TXDR = *data->current.buf;
		data->current.len--;
		data->current.buf++;

		/* Transfer Complete, no reload this time so either do stop now or restart
		 * in thread
		 */
	} else if (isr & I2C_ISR_TC) {

		/* Send stop if flag set in message */
		if (data->current.msg->flags & I2C_MSG_STOP) {
			/* Setting STOP here will clear TC, expect I2C_ICR_STOPCF next */
			LL_I2C_GenerateStopCondition(reg);
		} else {
			/* Keep TC set and handover to thread for restart */
			goto irq_done;
		}
	}
	return;

irq_done:
	/* Disable IRQ:s involved in data transfer */
	stm32_i2c_disable_transfer_interrupts(dev);
	/* Wakeup thread */
	k_sem_give(&data->device_sync_sem);
}

static int stm32_i2c_error(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

#if defined(CONFIG_I2C_TARGET)
	if (data->slave_attached && !data->master_active) {
		/* No need for a slave error function right now. */
		return 0;
	}
#endif

	if (LL_I2C_IsActiveFlag_ARLO(i2c)) {
		LL_I2C_ClearFlag_ARLO(i2c);
		data->current.is_arlo = 1U;
		goto end;
	}

	if (LL_I2C_IsActiveFlag_BERR(i2c)) {
		LL_I2C_ClearFlag_BERR(i2c);
		data->current.is_err = 1U;
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
	stm32_i2c_master_mode_end(dev);
	return -EIO;
}

#ifdef CONFIG_I2C_STM32_COMBINED_INTERRUPT
void stm32_i2c_combined_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;

	if (stm32_i2c_error(dev)) {
		return;
	}
	stm32_i2c_event(dev);
}
#else

void stm32_i2c_event_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;

	stm32_i2c_event(dev);
}

void stm32_i2c_error_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;

	stm32_i2c_error(dev);
}
#endif

static int stm32_i2c_irq_xfer(const struct device *dev, struct i2c_msg *msg,
			      uint8_t *next_msg_flags, uint16_t slave)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *reg = cfg->i2c;
	bool is_timeout = false;

	data->current.len = msg->len;
	data->current.buf = msg->buf;
	data->current.is_arlo = 0U;
	data->current.is_nack = 0U;
	data->current.is_err = 0U;
	data->current.msg = msg;

#if defined(CONFIG_I2C_TARGET)
	data->master_active = true;
#endif
	/* Flush TX register */
	reg->ISR = I2C_ISR_TXE;

	/* Enable I2C peripheral if not already done */
	reg->CR1 |= I2C_CR1_PE;

	uint32_t cr2 = reg->CR2;
	uint32_t isr = reg->ISR;

	stm32_i2c_assert((isr & I2C_ISR_RXNE) == 0);
	stm32_i2c_assert((isr & I2C_ISR_STOPF) == 0);
	stm32_i2c_assert((isr & I2C_ISR_TXIS) == 0);

	/* Clear fields in CR2 which will be filled in later in function */
	cr2 &= ~(I2C_CR2_RELOAD | I2C_CR2_AUTOEND | I2C_CR2_NBYTES_Msk | I2C_CR2_SADD_Msk);

	if (I2C_ADDR_10_BITS & data->dev_config) {
		cr2 |= (uint32_t)slave | I2C_CR2_ADD10;
	} else {
		cr2 |= (uint32_t)slave << 1;
	}

	/*
	 * If this is not a stop message and more messages follow without change of direction,
	 * reload mode must be used during this transaction
	 * also a helper variable is set to inform IRQ handler about that it should
	 * keep reload mode turned on ready for next message
	 */
	if (!(msg->flags & I2C_MSG_STOP) && next_msg_flags &&
	    !(*next_msg_flags & I2C_MSG_RESTART)) {
		cr2 |= I2C_CR2_RELOAD;
		data->current.continue_in_next = 1;
	} else {
		data->current.continue_in_next = 0;
	}

	/*
	 * For messages larger than 255 bytes, transactions must be split i chunks
	 * Use reload mode and let IRQ handler take care of jumping to next chunk
	 */
	if (msg->len > 255) {
		cr2 |= (255ul << I2C_CR2_NBYTES_Pos) | I2C_CR2_RELOAD;
	} else {
		/* Whole message can be sent in one I2C HW transaction */
		cr2 |= msg->len << I2C_CR2_NBYTES_Pos;
	}

	/*
	 * If reload mode transfer is pending since last message
	 * transfer will start right after writing new length to CR2
	 */
	if (isr & I2C_ISR_TCR) {
		/* Transfer complete should not be set */
		stm32_i2c_assert((isr & I2C_ISR_TC) == 0);
		/* Set new length and continue transfer */
		reg->CR2 = cr2;

		/*
		 * A start condition shall be sent if
		 * - msg->flags contains I2C_MSG_RESTART (for first start) or
		 * - TC in ISR register is set, happens when IRQ handler
		 *   has finalized its transfer and is waiting for restart
		 */
	} else if ((isr & I2C_ISR_TC) || (msg->flags & I2C_MSG_RESTART)) {

		if ((msg->flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
			cr2 &= ~I2C_CR2_RD_WRN;
			/* Prepare first byte in TX buffer before transfer start as a
			 * workaround for errata: "Transmission stalled after first byte transfer"
			 */
			stm32_i2c_assert(data->current.len > 0);
			stm32_i2c_assert(isr & I2C_ISR_TXE);
			reg->TXDR = *data->current.buf;
			data->current.len--;
			data->current.buf++;

		} else {
			cr2 |= I2C_CR2_RD_WRN;
		}
		/* Issue start condition */
		cr2 |= I2C_CR2_START;
	}
	/* Commit to I2C controller */
	reg->CR2 = cr2;

	/* Enable interrupts */
	reg->CR1 |= I2C_CR1_TXIE | I2C_CR1_RXIE | I2C_CR1_ERRIE | I2C_CR1_STOPIE | I2C_CR1_TCIE |
		    I2C_CR1_NACKIE;

	/* Wait for IRQ to complete or timeout
	 * Timeout scales with one millisecond for each byte to
	 * transfer so that slave can do some clock stretching
	 */
	if (k_sem_take(&data->device_sync_sem, K_MSEC(msg->len + 10)) != 0) {
		is_timeout = true;
	}
	/* Check for transfer errors or timeout */
	if (data->current.is_nack || data->current.is_arlo || is_timeout) {
		LL_I2C_Disable(reg);
		goto error;

	} else if (msg->flags & I2C_MSG_STOP) {
		/* Disable I2C if this was last message and SMBus alert is not active */
#if defined(CONFIG_I2C_TARGET)
		data->master_active = false;
		if (!data->slave_attached && !data->smbalert_active) {
			LL_I2C_Disable(reg);
		}
#else
		if (!data->smbalert_active) {
			LL_I2C_Disable(reg);
		}
#endif
	}

	return 0;
error:
	if (data->current.is_arlo) {
		LOG_DBG("%s: ARLO %d", __func__, data->current.is_arlo);
		data->current.is_arlo = 0U;
	}

	if (data->current.is_nack) {
		LOG_DBG("%s: NACK", __func__);
		data->current.is_nack = 0U;
	}

	if (data->current.is_err) {
		LOG_DBG("%s: ERR %d", __func__, data->current.is_err);
		data->current.is_err = 0U;
	}

	if (is_timeout) {
		LOG_DBG("%s: TIMEOUT", __func__);
	}

	return -EIO;
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

static inline int msg_done(const struct device *dev, unsigned int current_msg_flags)
{
	const struct i2c_stm32_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->i2c;

	/* Wait for transfer to complete */
	while (!LL_I2C_IsActiveFlag_TC(i2c) && !LL_I2C_IsActiveFlag_TCR(i2c)) {
		if (check_errors(dev, __func__)) {
			return -EIO;
		}
	}
	/* Issue stop condition if necessary */
	if (current_msg_flags & I2C_MSG_STOP) {
		LL_I2C_GenerateStopCondition(i2c);
		while (!LL_I2C_IsActiveFlag_STOP(i2c)) {
		}

		LL_I2C_ClearFlag_STOP(i2c);
		LL_I2C_DisableReloadMode(i2c);
	}

	return 0;
}

static int stm32_i2c_msg_write(const struct device *dev, struct i2c_msg *msg,
			       uint8_t *next_msg_flags, uint16_t slave)
{
	const struct i2c_stm32_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->i2c;
	unsigned int len = 0U;
	uint8_t *buf = msg->buf;

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
		}

		LL_I2C_TransmitData8(i2c, *buf);
		buf++;
		len--;
	}

	return msg_done(dev, msg->flags);
}

static int stm32_i2c_msg_read(const struct device *dev, struct i2c_msg *msg,
			      uint8_t *next_msg_flags, uint16_t slave)
{
	const struct i2c_stm32_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->i2c;
	unsigned int len = 0U;
	uint8_t *buf = msg->buf;

	msg_init(dev, msg, next_msg_flags, slave, LL_I2C_REQUEST_READ);

	len = msg->len;
	while (len) {
		while (!LL_I2C_IsActiveFlag_RXNE(i2c)) {
			if (check_errors(dev, __func__)) {
				return -EIO;
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
#define I2C_LOOP_SCLH()                                                                            \
	;                                                                                          \
	if ((tscl >= clk_min) && (tscl <= clk_max) &&                                              \
	    (tscl_h >= stm32_i2c_charac[i2c_speed].hscl_min) && (ti2cclk < tscl_h)) {              \
                                                                                                   \
		int32_t error = (int32_t)tscl - (int32_t)ti2cspeed;                                \
                                                                                                   \
		if (error < 0) {                                                                   \
			error = -error;                                                            \
		}                                                                                  \
                                                                                                   \
		if ((uint32_t)error < prev_error) {                                                \
			prev_error = (uint32_t)error;                                              \
			i2c_valid_timing[count].scll = scll;                                       \
			i2c_valid_timing[count].sclh = sclh;                                       \
			ret = count;                                                               \
		}                                                                                  \
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
	ti2cspeed = (NSEC_PER_SEC + (stm32_i2c_charac[i2c_speed].freq / 2U)) /
		    stm32_i2c_charac[i2c_speed].freq;

	tafdel_min = (STM32_I2C_USE_ANALOG_FILTER == 1U) ? STM32_I2C_ANALOG_FILTER_DELAY_MIN : 0U;

	/* tDNF = DNF x tI2CCLK */
	dnf_delay = stm32_i2c_charac[i2c_speed].dnf * ti2cclk;

	clk_max = NSEC_PER_SEC / stm32_i2c_charac[i2c_speed].freq_min;
	clk_min = NSEC_PER_SEC / stm32_i2c_charac[i2c_speed].freq_max;

	prev_error = ti2cspeed;

	for (uint32_t count = 0; count < STM32_I2C_VALID_TIMING_NBR; count++) {
		/* tPRESC = (PRESC+1) x tI2CCLK*/
		uint32_t tpresc = (i2c_valid_timing[count].presc + 1U) * ti2cclk;

		for (scll = 0; scll < STM32_I2C_SCLL_MAX; scll++) {
			/* tLOW(min) <= tAF(min) + tDNF + 2 x tI2CCLK + [(SCLL+1) x tPRESC ] */
			uint32_t tscl_l =
				tafdel_min + dnf_delay + (2U * ti2cclk) + ((scll + 1U) * tpresc);

			/*
			 * The I2CCLK period tI2CCLK must respect the following conditions:
			 * tI2CCLK < (tLOW - tfilters) / 4 and tI2CCLK < tHIGH
			 */
			if ((tscl_l > stm32_i2c_charac[i2c_speed].lscl_min) &&
			    (ti2cclk < ((tscl_l - tafdel_min - dnf_delay) / 4U))) {
				for (sclh = 0; sclh < STM32_I2C_SCLH_MAX; sclh++) {
					/*
					 * tHIGH(min) <= tAF(min) + tDNF +
					 * 2 x tI2CCLK + [(SCLH+1) x tPRESC]
					 */
					uint32_t tscl_h = tafdel_min + dnf_delay + (2U * ti2cclk) +
							  ((sclh + 1U) * tpresc);

					/* tSCL = tf + tLOW + tr + tHIGH */
					uint32_t tscl = tscl_l + tscl_h +
							stm32_i2c_charac[i2c_speed].trise +
							stm32_i2c_charac[i2c_speed].tfall;

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
#define I2C_LOOP_SDADEL()                                                                          \
	;                                                                                          \
                                                                                                   \
	if ((tsdadel >= (uint32_t)tsdadel_min) && (tsdadel <= (uint32_t)tsdadel_max)) {            \
		if (presc != prev_presc) {                                                         \
			i2c_valid_timing[i2c_valid_timing_nbr].presc = presc;                      \
			i2c_valid_timing[i2c_valid_timing_nbr].tscldel = scldel;                   \
			i2c_valid_timing[i2c_valid_timing_nbr].tsdadel = sdadel;                   \
			prev_presc = presc;                                                        \
			i2c_valid_timing_nbr++;                                                    \
                                                                                                   \
			if (i2c_valid_timing_nbr >= STM32_I2C_VALID_TIMING_NBR) {                  \
				break;                                                             \
			}                                                                          \
		}                                                                                  \
	}

/*
 * @brief  Compute PRESC, SCLDEL and SDADEL.
 * @param  clock_src_freq I2C source clock in Hz.
 * @param  i2c_speed I2C frequency (index).
 * @retval None.
 */
void i2c_compute_presc_scldel_sdadel(uint32_t clock_src_freq, uint32_t i2c_speed)
{
	uint32_t prev_presc = STM32_I2C_PRESC_MAX;
	uint32_t ti2cclk;
	int32_t tsdadel_min, tsdadel_max;
	int32_t tscldel_min;
	uint32_t presc, scldel, sdadel;
	uint32_t tafdel_min, tafdel_max;

	ti2cclk = (NSEC_PER_SEC + (clock_src_freq / 2U)) / clock_src_freq;

	tafdel_min = (STM32_I2C_USE_ANALOG_FILTER == 1U) ? STM32_I2C_ANALOG_FILTER_DELAY_MIN : 0U;
	tafdel_max = (STM32_I2C_USE_ANALOG_FILTER == 1U) ? STM32_I2C_ANALOG_FILTER_DELAY_MAX : 0U;
	/*
	 * tDNF = DNF x tI2CCLK
	 * tPRESC = (PRESC+1) x tI2CCLK
	 * SDADEL >= {tf +tHD;DAT(min) - tAF(min) - tDNF - [3 x tI2CCLK]} / {tPRESC}
	 * SDADEL <= {tVD;DAT(max) - tr - tAF(max) - tDNF- [4 x tI2CCLK]} / {tPRESC}
	 */
	tsdadel_min = (int32_t)stm32_i2c_charac[i2c_speed].tfall +
		      (int32_t)stm32_i2c_charac[i2c_speed].hddat_min - (int32_t)tafdel_min -
		      (int32_t)(((int32_t)stm32_i2c_charac[i2c_speed].dnf + 3) * (int32_t)ti2cclk);

	tsdadel_max = (int32_t)stm32_i2c_charac[i2c_speed].vddat_max -
		      (int32_t)stm32_i2c_charac[i2c_speed].trise - (int32_t)tafdel_max -
		      (int32_t)(((int32_t)stm32_i2c_charac[i2c_speed].dnf + 4) * (int32_t)ti2cclk);

	/* {[tr+ tSU;DAT(min)] / [tPRESC]} - 1 <= SCLDEL */
	tscldel_min = (int32_t)stm32_i2c_charac[i2c_speed].trise +
		      (int32_t)stm32_i2c_charac[i2c_speed].sudat_min;

	if (tsdadel_min <= 0) {
		tsdadel_min = 0;
	}

	if (tsdadel_max <= 0) {
		tsdadel_max = 0;
	}

	for (presc = 0; presc < STM32_I2C_PRESC_MAX; presc++) {
		for (scldel = 0; scldel < STM32_I2C_SCLDEL_MAX; scldel++) {
			/* TSCLDEL = (SCLDEL+1) * (PRESC+1) * TI2CCLK */
			uint32_t tscldel = (scldel + 1U) * (presc + 1U) * ti2cclk;

			if (tscldel >= (uint32_t)tscldel_min) {
				for (sdadel = 0; sdadel < STM32_I2C_SDADEL_MAX; sdadel++) {
					/* TSDADEL = SDADEL * (PRESC+1) * TI2CCLK */
					uint32_t tsdadel = (sdadel * (presc + 1U)) * ti2cclk;

					I2C_LOOP_SDADEL();
				}

				if (i2c_valid_timing_nbr >= STM32_I2C_VALID_TIMING_NBR) {
					return;
				}
			}
		}
	}
}

int stm32_i2c_configure_timing(const struct device *dev, uint32_t clock)
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
		for (speed = 0; speed <= (uint32_t)STM32_I2C_SPEED_FREQ_FAST_PLUS; speed++) {
			if ((i2c_freq >= stm32_i2c_charac[speed].freq_min) &&
			    (i2c_freq <= stm32_i2c_charac[speed].freq_max)) {
				i2c_compute_presc_scldel_sdadel(clock, speed);
				idx = i2c_compute_scll_sclh(clock, speed);
				if (idx < STM32_I2C_VALID_TIMING_NBR) {
					timing = ((i2c_valid_timing[idx].presc & 0x0FU) << 28) |
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
#else  /* CONFIG_I2C_STM32_V2_TIMING */

int stm32_i2c_configure_timing(const struct device *dev, uint32_t clock)
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

		if ((I2C_SPEED_GET(speed) == I2C_SPEED_GET(data->dev_config)) &&
		    (preset->periph_clock == clock)) {
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

		if ((sclh - 1) > 255 || (scll - 1) > 255) {
			++presc;
			continue;
		}

		if (sdadel > 15 || (scldel - 1) > 15) {
			++presc;
			continue;
		}

		timing =
			__LL_I2C_CONVERT_TIMINGS(presc - 1, scldel - 1, sdadel, sclh - 1, scll - 1);
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

int stm32_i2c_transaction(const struct device *dev, struct i2c_msg msg, uint8_t *next_msg_flags,
			  uint16_t periph)
{
	int ret;
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
	const uint32_t i2c_stm32_maxchunk = 255U;
	const uint8_t saved_flags = msg.flags;
	uint8_t combine_flags = saved_flags & ~(I2C_MSG_STOP | I2C_MSG_RESTART);
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
			ret = stm32_i2c_msg_write(dev, &msg, flagsp, periph);
		} else {
			ret = stm32_i2c_msg_read(dev, &msg, flagsp, periph);
		}
		if (ret < 0) {
			break;
		}
		rest -= msg.len;
		msg.buf += msg.len;
		msg.len = rest;
	} while (rest > 0U);
#endif
	return ret;
}
