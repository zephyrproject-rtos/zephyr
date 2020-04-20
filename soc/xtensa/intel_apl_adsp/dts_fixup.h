/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* SoC level DTS fixup file */

#define DT_L2_SRAM_BASE			CONFIG_SRAM_BASE_ADDRESS
#define DT_L2_SRAM_SIZE			(CONFIG_SRAM_SIZE * 1024)

#define DT_LP_SRAM_BASE			DT_REG_ADDR(DT_INST(1, mmio_sram))
#define DT_LP_SRAM_SIZE			DT_REG_SIZE(DT_INST(1, mmio_sram))

/* End of SoC Level DTS fixup file */
