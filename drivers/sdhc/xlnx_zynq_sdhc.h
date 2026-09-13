/*
 * Copyright (c) 2026 David Paul-Beier
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SDHC_XLNX_ZYNQ_SDHC_H_
#define ZEPHYR_DRIVERS_SDHC_XLNX_ZYNQ_SDHC_H_

/* Bit map for status register */
#define XLNX_ZYNQ_SDHC_INTR_CC_MASK  BIT(0)
#define XLNX_ZYNQ_SDHC_INTR_TC_MASK  BIT(1)
#define XLNX_ZYNQ_SDHC_INTR_BRR_MASK BIT(5)
#define XLNX_ZYNQ_SDHC_INTR_BWR_MASK BIT(4)
#define XLNX_ZYNQ_SDHC_INTR_ERR_MASK BIT(15)
#define XLNX_ZYNQ_SDHC_NORM_INTR_ALL 0xFFFFU

/* Bit map for error register */
#define XLNX_ZYNQ_SDHC_ERROR_INTR_ALL 0xF3FFU

/* Bit map for present state register */
#define XLNX_ZYNQ_SDHC_PSR_CMD_INHIBIT_MASK BIT(0)
#define XLNX_ZYNQ_SDHC_PSR_INHIBIT_DAT_MASK BIT(1)
#define XLNX_ZYNQ_SDHC_PSR_CARD_INSRT_MASK  BIT(16)

/* Bit map for transfer mode register */
#define XLNX_ZYNQ_SDHC_TM_BLK_CNT_EN_MASK      BIT(1)
#define XLNX_ZYNQ_SDHC_TM_AUTO_CMD12_EN_MASK   BIT(2)
#define XLNX_ZYNQ_SDHC_TM_DAT_DIR_SEL_MASK     BIT(4)
#define XLNX_ZYNQ_SDHC_TM_MUL_SIN_BLK_SEL_MASK BIT(5)

/* Bit map for host control1 register. No 8-bit width -- the controller
 * exposes only an SD slot (no eMMC) on every known Zynq-7000 carrier board.
 */
#define XLNX_ZYNQ_SDHC_DAT_WIDTH4_MASK       BIT(1)
#define XLNX_ZYNQ_SDHC_HS_SPEED_MODE_EN_MASK BIT(2)

/* Bit map for power control register */
#define XLNX_ZYNQ_SDHC_PC_BUS_PWR_MASK BIT(0)
#define XLNX_ZYNQ_SDHC_PC_BUS_VSEL_3V3 0x0EU
#define XLNX_ZYNQ_SDHC_PC_BUS_VSEL_3V0 0x0CU

/* Bit map for host control2 register -- 1.8V switch only. This controller
 * generation has no execute-tuning / sampling-clock-select bits (no UHS).
 */
#define XLNX_ZYNQ_SDHC_HC2_1V8_EN_MASK BIT(3)

/* Bit map to read host capabilities register (SDHCI 3.0 layout) */
#define XLNX_ZYNQ_SDHC_1P8_VOL_SUPPORT           26U
#define XLNX_ZYNQ_SDHC_3P0_VOL_SUPPORT           25U
#define XLNX_ZYNQ_SDHC_3P3_VOL_SUPPORT           24U
#define XLNX_ZYNQ_SDHC_3P0_CURRENT_SUPPORT_SHIFT 8U
#define XLNX_ZYNQ_SDHC_1P8_CURRENT_SUPPORT_SHIFT 16U
#define XLNX_ZYNQ_SDHC_CURRENT_BYTE              0xFFU
#define XLNX_ZYNQ_SDHC_SDMA_SUPPORT              22U
#define XLNX_ZYNQ_SDHC_HIGH_SPEED_SUPPORT        21U
#define XLNX_ZYNQ_SDHC_ADMA2_SUPPORT             19U
#define XLNX_ZYNQ_SDHC_MAX_BLK_LEN_SHIFT         16U
#define XLNX_ZYNQ_SDHC_MAX_BLK_LEN               3U
#define XLNX_ZYNQ_SDHC_4BIT_SUPPORT              18U
#define XLNX_ZYNQ_SDHC_BASE_CLK_SHIFT            8U
#define XLNX_ZYNQ_SDHC_BASE_CLK_MASK             0xFFU

/*
 * HW-observed quirk (TE0726 ZynqBerry, xc7z010, Trenz factory FSBL):
 * the Capabilities base-clock-frequency field reads 0 on real silicon
 * instead of the documented value. Every Zynq-7000 board known to this
 * driver ties SDIO's ref clock to the same 50 MHz IOPLL-derived source
 * the FSBL programs, so this is the fallback when the field is 0 -- NOT a
 * datasheet constant. Re-verify per board if the capability field ever
 * reports non-zero on a given carrier.
 */
#define XLNX_ZYNQ_SDHC_FALLBACK_BASE_CLK_HZ 50000000U

#define XLNX_ZYNQ_SDHC_SD_SLOT 0x0U

/* Bit map for response types */
#define XLNX_ZYNQ_SDHC_CMD_RESP_NONE        0x0U
#define XLNX_ZYNQ_SDHC_CMD_RESP_L136_MASK   BIT(0)
#define XLNX_ZYNQ_SDHC_CMD_RESP_L48_MASK    BIT(1)
#define XLNX_ZYNQ_SDHC_CMD_RESP_L48_BSY_CHK 0x3U
#define XLNX_ZYNQ_SDHC_CMD_CRC_CHK_EN_MASK  BIT(3)
#define XLNX_ZYNQ_SDHC_CMD_INX_CHK_EN_MASK  BIT(4)
#define XLNX_ZYNQ_SDHC_CMD_RESP_INVAL       0xFFU
#define XLNX_ZYNQ_SDHC_OPCODE_SHIFT         0x8U
#define XLNX_ZYNQ_SDHC_RESP                 0xFU

#define XLNX_ZYNQ_SDHC_RESP_NONE XLNX_ZYNQ_SDHC_CMD_RESP_NONE
#define XLNX_ZYNQ_SDHC_RESP_R1B                                                                    \
	(XLNX_ZYNQ_SDHC_CMD_RESP_L48_BSY_CHK | XLNX_ZYNQ_SDHC_CMD_CRC_CHK_EN_MASK |                \
	 XLNX_ZYNQ_SDHC_CMD_INX_CHK_EN_MASK)
#define XLNX_ZYNQ_SDHC_RESP_R1                                                                     \
	(XLNX_ZYNQ_SDHC_CMD_RESP_L48_MASK | XLNX_ZYNQ_SDHC_CMD_CRC_CHK_EN_MASK |                   \
	 XLNX_ZYNQ_SDHC_CMD_INX_CHK_EN_MASK)
#define XLNX_ZYNQ_SDHC_RESP_R2                                                                     \
	(XLNX_ZYNQ_SDHC_CMD_RESP_L136_MASK | XLNX_ZYNQ_SDHC_CMD_CRC_CHK_EN_MASK)
#define XLNX_ZYNQ_SDHC_RESP_R3 XLNX_ZYNQ_SDHC_CMD_RESP_L48_MASK
#define XLNX_ZYNQ_SDHC_RESP_R6                                                                     \
	(XLNX_ZYNQ_SDHC_CMD_RESP_L48_MASK | XLNX_ZYNQ_SDHC_CMD_CRC_CHK_EN_MASK |                   \
	 XLNX_ZYNQ_SDHC_CMD_INX_CHK_EN_MASK)

/* Bit map to update response type */
#define XLNX_ZYNQ_SDHC_CRC_LEFT_SHIFT  0x8U
#define XLNX_ZYNQ_SDHC_CRC_RIGHT_SHIFT 0x18U

/* Bit map for clock configuration (SDHCI 3.0 10-bit divided-clock mode) */
#define XLNX_ZYNQ_SDHC_CC_DIV_SHIFT           0x8U
#define XLNX_ZYNQ_SDHC_CC_EXT_MAX_DIV_CNT     0x3FFU
#define XLNX_ZYNQ_SDHC_CC_SDCLK_FREQ_SEL      0xFFU
#define XLNX_ZYNQ_SDHC_CC_INT_CLK_EN_MASK     BIT(0)
#define XLNX_ZYNQ_SDHC_CC_INT_CLK_STABLE_MASK BIT(1)
#define XLNX_ZYNQ_SDHC_CC_SD_CLK_EN_MASK      BIT(2)

#define XLNX_ZYNQ_SDHC_DAT_PRESENT_SEL_MASK BIT(5)
#define XLNX_ZYNQ_SDHC_DAT_LINE_TIMEOUT     0xEU
#define XLNX_ZYNQ_SDHC_BLK_SIZE_512         0x200U

/* Bit map for software reset register */
#define XLNX_ZYNQ_SDHC_SWRST_ALL_MASK BIT(0)

/*
 * SDHCI-standard register block (UG585 ch. 13 "SD Controller", Arasan-derived
 * IP). Trimmed at host_cntrl_version: this controller generation has no Command
 * Queue Engine and no PHY/tap-delay block (both Versal/ZynqMP-only -- see
 * xlnx_sdhc.h's struct reg_base for that superset).
 *
 * A plain (non-packed) struct on purpose: every SDHCI register already sits at
 * its naturally-aligned offset, so this matches the hardware layout with no
 * padding AND lets the compiler emit natural-width accesses. A __packed struct
 * would make the compiler assume 1-byte alignment and split each 16/32-bit
 * access into byte accesses -- which the interrupt status/enable registers
 * silently drop, breaking card detection. The BUILD_ASSERTs next to this
 * struct's user (in the .c) pin the offsets so the layout can't drift.
 */
struct zynq_sdhc_reg_base {
	volatile uint32_t sdma_sysaddr;           /**< SDMA System Address */
	volatile uint16_t block_size;             /**< Block Size */
	volatile uint16_t block_count;            /**< Block Count */
	volatile uint32_t argument;               /**< Argument */
	volatile uint16_t transfer_mode;          /**< Transfer Mode */
	volatile uint16_t cmd;                    /**< Command */
	volatile uint32_t resp_0;                 /**< Response Register 0 */
	volatile uint32_t resp_1;                 /**< Response Register 1 */
	volatile uint32_t resp_2;                 /**< Response Register 2 */
	volatile uint32_t resp_3;                 /**< Response Register 3 */
	volatile uint32_t data_port;              /**< Buffer Data Port */
	volatile uint32_t present_state;          /**< Present State */
	volatile uint8_t host_ctrl1;              /**< Host Control 1 */
	volatile uint8_t power_ctrl;              /**< Power Control */
	volatile uint8_t block_gap_ctrl;          /**< Block Gap Control */
	volatile uint8_t wake_up_ctrl;            /**< Wakeup Control */
	volatile uint16_t clock_ctrl;             /**< Clock Control */
	volatile uint8_t timeout_ctrl;            /**< Timeout Control */
	volatile uint8_t sw_reset;                /**< Software Reset */
	volatile uint16_t normal_int_stat;        /**< Normal Interrupt Status */
	volatile uint16_t err_int_stat;           /**< Error Interrupt Status */
	volatile uint16_t normal_int_stat_en;     /**< Normal Interrupt Status Enable */
	volatile uint16_t err_int_stat_en;        /**< Error Interrupt Status Enable */
	volatile uint16_t normal_int_signal_en;   /**< Normal Interrupt Signal Enable */
	volatile uint16_t err_int_signal_en;      /**< Error Interrupt Signal Enable */
	volatile uint16_t auto_cmd_err_stat;      /**< Auto CMD Error Status */
	volatile uint16_t host_ctrl2;             /**< Host Control 2 */
	volatile uint64_t capabilities;           /**< Capabilities */
	volatile uint64_t max_current_cap;        /**< Max Current Capabilities */
	volatile uint16_t force_err_autocmd_stat; /**< Force Event for Auto CMD Err Status */
	volatile uint16_t force_err_int_stat;     /**< Force Event for Error Int Status */
	volatile uint8_t adma_err_stat;           /**< ADMA Error Status */
	volatile uint8_t reserved0[3];
	volatile uint64_t adma_sys_addr; /**< ADMA System Address (unused -- PIO only) */
	volatile uint16_t preset_val_0;  /**< Preset Value 0 */
	volatile uint16_t preset_val_1;  /**< Preset Value 1 */
	volatile uint16_t preset_val_2;  /**< Preset Value 2 */
	volatile uint16_t preset_val_3;  /**< Preset Value 3 */
	volatile uint16_t preset_val_4;  /**< Preset Value 4 */
	volatile uint16_t preset_val_5;  /**< Preset Value 5 */
	volatile uint16_t preset_val_6;  /**< Preset Value 6 */
	volatile uint16_t preset_val_7;  /**< Preset Value 7 */
	volatile uint32_t boot_timeout;  /**< Boot Timeout */
	volatile uint16_t reserved1[58];
	volatile uint32_t reserved2[5];
	volatile uint16_t slot_intr_stat;     /**< Slot Interrupt Status */
	volatile uint16_t host_cntrl_version; /**< Host Controller Version */
};

#endif /* ZEPHYR_DRIVERS_SDHC_XLNX_ZYNQ_SDHC_H_ */
