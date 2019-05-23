/*
 * Copyright (c) 2019 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* SoC level DTS fixup file */

#if defined(CONFIG_SOC_OPENISA_RV32M1_RISCV32)

#define DT_FLASH_DEV_BASE_ADDRESS DT_OPENISA_RV32M1_FTFE_40023000_BASE_ADDRESS
#define DT_FLASH_DEV_NAME         DT_OPENISA_RV32M1_FTFE_40023000_LABEL
#define DT_START_UP_ENTRY_OFFSET  0x80

#endif /* CONFIG_SOC_OPENISA_RV32M1_RISCV32 */

/* End of SoC Level DTS fixup file */
