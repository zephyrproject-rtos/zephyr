/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2022 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_ps2

#include <cmsis_core.h>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#ifdef CONFIG_SOC_SERIES_MEC172X
#include <zephyr/drivers/clock_control/mchp_xec_clock_control.h>
#include <zephyr/drivers/interrupt_controller/intc_mchp_xec_ecia.h>
#endif
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/drivers/ps2.h>
#include <soc.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/gpio.h>

#define LOG_LEVEL CONFIG_PS2_LOG_LEVEL
LOG_MODULE_REGISTER(ps2_mchp_xec);

/* in 50us units */
#define PS2_TIMEOUT 10000

struct ps2_xec_config {
	struct ps2_regs * const regs;
	int isr_nvic;
	uint8_t girq_id;
	uint8_t girq_bit;
	uint8_t girq_id_wk;
	uint8_t girq_bit_wk;
	uint8_t pcr_idx;
	uint8_t pcr_pos;
	void (*irq_config_func)(void);
	const struct pinctrl_dev_config *pcfg;
#ifdef CONFIG_PM_DEVICE
	struct gpio_dt_spec wakerx_gpio;
	bool wakeup_source;
#endif
};


struct ps2_xec_data {
	ps2_callback_t callback_isr;
	struct k_sem tx_lock;
};


#ifdef CONFIG_SOC_SERIES_MEC172X
static inline void ps2_xec_slp_en_clr(const struct device *dev)
{
	const struct ps2_xec_config * const cfg = dev->config;

	z_mchp_xec_pcr_periph_sleep(cfg->pcr_idx, cfg->pcr_pos, 0);
}

static inline void ps2_xec_girq_clr(uint8_t girq_idx, uint8_t girq_posn)
{
	mchp_soc_ecia_girq_src_clr(girq_idx, girq_posn);
}

static inline void ps2_xec_girq_en(uint8_t girq_idx, uint8_t girq_posn)
{
	mchp_xec_ecia_girq_src_en(girq_idx, girq_posn);
}

static inline void ps2_xec_girq_dis(uint8_t girq_idx, uint8_t girq_posn)
{
	mchp_xec_ecia_girq_src_dis(girq_idx, girq_posn);
}
#else
static inline void ps2_xec_slp_en_clr(const struct device *dev)
{
	const struct ps2_xec_config * const cfg = dev->config;

	if (cfg->pcr_pos == MCHP_PCR3_PS2_0_POS) {
		mchp_pcr_periph_slp_ctrl(PCR_PS2_0, 0);
	} else {
		mchp_pcr_periph_slp_ctrl(PCR_PS2_1, 0);
	}
}

static inline void ps2_xec_girq_clr(uint8_t girq_idx, uint8_t girq_posn)
{
	MCHP_GIRQ_SRC(girq_idx) = BIT(girq_posn);
}

static inline void ps2_xec_girq_en(uint8_t girq_idx, uint8_t girq_posn)
{
	MCHP_GIRQ_ENSET(girq_idx) = BIT(girq_posn);
}

static inline void ps2_xec_girq_dis(uint8_t girq_idx, uint8_t girq_posn)
{
	MCHP_GIRQ_ENCLR(girq_idx) = MCHP_KBC_IBF_GIRQ;
}
#endif /* CONFIG_SOC_SERIES_MEC172X */

static int ps2_xec_configure(const struct device *dev,
			     ps2_callback_t callback_isr)
{
	const struct ps2_xec_config * const config = dev->config;
	struct ps2_xec_data * const data = dev->data;
	struct ps2_regs * const regs = config->regs;

	uint8_t  __attribute__((unused)) temp;

	if (!callback_isr) {
		return -EINVAL;
	}

	data->callback_isr = callback_isr;

	/* In case the self test for a PS2 device already finished and
	 * set the SOURCE bit to 1 we clear it before enabling the
	 * interrupts. Instances must be allocated before the BAT
	 * (Basic Assurance Test) or the host may time out.
	 */
	temp = regs->TRX_BUFF;
	regs->STATUS = MCHP_PS2_STATUS_RW1C_MASK;
	/* clear next higher level */
	ps2_xec_girq_clr(config->girq_id, config->girq_bit);

	/* Enable FSM and init instance in rx mode*/
	regs->CTRL = MCHP_PS2_CTRL_EN_POS;

	/* We enable the interrupts in the EC aggregator so that the
	 * result  can be forwarded to the ARM NVIC
	 */
	ps2_xec_girq_en(config->girq_id, config->girq_bit);

	k_sem_give(&data->tx_lock);

	return 0;
}


static int ps2_xec_write(const struct device *dev, uint8_t value)
{
	const struct ps2_xec_config * const config = dev->config;
	struct ps2_xec_data * const data = dev->data;
	struct ps2_regs * const regs = config->regs;
	int i = 0;

	uint8_t  __attribute__((unused)) temp;

	if (k_sem_take(&data->tx_lock, K_NO_WAIT)) {
		return -EACCES;
	}
	/* Allow the PS2 controller to complete a RX transaction. This
	 * is because the channel may be actively receiving data.
	 * In addition, it is necessary to wait for a previous TX
	 * transaction to complete. The PS2 block has a single
	 * FSM.
	 */
	while (((regs->STATUS &
		(MCHP_PS2_STATUS_RX_BUSY | MCHP_PS2_STATUS_TX_IDLE))
		!= MCHP_PS2_STATUS_TX_IDLE) && (i < PS2_TIMEOUT)) {
		k_busy_wait(50);
		i++;
	}

	if (unlikely(i == PS2_TIMEOUT)) {
		LOG_DBG("PS2 write timed out");
		return -ETIMEDOUT;
	}

	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	/* Inhibit ps2 controller and clear status register */
	regs->CTRL = 0x00;

	/* Read to clear data ready bit in the status register*/
	temp = regs->TRX_BUFF;
	k_sleep(K_MSEC(1));
	regs->STATUS = MCHP_PS2_STATUS_RW1C_MASK;

	/* Switch the interface to TX mode and enable state machine */
	regs->CTRL = MCHP_PS2_CTRL_TR_TX | MCHP_PS2_CTRL_EN;

	/* Write value to TX/RX register */
	regs->TRX_BUFF = value;

	k_sem_give(&data->tx_lock);

	return 0;
}

static int ps2_xec_inhibit_interface(const struct device *dev)
{
	const struct ps2_xec_config * const config = dev->config;
	struct ps2_xec_data * const data = dev->data;
	struct ps2_regs * const regs = config->regs;

	if (k_sem_take(&data->tx_lock, K_MSEC(10)) != 0) {
		return -EACCES;
	}

	regs->CTRL = 0x00;
	regs->STATUS = MCHP_PS2_STATUS_RW1C_MASK;
	ps2_xec_girq_clr(config->girq_id, config->girq_bit);
	NVIC_ClearPendingIRQ(config->isr_nvic);

	k_sem_give(&data->tx_lock);

	return 0;
}

static int ps2_xec_enable_interface(const struct device *dev)
{
	const struct ps2_xec_config * const config = dev->config;
	struct ps2_xec_data * const data = dev->data;
	struct ps2_regs * const regs = config->regs;

	ps2_xec_girq_clr(config->girq_id, config->girq_bit);
	regs->CTRL = MCHP_PS2_CTRL_EN;

	k_sem_give(&data->tx_lock);

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int ps2_xec_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct ps2_xec_config *const devcfg = dev->config;
	struct ps2_regs * const regs = devcfg->regs;
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		if (devcfg->wakeup_source) {
			/* Disable PS2 wake interrupt
			 * Disable interrupt on PS2DAT pin
			 */
			if (devcfg->wakerx_gpio.port != NULL) {
				ret = gpio_pin_interrupt_configure_dt(
						&devcfg->wakerx_gpio,
						GPIO_INT_DISABLE);
				if (ret < 0) {
					LOG_ERR("Fail to disable PS2 wake interrupt (ret %d)", ret);
					return ret;
				}
			}
			ps2_xec_girq_dis(devcfg->girq_id_wk, devcfg->girq_bit_wk);
			ps2_xec_girq_clr(devcfg->girq_id_wk, devcfg->girq_bit_wk);
		} else {
			ret = pinctrl_apply_state(devcfg->pcfg, PINCTRL_STATE_DEFAULT);
			regs->CTRL |= MCHP_PS2_CTRL_EN;
		}
	break;
	case PM_DEVICE_ACTION_SUSPEND:
		if (devcfg->wakeup_source) {
			/* Enable PS2 wake interrupt
			 * Configure Falling Edge Trigger interrupt on PS2DAT pin
			 */
			ps2_xec_girq_clr(devcfg->girq_id_wk, devcfg->girq_bit_wk);
			ps2_xec_girq_en(devcfg->girq_id_wk, devcfg->girq_bit_wk);
			if (devcfg->wakerx_gpio.port != NULL) {
				ret = gpio_pin_interrupt_configure_dt(
					&devcfg->wakerx_gpio,
					GPIO_INT_MODE_EDGE | GPIO_INT_TRIG_LOW);
				if (ret < 0) {
					LOG_ERR("Fail to enable PS2 wake interrupt(ret %d)", ret);
					return ret;
				}
			}
		} else {
			regs->CTRL &= ~MCHP_PS2_CTRL_EN;
			/* If application does not want to turn off PS2 pins it will
			 * not define pinctrl-1 for this node.
			 */
			ret = pinctrl_apply_state(devcfg->pcfg, PINCTRL_STATE_SLEEP);
			if (ret == -ENOENT) { /* pinctrl-1 does not exist.  */
				ret = 0;
			}
		}
	break;
	default:
		ret = -ENOTSUP;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static void ps2_xec_isr(const struct device *dev)
{
	const struct ps2_xec_config * const config = dev->config;
	struct ps2_xec_data * const data = dev->data;
	struct ps2_regs * const regs = config->regs;
	uint32_t status;

	/* Read and clear status */
	status = regs->STATUS;

	/* clear next higher level the GIRQ */
	ps2_xec_girq_clr(config->girq_id, config->girq_bit);

	if (status & MCHP_PS2_STATUS_RXD_RDY) {
		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

		regs->CTRL = 0x00;
		if (data->callback_isr) {
			data->callback_isr(dev, regs->TRX_BUFF);
		}

		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	} else if (status &
		    (MCHP_PS2_STATUS_TX_TMOUT | MCHP_PS2_STATUS_TX_ST_TMOUT)) {
		/* Clear sticky bits and go to read mode */
		regs->STATUS = MCHP_PS2_STATUS_RW1C_MASK;
		LOG_ERR("TX time out: %0x", status);

		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	} else if (status &
			(MCHP_PS2_STATUS_RX_TMOUT | MCHP_PS2_STATUS_PE | MCHP_PS2_STATUS_FE)) {
		/* catch and clear rx error if any */
		regs->STATUS = MCHP_PS2_STATUS_RW1C_MASK;
	} else if (status & MCHP_PS2_STATUS_TX_IDLE) {
		/* Transfer completed, release the lock to enter low per mode */

		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	}

	/* The control register reverts to RX automatically after
	 * transmitting the data
	 */
	regs->CTRL = MCHP_PS2_CTRL_EN;
}

static const struct ps2_driver_api ps2_xec_driver_api = {
	.config = ps2_xec_configure,
	.read = NULL,
	.write = ps2_xec_write,
	.disable_callback = ps2_xec_inhibit_interface,
	.enable_callback = ps2_xec_enable_interface,
};

static int ps2_xec_init(const struct device *dev)
{
	const struct ps2_xec_config * const cfg = dev->config;
	struct ps2_xec_data * const data = dev->data;
	int ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("XEC PS2 pinctrl init failed (%d)", ret);
		return ret;
	}

	ps2_xec_slp_en_clr(dev);

	k_sem_init(&data->tx_lock, 0, 1);

	cfg->irq_config_func();

	return 0;
}

/* To enable wakeup on the PS2, the DTS needs to have two entries defined
 * in the corresponding PS2 node in the DTS specifying it as a wake source
 * and specifying the PS2DAT GPIO; example as below
 *
 *	wakerx-gpios = <MCHP_GPIO_DECODE_115 GPIO_ACTIVE_HIGH>
 *	wakeup-source;
 */
#ifdef CONFIG_PM_DEVICE
#define XEC_PS2_PM_WAKEUP(n)						\
		.wakeup_source = (uint8_t)DT_INST_PROP_OR(n, wakeup_source, 0),	\
		.wakerx_gpio = GPIO_DT_SPEC_INST_GET_OR(n, wakerx_gpios, {0}),
#else
#define XEC_PS2_PM_WAKEUP(index) /* Not used */
#endif

#define XEC_PS2_PINCTRL_CFG(inst) PINCTRL_DT_INST_DEFINE(inst)
#define XEC_PS2_CONFIG(inst)							\
	static const struct ps2_xec_config ps2_xec_config_##inst = {		\
		.regs = (struct ps2_regs * const)(DT_INST_REG_ADDR(inst)),	\
		.isr_nvic = DT_INST_IRQN(inst),					\
		.girq_id = (uint8_t)(DT_INST_PROP_BY_IDX(inst, girqs, 0)),	\
		.girq_bit = (uint8_t)(DT_INST_PROP_BY_IDX(inst, girqs, 1)),	\
		.girq_id_wk = (uint8_t)(DT_INST_PROP_BY_IDX(inst, girqs, 2)),	\
		.girq_bit_wk = (uint8_t)(DT_INST_PROP_BY_IDX(inst, girqs, 3)),	\
		.pcr_idx = (uint8_t)(DT_INST_PROP_BY_IDX(inst, pcrs, 0)),	\
		.pcr_pos = (uint8_t)(DT_INST_PROP_BY_IDX(inst, pcrs, 1)),	\
		.irq_config_func = ps2_xec_irq_config_func_##inst,		\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),			\
		XEC_PS2_PM_WAKEUP(inst)						\
	}

#define PS2_XEC_DEVICE(i)						\
									\
	static void ps2_xec_irq_config_func_##i(void)			\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(i),				\
			    DT_INST_IRQ(i, priority),			\
			    ps2_xec_isr,				\
			    DEVICE_DT_INST_GET(i), 0);			\
		irq_enable(DT_INST_IRQN(i));				\
	}								\
									\
	static struct ps2_xec_data ps2_xec_port_data_##i;		\
									\
	XEC_PS2_PINCTRL_CFG(i);						\
									\
	XEC_PS2_CONFIG(i);						\
									\
	PM_DEVICE_DT_INST_DEFINE(i, ps2_xec_pm_action);		\
									\
	DEVICE_DT_INST_DEFINE(i, &ps2_xec_init,				\
		PM_DEVICE_DT_INST_GET(i),				\
		&ps2_xec_port_data_##i, &ps2_xec_config_##i,		\
		POST_KERNEL, CONFIG_PS2_INIT_PRIORITY,			\
		&ps2_xec_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PS2_XEC_DEVICE)
