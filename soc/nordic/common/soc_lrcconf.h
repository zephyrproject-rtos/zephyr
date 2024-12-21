/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file nRF SoC specific helpers for lrcconf management
 */

#ifndef ZEPHYR_SOC_NORDIC_COMMON_LRCCONF_H_
#define ZEPHYR_SOC_NORDIC_COMMON_LRCCONF_H_

#include <hal/nrf_lrcconf.h>

/**
 * @brief Request lrcconf power domain
 *
 * @param node	 Pointer to the @ref sys_snode_t structure which is the ID of the
 *		 requesting module.
 * @param domain The mask that represents the power domain ID.
 */
void soc_lrcconf_poweron_request(sys_snode_t *node, nrf_lrcconf_power_domain_mask_t domain);

/**
 * @brief Release lrcconf power domain
 *
 * @param node	 Pointer to the @ref sys_snode_t structure which is the ID of the
 *		 requesting module.
 * @param domain The mask that represents the power domain ID.
 */
void soc_lrcconf_poweron_release(sys_snode_t *node, nrf_lrcconf_power_domain_mask_t domain);

#endif /* ZEPHYR_SOC_NORDIC_COMMON_LRCCONF_H_ */
