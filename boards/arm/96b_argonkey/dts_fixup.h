/*
 * Copyright (c) 2018 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is a temporary workaround for mapping of the generated information
 * to the current driver definitions.  This will be removed when the drivers
 * are modified to handle the generated information, or the mapping of
 * generated data matches the driver definitions.
 */

#define CONFIG_LP3943_DEV_NAME			DT_ST_STM32_I2C_V1_40005C00_TI_LP3943_60_LABEL
#define CONFIG_LP3943_I2C_ADDRESS		DT_ST_STM32_I2C_V1_40005C00_TI_LP3943_60_BASE_ADDRESS
#define CONFIG_LP3943_I2C_MASTER_DEV_NAME	DT_ST_STM32_I2C_V1_40005C00_TI_LP3943_60_BUS_NAME
