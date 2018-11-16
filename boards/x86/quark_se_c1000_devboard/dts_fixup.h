/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is a temporary workaround for mapping of the generated information
 * to the current driver definitions.  This will be removed when the drivers
 * are modified to handle the generated information, or the mapping of
 * generated data matches the driver definitions.
 */

#define DT_IEEE802154_CC2520_SPI_DRV_NAME		DT_SNPS_DESIGNWARE_SPI_B0001400_TI_CC2520_0_BUS_NAME
#define DT_IEEE802154_CC2520_SPI_SLAVE			DT_SNPS_DESIGNWARE_SPI_B0001400_TI_CC2520_0_BASE_ADDRESS
#define DT_IEEE802154_CC2520_SPI_FREQ			DT_SNPS_DESIGNWARE_SPI_B0001400_TI_CC2520_0_SPI_MAX_FREQUENCY
#define DT_IEEE802154_CC2520_GPIO_SPI_CS_DRV_NAME	DT_SNPS_DESIGNWARE_SPI_B0001400_CS_GPIOS_CONTROLLER
#define DT_IEEE802154_CC2520_GPIO_SPI_CS_PIN		DT_SNPS_DESIGNWARE_SPI_B0001400_CS_GPIOS_PIN
