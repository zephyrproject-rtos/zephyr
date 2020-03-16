/*
 * Copyright (c) 2019 Max van Kessel
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <soc.h>
#include <device.h>
#include <drivers/clock_control/stm32_clock_control.h>
#include <mfd/mfd_timer_stm32.h>

#define DEV_CFG(dev)							\
	((const struct mfd_timer_stm32_config * const)			\
			(dev)->config->config_info)
#define DEV_DATA(dev)							\
	((struct mfd_timer_stm32_data * const)(dev)->driver_data)

struct mfd_timer_stm32_config {
	u32_t tim_base;		    /**< Timer base */
	struct stm32_pclken pclken; /**< subsystem driving this peripheral */

	u8_t align_mode;
	u8_t dir;
	u8_t msm;
	u8_t slave_mode;
	u8_t slave_trig;
	u8_t master_trig;
	u32_t prescaler;
};

enum {
	ALIGN_EDGE = 0,
	ALIGN_CENTER_1,
	ALIGN_CENTER_2,
	ALIGN_CENTER_3
};

static inline void tim_stm32_get_clock(struct device *dev)
{
	struct mfd_timer_stm32_data *data = DEV_DATA(dev);
	struct device *clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);

	__ASSERT_NO_MSG(clk);

	data->clock = clk;
}

static u32_t tim_stm32_get_rate(u32_t bus_clk,
                                clock_control_subsys_t *sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);
	u32_t tim_clk, apb_psc;

	if (pclken->bus == STM32_CLOCK_BUS_APB1) {
		apb_psc = CONFIG_CLOCK_STM32_APB1_PRESCALER;
	}
#if !defined(CONFIG_SOC_SERIES_STM32F0X) && !defined(CONFIG_SOC_SERIES_STM32G0X)
	else {
		apb_psc = CONFIG_CLOCK_STM32_APB2_PRESCALER;
	}
#endif

	/*
	 * If the APB prescaler equals 1, the timer clock frequencies
	 * are set to the same frequency as that of the APB domain.
	 * Otherwise, they are set to twice (×2) the frequency of the
	 * APB domain.
	 */
	if (apb_psc == 1U) {
		tim_clk = bus_clk;
	} else	{
		tim_clk = bus_clk * 2U;
	}

	return tim_clk;
}

static int init(struct device *dev)
{
	int err = -EIO;
	const struct mfd_timer_stm32_config *cfg = DEV_CFG(dev);
	struct mfd_timer_stm32_data *data = DEV_DATA(dev);

	tim_stm32_get_clock(dev);

	/* enable clock */
	if (clock_control_on(data->clock,
				(clock_control_subsys_t *)&cfg->pclken) == 0) {
		err = 0;
	}

	if (err == 0) {
		TIM_TypeDef *tim = (TIM_TypeDef *)cfg->tim_base;
		u32_t mode = 0;

		data->tim = tim;

		/* TODO: create binding for me */
		LL_TIM_SetClockSource(tim, LL_TIM_CLOCKSOURCE_INTERNAL);

		if (cfg->prescaler > 0) {
			LL_TIM_SetPrescaler(tim, cfg->prescaler - 1);
		} else {
			LL_TIM_SetPrescaler(tim, 0);
		}

		/* TODO find a more consistent solution for all soc's */
		if (cfg->align_mode == ALIGN_EDGE) {
			mode = cfg->dir << TIM_CR1_DIR_Pos;
		} else {
			mode = cfg->align_mode << TIM_CR1_CMS_Pos;
		}

		LL_TIM_SetCounterMode(tim, mode);

		if (cfg->msm > 0) {
			/* Trigger input delayed to allow synchronization */
			LL_TIM_EnableMasterSlaveMode(tim);
		}

		mode = cfg->slave_mode << TIM_SMCR_SMS_Pos;
		LL_TIM_SetSlaveMode(tim, mode);

		if (cfg->slave_mode != 0) {
			mode = cfg->slave_trig << TIM_SMCR_TS_Pos;
			LL_TIM_SetTriggerInput(tim, mode);
		}

		mode = cfg->master_trig << TIM_CR2_MMS_Pos;
		LL_TIM_SetTriggerOutput(tim, mode);

		/* TODO: create binding for me ?*/
		LL_TIM_DisableARRPreload(tim);
	}

	return err;
}

static void enable(struct device *dev)
{
	const struct mfd_timer_stm32_config *cfg = DEV_CFG(dev);

	/* Timer is enabled by master timer if slave mode is set */
	if ((cfg->slave_mode << TIM_SMCR_SMS_Pos) != LL_TIM_SLAVEMODE_TRIGGER) {
		struct mfd_timer_stm32_data *data = DEV_DATA(dev);

		LL_TIM_EnableCounter(data->tim);
	}
}

static void disable(struct device *dev)
{
	struct mfd_timer_stm32_data *data = DEV_DATA(dev);

	LL_TIM_DisableCounter(data->tim);
}

static int get_cycles_per_sec(struct device *dev, u64_t *cycles)
{
	int err = -EINVAL;
	const struct mfd_timer_stm32_config *cfg = DEV_CFG(dev);
	struct mfd_timer_stm32_data *data = DEV_DATA(dev);
	u32_t bus_clk, tim_clk;

	if (cycles != NULL) {
		/* Timer clock depends on APB prescaler */
		err = clock_control_get_rate(data->clock,
				(clock_control_subsys_t *)&cfg->pclken,
				&bus_clk);

		if (err >= 0) {
			tim_clk = tim_stm32_get_rate(bus_clk,
					(clock_control_subsys_t *)&cfg->pclken);

			*cycles = (u64_t) (tim_clk / (cfg->prescaler));

			err = 0;
		}
	}
	return err;
}

static const struct mfd_timer_stm32 api = {
	.enable = enable,
	.disable = disable,
	.get_cycles_per_sec = get_cycles_per_sec,
};

#define TIMER_DEVICE_INIT(n)						\
static struct mfd_timer_stm32_data mfd_timer_stm32_dev_data_ ## n;	\
static const struct mfd_timer_stm32_config mfd_timer_stm32_dev_cfg_ ## n = { \
	.tim_base = DT_INST_## n ##_ST_STM32_TIMERS_BASE_ADDRESS,	\
	.align_mode = DT_INST_## n ##_ST_STM32_TIMERS_ST_ALIGN_MODE_ENUM,\
	.dir = DT_INST_## n ##_ST_STM32_TIMERS_ST_COUNTER_DIR_ENUM,	\
	.msm = DT_INST_## n ##_ST_STM32_TIMERS_ST_MASTER_SLAVE_MODE,	\
	.slave_mode = DT_INST_## n ##_ST_STM32_TIMERS_ST_SLAVE_MODE,	\
	.slave_trig = DT_INST_## n ##_ST_STM32_TIMERS_ST_SLAVE_TRIGGER_IN, \
	.master_trig = DT_INST_## n ##_ST_STM32_TIMERS_ST_MASTER_TRIGGER_OUT,\
	.prescaler = DT_INST_## n ##_ST_STM32_TIMERS_ST_PRESCALER,	\
	.pclken = {							\
		.bus = DT_INST_## n ##_ST_STM32_TIMERS_CLOCK_BUS,	\
		.enr = DT_INST_## n ##_ST_STM32_TIMERS_CLOCK_BITS },	\
};									\
									\
DEVICE_AND_API_INIT(timer_stm32_ ## n,					\
		    DT_INST_## n ##_ST_STM32_TIMERS_LABEL,		\
		    &init,						\
		    &mfd_timer_stm32_dev_data_ ## n,			\
		    &mfd_timer_stm32_dev_cfg_ ## n,			\
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
		    &api)

#ifdef DT_INST_0_ST_STM32_TIMERS
TIMER_DEVICE_INIT(0);
#endif

#ifdef DT_INST_1_ST_STM32_TIMERS
TIMER_DEVICE_INIT(1);
#endif

#ifdef DT_INST_2_ST_STM32_TIMERS
TIMER_DEVICE_INIT(2);
#endif

#ifdef DT_INST_3_ST_STM32_TIMERS
TIMER_DEVICE_INIT(3);
#endif

#ifdef DT_INST_4_ST_STM32_TIMERS
TIMER_DEVICE_INIT(4);
#endif

#ifdef DT_INST_5_ST_STM32_TIMERS
TIMER_DEVICE_INIT(5);
#endif

#ifdef DT_INST_6_ST_STM32_TIMERS
TIMER_DEVICE_INIT(6);
#endif

#ifdef DT_INST_7_ST_STM32_TIMERS
TIMER_DEVICE_INIT(7);
#endif

#ifdef DT_INST_8_ST_STM32_TIMERS
TIMER_DEVICE_INIT(8);
#endif

#ifdef DT_INST_9_ST_STM32_TIMERS
TIMER_DEVICE_INIT(9);
#endif

#ifdef DT_INST_10_ST_STM32_TIMERS
TIMER_DEVICE_INIT(10);
#endif

#ifdef DT_INST_11_ST_STM32_TIMERS
TIMER_DEVICE_INIT(11);
#endif

#ifdef DT_INST_12_ST_STM32_TIMERS
TIMER_DEVICE_INIT(12);
#endif

#ifdef DT_INST_13_ST_STM32_TIMERS
TIMER_DEVICE_INIT(13);
#endif

#ifdef DT_INST_14_ST_STM32_TIMERS
TIMER_DEVICE_INIT(14);
#endif

#ifdef DT_INST_15_ST_STM32_TIMERS
TIMER_DEVICE_INIT(15);
#endif

