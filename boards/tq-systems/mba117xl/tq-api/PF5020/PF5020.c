/*!
 * SPDX-License-Identifier: Apache 2.0
 * Copyright (c) 2025 TQ-Systems GmbH <oss@ew.tq-group.com>,
 * D-82229 Seefeld, Germany.
 * Author: Isaac L. L. Yuki
 */

/*!
 * \addtogroup PF5020
 * \brief PF5020 Driver.
 * @{
 * \file
 */

/*******************************************************************************
 * Includes
 ******************************************************************************/

#include "PF5020.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define PMIC_DEVICE_ID_ADDRESS 0x0
#define PMIC_REG_DATA_SIZE     (1U)
#define PMIC_REG_ADDRESS_SIZE  (1U)
#define PMIC_DEVICE_ADDRESS    0x08

#define PMIC_ERROR_NO_MATCH_WITH_FACTORY_DATA 200
#define PMIC_ERROR_VALUE_ND                   201

typedef const enum PMIC_Banks {
	FREQ_CTRL,
	SW1_RUN_VOLT,
	SW1_PWRUP,
	SW1_CONFIG1,
	SW1_CONFIG2,
	SW2_RUN_VOLT,
	SW2_PWRUP,
	SW2_CONFIG1,
	SW2_CONFIG2,
	SW_RAMP,
	PMIC_BANK_ARRAY_SIZE
} PMIC_Banks_t;

typedef struct Register_tag {
	uint8_t address;
	uint8_t value;
	uint8_t mask;
} Register_t;

/*******************************************************************************
 * Variables
 ******************************************************************************/

static Register_t Banks[PMIC_BANK_ARRAY_SIZE] = {
	[FREQ_CTRL] = {.address = 0x3A},   [SW1_RUN_VOLT] = {.address = 0x4B},
	[SW1_PWRUP] = {.address = 0x49},   [SW1_CONFIG1] = {.address = 0x47},
	[SW1_CONFIG2] = {.address = 0x48}, [SW2_RUN_VOLT] = {.address = 0x53},
	[SW2_PWRUP] = {.address = 0x51},   [SW2_CONFIG1] = {.address = 0x4F},
	[SW2_CONFIG2] = {.address = 0x50}, [SW_RAMP] = {.address = 0x46},
};

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 *  Code
 ******************************************************************************/

/*!
 * \brief PMIC_SetCoreVoltage
 * This function configures a new VCC_SOC voltage. Range 0.9V...1.1V.
 * Voltages fixed by PMIC_VCC_SEL_t.
 * \param[in] newVoltage	Voltage by defined by PMIC_VCC_SEL_t.
 * \return Status of transfer. See API documentation for error codes.
 * \note Assumes that the I2C master is properly configured before this
 * function is called.
 */
uint32_t PF5020_setCoreVoltage(PF5020_VCC_SEL_t newVoltage, PF5020_Handle_t *handle)
{
	uint32_t status;

	if ((newVoltage >= PMIC_VDD_SOC_0V900) && (newVoltage <= PMIC_VDD_SOC_1V100)) {
		Banks[SW1_RUN_VOLT].value = newVoltage;

		status = handle->transfer(handle->peripheral, Banks[SW1_RUN_VOLT].address,
					  PMIC_REG_ADDRESS_SIZE, &Banks[SW1_RUN_VOLT].value,
					  PMIC_REG_DATA_SIZE, PF5020_WRITE);
	} else {
		status = PMIC_ERROR_VALUE_ND;
	}
	return status;
}

/*!
 * \brief Reads the core voltage from the PMIC.
 * This function reads the core voltage level from the PMIC by accessing a
 * specific register over I2C. The voltage is calculated based on the register
 * value.
 * \param[out] voltage Pointer to a float where the core voltage will be stored.
 * \return Status of the I2C transfer. Refer to the LPI2C_MasterTransferBlocking
 * documentation for potential error codes.
 * \note Assumes that the I2C master is properly configured before this function
 * is called.
 */
uint32_t PF5020_readCoreVoltage(float *voltage, PF5020_Handle_t *handle)
{
	uint32_t status;
	uint8_t registerValue;
	/* read */
	status = handle->transfer(handle->peripheral, Banks[SW1_RUN_VOLT].address,
				  PMIC_REG_ADDRESS_SIZE, &registerValue, PMIC_REG_DATA_SIZE,
				  PF5020_READ);
	;
	*voltage = (float)(0.4 + ((double)registerValue * 0.00625));
	return status;
}

/*!
 * \brief Reads the device ID from the PMIC.
 * This function reads the device ID of the PMIC by sending a read command to
 * the I2C address reserved for the device ID register.
 * \param[out] deviceId Pointer to a uint8_t where the device ID will be stored.
 * \return Status of the I2C transfer. Refer to the LPI2C_MasterTransferBlocking
 * documentation for potential error codes.
 * \note Assumes that the I2C master is properly configured before this function
 * is called.
 */
uint32_t PF5020_readDeviceID(uint8_t *deviceId, PF5020_Handle_t *handle)
{
	uint32_t status;
	status = handle->transfer(handle->peripheral, PMIC_DEVICE_ID_ADDRESS, PMIC_REG_ADDRESS_SIZE,
				  deviceId, PMIC_REG_DATA_SIZE, PF5020_READ);

	return status;
}

/*!@}*/
