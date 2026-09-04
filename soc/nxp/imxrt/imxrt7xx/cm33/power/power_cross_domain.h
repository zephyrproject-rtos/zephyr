/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_NXP_IMXRT_IMXRT7XX_CM33_POWER_POWER_CROSS_DOMAIN_H_
#define SOC_NXP_IMXRT_IMXRT7XX_CM33_POWER_POWER_CROSS_DOMAIN_H_

#include <zephyr/toolchain.h>

#include "power_resources.h"

#if defined(CONFIG_PM) || defined(CONFIG_POWEROFF)
/**
 * @brief Declare that the peer domain needs @p res.
 *
 * Some resources are shared, but control over them resides in only one domain.
 * When exercising resource control, that domain needs to know whether the
 * peer domain uses the resource.
 *
 * @param res Resource the peer domain needs kept alive.
 */
void power_cross_domain_request(enum power_resource res);
#else
static inline void power_cross_domain_request(enum power_resource res)
{
	ARG_UNUSED(res);
}
#endif

#endif /* SOC_NXP_IMXRT_IMXRT7XX_CM33_POWER_POWER_CROSS_DOMAIN_H_ */
