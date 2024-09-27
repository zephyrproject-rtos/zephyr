/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NPCK_FIU_QSPI_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NPCK_FIU_QSPI_H_

#include <zephyr/dt-bindings/dt-util.h>

/* Software controlled Chip-Select number for UMA transactions */
#define NPCX_QSPI_SW_CS0	BIT(0)
#define NPCX_QSPI_SW_CS1	BIT(1)
#define NPCX_QSPI_SW_CS2	BIT(2)
#define NPCX_QSPI_SW_CS_MASK	(NPCX_QSPI_SW_CS0 | NPCX_QSPI_SW_CS1 | NPCX_QSPI_SW_CS2)

/* Supported flash interfaces for UMA transactions */
#define NPCX_QSPI_SEC_FLASH_SL	BIT(4)

/* Supported read mode for Direct Read Access */
#define NPCX_RD_MODE_NORMAL	0
#define NPCX_RD_MODE_FAST	1
#define NPCX_RD_MODE_FAST_DUAL	3

#define NPCX_SPI_DEV_SIZE_1M    0
#define NPCX_SPI_DEV_SIZE_2M    1
#define NPCX_SPI_DEV_SIZE_4M    2
#define NPCX_SPI_DEV_SIZE_8M    3
#define NPCX_SPI_DEV_SIZE_16M   4
#define NPCX_SPI_DEV_SIZE_32M   5
#define NPCX_SPI_DEV_SIZE_64M   6
#define NPCX_SPI_DEV_SIZE_128M  7

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NPCK_FIU_QSPI_H_ */
