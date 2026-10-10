/*
 * Copyright (c) 2026 Gail Rojas <gailroco@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_msp432p4xx_i2c

#include <errno.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <soc.h>

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_msp432p4xx);

#include "i2c-priv.h"

/*
 * CTLW0 base value: I2C mode (MODE=11b), master, SMCLK source,
 * synchronous, software reset held.  SWRST must be set while writing
 * BRW; clear it after to release the module.
 */
#define CTLW0_I2C_MASTER                             \
	((uint16_t)(EUSCI_B_CTLW0_SWRST            | \
		    EUSCI_B_CTLW0_MODE_3            | \
		    EUSCI_B_CTLW0_MST               | \
		    EUSCI_B_CTLW0_SSEL__SMCLK       | \
		    EUSCI_B_CTLW0_SYNC))

/* Interrupt sources kept enabled at all times (not during a transfer). */
#define IE_BASE                                      \
	((uint16_t)(EUSCI_B_IE_STPIE | EUSCI_B_IE_NACKIE))

enum i2c_msp432p4xx_state {
	I2C_MSP432P4XX_IDLE  = 0,
	I2C_MSP432P4XX_WRITE,
	I2C_MSP432P4XX_READ,
	I2C_MSP432P4XX_ERROR,
};

struct i2c_msp432p4xx_config {
	EUSCI_B_Type *base;
	uint32_t bitrate;
	uint32_t smclk_freq;
	void (*irq_cfg_func)(void);
};

struct i2c_msp432p4xx_data {
	struct k_sem mutex;
	struct k_sem transfer_complete;
	volatile enum i2c_msp432p4xx_state state;
	struct i2c_msg msg;
	uint16_t target_addr;
};

static int i2c_msp432p4xx_configure(const struct device *dev,
				     uint32_t dev_config_raw)
{
	const struct i2c_msp432p4xx_config *config = dev->config;
	EUSCI_B_Type *hw = config->base;
	uint32_t bitrate;

	if (!(dev_config_raw & I2C_MODE_CONTROLLER)) {
		return -EINVAL;
	}
	if (dev_config_raw & I2C_ADDR_10_BITS) {
		return -EINVAL;
	}

	switch (I2C_SPEED_GET(dev_config_raw)) {
	case I2C_SPEED_STANDARD:
		bitrate = 100000U;
		break;
	case I2C_SPEED_FAST:
		bitrate = 400000U;
		break;
	default:
		return -EINVAL;
	}

	/*
	 * SWRST must be set when writing BRW (TRM section 24.3.1).
	 * Setting CTLW0 = CTLW0_I2C_MASTER also clears UCBxIE and UCBxIFG,
	 * so both are re-initialised after clearing SWRST.
	 */
	hw->CTLW0 = CTLW0_I2C_MASTER;
	hw->BRW   = (uint16_t)(config->smclk_freq / bitrate);
	hw->CTLW0 &= ~EUSCI_B_CTLW0_SWRST;

	hw->IFG = 0;
	hw->IE  = IE_BASE;

	return 0;
}

static void prime_transfer(const struct device *dev,
			    struct i2c_msg *msg, uint16_t addr)
{
	const struct i2c_msp432p4xx_config *config = dev->config;
	struct i2c_msp432p4xx_data *data = dev->data;
	EUSCI_B_Type *hw = config->base;

	data->msg         = *msg;
	data->target_addr = addr;
	hw->I2CSA         = addr;

	if ((msg->flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
		data->state = I2C_MSP432P4XX_WRITE;
		/*
		 * Set TR (transmit) and TXSTT before enabling TXIE0 so the
		 * ISR cannot fire before the address phase begins.  If the
		 * previous message ended without STOP the bus is still active
		 * and TXSTT generates a repeated START instead of a new START.
		 */
		hw->CTLW0 |= EUSCI_B_CTLW0_TR | EUSCI_B_CTLW0_TXSTT;
		hw->IE    |= EUSCI_B_IE_TXIE0;
	} else {
		data->state = I2C_MSP432P4XX_READ;
		/*
		 * Clear TR (receive mode) and set TXSTT (START) atomically.
		 * For single-byte reads, TXSTP must be set before the byte
		 * is fully received so the hardware NACKs it and generates
		 * STOP (TRM section 24.3.5).
		 */
		hw->CTLW0 = (uint16_t)((hw->CTLW0 & ~EUSCI_B_CTLW0_TR) |
					EUSCI_B_CTLW0_TXSTT);
		if (msg->len == 1U) {
			hw->CTLW0 |= EUSCI_B_CTLW0_TXSTP;
		}
		hw->IE |= EUSCI_B_IE_RXIE0;
	}
}

static int i2c_msp432p4xx_transfer(const struct device *dev,
				    struct i2c_msg *msgs,
				    uint8_t num_msgs,
				    uint16_t addr)
{
	struct i2c_msp432p4xx_data *data = dev->data;
	int retval = 0;

	__ASSERT(msgs != NULL, "msgs must not be NULL");
	__ASSERT(num_msgs > 0U, "num_msgs must be greater than zero");

	k_sem_take(&data->mutex, K_FOREVER);

	for (int i = 0; i < (int)num_msgs; i++) {
		prime_transfer(dev, msgs, addr);
		k_sem_take(&data->transfer_complete, K_FOREVER);

		if (data->state == I2C_MSP432P4XX_ERROR) {
			retval = -EIO;
			break;
		}
		msgs++;
	}

	k_sem_give(&data->mutex);
	return retval;
}

static void isr_handle_write(EUSCI_B_Type *hw,
			      struct i2c_msp432p4xx_data *data)
{
	if (data->msg.len == 0U) {
		/*
		 * No payload (address probe).  Disable TXIE0 before asserting
		 * TXSTP: TXIFG0 is a level-sensitive flag that stays set while
		 * the TX buffer is empty, so leaving TXIE0 enabled would cause
		 * the ISR to re-enter in a tight loop until the STOP completes.
		 */
		hw->IE &= ~(uint16_t)EUSCI_B_IE_TXIE0;
		hw->CTLW0 |= EUSCI_B_CTLW0_TXSTP;
		return;
	}

	hw->TXBUF = *data->msg.buf;
	data->msg.buf++;
	data->msg.len--;

	if (data->msg.len == 0U) {
		hw->IE &= ~(uint16_t)EUSCI_B_IE_TXIE0;
		if (data->msg.flags & I2C_MSG_STOP) {
			/* Last or only message: release the bus. */
			hw->CTLW0 |= EUSCI_B_CTLW0_TXSTP;
		} else {
			/*
			 * More messages follow with no STOP: signal the
			 * transfer loop so it can prime the next message.
			 * prime_transfer() will assert TXSTT while the bus
			 * is still active, generating a repeated START
			 * (TRM section 24.3.4).
			 */
			k_sem_give(&data->transfer_complete);
		}
	}
}

static void isr_handle_read(EUSCI_B_Type *hw,
			     struct i2c_msp432p4xx_data *data)
{
	if (data->msg.len == 0U) {
		/* Discard unexpected byte; a length underflow would corrupt state. */
		(void)hw->RXBUF;
		return;
	}

	/*
	 * When two bytes remain, assert TXSTP only if this message ends the
	 * transfer (I2C_MSG_STOP set).  For chained messages without a STOP,
	 * the next prime_transfer call issues a repeated START instead, so
	 * asserting TXSTP here would generate a premature STOP and return the
	 * read buffer one byte short.  The timing requirement (TXSTP must be
	 * set before the penultimate byte is fully received, TRM 24.3.5) is
	 * still met because we are inside the RXIFG0 ISR for the preceding
	 * byte.
	 */
	if (data->msg.len == 2U && (data->msg.flags & I2C_MSG_STOP)) {
		hw->CTLW0 |= EUSCI_B_CTLW0_TXSTP;
	}

	*data->msg.buf = (uint8_t)hw->RXBUF;
	data->msg.buf++;
	data->msg.len--;
}

static void i2c_msp432p4xx_isr(const struct device *dev)
{
	const struct i2c_msp432p4xx_config *config = dev->config;
	struct i2c_msp432p4xx_data *data = dev->data;
	EUSCI_B_Type *hw = config->base;
	uint16_t ifg;

	ifg    = hw->IFG;
	hw->IFG = 0;

	if (ifg & EUSCI_B_IFG_NACKIFG) {
		/*
		 * Address or data NACK: generate STOP to release the bus,
		 * then signal error to the transfer function via STPIFG.
		 */
		hw->CTLW0 |= EUSCI_B_CTLW0_TXSTP;
		hw->IE     = IE_BASE;
		data->state = I2C_MSP432P4XX_ERROR;
	} else if (ifg & EUSCI_B_IFG_TXIFG0) {
		if (data->state == I2C_MSP432P4XX_WRITE) {
			isr_handle_write(hw, data);
		}
	} else if (ifg & (EUSCI_B_IFG_RXIFG0 | EUSCI_B_IFG_STPIFG)) {
		/*
		 * RXIFG0 and STPIFG can be latched simultaneously on the last
		 * byte of a receive transfer: the hardware sets STPIFG while
		 * the final byte is still in RXBUF.  Always drain RXBUF before
		 * signalling completion so the caller never receives a
		 * truncated buffer.
		 */
		if ((ifg & EUSCI_B_IFG_RXIFG0) &&
		    (data->state == I2C_MSP432P4XX_READ)) {
			isr_handle_read(hw, data);
		}
		if (ifg & EUSCI_B_IFG_STPIFG) {
			/* STOP condition: transfer complete (normal or after NACK). */
			hw->IE = IE_BASE;
			k_sem_give(&data->transfer_complete);
		}
	} else {
		LOG_ERR("unexpected interrupt: IFG=0x%04x", (unsigned int)ifg);
		data->state = I2C_MSP432P4XX_ERROR;
		hw->IE      = IE_BASE;
		k_sem_give(&data->transfer_complete);
	}
}

static int i2c_msp432p4xx_init(const struct device *dev)
{
	const struct i2c_msp432p4xx_config *config = dev->config;
	struct i2c_msp432p4xx_data *data = dev->data;
	uint32_t bitrate_cfg;
	int err;

	k_sem_init(&data->mutex, 1, 1);
	k_sem_init(&data->transfer_complete, 0, 1);
	data->state = I2C_MSP432P4XX_IDLE;

	/*
	 * Configure P1.6 (UCB0SDA) and P1.7 (UCB0SCL) for eUSCI_B0 I2C.
	 * TRM Table 4-1: SEL0=1, SEL1=0 selects the secondary (I2C) function.
	 * No pinctrl driver exists for MSP432P4xx; pin assignment is done here
	 * and is only correct for instance 0 (eUSCI_B0) on the MSP-EXP432P401R
	 * LaunchPad (P1.6=SDA, P1.7=SCL).  A pinctrl driver for this SoC is
	 * required to support other boards or instances.
	 *
	 * Internal pull-ups hold both lines high when the bus is idle.
	 * The ~47 kΩ internal resistors are sufficient for a no-load address
	 * scan; replace them with external 4.7 kΩ resistors to VCC for
	 * production I2C traffic.
	 */
	P1->SEL0 |=  (uint8_t)(BIT(6) | BIT(7));
	P1->SEL1 &= ~(uint8_t)(BIT(6) | BIT(7));
	P1->REN  |=  (uint8_t)(BIT(6) | BIT(7));
	P1->OUT  |=  (uint8_t)(BIT(6) | BIT(7));

	bitrate_cfg = i2c_map_dt_bitrate(config->bitrate);
	err = i2c_msp432p4xx_configure(dev, I2C_MODE_CONTROLLER | bitrate_cfg);
	if (err != 0) {
		return err;
	}

	config->irq_cfg_func();

	return 0;
}

static DEVICE_API(i2c, i2c_msp432p4xx_driver_api) = {
	.configure  = i2c_msp432p4xx_configure,
	.transfer   = i2c_msp432p4xx_transfer,
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

#define I2C_MSP432P4XX_INIT(n)						\
	static void i2c_msp432p4xx_irq_cfg_##n(void)			\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    DT_INST_IRQ(n, priority),			\
			    i2c_msp432p4xx_isr,				\
			    DEVICE_DT_INST_GET(n), 0);			\
		irq_enable(DT_INST_IRQN(n));				\
	}								\
									\
	static const struct i2c_msp432p4xx_config			\
		i2c_msp432p4xx_config_##n = {				\
		.base         = (EUSCI_B_Type *)DT_INST_REG_ADDR(n),	\
		.bitrate      = DT_INST_PROP(n, clock_frequency),	\
		.smclk_freq   = DT_PROP(DT_INST_PHANDLE(n, clocks),	\
					clock_frequency),		\
		.irq_cfg_func = i2c_msp432p4xx_irq_cfg_##n,		\
	};								\
									\
	static struct i2c_msp432p4xx_data i2c_msp432p4xx_data_##n;	\
									\
	I2C_DEVICE_DT_INST_DEFINE(n, i2c_msp432p4xx_init, NULL,	\
				  &i2c_msp432p4xx_data_##n,		\
				  &i2c_msp432p4xx_config_##n,		\
				  POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,\
				  &i2c_msp432p4xx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_MSP432P4XX_INIT)
