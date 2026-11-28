/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Select which macsw config file to include based on Kconfig choices for WIFI6 stack
 * These files define:
 * - Aggregated MAC Service Data Unit (AMSDU) Size
 * - Reorder buffer count
 * - Number of RX and TX descriptors
 * - Number of RX and TX 'Block Acks'
 * - Target Wake Time
 *
 * The SDK offers only Default, ACK, and Bridge blobs, so only these's files are picked.
 */

#ifndef MODULES_HAL_BOUFFALOLAB_SHIM_WIFI_MACSW_CONFIG_SHIM_H_
#define MODULES_HAL_BOUFFALOLAB_SHIM_WIFI_MACSW_CONFIG_SHIM_H_

#if defined(CONFIG_WIFI_BFLB_WIFI6_MACSW_DEFAULT)

#include "macsw_default_config.h"

#elif defined(CONFIG_WIFI_BFLB_WIFI6_MACSW_BRIDGE)

#include "macsw_bridge_config.h"

#elif defined(CONFIG_WIFI_BFLB_WIFI6_MACSW_ACK)

#include "macsw_ack_config.h"

#else

#error Unknown MACSW configuration

#endif

#endif /* MODULES_HAL_BOUFFALOLAB_SHIM_WIFI_MACSW_CONFIG_SHIM_H_ */
