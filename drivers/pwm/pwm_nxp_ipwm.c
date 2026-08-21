/*
 * Copyright 2026 Toradex
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_ipwm

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <fsl_pwm.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pwm_nxp_ipwm, CONFIG_PWM_LOG_LEVEL);

/* The counter, period and sample registers are all 16 bit wide. */
#define NXP_IPWM_COUNTER_MAX 0xFFFFU

/*
 * PWMPR holds the period minus two, the counter spending one extra cycle at
 * rollover and one at the comparison. A PWMPR of 0xFFFE or 0xFFFF is treated by
 * the hardware as 0xFFFD, so the usable period is bounded accordingly.
 */
#define NXP_IPWM_PERIOD_OFFSET 2U

/* Depth of the sample FIFO feeding PWMSAR. */
#define NXP_IPWM_FIFO_DEPTH 4U

/* Bounded spin waiting for the self-clearing software reset bit. */
#define NXP_IPWM_SWR_RETRIES  100U
#define NXP_IPWM_SWR_DELAY_US 1U

struct nxp_ipwm_config {
	DEVICE_MMIO_NAMED_ROM(reg_base);
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct pinctrl_dev_config *pincfg;
	uint16_t prescaler;
};

struct nxp_ipwm_data {
	DEVICE_MMIO_NAMED_RAM(reg_base);
	uint32_t period_cycles;
	pwm_flags_t flags;
	bool configured;
};

#define DEV_CFG(dev)  ((const struct nxp_ipwm_config *)(dev)->config)
#define DEV_DATA(dev) ((struct nxp_ipwm_data *)(dev)->data)

static inline PWM_Type *nxp_ipwm_base(const struct device *dev)
{
	return (PWM_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
}

/*
 * Program PWMCR. The prescaler and the clock source are fixed by devicetree,
 * the output configuration follows the requested polarity. The block is left
 * stopped; the caller starts it once the period and the first sample are in.
 */
static void nxp_ipwm_apply_config(const struct device *dev, pwm_flags_t flags)
{
	const struct nxp_ipwm_config *config = DEV_CFG(dev);
	PWM_Type *base = nxp_ipwm_base(dev);
	pwm_config_t pwm_config;

	PWM_GetDefaultConfig(&pwm_config);

	/* Divide by (prescale + 1); the binding expresses the divider itself. */
	pwm_config.prescale = config->prescaler - 1U;
	pwm_config.clockSource = kPWM_HighFrequencyClock;
	pwm_config.outputConfig = (flags & PWM_POLARITY_INVERTED)
					  ? kPWM_ClearAtRolloverAndSetAtcomparison
					  : kPWM_SetAtRolloverAndClearAtcomparison;

	/* PWM_Init() also ungates the peripheral clock. */
	(void)PWM_Init(base, &pwm_config);
}

/*
 * Stop the block and drain the sample FIFO. Samples already queued were
 * computed for the old period or polarity, so they must not reach the output.
 */
static int nxp_ipwm_reset(const struct device *dev)
{
	PWM_Type *base = nxp_ipwm_base(dev);

	PWM_StopTimer(base);
	PWM_SoftwareReset(base);

	for (unsigned int i = 0U; i < NXP_IPWM_SWR_RETRIES; i++) {
		if ((base->PWMCR & PWM_PWMCR_SWR_MASK) == 0U) {
			return 0;
		}
		k_busy_wait(NXP_IPWM_SWR_DELAY_US);
	}

	LOG_ERR("%s: software reset did not complete", dev->name);

	return -EIO;
}

static int nxp_ipwm_get_cycles_per_sec(const struct device *dev, uint32_t channel,
				       uint64_t *cycles)
{
	const struct nxp_ipwm_config *config = DEV_CFG(dev);
	uint32_t rate;
	int err;

	if (channel != 0U) {
		return -EINVAL;
	}

	err = clock_control_get_rate(config->clock_dev, config->clock_subsys, &rate);
	if (err) {
		return err;
	}

	*cycles = (uint64_t)rate / config->prescaler;

	return 0;
}

static int nxp_ipwm_set_cycles(const struct device *dev, uint32_t channel,
			       uint32_t period_cycles, uint32_t pulse_cycles,
			       pwm_flags_t flags)
{
	struct nxp_ipwm_data *data = DEV_DATA(dev);
	PWM_Type *base = nxp_ipwm_base(dev);
	uint32_t sample;
	int err;

	/* One output per instance. */
	if (channel != 0U) {
		return -EINVAL;
	}

	if ((flags & ~((pwm_flags_t)PWM_POLARITY_MASK)) != 0U) {
		return -ENOTSUP;
	}

	/*
	 * The counter always runs, so a constant output level cannot be
	 * expressed by a zero period.
	 */
	if (period_cycles <= NXP_IPWM_PERIOD_OFFSET) {
		return -ENOTSUP;
	}

	if ((period_cycles - NXP_IPWM_PERIOD_OFFSET) > NXP_IPWM_COUNTER_MAX) {
		LOG_ERR("%s: period of %u cycles exceeds the 16 bit counter",
			dev->name, period_cycles);
		return -ENOTSUP;
	}

	if (pulse_cycles > period_cycles) {
		return -EINVAL;
	}

	/*
	 * Changing the period or the polarity means reprogramming PWMCR and
	 * PWMPR, which the queued samples no longer match.
	 */
	if (!data->configured || period_cycles != data->period_cycles ||
	    flags != data->flags) {
		err = nxp_ipwm_reset(dev);
		if (err) {
			return err;
		}

		nxp_ipwm_apply_config(dev, flags);
		PWM_SetPeriodValue(base, period_cycles - NXP_IPWM_PERIOD_OFFSET);

		data->period_cycles = period_cycles;
		data->flags = flags;
		data->configured = true;
	} else if (PWM_GetFIFOAvailable(base) >= NXP_IPWM_FIFO_DEPTH) {
		/*
		 * A full FIFO would discard the write and raise the write error
		 * flag. Report it instead of blocking; at any sane PWM frequency
		 * the FIFO drains long before the next update.
		 */
		LOG_WRN("%s: sample FIFO full, update dropped", dev->name);
		return -EAGAIN;
	}

	/*
	 * PWMSAR is 16 bit. A sample at or above the period keeps the output at
	 * its active level for the whole cycle, which is the 100% duty case.
	 */
	sample = MIN(pulse_cycles, NXP_IPWM_COUNTER_MAX);

	PWM_SetSampleValue(base, sample);
	PWM_StartTimer(base);

	return 0;
}

static int nxp_ipwm_init(const struct device *dev)
{
	const struct nxp_ipwm_config *config = DEV_CFG(dev);
	uint32_t rate;
	int err;

	DEVICE_MMIO_NAMED_MAP(dev, reg_base, K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("%s: clock controller not ready", dev->name);
		return -ENODEV;
	}

	err = clock_control_get_rate(config->clock_dev, config->clock_subsys, &rate);
	if (err) {
		LOG_ERR("%s: cannot read the PWM root clock rate (%d)", dev->name, err);
		return err;
	}

	if (rate == 0U) {
		LOG_ERR("%s: PWM root clock is not running", dev->name);
		return -EINVAL;
	}

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	/*
	 * Leave the output idle until the first set_cycles(): a period is
	 * required before the block can produce anything meaningful.
	 */
	return nxp_ipwm_reset(dev);
}

static DEVICE_API(pwm, nxp_ipwm_driver_api) = {
	.set_cycles = nxp_ipwm_set_cycles,
	.get_cycles_per_sec = nxp_ipwm_get_cycles_per_sec,
};

#define NXP_IPWM_INIT(n)								\
	PINCTRL_DT_INST_DEFINE(n);							\
											\
	BUILD_ASSERT(DT_INST_PROP(n, nxp_prescaler) >= 1 &&				\
		     DT_INST_PROP(n, nxp_prescaler) <= 4096,				\
		     "nxp,prescaler must be a divider between 1 and 4096");		\
											\
	static const struct nxp_ipwm_config nxp_ipwm_config_##n = {			\
		DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_DRV_INST(n)),			\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),			\
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),	\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),				\
		.prescaler = DT_INST_PROP(n, nxp_prescaler),				\
	};										\
											\
	static struct nxp_ipwm_data nxp_ipwm_data_##n;					\
											\
	DEVICE_DT_INST_DEFINE(n, nxp_ipwm_init, NULL,					\
			      &nxp_ipwm_data_##n, &nxp_ipwm_config_##n,			\
			      POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,			\
			      &nxp_ipwm_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_IPWM_INIT)
