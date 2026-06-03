/*
 * Copyright (c) 2022, Prevas A/S
 * Copyright (c) 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for the NXP TPM-based quadrature decoder.
 * @ingroup qdec_nxp_tpm_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_QDEC_NXP_TPM_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_QDEC_NXP_TPM_H_

/**
 * @brief NXP TPM-based quadrature decoder
 * @defgroup qdec_nxp_tpm_interface NXP TPM QDEC
 * @ingroup sensor_interface_ext
 * @{
 */

#include <zephyr/drivers/sensor.h>

/** @brief Extended sensor attributes for the NXP TPM quadrature decoder. */
enum sensor_attribute_qdec_tpm {
	/** Number of counts per revolution. */
	SENSOR_ATTR_QDEC_MOD_VAL = SENSOR_ATTR_PRIV_START,
};

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_QDEC_NXP_TPM_H_ */
