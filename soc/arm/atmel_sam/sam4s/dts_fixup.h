/*
 * Copyright (c) 2019 Gerson Fernando Budke
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* This file is a temporary workaround for mapping of the generated information
 * to the current driver definitions.  This will be removed when the drivers
 * are modified to handle the generated information, or the mapping of
 * generated data matches the driver definitions.
 */

/* SoC level DTS fixup file */

#define DT_SPI_0_BASE_ADDRESS                DT_ATMEL_SAM_SPI_40008000_BASE_ADDRESS
#define DT_SPI_0_NAME                        DT_ATMEL_SAM_SPI_40008000_LABEL
#define DT_SPI_0_IRQ                         DT_ATMEL_SAM_SPI_40008000_IRQ_0
#define DT_SPI_0_IRQ_PRI                     DT_ATMEL_SAM_SPI_40008000_IRQ_0_PRIORITY
#define DT_SPI_0_PERIPHERAL_ID               DT_ATMEL_SAM_SPI_40008000_PERIPHERAL_ID

#define DT_FLASH_DEV_BASE_ADDRESS            DT_ATMEL_SAM_FLASH_CONTROLLER_400E0A00_BASE_ADDRESS
#define DT_FLASH_DEV_NAME                    DT_ATMEL_SAM_FLASH_CONTROLLER_400E0A00_LABEL
/* End of SoC Level DTS fixup file */
