/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_pwm_g1

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control/mchp_sam_pmc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pwm/pwm_mchp_pwm_g1.h>
#include <zephyr/drivers/pwm/pwm_utils.h>
#ifdef CONFIG_PWM_EVENT
#include <zephyr/irq.h>
#endif /* CONFIG_PWM_EVENT */
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/sys_io.h>

LOG_MODULE_REGISTER(mchp_pwm, CONFIG_PWM_LOG_LEVEL);

struct mchp_pwm_config {
	DEVICE_MMIO_ROM;
	const struct sam_clk_cfg clock_cfg;
	const struct pinctrl_dev_config *pcfg;
	uint8_t pre_a, div_a, pre_b, div_b;
#ifdef CONFIG_PWM_EVENT
	void (*irq_config)(void);
#endif /* CONFIG_PWM_EVENT */
};

struct mchp_pwm_data {
	DEVICE_MMIO_RAM;
#ifdef CONFIG_PWM_EVENT
	sys_slist_t event_callbacks;
	struct k_spinlock lock;
#endif /* CONFIG_PWM_EVENT */
};

static int mchp_pwm_get_cycles_per_sec(const struct device *dev, uint32_t channel, uint64_t *cycles)
{
	const struct device *const pmc = DEVICE_DT_GET(DT_NODELABEL(pmc));
	const struct mchp_pwm_config *config = dev->config;
	pwm_ch_num_registers_t *ptr_ch_reg;
	uint8_t cmr, cpre;
	uint8_t prescaler, divider;
	uint32_t rate;
	uint32_t clk;
	int ret;

	if (channel >= PWM_CH_NUM_NUMBER) {
		LOG_ERR("channel %d is invalid", channel);
		return -EINVAL;
	}

	ptr_ch_reg = &((pwm_registers_t *)DEVICE_MMIO_GET(dev))->PWM_CH_NUM[channel];

	cmr = sys_read32((mem_addr_t)ptr_ch_reg + PWM_CMR_REG_OFST);

	cpre = FIELD_GET(PWM_CMR_CPRE_Msk, cmr);

	clk = sys_read32(DEVICE_MMIO_GET(dev) + PWM_CLK_REG_OFST);
	switch (cpre) {
	case PWM_CMR_CPRE_CLKA_Val:
		prescaler = FIELD_GET(PWM_CLK_PREA_Msk, clk);
		divider = FIELD_GET(PWM_CLK_DIVA_Msk, clk);
		break;
	case PWM_CMR_CPRE_CLKB_Val:
		prescaler = FIELD_GET(PWM_CLK_PREB_Msk, clk);
		divider = FIELD_GET(PWM_CLK_DIVB_Msk, clk);
		break;
	default:
		prescaler = cpre;
		divider = 1;
		break;
	}

	if (prescaler > PWM_CMR_CPRE_MCK_DIV_1024_Val) {
		LOG_ERR("prescaler value %d is invalid", prescaler);
		return -EINVAL;
	}

	if (divider == 0) {
		LOG_ERR("clock chosen is turned off");
		return -EINVAL;
	}

	ret = clock_control_get_rate(pmc,
				     (clock_control_subsys_t)(uintptr_t)&config->clock_cfg,
				     &rate);
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("clock rate %d Hz, prescaler %d, divider %d", rate, prescaler, divider);

	*cycles = (rate >> prescaler) / divider;

	LOG_DBG("channel %d, cycles per second %" PRIu64, channel, *cycles);

	return 0;
}

static int mchp_pwm_set_cycles(const struct device *dev, uint32_t channel, uint32_t period_cycles,
			       uint32_t pulse_cycles, pwm_flags_t flags)
{
	mem_addr_t ch_reg_ofst;
	uint32_t cmr;

	LOG_DBG("channel %d, period_cycles %d, pulse_cycles %d, flags 0x%x",
		channel, period_cycles, pulse_cycles, flags);

	if (channel >= PWM_CH_NUM_NUMBER) {
		LOG_ERR("channel %d is invalid", channel);
		return -EINVAL;
	}

	if (period_cycles == 0U) {
		LOG_ERR("period cycles should not be 0");
		return -ENOTSUP;
	}

	if ((period_cycles > PWM_CPRD_CPRD_Msk) || (pulse_cycles > PWM_CDTY_CDTY_Msk)) {
		LOG_ERR("period cycles or pulse cycles invalid");
		return -ENOTSUP;
	}

	/* Select clock */
	cmr = PWM_CMR_CPRE(FIELD_GET(PWM_PRESCALER_MASK, flags));

	if ((flags & PWM_POLARITY_MASK) == PWM_POLARITY_NORMAL) {
		cmr |= PWM_CMR_CPOL_Msk;
	}

	ch_reg_ofst = (mem_addr_t)&((pwm_registers_t *)DEVICE_MMIO_GET(dev))->PWM_CH_NUM[channel];

	/* Disable the output if changing polarity (or clock) */
	if (sys_read32(ch_reg_ofst + PWM_CMR_REG_OFST) != cmr) {
		sys_write32(BIT(channel), DEVICE_MMIO_GET(dev) + PWM_DIS_REG_OFST);

		sys_write32(cmr, ch_reg_ofst + PWM_CMR_REG_OFST);
		sys_write32(period_cycles, ch_reg_ofst + PWM_CPRD_REG_OFST);
		sys_write32(pulse_cycles, ch_reg_ofst + PWM_CDTY_REG_OFST);
	} else {
		/* Update period and pulse using the update registers, so that the
		 * change is triggered at the next PWM period.
		 */
		sys_write32(period_cycles, ch_reg_ofst + PWM_CPRDUPD_REG_OFST);
		sys_write32(pulse_cycles, ch_reg_ofst + PWM_CDTYUPD_REG_OFST);
	}

	/* Enable the output */
	sys_write32(BIT(channel), DEVICE_MMIO_GET(dev) + PWM_ENA_REG_OFST);

	return 0;
}

#ifdef CONFIG_PWM_EVENT
static void update_interrupts(const struct device *dev)
{
	struct mchp_pwm_data *data = dev->data;
	struct pwm_event_callback *cb, *tmp;
	uint32_t pwm_ier1 = 0;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&data->event_callbacks, cb, tmp, node) {
		if ((cb->event_mask & PWM_EVENT_TYPE_PERIOD) != 0) {
			pwm_ier1 |= PWM_IER1_CHID0_Msk << cb->channel;
		}
		if ((cb->event_mask & PWM_EVENT_TYPE_FAULT) != 0) {
			pwm_ier1 |= PWM_IER1_FCHID0_Msk << cb->channel;
		}
	}

	/* Disable all interrupts */
	sys_write32(PWM_IDR1_Msk, DEVICE_MMIO_GET(dev) + PWM_IDR1_REG_OFST);

	/* Dummy read to clear status register */
	(void)sys_read32(DEVICE_MMIO_GET(dev) + PWM_ISR1_REG_OFST);

	/* Reenable interrupts */
	sys_write32(pwm_ier1, DEVICE_MMIO_GET(dev) + PWM_IER1_REG_OFST);
}

static void mchp_pwm_isr(const struct device *dev)
{
	struct mchp_pwm_data *data = dev->data;
	pwm_events_t events;

	uint32_t status = sys_read32(DEVICE_MMIO_GET(dev) + PWM_ISR1_REG_OFST);

	for (uint32_t i = 0; i < PWM_CH_NUM_NUMBER; i++) {
		events = 0;
		if ((status & (PWM_ISR1_CHID0_Msk << i)) != 0) {
			events |= PWM_EVENT_TYPE_PERIOD;
		}
		if ((status & (PWM_ISR1_FCHID0_Msk << i)) != 0) {
			events |= PWM_EVENT_TYPE_FAULT;
		}

		if (events > 0) {
			pwm_fire_event_callbacks(&data->event_callbacks, dev, i, events);
		}
	}
}

static int mchp_pwm_manage_event_callback(const struct device *dev,
					  struct pwm_event_callback *callback, bool set)
{
	struct mchp_pwm_data *data = dev->data;
	int ret;

	ret = pwm_manage_event_callback(&data->event_callbacks, callback, set);
	if (ret < 0) {
		return ret;
	}

	K_SPINLOCK(&data->lock) {
		update_interrupts(dev);
	}

	return 0;
}
#endif /* CONFIG_PWM_EVENT */

static int mchp_pwm_init(const struct device *dev)
{
	const struct device *const pmc = DEVICE_DT_GET(DT_NODELABEL(pmc));
	const struct mchp_pwm_config *config = dev->config;
	int retval;

#ifdef CONFIG_PWM_EVENT
	struct mchp_pwm_data *data = dev->data;

	sys_slist_init(&data->event_callbacks);
	config->irq_config();
#endif /* CONFIG_PWM_EVENT */

	/* Enable PWM clock in PMC */
	(void)clock_control_on(pmc, (clock_control_subsys_t)(uintptr_t)&config->clock_cfg);

	retval = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (retval < 0) {
		LOG_ERR("PWM pin init failed");
		return retval;
	}

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	/* Configure the clock A and clock B that might be used by all the channels */
	sys_write32(PWM_CLK_PREB(config->pre_b) | PWM_CLK_DIVB(config->div_b) |
		    PWM_CLK_PREA(config->pre_a) | PWM_CLK_DIVA(config->div_a),
		    DEVICE_MMIO_GET(dev) + PWM_CLK_REG_OFST);

	return 0;
}

static DEVICE_API(pwm, mchp_pwm_driver_api) = {
	.set_cycles = mchp_pwm_set_cycles,
	.get_cycles_per_sec = mchp_pwm_get_cycles_per_sec,
#ifdef CONFIG_PWM_EVENT
	.manage_event_callback = mchp_pwm_manage_event_callback,
#endif /* CONFIG_PWM_EVENT */
};

#define MCHP_PWM_INTERRUPT_INIT(inst)								\
	static void mchp_pwm_irq_config_##inst(void)						\
	{											\
		IRQ_CONNECT(DT_INST_IRQN(inst), 0, mchp_pwm_isr, DEVICE_DT_INST_GET(inst), 0);	\
		irq_enable(DT_INST_IRQN(inst));							\
	}

#define MCHP_PWM_INST_INIT(inst)								\
	PINCTRL_DT_INST_DEFINE(inst);								\
												\
	IF_ENABLED(CONFIG_PWM_EVENT, (MCHP_PWM_INTERRUPT_INIT(inst)))				\
												\
	static const struct mchp_pwm_config mchp_pwm_config_##inst = {				\
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(inst)),					\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),					\
		.clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(inst),					\
		.pre_a = DT_INST_ENUM_IDX(inst, clka_prescaler),				\
		.div_a = DT_INST_PROP(inst, clka_divide_factor),				\
		.pre_b = DT_INST_ENUM_IDX(inst, clkb_prescaler),				\
		.div_b = DT_INST_PROP(inst, clkb_divide_factor),				\
		IF_ENABLED(CONFIG_PWM_EVENT, (.irq_config = mchp_pwm_irq_config_##inst,))	\
	};											\
												\
	static struct mchp_pwm_data mchp_pwm_data_##inst;					\
												\
	DEVICE_DT_INST_DEFINE(inst,								\
			      &mchp_pwm_init, NULL,						\
			      &mchp_pwm_data_##inst,						\
			      &mchp_pwm_config_##inst,						\
			      POST_KERNEL,							\
			      CONFIG_PWM_INIT_PRIORITY,						\
			      &mchp_pwm_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MCHP_PWM_INST_INIT)
