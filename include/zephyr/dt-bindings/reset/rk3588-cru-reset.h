/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 KylinSoft Corporation
 * SPDX-License-Identifier: Apache-2.0
 *
 * RK3588 CRU reset identifiers.
 *
 * An identifier encodes the register and bit of the reset in the CRU mapping:
 *
 *   id = cru_block_offset * 4 + register_index * 16 + bit
 *
 * where cru_block_offset is 0 for the main CRU, 0x8000 for the PHPTOP CRU,
 * 0x10000 for the secure CRU and 0x30000 for the PMU CRU. The reset driver
 * derives the SOFT_RST_CON register offset from (id / 16) * 4 + 0xa00 and the
 * reset bit from id % 16.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_RK3588_CRU_RESET_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_RK3588_CRU_RESET_H_

#define SRST_A_GMAC0        522
#define SRST_A_GMAC1        523
#define SRST_PCIE4_POWER_UP 529
#define SRST_P_PCIE4        544
#define SRST_REF_PIPE_PHY0  1238
#define SRST_P_PCIE2_PHY0   131077

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RESET_RK3588_CRU_RESET_H_ */
