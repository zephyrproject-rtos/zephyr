/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nrfx.h>
#include <zephyr/kernel.h>
#include <soc/nrfx_coredep.h>

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

char const *nrfx_error_string_get(nrfx_err_t code)
{
	#define NRFX_ERROR_STRING_CASE(code)  case code: return #code
	switch (code) {
		NRFX_ERROR_STRING_CASE(NRFX_SUCCESS);
		NRFX_ERROR_STRING_CASE(NRFX_ERROR_INTERNAL);
		NRFX_ERROR_STRING_CASE(NRFX_ERROR_NO_MEM);
		NRFX_ERROR_STRING_CASE(NRFX_ERROR_NOT_SUPPORTED);
		NRFX_ERROR_STRING_CASE(NRFX_ERROR_INVALID_PARAM);
		NRFX_ERROR_STRING_CASE(NRFX_ERROR_INVALID_STATE);
		NRFX_ERROR_STRING_CASE(NRFX_ERROR_INVALID_LENGTH);
		NRFX_ERROR_STRING_CASE(NRFX_ERROR_TIMEOUT);
		NRFX_ERROR_STRING_CASE(NRFX_ERROR_FORBIDDEN);
		NRFX_ERROR_STRING_CASE(NRFX_ERROR_NULL);
		NRFX_ERROR_STRING_CASE(NRFX_ERROR_INVALID_ADDR);
		NRFX_ERROR_STRING_CASE(NRFX_ERROR_BUSY);
		NRFX_ERROR_STRING_CASE(NRFX_ERROR_ALREADY);
		NRFX_ERROR_STRING_CASE(NRFX_ERROR_DRV_TWI_ERR_OVERRUN);
		NRFX_ERROR_STRING_CASE(NRFX_ERROR_DRV_TWI_ERR_ANACK);
		NRFX_ERROR_STRING_CASE(NRFX_ERROR_DRV_TWI_ERR_DNACK);
		default: return "unknown";
	}
}
