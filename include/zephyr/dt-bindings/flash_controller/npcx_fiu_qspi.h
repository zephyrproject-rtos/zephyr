/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_FLASH_CONTROLLER_NPCX_FIU_QSPI_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_FLASH_CONTROLLER_NPCX_FIU_QSPI_H_

#include <zephyr/dt-bindings/dt-util.h>

/* Software controlled Chip-Select number for UMA transactions */
#define NPCX_QSPI_SW_CS0	BIT(0)
#define NPCX_QSPI_SW_CS1	BIT(1)
#define NPCX_QSPI_SW_CS2	BIT(2)
#define NPCX_QSPI_SW_CS_MASK	(NPCX_QSPI_SW_CS0 | NPCX_QSPI_SW_CS1 | NPCX_QSPI_SW_CS2)

/* Supported flash interfaces for UMA transactions */
#define NPCX_QSPI_SEC_FLASH_SL	BIT(4)
#define NPCX_QSPI_PVT_FLASH_SL	BIT(5)
#define NPCX_QSPI_SHD_FLASH_SL	BIT(6)
#define NPCX_QSPI_BKP_FLASH_SL	BIT(7)

/* Supported read mode for Direct Read Access */
#define NPCX_RD_MODE_NORMAL	0
#define NPCX_RD_MODE_FAST	1
#define NPCX_RD_MODE_FAST_DUAL	3

#if defined(CONFIG_NPCX_SOC_VARIANT_NPCKN)
/* Maximum length of a valid read burst */
#define NPCX_BURST_MODE_1_BYTE  0
#define NPCX_BURST_MODE_16_BYTE 3
#endif

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_FLASH_CONTROLLER_NPCX_FIU_QSPI_H_ */
