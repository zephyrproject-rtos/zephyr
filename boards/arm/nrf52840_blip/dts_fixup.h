/*
 * Copyright (c) 2019 Tavish Naruka <tavishnaruka@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* This file is a temporary workaround for mapping of the generated information
 * to the current driver definitions.  This will be removed when the drivers
 * are modified to handle the generated information, or the mapping of
 * generated data matches the driver definitions.
 */

#define DT_DISK_SDHC0_BUS_ADDRESS			DT_NORDIC_NRF_SPI_40004000_ZEPHYR_MMC_SPI_SLOT_0_BASE_ADDRESS
#define DT_DISK_SDHC0_BUS_NAME				DT_NORDIC_NRF_SPI_40004000_ZEPHYR_MMC_SPI_SLOT_0_BUS_NAME
#define DT_DISK_SDHC0_CS_GPIOS_CONTROLLER		SPI_1_CS_GPIOS_CONTROLLER
#define DT_DISK_SDHC0_CS_GPIOS_PIN			SPI_1_CS_GPIOS_PIN
