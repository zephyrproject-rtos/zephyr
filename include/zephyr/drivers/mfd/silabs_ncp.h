/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Silicon Labs network co-processor: CPC bus of a "silabs,ncp" device.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_SILABS_NCP_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_SILABS_NCP_H_

#include <zephyr/device.h>

#include "sl_cpc_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup mfd_interface_silabs_ncp MFD Silicon Labs network co-processor interface
 * @ingroup mfd_interfaces
 * @{
 */

/**
 * @brief Common data of the "silabs,ncp" drivers.
 *
 * Must be the first member of the data of every transport.
 */
struct mfd_silabs_ncp_data {
	/** CPC bus of the co-processor */
	struct sl_cpc_bus cpc_bus;
};

/**
 * @brief Get the CPC bus of a Silicon Labs network co-processor.
 *
 * Child drivers use the bus to open CPC endpoints on the co-processor. The
 * bus is usable once the "silabs,ncp" device is ready.
 *
 * @param dev "silabs,ncp" device.
 * @return The CPC bus.
 */
static inline struct sl_cpc_bus *mfd_silabs_ncp_cpc_bus(const struct device *dev)
{
	struct mfd_silabs_ncp_data *data = dev->data;

	return &data->cpc_bus;
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_SILABS_NCP_H_ */
