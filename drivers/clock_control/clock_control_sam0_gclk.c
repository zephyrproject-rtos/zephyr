/*
 * Copyright (c) 2019 Derek Hageman <hageman@inthat.cloud>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <clock_control.h>

struct sam0_gclk_config {
	u32_t clock_frequency;
	u8_t gen_id;
};


static int sam0_gclk_on(struct device *dev,
			clock_control_subsys_t sub_system)
{
	const struct sam0_gclk_config *const cfg = dev->config->config_info;

#ifdef GCLK_PCHCTRL_GEN
	GCLK->PCHCTRL[(u32_t)sub_system].reg = GCLK_PCHCTRL_GEN(cfg->gen_id) |
					       GCLK_PCHCTRL_CHEN;
#else
	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_GEN(cfg->gen_id) |
			    GCLK_CLKCTRL_CLKEN |
			    GCLK_CLKCTRL_ID((u32_t)sub_system);
#endif

	return 0;
}

static int sam0_gclk_off(struct device *dev,
			 clock_control_subsys_t sub_system)
{
	const struct sam0_gclk_config *const cfg = dev->config->config_info;

#ifdef GCLK_PCHCTRL_GEN
	GCLK->PCHCTRL[(u32_t)sub_system].reg = GCLK_PCHCTRL_GEN(cfg->gen_id);
#else
	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_GEN(cfg->gen_id) |
			    GCLK_CLKCTRL_ID((u32_t)sub_system);
#endif

	return 0;
}

static int sam0_gclk_get_rate(struct device *dev,
			      clock_control_subsys_t sub_system,
			      u32_t *rate)
{
	const struct sam0_gclk_config *const cfg = dev->config->config_info;

	ARG_UNUSED(sub_system);

	*rate = cfg->clock_frequency;

	return 0;
}

static int sam0_gclk_init(struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static const struct clock_control_driver_api sam0_gclk_api = {
	.on = sam0_gclk_on,
	.off = sam0_gclk_off,
	.get_rate = sam0_gclk_get_rate,
};

#define SAM0_GCLK_DECLARE(n)						      \
	static const struct sam0_gclk_config sam0_gclk_config_##n = {	      \
		.clock_frequency = DT_ATMEL_SAM0_GCLK_GCLK##n##_CLOCK_FREQUENCY,\
		.gen_id = n,						      \
	};								      \
	DEVICE_AND_API_INIT(sam0_gclk_##n,				      \
			    DT_ATMEL_SAM0_GCLK_GCLK##n##_CLOCK_OUTPUT_NAMES_0,\
			    sam0_gclk_init, NULL, &sam0_gclk_config_##n,      \
			    PRE_KERNEL_1, 0, &sam0_gclk_api)

#ifdef DT_ATMEL_SAM0_GCLK_GCLK0_CLOCK_FREQUENCY
SAM0_GCLK_DECLARE(0);
#endif

#ifdef DT_ATMEL_SAM0_GCLK_GCLK1_CLOCK_FREQUENCY
SAM0_GCLK_DECLARE(1);
#endif

#ifdef DT_ATMEL_SAM0_GCLK_GCLK2_CLOCK_FREQUENCY
SAM0_GCLK_DECLARE(2);
#endif

#ifdef DT_ATMEL_SAM0_GCLK_GCLK3_CLOCK_FREQUENCY
SAM0_GCLK_DECLARE(3);
#endif

#ifdef DT_ATMEL_SAM0_GCLK_GCLK4_CLOCK_FREQUENCY
SAM0_GCLK_DECLARE(4);
#endif

#ifdef DT_ATMEL_SAM0_GCLK_GCLK5_CLOCK_FREQUENCY
SAM0_GCLK_DECLARE(5);
#endif

#ifdef DT_ATMEL_SAM0_GCLK_GCLK6_CLOCK_FREQUENCY
SAM0_GCLK_DECLARE(6);
#endif

#ifdef DT_ATMEL_SAM0_GCLK_GCLK7_CLOCK_FREQUENCY
SAM0_GCLK_DECLARE(7);
#endif

#ifdef DT_ATMEL_SAM0_GCLK_GCLK8_CLOCK_FREQUENCY
SAM0_GCLK_DECLARE(8);
#endif

#ifdef DT_ATMEL_SAM0_GCLK_GCLK9_CLOCK_FREQUENCY
SAM0_GCLK_DECLARE(9);
#endif

#ifdef DT_ATMEL_SAM0_GCLK_GCLK10_CLOCK_FREQUENCY
SAM0_GCLK_DECLARE(10);
#endif

#ifdef DT_ATMEL_SAM0_GCLK_GCLK11_CLOCK_FREQUENCY
SAM0_GCLK_DECLARE(11);
#endif
