/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RISCV_TELINK_W91_SOC_H
#define RISCV_TELINK_W91_SOC_H

#define NDS_MXSTATUS            0x7c4
#define NDS_MCACHE_CTL          0x7ca
#define NDS_MMISC_CTL           0x7d0

#define D25_RESET_VECTOR        0xf0100200
#define N22_RESET_VECTOR        0xf0100204
#define HART_RESET_CTRL         0xf1700218

#define D25_START_OFFSET        0xa0
#define D25_START_ADDR          (CONFIG_FLASH_BASE_ADDRESS + D25_START_OFFSET)
#define N22_START_ADDR          (CONFIG_FLASH_BASE_ADDRESS + CONFIG_TELINK_W91_N22_PARTITION_ADDR)

#define D25_MASK                (1 << (CONFIG_RV_BOOT_HART * 4))
#define N22_MASK                (1 << (CONFIG_TELINK_W91_N22_HART * 4))

#endif /* RISCV_TELINK_W91_SOC_H */
