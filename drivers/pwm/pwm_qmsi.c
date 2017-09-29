/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <pwm.h>
#include <device.h>
#include <kernel.h>
#include <init.h>
#include <power.h>
#include <misc/util.h>

#include "qm_pwm.h"
#include "clk.h"

#define HW_CLOCK_CYCLES_PER_USEC  (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / \
				   USEC_PER_SEC)

/* pwm uses 32 bits counter to control low and high period */
#define MAX_LOW_PERIOD_IN_HW_CLOCK_CYCLES (((u64_t)1) << 32)
#define MAX_HIGH_PERIOD_IN_HW_CLOCK_CYCLES (((u64_t)1) << 32)

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
#ifdef CONFIG_PWM_QMSI_API_REENTRANCY
	struct k_sem sem;
#endif
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	u32_t device_power_state;
#endif
	u32_t channel_period[CONFIG_PWM_QMSI_NUM_PORTS];
};

static struct pwm_data pwm_context;

#ifdef CONFIG_PWM_QMSI_API_REENTRANCY
#define RP_GET(dev) (&((struct pwm_data *)(dev->driver_data))->sem)
#else
#define RP_GET(dev) (NULL)
#endif

static int __set_one_port(struct device *dev, qm_pwm_t id, u32_t pwm,
				u32_t on, u32_t off)
{
	qm_pwm_config_t cfg;
	int ret_val = 0;

	if (IS_ENABLED(CONFIG_PWM_QMSI_API_REENTRANCY)) {
		k_sem_take(RP_GET(dev), K_FOREVER);
	}

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
	cfg.callback_data = NULL;

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
	if (IS_ENABLED(CONFIG_PWM_QMSI_API_REENTRANCY)) {
		k_sem_give(RP_GET(dev));
	}

	return ret_val;
}

/*
 * Set the period and pulse width for a PWM pin.
 *
 * For example, with a nominal system clock of 32MHz, each count represents
 * 31.25ns (e.g. period = 100 means the pulse is to repeat every 3125ns). The
 * duration of one count depends on system clock. Refer to the hardware manual
 * for more information.
 *
 * Parameters
 * dev: Pointer to PWM device structure
 * pwm: PWM port number to set
 * period_cycles: Period (in timer count)
 * pulse_cycles: Pulse width (in timer count).
 *
 * return 0, or negative errno code
 */
static int pwm_qmsi_pin_set(struct device *dev, u32_t pwm,
			    u32_t period_cycles, u32_t pulse_cycles)
{
	u32_t high, low;

	if (pwm >= CONFIG_PWM_QMSI_NUM_PORTS) {
		return -EINVAL;
	}

	if (period_cycles == 0 || pulse_cycles > period_cycles) {
		return -EINVAL;
	}

	high = pulse_cycles;
	low = period_cycles - pulse_cycles;

	/*
	 * low must be more than zero. Otherwise, the PWM pin will be
	 * turned off. Let's make sure low is always more than zero.
	 */
	if (low == 0) {
		high--;
		low = 1;
	}

	return __set_one_port(dev, QM_PWM_0, pwm, high, low);
}

/*
 * Get the clock rate (cycles per second) for a PWM pin.
 *
 * Parameters
 * dev: Pointer to PWM device structure
 * pwm: PWM port number
 * cycles: Pointer to the memory to store clock rate (cycles per second)
 *
 * return 0, or negative errno code
 */
static int pwm_qmsi_get_cycles_per_sec(struct device *dev, u32_t pwm,
				       u64_t *cycles)
{
	if (cycles == NULL) {
		return -EINVAL;
	}

	*cycles = (u64_t)clk_sys_get_ticks_per_us() * USEC_PER_SEC;

	return 0;
}

static const struct pwm_driver_api pwm_qmsi_drv_api_funcs = {
	.pin_set = pwm_qmsi_pin_set,
	.get_cycles_per_sec = pwm_qmsi_get_cycles_per_sec,
};

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static void pwm_qmsi_set_power_state(struct device *dev, u32_t power_state)
{
	struct pwm_data *context = dev->driver_data;

	context->device_power_state = power_state;
}
#else
#define pwm_qmsi_set_power_state(...)
#endif

static int pwm_qmsi_init(struct device *dev)
{
	struct pwm_data *context = dev->driver_data;
	u32_t *channel_period = context->channel_period;

	for (int i = 0; i < CONFIG_PWM_QMSI_NUM_PORTS; i++) {
		channel_period[i] = DEFAULT_PERIOD *
				    HW_CLOCK_CYCLES_PER_USEC;
	}

	clk_periph_enable(CLK_PERIPH_PWM_REGISTER | CLK_PERIPH_CLK);

	if (IS_ENABLED(CONFIG_PWM_QMSI_API_REENTRANCY)) {
		k_sem_init(RP_GET(dev), 1, UINT_MAX);
	}

	pwm_qmsi_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);

	return 0;
}

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static qm_pwm_context_t pwm_ctx;

static u32_t pwm_qmsi_get_power_state(struct device *dev)
{
	struct pwm_data *context = dev->driver_data;

	return context->device_power_state;
}

static int pwm_qmsi_suspend(struct device *dev)
{
	qm_pwm_save_context(QM_PWM_0, &pwm_ctx);

	pwm_qmsi_set_power_state(dev, DEVICE_PM_SUSPEND_STATE);

	return 0;
}

static int pwm_qmsi_resume_from_suspend(struct device *dev)
{
	qm_pwm_restore_context(QM_PWM_0, &pwm_ctx);

	pwm_qmsi_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);

	return 0;
}

/*
* Implements the driver control management functionality
* the *context may include IN data or/and OUT data
*/
static int pwm_qmsi_device_ctrl(struct device *dev, u32_t ctrl_command,
				void *context)
{
	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		if (*((u32_t *)context) == DEVICE_PM_SUSPEND_STATE) {
			return pwm_qmsi_suspend(dev);
		} else if (*((u32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
			return pwm_qmsi_resume_from_suspend(dev);
		}
	} else if (ctrl_command == DEVICE_PM_GET_POWER_STATE) {
		*((u32_t *)context) = pwm_qmsi_get_power_state(dev);
		return 0;
	}

	return 0;
}
#endif

DEVICE_DEFINE(pwm_qmsi_0, CONFIG_PWM_QMSI_DEV_NAME, pwm_qmsi_init,
	      pwm_qmsi_device_ctrl, &pwm_context, NULL,
	      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
	      &pwm_qmsi_drv_api_funcs);
