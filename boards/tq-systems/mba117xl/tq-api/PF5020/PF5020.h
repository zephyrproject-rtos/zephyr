/*!
 * Copyright (c) 2025 TQ-Systems GmbH <oss@ew.tq-group.com>
 * SPDX-License-Identifier: Apache 2.0
 * Author: Isaac L. L. Yuki
 */

#ifndef PF5020_H_
#define PF5020_H_

/*!
 * \defgroup PF5020 PF5020
 * \brief PF5020 Driver.
 * @{
 * \file
 */

/*******************************************************************************
 * Includes
 ******************************************************************************/

#include <stdint.h>
#include <stddef.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*!
 * \brief Enumeration of possible data transfer directions.
 */
typedef enum PF5020_TransferDirection {
	PF5020_READ, /**< Indicates a read operation.*/
	PF5020_WRITE /**< Indicates a write operation.*/
} PF5020_TransferDirection_t;

typedef uint32_t (*PF5020_TransferFunction_t)(const void *peripheral, uint8_t regAddress,
					      size_t regAddressSize, uint8_t *buffer,
					      uint8_t dataSize,
					      PF5020_TransferDirection_t transferDirection);

/*!
 * \brief Enumeration for selecting voltage levels for VDD_SOC.
 * This enumeration is used to set different voltage levels for the System on
 * Chip (SoC) power supply (VDD_SOC) through the PMIC.
 */
typedef enum PF5020_VCC_SEL {
	PMIC_VDD_SOC_1V000 = 96,  /**< Selects a core voltage of 1.000V for the SoC.*/
	PMIC_VDD_SOC_1V100 = 112, /**< Selects a core voltage of 1.100V for the SoC.*/
	PMIC_VDD_SOC_0V900 = 80,  /**< Selects a core voltage of 0.900V for the SoC.*/
} PF5020_VCC_SEL_t;

/*!
 * \brief handle structure for the SE97BTP temperature sensor driver.
 * \details The data buffer is sized with 2 bytes due to the sensor output.
 */
typedef struct PF5020_Handle {
	const void *peripheral; /**< Data buffer (1 bytes) where raw data is stored.*/
	PF5020_TransferFunction_t transfer;
} PF5020_Handle_t;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

extern uint32_t PF5020_setCoreVoltage(PF5020_VCC_SEL_t newVoltage, PF5020_Handle_t *handle);
extern uint32_t PF5020_readCoreVoltage(float *voltage, PF5020_Handle_t *handle);
extern uint32_t PF5020_readDeviceID(uint8_t *deviceId, PF5020_Handle_t *handle);

/*!@}*/

#endif /* PMIC_H_ */
