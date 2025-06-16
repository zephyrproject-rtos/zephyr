/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NCT_QSPI_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NCT_QSPI_H_

#include <zephyr/dt-bindings/dt-util.h>

/* Software controlled Chip-Select number for transactions */
#define NCT_QSPI_SW_CS0	BIT(0)
#define NCT_QSPI_SW_CS1	BIT(1)
#define NCT_QSPI_SW_CS2	BIT(2)
#define NCT_QSPI_SW_CS_MASK	(NCT_QSPI_SW_CS0 | NCT_QSPI_SW_CS1 | NCT_QSPI_SW_CS2)

/* Supported read mode for Direct Read Access */
#define NCT_RD_MODE_NORMAL	0
#define NCT_RD_MODE_FAST	1
#define NCT_RD_MODE_FAST_DUAL	2
#define NCT_RD_MODE_QUAD	3

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NCT_QSPI_H_ */
