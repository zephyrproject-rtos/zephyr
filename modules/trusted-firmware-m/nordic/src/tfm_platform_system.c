/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "platform/include/tfm_platform_system.h"
#include "cmsis.h"
#include "tfm_platform_hal_ioctl.h"
#include "tfm_ioctl_core_api.h"

void tfm_platform_hal_system_reset(void)
{
	/* Reset the system */
	NVIC_SystemReset();
}

enum tfm_platform_err_t tfm_platform_hal_ioctl(tfm_platform_ioctl_req_t request,
					       psa_invec  *in_vec,
					       psa_outvec *out_vec)
{
	/* Core IOCTL services */
	switch (request) {
	case TFM_PLATFORM_IOCTL_READ_SERVICE:
		return tfm_platform_hal_read_service(in_vec, out_vec);
#if defined(GPIO_PIN_CNF_MCUSEL_Msk)
	case TFM_PLATFORM_IOCTL_GPIO_SERVICE:
		return tfm_platform_hal_gpio_service(in_vec, out_vec);
#endif /* defined(GPIO_PIN_CNF_MCUSEL_Msk) */


	/* Board specific IOCTL services */
#if defined(TFM_NRF_RAM_CTRL_SERVICE)
	case TFM_PLATFORM_IOCTL_RAM_CTRL_SERVICE:
		return tfm_platform_hal_ram_ctrl_service(in_vec, out_vec);
#endif

	/* Not a supported IOCTL service.*/
	default:
		return TFM_PLATFORM_ERR_NOT_SUPPORTED;
	}
}
