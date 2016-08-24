/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>

#include <nanokernel.h>
#include <pwm.h>
#include <device.h>
#include <init.h>
#include <power.h>

#include "qm_pwm.h"
#include "clk.h"

#define HW_CLOCK_CYCLES_PER_USEC  (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / \
				   USEC_PER_SEC)

/* pwm uses 32 bits counter to control low and high period */
#define MAX_LOW_PERIOD_IN_HW_CLOCK_CYCLES (((uint64_t)1) << 32)
#define MAX_HIGH_PERIOD_IN_HW_CLOCK_CYCLES (((uint64_t)1) << 32)

#define MAX_PERIOD_IN_HW_CLOCK_CYCLES (MAX_LOW_PERIOD_IN_HW_CLOCK_CYCLES + \
				       MAX_HIGH_PERIOD_IN_HW_CLOCK_CYCLES)

/* in micro seconds. */
#define MAX_PERIOD (MAX_PERIOD_IN_HW_CLOCK_CYCLES / HW_CLOCK_CYCLES_PER_USEC)

/**
 * in micro seconds. To be able to get 1% granularity, MIN_PERIOD should
 * have at least 100 HW clock cycles.
 */
#define MIN_PERIOD ((100 + (HW_CLOCK_CYCLES_PER_USEC - 1)) / \
		    HW_CLOCK_CYCLES_PER_USEC)

/* in micro seconds */
#define DEFAULT_PERIOD 2000

struct pwm_data {
	struct nano_sem sem;
};

#ifdef CONFIG_PWM_QMSI_API_REENTRANCY
static struct pwm_data pwm_context;
#define PWM_CONTEXT (&pwm_context)
static const int reentrancy_protection = 1;
#else
#define PWM_CONTEXT (NULL)
static const int reentrancy_protection;
#endif /* CONFIG_PWM_QMSI_API_REENTRANCY */

static void pwm_reentrancy_init(struct device *dev)
{
	struct pwm_data *context = dev->driver_data;

	if (!reentrancy_protection) {
		return;
	}

	nano_sem_init(&context->sem);
	nano_sem_give(&context->sem);
}

static void pwm_critical_region_start(struct device *dev)
{
	struct pwm_data *context = dev->driver_data;

	if (!reentrancy_protection) {
		return;
	}

	nano_sem_take(&context->sem, TICKS_UNLIMITED);
}

static void pwm_critical_region_end(struct device *dev)
{
	struct pwm_data *context = dev->driver_data;

	if (!reentrancy_protection) {
		return;
	}

	nano_sem_give(&context->sem);
}

static uint32_t  pwm_channel_period[CONFIG_PWM_QMSI_NUM_PORTS];

static int pwm_qmsi_configure(struct device *dev, int access_op,
				 uint32_t pwm, int flags)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(access_op);
	ARG_UNUSED(pwm);
	ARG_UNUSED(flags);

	return 0;
}

static int __set_one_port(struct device *dev, qm_pwm_t id, uint32_t pwm,
				uint32_t on, uint32_t off)
{
	qm_pwm_config_t cfg;
	int ret_val = 0;

	pwm_critical_region_start(dev);

	/* Disable timer to prevent any output */
	qm_pwm_stop(id, pwm);

	if (on == 0) {
		/* stop PWM if so specified */
		goto pwm_set_port_return;
	}

	/**
	 * off period must be more than zero. Otherwise, the PWM pin will be
	 * turned off. Let's use the minimum value which is 1 for this case.
	 */
	if (off == 0) {
		off = 1;
	}

	/* PWM mode, user-defined count mode, timer disabled */
	cfg.mode = QM_PWM_MODE_PWM;

	/* No interrupts */
	cfg.mask_interrupt = true;
	cfg.callback = NULL;

	/* Data for the timer to stay high and low */
	cfg.hi_count = on;
	cfg.lo_count = off;

	if (qm_pwm_set_config(id, pwm, &cfg) != 0) {
		ret_val = -EIO;
		goto pwm_set_port_return;
	}

	/* Enable timer so it starts running and counting */
	qm_pwm_start(id, pwm);

pwm_set_port_return:
	pwm_critical_region_end(dev);

	return ret_val;
}

/*
 * Set the time to assert and de-assert the PWM pin.
 *
 * This sets the duration for the pin to stay low or high.
 *
 * For example, with a nominal system clock of 32MHz, each count of on/off
 * represents 31.25ns (e.g. off == 2 means the pin is to be de-asserted at
 * 62.5ns from the beginning of a PWM cycle). The duration of 1 count depends
 * on system clock. Refer to the hardware manual for more information.
 *
 * Parameters
 * dev: Pointer to PWM device structure
 * access_op: whether to set one pin or all
 * pwm: PWM port number to set
 * on: How far (in timer count) from the beginning of a PWM cycle the PWM
 *     pin should be asserted. Must be zero, since PWM from Quark MCU always
 *     starts from high.
 * off: How far (in timer count) from the beginning of a PWM cycle the PWM
 *	pin should be de-asserted.
 *
 * return 0, or negative errno code
 */
static int pwm_qmsi_set_values(struct device *dev, int access_op,
			       uint32_t pwm, uint32_t on, uint32_t off)
{
	uint32_t *channel_period = dev->config->config_info;
	int i, high, low;

	if (on) {
		return -EINVAL;
	}

	switch (access_op) {
	case PWM_ACCESS_BY_PIN:
		/* make sure the PWM port exists */
		if (pwm >= CONFIG_PWM_QMSI_NUM_PORTS) {
			return -EIO;
		}

		high = off;
		low = channel_period[pwm] - off;

		if (off >= channel_period[pwm]) {
			high = channel_period[pwm] - 1;
			low = 1;
		}

		if (off == 0) {
			high = 1;
			low = channel_period[pwm] - 1;
		}

		return __set_one_port(dev, QM_PWM_0, pwm, high, low);

	case PWM_ACCESS_ALL:
		for (i = 0; i < CONFIG_PWM_QMSI_NUM_PORTS; i++) {
			high = off;
			low = channel_period[i] - off;

			if (off >= channel_period[i]) {
				high = channel_period[i] - 1;
				low = 1;
			}

			if (off == 0) {
				high = 1;
				low = channel_period[i] - 1;
			}

			return __set_one_port(dev, QM_PWM_0, i, high, low);
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;

}

static int pwm_qmsi_set_period(struct device *dev, int access_op,
			       uint32_t pwm, uint32_t period)
{
	uint32_t *channel_period = dev->config->config_info;
	int ret_val = 0;

	if (channel_period == NULL) {
		return -EIO;
	}

	if (period < MIN_PERIOD || period > MAX_PERIOD) {
		return -ENOTSUP;
	}

	pwm_critical_region_start(dev);

	switch (access_op) {
	case PWM_ACCESS_BY_PIN:
		/* make sure the PWM port exists */
		if (pwm >= CONFIG_PWM_QMSI_NUM_PORTS) {
			ret_val = -EIO;
			goto pwm_set_period_return;
		}
		channel_period[pwm] = period * HW_CLOCK_CYCLES_PER_USEC;
		break;
	case PWM_ACCESS_ALL:
		for (int i = 0; i < CONFIG_PWM_QMSI_NUM_PORTS; i++) {
			channel_period[i] = period *
					    HW_CLOCK_CYCLES_PER_USEC;
		}
		break;
	default:
		ret_val = -ENOTSUP;
	}

pwm_set_period_return:
	pwm_critical_region_end(dev);

	return ret_val;
}

static int pwm_qmsi_set_duty_cycle(struct device *dev, int access_op,
				   uint32_t pwm, uint8_t duty)
{
	uint32_t *channel_period = dev->config->config_info;
	uint32_t on, off;

	if (channel_period == NULL) {
		return -EIO;
	}

	if (duty > 100) {
		return -ENOTSUP;
	}

	switch (access_op) {
	case PWM_ACCESS_BY_PIN:
		/* make sure the PWM port exists */
		if (pwm >= CONFIG_PWM_QMSI_NUM_PORTS) {
			return -EIO;
		}
		on = (channel_period[pwm] * duty) / 100;
		off = channel_period[pwm] - on;
		if (off == 0) {
			on--;
			off = 1;
		}
		return __set_one_port(dev, QM_PWM_0, pwm, on, off);
	case PWM_ACCESS_ALL:
		for (int i = 0; i < CONFIG_PWM_QMSI_NUM_PORTS; i++) {
			on = (channel_period[i] * duty) / 100;
			off = channel_period[i] - on;
			if (off == 0) {
				on--;
				off = 1;
			}
			if (__set_one_port(dev, QM_PWM_0, i, on, off) != 0) {
				return -EIO;
			}
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int pwm_qmsi_set_phase(struct device *dev, int access_op,
			      uint32_t pwm, uint8_t phase)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(access_op);
	ARG_UNUSED(pwm);
	ARG_UNUSED(phase);

	return -ENOTSUP;
}

static struct pwm_driver_api pwm_qmsi_drv_api_funcs = {
	.config = pwm_qmsi_configure,
	.set_values = pwm_qmsi_set_values,
	.set_period = pwm_qmsi_set_period,
	.set_duty_cycle = pwm_qmsi_set_duty_cycle,
	.set_phase = pwm_qmsi_set_phase
};

static int pwm_qmsi_init(struct device *dev)
{
	uint32_t *channel_period = dev->config->config_info;

	for (int i = 0; i < CONFIG_PWM_QMSI_NUM_PORTS; i++) {
		channel_period[i] = DEFAULT_PERIOD *
				    HW_CLOCK_CYCLES_PER_USEC;
	}

	clk_periph_enable(CLK_PERIPH_PWM_REGISTER | CLK_PERIPH_CLK);

	pwm_reentrancy_init(dev);

	return 0;
}

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
struct pwm_channel_ctx {
	uint32_t loadcount1;
	uint32_t loadcount2;
	uint32_t controlreg;
};

struct pwm_ctx {
	struct pwm_channel_ctx channels[CONFIG_PWM_QMSI_NUM_PORTS];
	uint32_t int_pwm_timer_mask;
};

static struct pwm_ctx pwm_ctx_save;

static int pwm_qmsi_suspend(struct device *dev, int pm_policy)
{
	if (pm_policy == SYS_PM_DEEP_SLEEP) {
		int i;

		pwm_ctx_save.int_pwm_timer_mask =
			QM_SCSS_INT->int_pwm_timer_mask;
		for (i = 0; i < CONFIG_PWM_QMSI_NUM_PORTS; i++) {
			qm_pwm_channel_t *channel;
			struct pwm_channel_ctx *channel_save;

			channel = &QM_PWM->timer[i];
			channel_save = &pwm_ctx_save.channels[i];
			channel_save->loadcount1 = channel->loadcount;
			channel_save->controlreg = channel->controlreg;
			channel_save->loadcount2 = QM_PWM->timer_loadcount2[i];
		}
	}

	return 0;
}

static int pwm_qmsi_resume(struct device *dev, int pm_policy)
{
	if (pm_policy == SYS_PM_DEEP_SLEEP) {
		int i;

		for (i = 0; i < CONFIG_PWM_QMSI_NUM_PORTS; i++) {
			qm_pwm_channel_t *channel;
			struct pwm_channel_ctx *channel_save;

			channel = &QM_PWM->timer[i];
			channel_save = &pwm_ctx_save.channels[i];
			channel->loadcount = channel_save->loadcount1;
			channel->controlreg = channel_save->controlreg;
			QM_PWM->timer_loadcount2[i] = channel_save->loadcount2;
		}
		QM_SCSS_INT->int_pwm_timer_mask =
			pwm_ctx_save.int_pwm_timer_mask;
	}

	return 0;
}
#endif

DEFINE_DEVICE_PM_OPS(pwm, pwm_qmsi_suspend, pwm_qmsi_resume);

DEVICE_AND_API_INIT_PM(pwm_qmsi_0, CONFIG_PWM_QMSI_DEV_NAME, pwm_qmsi_init,
		       DEVICE_PM_OPS_GET(pwm), PWM_CONTEXT, pwm_channel_period,
		       SECONDARY, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		       (void *)&pwm_qmsi_drv_api_funcs);
