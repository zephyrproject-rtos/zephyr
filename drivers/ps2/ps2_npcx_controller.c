/*
 * Copyright (c) 2021 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcx_ps2_ctrl

/**
 * @file
 * @brief Nuvoton NPCX PS/2 module (controller) driver
 *
 * This file contains the driver of PS/2 module (controller) which provides a
 * hardware accelerator mechanism to handle both incoming and outgoing data.
 * The hardware accelerator mechanism is shared by four PS/2 channels.
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/ps2.h>
#include <zephyr/dt-bindings/clock/npcx_clock.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ps2_npcx_ctrl, CONFIG_PS2_LOG_LEVEL);

#define NPCX_PS2_CH_COUNT 4

/*
 * Set WDAT3-0 and clear CLK3-0 in the PSOSIG register to
 * reset the shift mechanism.
 */
#define NPCX_PS2_SHIFT_MECH_RESET (uint8_t)~NPCX_PSOSIG_CLK_MASK_ALL

/* in 50us units */
#define PS2_RETRY_COUNT 10000

/*
 * The max duration of a PS/2 clock is about 100 micro-seconds.
 * A PS/2 transaction needs 11 clock cycles. It will take about 1.1 ms for a
 * complete transaction.
 */
#define PS2_TRANSACTION_TIMEOUT K_MSEC(2)

/* Device config */
struct ps2_npcx_ctrl_config {
	uintptr_t base;
	struct npcx_clk_cfg clk_cfg;
};

/* Driver data */
struct ps2_npcx_ctrl_data {
	/*
	 * Bit mask to record the enabled PS/2 channels.
	 * Only bit[7] and bit[5:3] are used
	 * (i.e. the bit position of CLK3-0 in the PS2_PSOSIG register)
	 */
	uint8_t channel_enabled_mask;
	/* The mutex of the PS/2 controller */
	struct k_sem lock;
	/* The semaphore to synchronize the Tx transaction */
	struct k_sem tx_sync_sem;
	/*
	 * The callback function to handle the data received from PS/2 device
	 */
	ps2_callback_t callback_isr[NPCX_PS2_CH_COUNT];
};

/* Driver convenience defines */
#define HAL_PS2_INSTANCE(dev)                                                                      \
	((struct ps2_reg *)((const struct ps2_npcx_ctrl_config *)(dev)->config)->base)

static uint8_t ps2_npcx_ctrl_get_ch_clk_mask(uint8_t channel_id)
{
	return BIT(NPCX_PSOSIG_CLK(channel_id));
}

int ps2_npcx_ctrl_configure(const struct device *dev, uint8_t channel_id,
			    ps2_callback_t callback_isr)
{
	struct ps2_npcx_ctrl_data *const data = dev->data;

	if (channel_id >= NPCX_PS2_CH_COUNT) {
		LOG_ERR("unexpected channel ID: %d", channel_id);
		return -EINVAL;
	}

	if (callback_isr == NULL) {
		return -EINVAL;
	}

	k_sem_take(&data->lock, K_FOREVER);
	data->callback_isr[channel_id] = callback_isr;
	k_sem_give(&data->lock);

	return 0;
}

int ps2_npcx_ctrl_enable_interface(const struct device *dev, uint8_t channel_id,
				   bool enable)
{
	struct ps2_npcx_ctrl_data *const data = dev->data;
	struct ps2_reg *const inst = HAL_PS2_INSTANCE(dev);
	uint8_t ch_clk_mask;

	k_sem_take(&data->lock, K_FOREVER);
	/*
	 * Disable the interrupt during changing the enabled channel mask to
	 * prevent from preemption.
	 */
	irq_disable(DT_INST_IRQN(0));

	if (channel_id >= NPCX_PS2_CH_COUNT) {
		LOG_ERR("unexpected channel ID: %d", channel_id);
		irq_enable(DT_INST_IRQN(0));
		k_sem_give(&data->lock);
		return -EINVAL;
	}

	ch_clk_mask = ps2_npcx_ctrl_get_ch_clk_mask(channel_id);
	if (enable) {
		data->channel_enabled_mask |= ch_clk_mask;
		/* Enable the relevant channel clock */
		inst->PSOSIG |= ch_clk_mask;
	} else {
		data->channel_enabled_mask &= ~ch_clk_mask;
		/* Disable the relevant channel clock */
		inst->PSOSIG &= ~ch_clk_mask;
	}

	irq_enable(DT_INST_IRQN(0));
	k_sem_give(&data->lock);

	return 0;
}

static int ps2_npcx_ctrl_bus_busy(const struct device *dev)
{
	struct ps2_reg *const inst = HAL_PS2_INSTANCE(dev);

	/*
	 * The driver pulls the CLK for non-active channels to low when Start
	 * bit is detected and pull the CLK of the active channel low after
	 * Stop bit detected. The EOT bit is set when Stop bit is detected,
	 * but both SOT and EOT are cleared when all CLKs are pull low
	 * (due to Shift Mechanism is reset)
	 */
	return (IS_BIT_SET(inst->PSTAT, NPCX_PSTAT_SOT) ||
		IS_BIT_SET(inst->PSTAT, NPCX_PSTAT_EOT)) ?
		       -EBUSY :
		       0;
}

int ps2_npcx_ctrl_write(const struct device *dev, uint8_t channel_id,
			uint8_t value)
{
	struct ps2_npcx_ctrl_data *const data = dev->data;
	struct ps2_reg *const inst = HAL_PS2_INSTANCE(dev);
	int i = 0;

	if (channel_id >= NPCX_PS2_CH_COUNT) {
		LOG_ERR("unexpected channel ID: %d", channel_id);
		return -EINVAL;
	}

	if (!(ps2_npcx_ctrl_get_ch_clk_mask(channel_id) &
	      data->channel_enabled_mask)) {
		LOG_ERR("channel %d is not enabled", channel_id);
		return -EINVAL;
	}

	k_sem_take(&data->lock, K_FOREVER);

	while (ps2_npcx_ctrl_bus_busy(dev) && (i < PS2_RETRY_COUNT)) {
		k_busy_wait(50);
		i++;
	}

	if (unlikely(i == PS2_RETRY_COUNT)) {
		LOG_ERR("PS2 write attempt timed out");
		goto timeout_invalid;
	}

	/* Set PS/2 in transmit mode */
	inst->PSCON |= BIT(NPCX_PSCON_XMT);
	/* Enable Start Of Transaction interrupt */
	inst->PSIEN |= BIT(NPCX_PSIEN_SOTIE);

	/* Reset the shift mechanism */
	inst->PSOSIG = NPCX_PS2_SHIFT_MECH_RESET;
	/* Inhibit communication should last at least 100 micro-seconds */
	k_busy_wait(100);

	/* Write the data to be transmitted */
	inst->PSDAT = value;
	/* Apply the Request-to-send */
	inst->PSOSIG &= ~BIT(NPCX_PSOSIG_WDAT(channel_id));
	inst->PSOSIG |= ps2_npcx_ctrl_get_ch_clk_mask(channel_id);
	if (k_sem_take(&data->tx_sync_sem, PS2_TRANSACTION_TIMEOUT) != 0) {
		irq_disable(DT_INST_IRQN(0));
		LOG_ERR("PS/2 Tx timeout");
		/* Reset the shift mechanism */
		inst->PSOSIG = NPCX_PS2_SHIFT_MECH_RESET;
		/* Change the PS/2 module to receive mode */
		inst->PSCON &= ~BIT(NPCX_PSCON_XMT);
		/*
		 * Restore the channel back to enable according to
		 * channel_enabled_mask.
		 */
		inst->PSOSIG |= data->channel_enabled_mask;
		irq_enable(DT_INST_IRQN(0));
		goto timeout_invalid;
	}

	k_sem_give(&data->lock);
	return 0;

timeout_invalid:
	k_sem_give(&data->lock);
	return -ETIMEDOUT;
}

static int ps2_npcx_ctrl_is_rx_error(const struct device *dev)
{
	struct ps2_reg *const inst = HAL_PS2_INSTANCE(dev);
	uint8_t status;

	status = inst->PSTAT & (BIT(NPCX_PSTAT_PERR) | BIT(NPCX_PSTAT_RFERR));
	if (status) {
		if (status & BIT(NPCX_PSTAT_PERR))
			LOG_ERR("RX parity error");
		if (status & BIT(NPCX_PSTAT_RFERR))
			LOG_ERR("RX Frame error");
		return -EIO;
	}

	return 0;
}

static void ps2_npcx_ctrl_isr(const struct device *dev)
{
	uint8_t active_ch, mask;
	struct ps2_reg *const inst = HAL_PS2_INSTANCE(dev);
	struct ps2_npcx_ctrl_data *const data = dev->data;

	/*
	 * ACH = 1 : Channel 0
	 * ACH = 2 : Channel 1
	 * ACH = 4 : Channel 2
	 * ACH = 5 : Channel 3
	 */
	active_ch = GET_FIELD(inst->PSTAT, NPCX_PSTAT_ACH);
	active_ch = active_ch > 2 ? (active_ch - 2) : (active_ch - 1);
	LOG_DBG("ACH: %d\n", active_ch);

	/*
	 * Inhibit PS/2 transaction of the other non-active channels by
	 * pulling down the clock signal
	 */
	mask = ~NPCX_PSOSIG_CLK_MASK_ALL | BIT(NPCX_PSOSIG_CLK(active_ch));
	inst->PSOSIG &= mask;

	/* PS/2 Start of Transaction */
	if (IS_BIT_SET(inst->PSTAT, NPCX_PSTAT_SOT) &&
	    IS_BIT_SET(inst->PSIEN, NPCX_PSIEN_SOTIE)) {
		/*
		 * Once set, SOT is not cleared until the shift mechanism
		 * is reset. Therefore, SOTIE should be cleared on the
		 * first occurrence of an SOT interrupt.
		 */
		inst->PSIEN &= ~BIT(NPCX_PSIEN_SOTIE);
		LOG_DBG("SOT");
		/* PS/2 End of Transaction */
	} else if (IS_BIT_SET(inst->PSTAT, NPCX_PSTAT_EOT)) {
		inst->PSIEN &= ~BIT(NPCX_PSIEN_EOTIE);

		/*
		 * Clear the CLK of active channel to reset
		 * the shift mechanism
		 */
		inst->PSOSIG &= ~BIT(NPCX_PSOSIG_CLK(active_ch));

		/* Tx is done */
		if (IS_BIT_SET(inst->PSCON, NPCX_PSCON_XMT)) {
			/* Change the PS/2 module to receive mode */
			inst->PSCON &= ~BIT(NPCX_PSCON_XMT);
			k_sem_give(&data->tx_sync_sem);
		} else {
			if (ps2_npcx_ctrl_is_rx_error(dev) == 0) {
				ps2_callback_t callback;
				uint8_t data_in = inst->PSDAT;

				LOG_DBG("Recv:0x%02x", data_in);
				callback = data->callback_isr[active_ch];
				if (callback != NULL) {
					callback(dev, data_in);
				}
			}
		}

		/* Restore the enabled channel */
		inst->PSOSIG |= data->channel_enabled_mask;
		/*
		 * Re-enable the Start Of Transaction interrupt when
		 * the shift mechanism is reset
		 */
		inst->PSIEN |= BIT(NPCX_PSIEN_SOTIE);
		inst->PSIEN |= BIT(NPCX_PSIEN_EOTIE);
		LOG_DBG("EOT");
	}
}

/* PS/2 driver registration */
static int ps2_npcx_ctrl_init(const struct device *dev);

static struct ps2_npcx_ctrl_data ps2_npcx_ctrl_data_0;

static const struct ps2_npcx_ctrl_config ps2_npcx_ctrl_config_0 = {
	.base = DT_INST_REG_ADDR(0),
	.clk_cfg = NPCX_DT_CLK_CFG_ITEM(0),
};

DEVICE_DT_INST_DEFINE(0, &ps2_npcx_ctrl_init, NULL, &ps2_npcx_ctrl_data_0,
		      &ps2_npcx_ctrl_config_0, POST_KERNEL,
		      CONFIG_PS2_INIT_PRIORITY, NULL);

static int ps2_npcx_ctrl_init(const struct device *dev)
{
	const struct ps2_npcx_ctrl_config *const config = dev->config;
	struct ps2_npcx_ctrl_data *const data = dev->data;
	struct ps2_reg *const inst = HAL_PS2_INSTANCE(dev);
	const struct device *clk_dev = DEVICE_DT_GET(NPCX_CLK_CTRL_NODE);
	int ret;

	if (!device_is_ready(clk_dev)) {
		LOG_ERR("%s device not ready", clk_dev->name);
		return -ENODEV;
	}

	/* Turn on PS/2 controller device clock */
	ret = clock_control_on(clk_dev,
			       (clock_control_subsys_t *)&config->clk_cfg);
	if (ret < 0) {
		LOG_ERR("Turn on PS/2 clock fail %d", ret);
		return ret;
	}

	/* Disable shift mechanism and configure PS/2 to received mode. */
	inst->PSCON = 0x0;
	/* Set WDAT3-0 and clear CLK3-0 before enabling shift mechanism */
	inst->PSOSIG = NPCX_PS2_SHIFT_MECH_RESET;
	/*
	 * PS/2 interrupt enable register
	 * [0] - : SOTIE   = 1: Start Of Transaction Interrupt Enable
	 * [1] - : EOTIE   = 1: End Of Transaction Interrupt Enable
	 * [4] - : WUE     = 1: Wake-Up Enable
	 * [7] - : CLK_SEL = 1: Select Free-Run clock as the basic clock
	 *                   0: Select APB1 clock as the basic clock
	 */
	inst->PSIEN = BIT(NPCX_PSIEN_SOTIE) | BIT(NPCX_PSIEN_EOTIE) |
		      BIT(NPCX_PSIEN_PS2_WUE);
	if (config->clk_cfg.bus == NPCX_CLOCK_BUS_FREERUN)
		inst->PSIEN |= BIT(NPCX_PSIEN_PS2_CLK_SEL);
	/* Enable weak internal pull-up */
	inst->PSCON |= BIT(NPCX_PSCON_WPUED);
	/* Enable shift mechanism */
	inst->PSCON |= BIT(NPCX_PSCON_EN);

	k_sem_init(&data->lock, 1, 1);
	k_sem_init(&data->tx_sync_sem, 0, 1);

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    ps2_npcx_ctrl_isr, DEVICE_DT_INST_GET(0), 0);

	irq_enable(DT_INST_IRQN(0));

	return 0;
}
