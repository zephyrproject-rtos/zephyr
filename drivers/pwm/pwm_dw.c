/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Driver for DesignWare PWM driver.
 *
 * The timer IP block can act as both timer and PWM. Under PWM mode, each port
 * has two registers to specify how long to stay low, and how long to stay high.
 * Care must be taken so that PWM and timer functions are not both enabled
 * on one port.
 *
 * The set of registers for each timer repeats every 0x14.
 * However, the load count 2 starts at 0xB0, and repeats every 0x04.
 * Accessing load count 2 registers, thus, requires some special treatment.
 */

#include <errno.h>

#include <kernel.h>
#include <drivers/pwm.h>

/* Register for component version */
#define REG_COMP_VER		0xAC

/* Timer Load Count register, for pin to stay low. */
#define REG_TMR_LOAD_CNT	0x00

/* Control for timer */
#define REG_TMR_CTRL		0x08

/* Offset from Timer 1 Load Count address
 * for other timers. (e.g. Timer 2 address +0x14,
 * timer 3 address + 0x28, etc.)
 *
 * This also applies to other registers for
 * different timers (except load count 2).
 */
#define REG_OFFSET		0x14

/* Timer Load Count 2 register, for pin to stay high. */
#define REG_TMR_LOAD_CNT2	0xB0

/* Offset from Timer 1 Load Count 2 address
 * for other timers. (e.g. Timer 2 address +0x04,
 * timer 3 address + 0x08, etc.)
 */
#define REG_OFFSET_LOAD_CNT2	0x04

/* Default for control register:
 * PWM mode, interrupt masked, user-defined count mode, but disabled
 */
#define TIMER_INIT_CTRL		0x0E

struct pwm_dw_config {
	/** Base address of registers */
	uint32_t	addr;

	/** Number of ports */
	uint32_t	num_ports;
};

/**
 * Find the base address for each timer
 *
 * @param dev Device struct
 * @param timer Which timer
 *
 * @return The base address of that particular timer
 */
static inline int pwm_dw_timer_base_addr(const struct device *dev,
					 uint32_t timer)
{
	const struct pwm_dw_config * const cfg =
	    (const struct pwm_dw_config *)dev->config;

	return (cfg->addr + (timer * REG_OFFSET));
}

/**
 * Find the load count 2 address for each timer
 *
 * @param dev Device struct
 * @param timer Which timer
 *
 * @return The load count 2 address of that particular timer
 */
static inline int pwm_dw_timer_ldcnt2_addr(const struct device *dev,
					   uint32_t timer)
{
	const struct pwm_dw_config * const cfg =
	    (const struct pwm_dw_config *)dev->config;

	return (cfg->addr + REG_TMR_LOAD_CNT2 + (timer * REG_OFFSET_LOAD_CNT2));
}


static int __set_one_port(const struct device *dev, uint32_t pwm,
			  uint32_t on, uint32_t off)
{
	uint32_t reg_addr;

	reg_addr = pwm_dw_timer_base_addr(dev, pwm);

	/* Disable timer to prevent any output */
	sys_write32(TIMER_INIT_CTRL, (reg_addr + REG_TMR_CTRL));

	if ((off == 0U) || (on == 0U)) {
		/* stop PWM if so specified */
		return 0;
	}

	/* write timer for pin to stay low */
	sys_write32(off, (reg_addr + REG_TMR_LOAD_CNT));

	/* write timer for pin to stay high */
	sys_write32(on, pwm_dw_timer_ldcnt2_addr(dev, pwm));

	/* Enable timer so it starts running and counting */
	sys_write32((TIMER_INIT_CTRL | 0x01), (reg_addr + REG_TMR_CTRL));

	return 0;
}

/**
 * Set the period and the pulse of PWM.
 *
 *
 * Assumes a nominal system clock of 32MHz, each count of on/off represents
 * 31.25ns (e.g. on == 2 means the pin stays high for 62.5ns).
 * The duration of 1 count depends on system clock. Refer to the hardware
 * manual for more information.
 *
 * @param dev Device struct
 * @param pwm Which PWM pin to set
 * @param period_cycles Period in clock cycles of the pwm.
 * @param pulse_cycles PWM width in clock cycles
 * @param flags Flags for pin configuration (polarity).
 *
 * @return 0
 */
static int pwm_dw_pin_set_cycles(const struct device *dev,
				 uint32_t pwm, uint32_t period_cycles,
				 uint32_t pulse_cycles, pwm_flags_t flags)
{
	const struct pwm_dw_config * const cfg =
	    (const struct pwm_dw_config *)dev->config;
	int i;
	uint32_t on, off;

	/* make sure the PWM port exists */
	if (pwm >= cfg->num_ports) {
		return -EIO;
	}

	if (flags) {
		/* PWM polarity not supported (yet?) */
		return -ENOTSUP;
	}

	if (period_cycles == 0U || pulse_cycles > period_cycles) {
		return -EINVAL;
	}
	on = pulse_cycles;
	off = period_cycles - pulse_cycles;

	if (off == 0U) {
		on--;
		off++;
	}

	return __set_one_port(dev, pwm, on, off);

}

static struct pwm_driver_api pwm_dw_drv_api_funcs = {
	.pin_set = pwm_dw_pin_set_cycles,
};

/**
 * @brief Initialization function of PCA9685
 *
 * @param dev Device struct
 * @return 0 if successful, failed otherwise.
 */
int pwm_dw_init(const struct device *dev)
{
	return 0;
}

/* Initialization for PWM_DW */
#if defined(CONFIG_PWM_DW)
#include <device.h>
#include <init.h>

static struct pwm_dw_config pwm_dw_cfg = {
	.addr = PWM_DW_BASE_ADDR,
	.num_ports = PWM_DW_NUM_PORTS,
};

DEVICE_AND_API_INIT(pwm_dw_0, CONFIG_PWM_DW_0_DRV_NAME, pwm_dw_init,
		    NULL, &pwm_dw_cfg,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &pwm_dw_drv_api_funcs);

#endif /* CONFIG_PWM_DW */
