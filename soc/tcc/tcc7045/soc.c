/*
 * Copyright 2024 Hounjoung Rim <hounjoung@tsnlab.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/drivers/gpio/gpio_tccvcp.h>
#include <zephyr/drivers/interrupt_controller/intc_tic.h>
#include <zephyr/drivers/clock_control/clock_control_tcc_ccu.h>

#include <cmsis_core.h>
#include <string.h>
#include "soc.h"

unsigned int z_soc_irq_get_active(void)
{
	return z_tic_irq_get_active();
}

void z_soc_irq_eoi(unsigned int irq)
{
	z_tic_irq_eoi(irq);
}

void z_soc_irq_init(void)
{
	z_tic_irq_init();
}

void z_soc_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	/* Configure interrupt type and priority */
	z_tic_irq_priority_set(irq, prio, flags);
}

void z_soc_irq_enable(unsigned int irq)
{
	/* Enable interrupt */
	z_tic_irq_enable(irq);
}

void z_soc_irq_disable(unsigned int irq)
{
	/* Disable interrupt */
	z_tic_irq_disable(irq);
}

int z_soc_irq_is_enabled(unsigned int irq)
{
	/* Check if interrupt is enabled */
	return z_tic_irq_is_enabled(irq);
}

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 */
void soc_early_init_hook(void)
{
	vcp_clock_init();

	/* Enable SYS_3P3 */
	vcp_gpio_config(SYS_PWR_EN, (unsigned long)(GPIO_FUNC(0U) | VCP_GPIO_OUTPUT));
	vcp_gpio_set(SYS_PWR_EN, 1UL);
}

static int32_t reduce_dividend(unsigned long long *dividend, uint32_t divisor,
			       unsigned long long *res)
{
	uint32_t high = (uint32_t)(*dividend >> 32ULL);

	if (high >= divisor) {
		high /= divisor;
		*res = ((unsigned long long)high) << 32ULL;

		if ((divisor > 0UL) &&
		    ((*dividend / (unsigned long long)divisor) >= (unsigned long long)high)) {
			return -EINVAL;
		}

		*dividend -= (((unsigned long long)high * (unsigned long long)divisor) << 32ULL);
	}
	return 0;
}

static void adjust_divisor_and_quotient(unsigned long long *b, unsigned long long *d,
					unsigned long long rem)
{
	while ((*b > 0ULL) && (*b < rem)) {
		*b = *b + *b;
		*d = *d + *d;
	}
}

static int32_t perform_division(unsigned long long *rem, unsigned long long *b,
				unsigned long long *d, unsigned long long *res)
{
	do {
		if (*rem >= *b) {
			*rem -= *b;

			if ((0xFFFFFFFFFFFFFFFFULL - *d) < *res) {
				return -EINVAL;
			}

			*res += *d;
		}

		*b >>= 1UL;
		*d >>= 1UL;
	} while (*d != 0ULL);

	return 0;
}

int32_t soc_div64_to_32(unsigned long long *dividend_ptr, uint32_t divisor, uint32_t *rem_ptr)
{
	int32_t ret_val = 0;
	unsigned long long rem = 0;
	unsigned long long b = divisor;
	unsigned long long d = 1;
	unsigned long long res = 0;

	if (dividend_ptr == NULL_PTR) {
		return -EINVAL;
	}

	rem = *dividend_ptr;

	ret_val = reduce_dividend(&rem, divisor, &res);
	if (ret_val != 0) {
		return ret_val;
	}

	adjust_divisor_and_quotient(&b, &d, rem);

	ret_val = perform_division(&rem, &b, &d, &res);
	if (ret_val != 0) {
		return ret_val;
	}

	*dividend_ptr = res;

	if (rem_ptr != NULL_PTR) {
		*rem_ptr = (uint32_t)rem;
	}

	return ret_val;
}
