/*
 * Copyright (c) 2020 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT xlnx_xps_timer_1_00_a_pwm

#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(xlnx_axi_timer_pwm, CONFIG_PWM_LOG_LEVEL);

/* AXI Timer v2.0 registers offsets (See Xilinx PG079 for details) */
#define TCSR0_OFFSET 0x00
#define TLR0_OFFSET  0x04
#define TCR0_OFFSET  0x08
#define TCSR1_OFFSET 0x10
#define TLR1_OFFSET  0x14
#define TCR1_OFFSET  0x18

/* TCSRx bit definitions */
#define TCSR_MDT   BIT(0)
#define TCSR_UDT   BIT(1)
#define TCSR_GENT  BIT(2)
#define TCSR_CAPT  BIT(3)
#define TCSR_ARHT  BIT(4)
#define TCSR_LOAD  BIT(5)
#define TCSR_ENIT  BIT(6)
#define TCSR_ENT   BIT(7)
#define TCSR_TINT  BIT(8)
#define TCSR_PWMA  BIT(9)
#define TCSR_ENALL BIT(10)
#define TCSR_CASC  BIT(11)

/* Generate PWM mode, count-down, auto-reload */
#define TCSR_PWM (TCSR_UDT | TCSR_GENT | TCSR_ARHT | TCSR_PWMA)

struct xlnx_axi_timer_config {
	mm_reg_t base;
	uint32_t cycles_max;
	uint32_t freq;
};

static inline uint32_t xlnx_axi_timer_read32(const struct device *dev,
					     mm_reg_t offset)
{
	const struct xlnx_axi_timer_config *config = dev->config;

	return sys_read32(config->base + offset);
}

static inline void xlnx_axi_timer_write32(const struct device *dev,
					  uint32_t value,
					  mm_reg_t offset)
{
	const struct xlnx_axi_timer_config *config = dev->config;

	sys_write32(value, config->base + offset);
}

static int xlnx_axi_timer_set_cycles(const struct device *dev, uint32_t channel,
				     uint32_t period_cycles,
				     uint32_t pulse_cycles, pwm_flags_t flags)
{
	const struct xlnx_axi_timer_config *config = dev->config;
	uint32_t tcsr0 = TCSR_PWM;
	uint32_t tcsr1 = TCSR_PWM;
	uint32_t tlr0;
	uint32_t tlr1;

	if (channel != 0) {
		return -ENOTSUP;
	}

	LOG_DBG("period = 0x%08x, pulse = 0x%08x", period_cycles, pulse_cycles);

	if (pulse_cycles == 0) {
		LOG_DBG("setting constant inactive level");

		if (flags & PWM_POLARITY_INVERTED) {
			tcsr0 |= TCSR_ENT;
		} else {
			tcsr1 |= TCSR_ENT;
		}
	} else if (pulse_cycles == period_cycles) {
		LOG_DBG("setting constant active level");

		if (flags & PWM_POLARITY_INVERTED) {
			tcsr1 |= TCSR_ENT;
		} else {
			tcsr0 |= TCSR_ENT;
		}
	} else {
		LOG_DBG("setting normal pwm");

		if (period_cycles < 2) {
			LOG_ERR("period cycles too narrow");
			return -ENOTSUP;
		}

		/* PWM_PERIOD = (TLR0 + 2) * AXI_CLOCK_PERIOD */
		tlr0 = period_cycles - 2;

		if (tlr0 > config->cycles_max) {
			LOG_ERR("tlr0 out of range (0x%08x > 0x%08x)", tlr0,
				config->cycles_max);
			return -ENOTSUP;
		}

		if (flags & PWM_POLARITY_INVERTED) {
			/*
			 * Since this is a single-channel PWM controller (with
			 * no other channels to phase align with) inverse
			 * polarity can be achieved simply by inverting the
			 * pulse.
			 */

			if ((period_cycles - pulse_cycles) < 2) {
				LOG_ERR("pulse cycles too narrow");
				return -ENOTSUP;
			}

			/* PWM_HIGH_TIME = (TLR1 + 2) * AXI_CLOCK_PERIOD */
			tlr1 = period_cycles - pulse_cycles - 2;
		} else {
			if (pulse_cycles < 2) {
				LOG_ERR("pulse cycles too narrow");
				return -ENOTSUP;
			}

			/* PWM_HIGH_TIME = (TLR1 + 2) * AXI_CLOCK_PERIOD */
			tlr1 = pulse_cycles - 2;
		}

		LOG_DBG("tlr0 = 0x%08x, tlr1 = 0x%08x", tlr0, tlr1);

		/* Stop both timers */
		xlnx_axi_timer_write32(dev, TCSR_PWM, TCSR0_OFFSET);
		xlnx_axi_timer_write32(dev, TCSR_PWM, TCSR1_OFFSET);

		/* Load period cycles */
		xlnx_axi_timer_write32(dev, tlr0, TLR0_OFFSET);
		xlnx_axi_timer_write32(dev, TCSR_PWM | TCSR_LOAD, TCSR0_OFFSET);

		/* Load pulse cycles */
		xlnx_axi_timer_write32(dev, tlr1, TLR1_OFFSET);
		xlnx_axi_timer_write32(dev, TCSR_PWM | TCSR_LOAD, TCSR1_OFFSET);

		/* Start both timers */
		tcsr1 |= TCSR_ENALL;
	}

	xlnx_axi_timer_write32(dev, tcsr0, TCSR0_OFFSET);
	xlnx_axi_timer_write32(dev, tcsr1, TCSR1_OFFSET);

	return 0;
}

static int xlnx_axi_timer_get_cycles_per_sec(const struct device *dev,
					     uint32_t channel, uint64_t *cycles)
{
	const struct xlnx_axi_timer_config *config = dev->config;

	ARG_UNUSED(channel);

	*cycles = config->freq;

	return 0;
}

static int xlnx_axi_timer_init(const struct device *dev)
{
	return 0;
}

static const struct pwm_driver_api xlnx_axi_timer_driver_api = {
	.set_cycles = xlnx_axi_timer_set_cycles,
	.get_cycles_per_sec = xlnx_axi_timer_get_cycles_per_sec,
};

#define XLNX_AXI_TIMER_ASSERT_PROP_VAL(n, prop, val, str)	\
	BUILD_ASSERT(DT_INST_PROP(n, prop) == val, str)

#define XLNX_AXI_TIMER_INIT(n)						\
	XLNX_AXI_TIMER_ASSERT_PROP_VAL(n, xlnx_gen0_assert, 1,		\
				   "xlnx,gen0-assert must be 1 for pwm"); \
	XLNX_AXI_TIMER_ASSERT_PROP_VAL(n, xlnx_gen1_assert, 1,		\
				   "xlnx,gen1-assert must be 1 for pwm"); \
	XLNX_AXI_TIMER_ASSERT_PROP_VAL(n, xlnx_one_timer_only, 0,	\
				   "xlnx,one-timer-only must be 0 for pwm"); \
									\
	static struct xlnx_axi_timer_config xlnx_axi_timer_config_##n = { \
		.base = DT_INST_REG_ADDR(n),				\
		.freq = DT_INST_PROP(n, clock_frequency),		\
		.cycles_max =						\
			GENMASK(DT_INST_PROP(n, xlnx_count_width) - 1, 0), \
	};								\
									\
	DEVICE_DT_INST_DEFINE(n, &xlnx_axi_timer_init,			\
			    NULL, NULL,					\
			    &xlnx_axi_timer_config_##n,			\
			    POST_KERNEL,				\
			    CONFIG_PWM_INIT_PRIORITY,			\
			    &xlnx_axi_timer_driver_api)

DT_INST_FOREACH_STATUS_OKAY(XLNX_AXI_TIMER_INIT);
