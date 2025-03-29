/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_I2C_I2C_RTK_RTS5912_API_H_
#define ZEPHYR_DRIVERS_I2C_I2C_RTK_RTS5912_API_H_

#include "reg/reg_i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

#define Z_REG_READ(__sz)  sys_read##__sz
#define Z_REG_WRITE(__sz) sys_write##__sz
#define Z_REG_SET_BIT     sys_set_bit
#define Z_REG_CLEAR_BIT   sys_clear_bit
#define Z_REG_TEST_BIT    sys_test_bit

#define DEFINE_MM_REG_READ(__reg, __off, __sz)                                                     \
	static inline uint32_t read_##__reg(uint32_t addr)                                         \
	{                                                                                          \
		return Z_REG_READ(__sz)(addr + __off);                                             \
	}
#define DEFINE_MM_REG_WRITE(__reg, __off, __sz)                                                    \
	static inline void write_##__reg(uint32_t data, uint32_t addr)                             \
	{                                                                                          \
		Z_REG_WRITE(__sz)(data, addr + __off);                                             \
	}

#define DEFINE_SET_BIT_OP(__reg_bit, __reg_off, __bit)                                             \
	static inline void set_bit_##__reg_bit(uint32_t addr)                                      \
	{                                                                                          \
		Z_REG_SET_BIT(addr + __reg_off, __bit);                                            \
	}

#define DEFINE_CLEAR_BIT_OP(__reg_bit, __reg_off, __bit)                                           \
	static inline void clear_bit_##__reg_bit(uint32_t addr)                                    \
	{                                                                                          \
		Z_REG_CLEAR_BIT(addr + __reg_off, __bit);                                          \
	}

#define DEFINE_TEST_BIT_OP(__reg_bit, __reg_off, __bit)                                            \
	static inline int test_bit_##__reg_bit(uint32_t addr)                                      \
	{                                                                                          \
		return Z_REG_TEST_BIT(addr + __reg_off, __bit);                                    \
	}

DEFINE_TEST_BIT_OP(con_master_mode, RTS5912_IC_REG_CON, RTS5912_IC_CON_MASTER_MODE_BIT)
DEFINE_MM_REG_WRITE(con, RTS5912_IC_REG_CON, 32)
DEFINE_MM_REG_READ(con, RTS5912_IC_REG_CON, 32)

DEFINE_MM_REG_WRITE(sdatimeout, RTS5912_IC_REG_SDA_TIMEOUT, 32)
DEFINE_MM_REG_WRITE(scltimeout, RTS5912_IC_REG_SCL_TIMEOUT, 32)

DEFINE_MM_REG_WRITE(cmd_data, RTS5912_IC_REG_DATA_CMD, 32)
DEFINE_MM_REG_READ(cmd_data, RTS5912_IC_REG_DATA_CMD, 32)

DEFINE_MM_REG_WRITE(ss_scl_hcnt, RTS5912_IC_REG_SS_SCL_HCNT, 32)
DEFINE_MM_REG_WRITE(ss_scl_lcnt, RTS5912_IC_REG_SS_SCL_LCNT, 32)

DEFINE_MM_REG_WRITE(fs_scl_hcnt, RTS5912_IC_REG_FS_SCL_HCNT, 32)
DEFINE_MM_REG_WRITE(fs_scl_lcnt, RTS5912_IC_REG_FS_SCL_LCNT, 32)

DEFINE_MM_REG_WRITE(hs_scl_hcnt, RTS5912_IC_REG_HS_SCL_HCNT, 32)
DEFINE_MM_REG_WRITE(hs_scl_lcnt, RTS5912_IC_REG_HS_SCL_LCNT, 32)

DEFINE_MM_REG_READ(txabrt_src, RTS5912_IC_REG_TXABRTSRC, 32)
DEFINE_MM_REG_READ(rawintr_stat, RTS5912_IC_REG_RAWINTR_MASK, 32)
DEFINE_MM_REG_READ(intr_stat, RTS5912_IC_REG_INTR_STAT, 32)
#define RTS5912_IC_INTR_STAT_TX_ABRT_BIT (6)
DEFINE_TEST_BIT_OP(intr_stat_tx_abrt, RTS5912_IC_REG_INTR_STAT, RTS5912_IC_INTR_STAT_TX_ABRT_BIT)

DEFINE_MM_REG_WRITE(intr_mask, RTS5912_IC_REG_INTR_MASK, 32)
#define RTS5912_IC_INTR_MASK_TX_EMPTY_BIT (4)
DEFINE_CLEAR_BIT_OP(intr_mask_tx_empty, RTS5912_IC_REG_INTR_MASK, RTS5912_IC_INTR_MASK_TX_EMPTY_BIT)
DEFINE_SET_BIT_OP(intr_mask_tx_empty, RTS5912_IC_REG_INTR_MASK, RTS5912_IC_INTR_MASK_TX_EMPTY_BIT)

DEFINE_MM_REG_WRITE(rx_tl, RTS5912_IC_REG_RX_TL, 32)
DEFINE_MM_REG_WRITE(tx_tl, RTS5912_IC_REG_TX_TL, 32)

DEFINE_MM_REG_READ(clr_intr, RTS5912_IC_REG_CLR_INTR, 32)
DEFINE_MM_REG_READ(clr_stop_det, RTS5912_IC_REG_CLR_STOP_DET, 32)
DEFINE_MM_REG_READ(clr_start_det, RTS5912_IC_REG_CLR_START_DET, 32)
DEFINE_MM_REG_READ(clr_gen_call, RTS5912_IC_REG_CLR_GEN_CALL, 32)
DEFINE_MM_REG_READ(clr_tx_abrt, RTS5912_IC_REG_CLR_TX_ABRT, 32)
DEFINE_MM_REG_READ(clr_rx_under, RTS5912_IC_REG_CLR_RX_UNDER, 32)
DEFINE_MM_REG_READ(clr_rx_over, RTS5912_IC_REG_CLR_RX_OVER, 32)
DEFINE_MM_REG_READ(clr_tx_over, RTS5912_IC_REG_CLR_TX_OVER, 32)
DEFINE_MM_REG_READ(clr_rx_done, RTS5912_IC_REG_CLR_RX_DONE, 32)
DEFINE_MM_REG_READ(clr_rd_req, RTS5912_IC_REG_CLR_RD_REQ, 32)
DEFINE_MM_REG_READ(clr_activity, RTS5912_IC_REG_CLR_ACTIVITY, 32)

#define RTS5912_IC_ENABLE_EN_BIT         (0)
#define RTS5912_IC_ENABLE_ABORT_BIT      (1)
#define RTS5912_IC_ENABLE_BLOCK_BIT      (2)
#define RTS5912_IC_ENABLE_SDARECOVEN_BIT (3)
#define RTS5912_IC_ENABLE_CLK_RESET_BIT  (16)
DEFINE_CLEAR_BIT_OP(enable_en, RTS5912_IC_REG_ENABLE, RTS5912_IC_ENABLE_EN_BIT)
DEFINE_SET_BIT_OP(enable_en, RTS5912_IC_REG_ENABLE, RTS5912_IC_ENABLE_EN_BIT)
DEFINE_CLEAR_BIT_OP(enable_block, RTS5912_IC_REG_ENABLE, RTS5912_IC_ENABLE_BLOCK_BIT)
DEFINE_SET_BIT_OP(enable_block, RTS5912_IC_REG_ENABLE, RTS5912_IC_ENABLE_BLOCK_BIT)
DEFINE_CLEAR_BIT_OP(enable_abort, RTS5912_IC_REG_ENABLE, RTS5912_IC_ENABLE_ABORT_BIT)
DEFINE_SET_BIT_OP(enable_abort, RTS5912_IC_REG_ENABLE, RTS5912_IC_ENABLE_ABORT_BIT)
DEFINE_TEST_BIT_OP(enable_abort, RTS5912_IC_REG_ENABLE, RTS5912_IC_ENABLE_ABORT_BIT)
DEFINE_CLEAR_BIT_OP(enable_clk_reset, RTS5912_IC_REG_ENABLE, RTS5912_IC_ENABLE_CLK_RESET_BIT)
DEFINE_SET_BIT_OP(enable_clk_reset, RTS5912_IC_REG_ENABLE, RTS5912_IC_ENABLE_CLK_RESET_BIT)
DEFINE_TEST_BIT_OP(enable_clk_reset, RTS5912_IC_REG_ENABLE, RTS5912_IC_ENABLE_CLK_RESET_BIT)
DEFINE_CLEAR_BIT_OP(enable_sdarecov, RTS5912_IC_REG_ENABLE, RTS5912_IC_ENABLE_SDARECOVEN_BIT)
DEFINE_SET_BIT_OP(enable_sdarecov, RTS5912_IC_REG_ENABLE, RTS5912_IC_ENABLE_SDARECOVEN_BIT)
DEFINE_TEST_BIT_OP(enable_sdarecov, RTS5912_IC_REG_ENABLE, RTS5912_IC_ENABLE_SDARECOVEN_BIT)
DEFINE_MM_REG_WRITE(enable, RTS5912_IC_REG_ENABLE, 32)

#define RTS5912_IC_STATUS_ACTIVITY_BIT    (0)
#define RTS5912_IC_STATUS_TFNT_BIT        (1)
#define RTS5912_IC_STATUS_RFNE_BIT        (3)
#define RTS5912_IC_STATUS_SDANOTRECOV_BIT (11)
DEFINE_TEST_BIT_OP(status_activity, RTS5912_IC_REG_STATUS, RTS5912_IC_STATUS_ACTIVITY_BIT)
DEFINE_TEST_BIT_OP(status_tfnt, RTS5912_IC_REG_STATUS, RTS5912_IC_STATUS_TFNT_BIT)
DEFINE_TEST_BIT_OP(status_rfne, RTS5912_IC_REG_STATUS, RTS5912_IC_STATUS_RFNE_BIT)
DEFINE_TEST_BIT_OP(status_sdanotrecov, RTS5912_IC_REG_STATUS, RTS5912_IC_STATUS_SDANOTRECOV_BIT)

DEFINE_MM_REG_READ(txflr, RTS5912_IC_REG_TXFLR, 32)
DEFINE_MM_REG_READ(rxflr, RTS5912_IC_REG_RXFLR, 32)

DEFINE_MM_REG_READ(dma_cr, RTS5912_IC_REG_DMA_CR, 32)
DEFINE_MM_REG_WRITE(dma_cr, RTS5912_IC_REG_DMA_CR, 32)

DEFINE_MM_REG_READ(tdlr, RTS5912_IC_REG_TDLR, 32)
DEFINE_MM_REG_WRITE(tdlr, RTS5912_IC_REG_TDLR, 32)
DEFINE_MM_REG_READ(rdlr, RTS5912_IC_REG_RDLR, 32)
DEFINE_MM_REG_WRITE(rdlr, RTS5912_IC_REG_RDLR, 32)

DEFINE_MM_REG_READ(fs_spklen, RTS5912_IC_REG_FS_SPKLEN, 32)
DEFINE_MM_REG_READ(hs_spklen, RTS5912_IC_REG_HS_SPKLEN, 32)
DEFINE_MM_REG_READ(spklen, RTS5912_IC_REG_FS_SPKLEN, 32)
DEFINE_MM_REG_READ(sdahold, RTS5912_IC_REG_SDAHOLD, 32)
DEFINE_MM_REG_WRITE(spklen, RTS5912_IC_REG_FS_SPKLEN, 32)
DEFINE_MM_REG_WRITE(sdahold, RTS5912_IC_REG_SDAHOLD, 32)

DEFINE_MM_REG_READ(comp_param_1, RTS5912_IC_REG_COMP_PARAM_1, 32)
DEFINE_MM_REG_READ(comp_type, RTS5912_IC_REG_COMP_TYPE, 32)
DEFINE_MM_REG_READ(tar, RTS5912_IC_REG_TAR, 32)
DEFINE_MM_REG_WRITE(tar, RTS5912_IC_REG_TAR, 32)
DEFINE_MM_REG_WRITE(sar, RTS5912_IC_REG_SAR, 32)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_I2C_I2C_RTK_RTS5912_API_H_ */
