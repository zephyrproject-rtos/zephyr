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

static int __set_one_port(qm_pwm_t id, uint32_t pwm, uint32_t on, uint32_t off)
{
	qm_pwm_config_t cfg;

	/* Disable timer to prevent any output */
	qm_pwm_stop(id, pwm);

	if (on == 0) {
		/* stop PWM if so specified */
		return 0;
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
		return -EIO;
	}
	/* Enable timer so it starts running and counting */
	qm_pwm_start(id, pwm);

	return 0;
}

/*
 * Set the duration for on/off timer of PWM.
 *
 * This sets the duration for the pin to low or high.
 *
 * Assumes a nominal system clock of 32MHz, each count of on/off represents
 * 31.25ns (e.g. on == 2 means the pin stays high for 62.5ns).
 * The duration of 1 count depends on system clock. Refer to the hardware
 * manual for more information.
 *
 * Parameters
 * dev: Device struct
 * access_op: whether to set one pin or all
 * pwm: Which PWM port to set
 * on: Duration for pin to stay high (must be >= 2)
 * off: Duration for pin to stay low (must be >= 2)
 *
 * return 0, -ENOTSUP
 */
static int pwm_qmsi_set_values(struct device *dev, int access_op,
				  uint32_t pwm, uint32_t on, uint32_t off)
{
	int i;

	if (off == 0) {
		on--;
		off = 1;
	}

	switch (access_op) {
	case PWM_ACCESS_BY_PIN:
		/* make sure the PWM port exists */
		if (pwm >= CONFIG_PWM_QMSI_NUM_PORTS) {
			return -EIO;
		}
		return __set_one_port(QM_PWM_0, pwm, on, off);

	case PWM_ACCESS_ALL:
		for (i = 0; i < CONFIG_PWM_QMSI_NUM_PORTS; i++) {
			__set_one_port(QM_PWM_0, i, on, off);
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

	if (channel_period == NULL) {
		return -EIO;
	}

	if (period < MIN_PERIOD || period > MAX_PERIOD) {
		return -ENOTSUP;
	}

	switch (access_op) {
	case PWM_ACCESS_BY_PIN:
		/* make sure the PWM port exists */
		if (pwm >= CONFIG_PWM_QMSI_NUM_PORTS) {
			return -EIO;
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
		return -ENOTSUP;
	}

	return 0;
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
		return __set_one_port(QM_PWM_0, pwm, on, off);
	case PWM_ACCESS_ALL:
		for (int i = 0; i < CONFIG_PWM_QMSI_NUM_PORTS; i++) {
			on = (channel_period[i] * duty) / 100;
			off = channel_period[i] - on;
			if (off == 0) {
				on--;
				off = 1;
			}
			if (__set_one_port(QM_PWM_0, i, on, off) != 0) {
				return -EIO;
			}
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

/*
 * Set the PWM IP block suspended/low power state
 * In this case, the PWN does not support power state handling
 *
 * Parameters
 * dev: Device struct
 * return -ENOTSUP
 */
static int pwm_qmsi_suspend(struct device *dev)
{
	ARG_UNUSED(dev);

	return -ENOTSUP;
}

/*
 * Bring back the PWM IP block from suspended/low power state
 * In this case, the PWN does not support power state handling
 *
 * Parameters
 * dev: Device struct
 * return -ENOTSUP
 */
static int pwm_qmsi_resume(struct device *dev)
{
	ARG_UNUSED(dev);

	return -ENOTSUP;
}

static struct pwm_driver_api pwm_qmsi_drv_api_funcs = {
	.config = pwm_qmsi_configure,
	.set_values = pwm_qmsi_set_values,
	.set_period = pwm_qmsi_set_period,
	.set_duty_cycle = pwm_qmsi_set_duty_cycle,
	.suspend = pwm_qmsi_suspend,
	.resume = pwm_qmsi_resume,
};

static int pwm_qmsi_init(struct device *dev)
{
	uint32_t *channel_period = dev->config->config_info;

	for (int i = 0; i < CONFIG_PWM_QMSI_NUM_PORTS; i++) {
		channel_period[i] = DEFAULT_PERIOD *
				    HW_CLOCK_CYCLES_PER_USEC;
	}

	clk_periph_enable(CLK_PERIPH_PWM_REGISTER | CLK_PERIPH_CLK);
	return 0;
}

DEVICE_AND_API_INIT(pwm_qmsi_0, CONFIG_PWM_QMSI_DEV_NAME, pwm_qmsi_init, NULL,
		    pwm_channel_period, SECONDARY,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    (void *)&pwm_qmsi_drv_api_funcs);
