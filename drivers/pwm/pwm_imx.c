/*
 * Copyright (c) 2018, Diego Sueiro <diego.sueiro@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#if defined(CONFIG_PWM_IMX_MCUX)
#include <fsl_device_registers.h>
#include <fsl_clock.h>
#else
#include <soc.h>
#include <device_imx.h>
#endif
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/device_mmio.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwm_imx, CONFIG_PWM_LOG_LEVEL);

#define PWM_PWMSR_FIFOAV_4WORDS	0x4

#if defined(CONFIG_PWM_IMX_MCUX)
/*
 * The MCUXpresso SDK exposes the classic i.MX PWM registers as C struct
 * members (base->PWMCR, base->PWMSR, ...) instead of the legacy
 * PWM_*_REG(base) accessor macros used by the device_imx.h HAL. Provide the
 * accessor macros the driver body relies on so the same code path works with
 * either HAL. The register bitfield macros (PWM_PWMCR_*_MASK/SHIFT,
 * PWM_PWMCR_PRESCALER(), PWM_PWMCR_CLKSRC(), PWM_PWMSR_FIFOAV_MASK/SHIFT) are
 * already provided by the MCUXpresso device header.
 */
#define PWM_PWMCR_REG(base)	((base)->PWMCR)
#define PWM_PWMSR_REG(base)	((base)->PWMSR)
#define PWM_PWMSAR_REG(base)	((base)->PWMSAR)
#define PWM_PWMPR_REG(base)	((base)->PWMPR)

/* Extract the FIFOAV field value from a PWMSR register read. */
#define PWM_PWMSR_FIFOAV_GET(sr) \
	(((uint32_t)(sr) & PWM_PWMSR_FIFOAV_MASK) >> PWM_PWMSR_FIFOAV_SHIFT)

/*
 * Return the PWM peripheral input clock frequency in Hz. The clock root is
 * selected by the peripheral physical base address, which is kept in the
 * device MMIO ROM data because the register accesses use the mapped virtual
 * address instead.
 */
static uint32_t imx_pwm_clock_freq(uintptr_t phys_base)
{
	switch (phys_base) {
	case PWM1_BASE:
		return CLOCK_GetClockRootFreq(kCLOCK_Pwm1ClkRoot);
	case PWM2_BASE:
		return CLOCK_GetClockRootFreq(kCLOCK_Pwm2ClkRoot);
	case PWM3_BASE:
		return CLOCK_GetClockRootFreq(kCLOCK_Pwm3ClkRoot);
	case PWM4_BASE:
		return CLOCK_GetClockRootFreq(kCLOCK_Pwm4ClkRoot);
	default:
		return 0;
	}
}
#else
/*
 * The legacy device_imx.h HAL provides a PWM_PWMSR_FIFOAV(sr) macro that
 * extracts the FIFOAV field. Map the common accessor onto it.
 */
#define PWM_PWMSR_FIFOAV_GET(sr)	PWM_PWMSR_FIFOAV(sr)

/*
 * The MCUXpresso device header (fsl_device_registers.h) already defines the
 * function-like PWM_PWMCR_SWR(x) macro. The legacy device_imx.h HAL only
 * provides PWM_PWMCR_SWR_MASK/SHIFT, so define the accessor here for that path
 * to avoid a redefinition warning when building against MCUXpresso.
 */
#define PWM_PWMCR_SWR(x) (((uint32_t)(((uint32_t)(x)) \
				<<PWM_PWMCR_SWR_SHIFT))&PWM_PWMCR_SWR_MASK)

/*
 * The legacy device_imx.h HAL identifies the clock by the peripheral base
 * address directly. On the Cortex-M ports that use this path there is no MMU,
 * so the mapped address equals the physical base and can be used as-is.
 */
static uint32_t imx_pwm_clock_freq(uintptr_t phys_base)
{
	return get_pwm_clock_freq((PWM_Type *)phys_base);
}
#endif /* CONFIG_PWM_IMX_MCUX */

struct imx_pwm_config {
	DEVICE_MMIO_ROM;
	uint16_t prescaler;
	const struct pinctrl_dev_config *pincfg;
};

struct imx_pwm_data {
	DEVICE_MMIO_RAM;
	uint32_t period_cycles;
};

/* Return the mapped register block base for the given device. */
static inline PWM_Type *imx_pwm_base(const struct device *dev)
{
	return (PWM_Type *)DEVICE_MMIO_GET(dev);
}

/* Return the peripheral physical base address, used to select the clock. */
static inline uintptr_t imx_pwm_phys_base(const struct device *dev)
{
#ifdef DEVICE_MMIO_IS_IN_RAM
	/* MMU platforms keep the physical base separate from the mapped one. */
	return DEVICE_MMIO_ROM_PTR(dev)->phys_addr;
#else
	/* No MMU: the mapped address is the physical base. */
	return DEVICE_MMIO_GET(dev);
#endif
}

static bool imx_pwm_is_enabled(PWM_Type *base)
{
	return PWM_PWMCR_REG(base) & PWM_PWMCR_EN_MASK;
}

static int imx_pwm_get_cycles_per_sec(const struct device *dev, uint32_t pwm,
				      uint64_t *cycles)
{
	const struct imx_pwm_config *config = dev->config;

	*cycles = imx_pwm_clock_freq(imx_pwm_phys_base(dev)) >> config->prescaler;

	return 0;
}

static int imx_pwm_set_cycles(const struct device *dev, uint32_t channel,
			      uint32_t period_cycles, uint32_t pulse_cycles,
			      pwm_flags_t flags)
{
	const struct imx_pwm_config *config = dev->config;
	struct imx_pwm_data *data = dev->data;
	PWM_Type *base = imx_pwm_base(dev);
	unsigned int period_ms;
	bool enabled = imx_pwm_is_enabled(base);
	int wait_count = 0, fifoav;
	uint32_t cr, sr;


	if (period_cycles == 0U) {
		LOG_ERR("Channel can not be set to inactive level");
		return -ENOTSUP;
	}

	if (flags) {
		/* PWM polarity not supported (yet?) */
		return -ENOTSUP;
	}

	LOG_DBG("enabled=%d, pulse_cycles=%d, period_cycles=%d,"
		    " duty_cycle=%d\n", enabled, pulse_cycles, period_cycles,
		    (pulse_cycles * 100U / period_cycles));

	/*
	 * i.MX PWMv2 has a 4-word sample FIFO.
	 * In order to avoid FIFO overflow issue, we do software reset
	 * to clear all sample FIFO if the controller is disabled or
	 * wait for a full PWM cycle to get a relinquished FIFO slot
	 * when the controller is enabled and the FIFO is fully loaded.
	 */
	if (enabled) {
		sr = PWM_PWMSR_REG(base);
		fifoav = PWM_PWMSR_FIFOAV_GET(sr);
		if (fifoav == PWM_PWMSR_FIFOAV_4WORDS) {
			period_ms = (imx_pwm_clock_freq(imx_pwm_phys_base(dev)) >>
					config->prescaler) * MSEC_PER_SEC;
			k_sleep(K_MSEC(period_ms));

			sr = PWM_PWMSR_REG(base);
			if (fifoav == PWM_PWMSR_FIFOAV_GET(sr)) {
				LOG_WRN("there is no free FIFO slot\n");
			}
		}
	} else {
		PWM_PWMCR_REG(base) = PWM_PWMCR_SWR(1);
		do {
			k_sleep(K_MSEC(1));
			cr = PWM_PWMCR_REG(base);
		} while ((PWM_PWMCR_SWR(cr)) &&
			 (++wait_count < CONFIG_PWM_PWMSWR_LOOP));

		if (PWM_PWMCR_SWR(cr)) {
			LOG_WRN("software reset timeout\n");
		}

	}

	/*
	 * according to imx pwm RM, the real period value should be
	 * PERIOD value in PWMPR plus 2.
	 */
	if (period_cycles > 2) {
		period_cycles -= 2U;
	} else {
		return -EINVAL;
	}

	PWM_PWMSAR_REG(base) = pulse_cycles;

	if (data->period_cycles != period_cycles) {
		LOG_WRN("Changing period cycles from %d to %d in %s",
			    data->period_cycles, period_cycles,
			    dev->name);

		data->period_cycles = period_cycles;
		PWM_PWMPR_REG(base) = period_cycles;
	}

	cr = PWM_PWMCR_EN_MASK | PWM_PWMCR_PRESCALER(config->prescaler) |
		PWM_PWMCR_DOZEN_MASK | PWM_PWMCR_WAITEN_MASK |
		PWM_PWMCR_DBGEN_MASK | PWM_PWMCR_CLKSRC(2);

	PWM_PWMCR_REG(base) = cr;

	return 0;
}

static int imx_pwm_init(const struct device *dev)
{
	const struct imx_pwm_config *config = dev->config;
	struct imx_pwm_data *data = dev->data;
	int err;

	/*
	 * Map the peripheral MMIO region through the device MMIO API instead of
	 * relying on a static mmu_regions.c entry. On the Cortex-M ports without
	 * an MMU this resolves to the physical address at build time.
	 */
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	PWM_PWMPR_REG(imx_pwm_base(dev)) = data->period_cycles;

	return 0;
}

static DEVICE_API(pwm, imx_pwm_driver_api) = {
	.set_cycles = imx_pwm_set_cycles,
	.get_cycles_per_sec = imx_pwm_get_cycles_per_sec,
};

#define PWM_IMX_INIT(n)							\
	PINCTRL_DT_INST_DEFINE(n);					\
	static const struct imx_pwm_config imx_pwm_config_##n = {	\
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),			\
		.prescaler = DT_INST_PROP(n, prescaler),		\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
	};								\
									\
	static struct imx_pwm_data imx_pwm_data_##n;			\
									\
	DEVICE_DT_INST_DEFINE(n, &imx_pwm_init, NULL,			\
			    &imx_pwm_data_##n,				\
			    &imx_pwm_config_##n, POST_KERNEL,		\
			    CONFIG_PWM_INIT_PRIORITY,			\
			    &imx_pwm_driver_api);

#if DT_HAS_COMPAT_STATUS_OKAY(fsl_imx27_pwm)
#define DT_DRV_COMPAT fsl_imx27_pwm
DT_INST_FOREACH_STATUS_OKAY(PWM_IMX_INIT)
#endif
