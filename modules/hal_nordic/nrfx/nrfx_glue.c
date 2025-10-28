/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nrfx.h>
#include <zephyr/kernel.h>
#include <lib/nrfx_coredep.h>

void nrfx_isr(const void *irq_handler)
{
	((nrfx_irq_handler_t)irq_handler)();
}

void nrfx_busy_wait(uint32_t usec_to_wait)
{
	if (IS_ENABLED(CONFIG_SYS_CLOCK_EXISTS)) {
		k_busy_wait(usec_to_wait);
	} else {
		nrfx_coredep_delay_us(usec_to_wait);
	}
}

char const *nrfx_error_string_get(int code)
{
	switch (-code) {
	case 0: return STRINGIFY(0);
	case ECANCELED: return STRINGIFY(ECANCELED);
	case ENOMEM: return STRINGIFY(ENOMEM);
	case ENOTSUP: return STRINGIFY(ENOTSUP);
	case EINVAL: return STRINGIFY(EINVAL);
	case EINPROGRESS: return STRINGIFY(EINPROGRESS);
	case E2BIG: return STRINGIFY(E2BIG);
	case ETIMEDOUT: return STRINGIFY(ETIMEDOUT);
	case EPERM: return STRINGIFY(EPERM);
	case EFAULT: return STRINGIFY(EFAULT);
	case EACCES: return STRINGIFY(EACCES);
	case EBUSY: return STRINGIFY(EBUSY);
	case EALREADY: return STRINGIFY(EALREADY);
	default: return "unknown";
	}
}
