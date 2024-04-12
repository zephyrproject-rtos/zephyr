/*
 * Copyright (c) 2024 SILA Embedded Solutions GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_BD8LB600FS_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_BD8LB600FS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/device.h>

/**
 * @defgroup mdf_interface_bd8lb600fs MFD BD8LB600FS interface
 * @ingroup mfd_interfaces
 * @{
 */

/**
 * @brief set outputs
 *
 * @param[in] dev instance of BD8LB600FS MFD
 * @param[in] values values for outputs, one bit per output
 *
 * @retval 0 if successful
 */
int mfd_bd8lb600fs_set_outputs(const struct device *dev, uint32_t values);
/**
 * @brief get output diagnostics
 *
 * Fetch the current diagnostics from all instances, as multiple
 * instances might be daisy chained together. Each bit in old
 * and ocp_or_tsd corresponds to one output. A set bit means that the
 * function is active, therefore either there is an open load, an over
 * current or a too high temperature.
 *
 * OLD - open load
 * OCP - over current protection
 * TSD - thermal shutdown
 *
 * @param[in] dev instance of BD8LB600FS MFD
 * @param[out] old open load values
 * @param[out] ocp_or_tsd over current protection or thermal shutdown values
 *
 * @retval 0 if successful
 */
int mfd_bd8lb600fs_get_output_diagnostics(const struct device *dev, uint32_t *old,
					  uint32_t *ocp_or_tsd);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_BD8LB600FS_H_ */
