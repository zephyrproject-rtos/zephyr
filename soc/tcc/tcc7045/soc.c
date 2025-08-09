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

uint32_t z_soc_irq_get_active(void)
{
	return z_tic_irq_get_active();
}

void z_soc_irq_eoi(uint32_t irq)
{
	z_tic_irq_eoi(irq);
}

void z_soc_irq_init(void)
{
	z_tic_irq_init();
}

void z_soc_irq_priority_set(uint32_t irq, uint32_t prio, uint32_t flags)
{
	/* Configure interrupt type and priority */
	z_tic_irq_priority_set(irq, prio, flags);
}

void z_soc_irq_enable(uint32_t irq)
{
	/* Enable interrupt */
	z_tic_irq_enable(irq);
}

void z_soc_irq_disable(uint32_t irq)
{
	/* Disable interrupt */
	z_tic_irq_disable(irq);
}

int32_t z_soc_irq_is_enabled(uint32_t irq)
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
	vcp_gpio_config(SYS_PWR_EN, (uint32_t)(GPIO_FUNC(0U) | VCP_GPIO_OUTPUT));
	vcp_gpio_set(SYS_PWR_EN, 1UL);
}

static int32_t reduce_dividend(uint64_t *dividend, uint32_t divisor, uint64_t *res)
{
	uint32_t high = (uint32_t)(*dividend >> 32ULL);

	if (divisor == 0) {
		return -EINVAL;
	}

	if (high >= divisor) {
		high /= divisor;
		*res = ((uint64_t)high) << 32ULL;

		if ((*dividend / (uint64_t)divisor) >= (uint64_t)high) {
			return -EINVAL;
		}

		*dividend -= (((uint64_t)high * (uint64_t)divisor) << 32ULL);
	}
	return 0;
}

static void adjust_divisor_and_quotient(uint64_t *b, uint64_t *d, uint64_t rem)
{
	while ((*b > 0ULL) && (*b < rem)) {
		*b = *b + *b;
		*d = *d + *d;
	}
}

static int32_t perform_division(uint64_t *rem, uint64_t *b, uint64_t *d, uint64_t *res)
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

int32_t soc_div64_to_32(uint64_t *dividend_ptr, uint32_t divisor, uint32_t *rem_ptr)
{
	int32_t ret_val = 0;
	uint64_t rem = 0;
	uint64_t b = divisor;
	uint64_t d = 1;
	uint64_t res = 0;

	if (dividend_ptr == TCC_NULL_PTR) {
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

	if (rem_ptr != TCC_NULL_PTR) {
		*rem_ptr = (uint32_t)rem;
	}

	return ret_val;
}
