/*
 * Copyright (c) 2022, Prevas A/S
 * Copyright (c) 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of NXP TPM QDEC sensor
 * @ingroup qdec_nxp_tpm_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_QDEC_NXP_TPM_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_QDEC_NXP_TPM_H_

#include <zephyr/drivers/sensor.h>

/**
 * @brief NXP TPM quadrature decoder
 * @defgroup qdec_nxp_tpm_interface QDEC NXP TPM
 * @ingroup sensor_interface_ext_nxp
 * @{
 */

/** @brief Custom sensor attributes for NXP TPM QDEC */
enum sensor_attribute_qdec_tpm {
	/** Number of counts per revolution */
	SENSOR_ATTR_QDEC_MOD_VAL = SENSOR_ATTR_PRIV_START,
};

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_QDEC_NXP_TPM_H_ */
