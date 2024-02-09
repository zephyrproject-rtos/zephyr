/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc_secure.h>
#include <errno.h>

#include "nrf.h"

#include "tfm_platform_api.h"
#include "tfm_ioctl_api.h"

#if NRF_GPIO_HAS_SEL
void soc_secure_gpio_pin_mcu_select(uint32_t pin_number, nrf_gpio_pin_sel_t mcu)
{
	uint32_t result;
	enum tfm_platform_err_t err;

	err = tfm_platform_gpio_pin_mcu_select(pin_number, mcu, &result);
	__ASSERT(err == TFM_PLATFORM_ERR_SUCCESS, "TFM platform error (%d)", err);
	__ASSERT(result == 0, "GPIO service error (%d)", result);
}
#endif /* NRF_GPIO_HAS_SEL */

int soc_secure_mem_read(void *dst, void *src, size_t len)
{
	enum tfm_platform_err_t status;
	uint32_t result;

	status = tfm_platform_mem_read(dst, (uintptr_t)src, len, &result);

	switch (status) {
	case TFM_PLATFORM_ERR_INVALID_PARAM:
		return -EINVAL;
	case TFM_PLATFORM_ERR_NOT_SUPPORTED:
		return -ENOTSUP;
	case TFM_PLATFORM_ERR_SUCCESS:
		if (result == 0) {
			return 0;
		}
		/* Fallthrough */
	default:
		return -EPERM;
	}
}
