/*
 * Copyright (c) 2024 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <mec_ecia_api.h>
#include <mec_ecs_api.h>

static void mec5_soc_init_debug_interface(void)
{
#if defined(CONFIG_SOC_MEC_DEBUG_DISABLED)
	mec_ecs_etm_pins(ECS_ETM_PINS_DISABLE);
	mec_hal_ecs_debug_port(MEC_DEBUG_MODE_DISABLE);
#else
#if defined(SOC_MEC_DEBUG_WITHOUT_TRACING)
	mec_ecs_etm_pins(ECS_ETM_PINS_DISABLE);
	mec_hal_ecs_debug_port(MEC_DEBUG_MODE_SWD);
#elif defined(SOC_MEC_DEBUG_AND_TRACING)
#if defined(SOC_MEC_DEBUG_AND_ETM_TRACING)
	mec_ecs_etm_pins(ECS_ETM_PINS_DISABLE);
	mec_hal_ecs_debug_port(MEC_DEBUG_MODE_SWD_SWV);
#elif defined(CONFIG_SOC_MEC_DEBUG_AND_ETM_TRACING)
	mec_ecs_debug_port(MEC_DEBUG_MODE_SWD);
	mec_hal_ecs_etm_pins(ECS_ETM_PINS_ENABLE);
#endif
#endif
#endif
}

int mec5_soc_common_init(void)
{
	mec5_soc_init_debug_interface();
	mec_hal_ecia_init(MEC5_ECIA_DIRECT_BITMAP, 1, 0);

	return 0;
}
