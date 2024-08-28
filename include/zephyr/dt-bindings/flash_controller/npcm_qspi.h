/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NPCM_QSPI_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NPCM_QSPI_H_

#include <zephyr/dt-bindings/dt-util.h>

/* Software controlled Chip-Select number for transactions */
#define NPCM_QSPI_SW_CS0	BIT(0)
#define NPCM_QSPI_SW_CS1	BIT(1)
#define NPCM_QSPI_SW_CS2	BIT(2)
#define NPCM_QSPI_SW_CS_MASK	(NPCM_QSPI_SW_CS0 | NPCM_QSPI_SW_CS1 | NPCM_QSPI_SW_CS2)

/* Supported read mode for Direct Read Access */
#define NPCM_RD_MODE_NORMAL	0
#define NPCM_RD_MODE_FAST	1
#define NPCM_RD_MODE_FAST_DUAL	2
#define NPCM_RD_MODE_QUAD	3

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NPCM_QSPI_H_ */
