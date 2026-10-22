/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc. (AMD)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __XLNX_SDHC_H__
#define __XLNX_SDHC_H__

/* Bit map for status register */
#define XLNX_SDHC_INTR_CC_MASK	BIT(0)
#define XLNX_SDHC_INTR_TC_MASK	BIT(1)
#define XLNX_SDHC_INTR_ERR_MASK	BIT(15)
#define XLNX_SDHC_NORM_INTR_ALL	0xFFFFU

/* Bit map for error register */
#define XLNX_SDHC_ERROR_INTR_ALL	0xF3FFU

/* Bit map for ADMA2 attribute */
#define XLNX_SDHC_DESC_VALID	BIT(0)
#define XLNX_SDHC_DESC_END	BIT(1)
#define XLNX_SDHC_DESC_TRAN	BIT(5)

/* Bit map and constant values for ADMA2 Configuration */
#define XLNX_SDHC_ADMA2_64		0x18U
#define XLNX_SDHC_DESC_MAX_LENGTH	65536U

/* Bit map for present stage register */
#define XLNX_SDHC_PSR_INHIBIT_DAT_MASK	BIT(1)
#define XLNX_SDHC_INTR_BRR_MASK		BIT(5)
#define XLNX_SDHC_PSR_CARD_INSRT_MASK	BIT(16)
#define XLNX_SDHC_CARD_BUSY		0x1F00000U

/* Bit map for transfer mode register */
#define XLNX_SDHC_TM_DMA_EN_MASK		BIT(0)
#define XLNX_SDHC_TM_BLK_CNT_EN_MASK		BIT(1)
#define XLNX_SDHC_TM_AUTO_CMD12_EN_MASK		BIT(2)
#define XLNX_SDHC_TM_DAT_DIR_SEL_MASK		BIT(4)
#define XLNX_SDHC_TM_MUL_SIN_BLK_SEL_MASK	BIT(5)

/* Bit map for host control1 register */
#define XLNX_SDHC_DAT_WIDTH4_MASK	BIT(1)
#define XLNX_SDHC_HS_SPEED_MODE_EN_MASK	BIT(2)
#define XLNX_SDHC_DAT_WIDTH8_MASK	BIT(5)

/* Bit map for power control register */
#define XLNX_SDHC_PC_BUS_PWR_MASK	BIT(0)
#define XLNX_SDHC_PC_EMMC_HW_RST_MASK	BIT(4)
#define XLNX_SDHC_PC_BUS_VSEL_3V3	0x0EU
#define XLNX_SDHC_PC_BUS_VSEL_3V0	0x0CU

/* Bit map for host control2 register */
#define XLNX_SDHC_HC2_1V8_EN_MASK	BIT(3)
#define XLNX_SDHC_HC2_EXEC_TNG_MASK	BIT(6)
#define XLNX_SDHC_HC2_SAMP_CLK_SEL_MASK	BIT(7)
#define XLNX_SDHC_UHS_SPEED_MODE_SDR12	0U
#define XLNX_SDHC_UHS_SPEED_MODE_SDR50	2U
#define XLNX_SDHC_UHS_SPEED_MODE_SDR104	3U
#define XLNX_SDHC_UHS_SPEED_MODE_DDR50	4U
#define XLNX_SDHC_UHS_SPEED_MODE_DDR200	5U
#define XLNX_SDHC_HC2_UHS_MODE		7U

/* Bit map to read host capabilities register */
#define XLNX_SDHC_1P8_VOL_SUPPORT		26U
#define XLNX_SDHC_3P0_VOL_SUPPORT		25U
#define XLNX_SDHC_3P3_VOL_SUPPORT		24U
#define XLNX_SDHC_3P0_CURRENT_SUPPORT_SHIFT	8U
#define XLNX_SDHC_1P8_CURRENT_SUPPORT_SHIFT	16U
#define XLNX_SDHC_CURRENT_BYTE			0xFFU
#define XLNX_SDHC_SDMA_SUPPORT			22U
#define XLNX_SDHC_HIGH_SPEED_SUPPORT		21U
#define XLNX_SDHC_ADMA2_SUPPORT			19U
#define XLNX_SDHC_MAX_BLK_LEN_SHIFT		16U
#define XLNX_SDHC_MAX_BLK_LEN			3U
#define XLNX_SDHC_DDR50_SUPPORT			34U
#define XLNX_SDHC_SDR104_SUPPORT		33U
#define XLNX_SDHC_SDR50_SUPPORT			32U
#define XLNX_SDHC_SLOT_TYPE_SHIFT		30U
#define XLNX_SDHC_SLOT_TYPE_GET			3U
#define XLNX_SDHC_8BIT_SUPPORT			18U
#define XLNX_SDHC_4BIT_SUPPORT			18U
#define XLNX_SDHC_SDR400_SUPPORT		63U

/* Bit map for tap delay register */
#define XLNX_SDHC_ITAPCHGWIN	BIT(9)
#define XLNX_SDHC_ITAPDLYENA	BIT(8)

/* Bit map for phy1 register */
#define XLNX_SDHC_PHYREG1_ITAP_DLY_SHIFT	0x1U
#define XLNX_SDHC_PHYREG1_ITAP_EN_MASK		BIT(0)
#define XLNX_SDHC_PHYREG1_STROBE_SEL_SHIFT	16U
#define XLNX_SDHC_PHYREG1_ITAP_CHGWIN_MASK	BIT(6)
#define XLNX_SDHC_PHYREG1_OTAP_EN_MASK		BIT(8)
#define XLNX_SDHC_PHYREG1_OTAP_DLY_SHIFT	0xCU
#define XLNX_SDHC_PHYREG1_ITAP_DLY		0x3EU
#define XLNX_SDHC_PHY_STRB_SEL_SIG		0x0077U
#define XLNX_SDHC_PHYREG1_OTAP_DLY		0xF000U
#define XLNX_SDHC_PHYREG1_STROBE_SEL		0xFF0000U

/* Bit map for phy2 register */
#define XLNX_SDHC_PHYREG2_DLL_EN_MASK		BIT(0)
#define XLNX_SDHC_PHYREG2_DLL_RDY_MASK		BIT(1)
#define XLNX_SDHC_PHYREG2_FREQ_SEL_SHIFT	BIT(2)
#define XLNX_SDHC_PHYREG2_TRIM_ICP_SHIFT	BIT(3)
#define XLNX_SDHC_PHYREG2_DLYTX_SEL_MASK	BIT(16)
#define XLNX_SDHC_PHYREG2_DLYRX_SEL_MASK	BIT(17)
#define XLNX_SDHC_PHYREG2_TRIM_ICP_DEF_VAL	0x8U
#define XLNX_SDHC_PHYREG2_FREQ_SEL		0x70U
#define XLNX_SDHC_PHYREG2_TRIM_ICP		0xF00U

/* Bit map for software register */
#define XLNX_SDHC_SWRST_ALL_MASK	BIT(0)

/* Bit map for response types */
#define RESP_NONE	XLNX_SDHC_CMD_RESP_NONE
#define RESP_R1B	XLNX_SDHC_CMD_RESP_L48_BSY_CHK | \
	XLNX_SDHC_CMD_CRC_CHK_EN_MASK | \
	XLNX_SDHC_CMD_INX_CHK_EN_MASK
#define RESP_R1	XLNX_SDHC_CMD_RESP_L48_MASK | \
	XLNX_SDHC_CMD_CRC_CHK_EN_MASK | \
	XLNX_SDHC_CMD_INX_CHK_EN_MASK
#define RESP_R2	XLNX_SDHC_CMD_RESP_L136_MASK | \
	XLNX_SDHC_CMD_CRC_CHK_EN_MASK
#define RESP_R3	XLNX_SDHC_CMD_RESP_L48_MASK
#define RESP_R6	XLNX_SDHC_CMD_RESP_L48_BSY_CHK | \
	XLNX_SDHC_CMD_CRC_CHK_EN_MASK | \
	XLNX_SDHC_CMD_INX_CHK_EN_MASK
#define XLNX_SDHC_CMD_RESP_NONE			0x0U
#define XLNX_SDHC_CMD_RESP_L136_MASK		BIT(0)
#define XLNX_SDHC_CMD_RESP_L48_MASK		BIT(1)
#define XLNX_SDHC_CMD_RESP_L48_BSY_CHK		0x3U
#define XLNX_SDHC_CMD_CRC_CHK_EN_MASK		BIT(3)
#define XLNX_SDHC_CMD_INX_CHK_EN_MASK		BIT(4)
#define XLNX_SDHC_CMD_RESP_INVAL		0xFFU
#define XLNX_SDHC_OPCODE_SHIFT			0x8U
#define XLNX_SDHC_RESP				0xFU

/* Bit map to update response type */
#define XLNX_SDHC_CRC_LEFT_SHIFT	0x8U
#define XLNX_SDHC_CRC_RIGHT_SHIFT	0x18U

/* Bit map for clock configuration */
#define XLNX_SDHC_CC_DIV_SHIFT			0x8U
#define XLNX_SDHC_CC_EXT_MAX_DIV_CNT		0x7FEU
#define XLNX_SDHC_CC_SDCLK_FREQ_SEL		0xFFU
#define XLNX_SDHC_CC_SDCLK_FREQ_SEL_EXT		0x3U
#define XLNX_SDHC_CC_EXT_DIV_SHIFT		0x6U
#define XLNX_SDHC_CLOCK_CNT_SHIFT		0x1U

/* Bit map for enable clock */
#define XLNX_SDHC_CC_INT_CLK_EN_MASK		BIT(0)
#define XLNX_SDHC_CC_INT_CLK_STABLE_MASK	BIT(1)
#define XLNX_SDHC_CC_SD_CLK_EN_MASK		BIT(2)

/* Tuning command parameters */
#define XLNX_SDHC_TUNING_CMD_BLKCOUNT	0x1U
#define XLNX_SDHC_MAX_TUNING_COUNT	0X28U
#define XLNX_SDHC_TUNING_CMD_BLKSIZE	0x40U
#define XLNX_SDHC_BLK_SIZE_512		0x200U

/* Constant tap delay values and mask */
#define XLNX_SDHC_SD_OTAP_DEFAULT_PHASES	{60, 0, 48, 0, 48, 72, 90, 36, 60, 90, 0}
#define XLNX_SDHC_SD_ITAP_DEFAULT_PHASES	{132, 0, 132, 0, 132, 0, 0, 162, 90, 0, 0}
#define XLNX_SDHC_EMMC_OTAP_DEFAULT_PHASES	{113, 0, 0, 0, 0, 0, 0, 0, 113, 79, 45}
#define XLNX_SDHC_EMMC_ITAP_DEFAULT_PHASES	{0, 0, 0, 0, 0, 0, 0, 0, 39, 0, 0}
#define XLNX_SDHC_TIMING_MMC_HS			0U
#define XLNX_SDHC_CLK_PHASES			2U
#define XLNX_SDHC_ITAP				0
#define XLNX_SDHC_OTAP				1
#define XLNX_SDHC_MAX_CLK_PHASE			360U
#define XLNX_SDHC_SD_200HZ_MAX_OTAP		8U
#define XLNX_SDHC_SD_50HZ_MAX_OTAP		30U
#define XLNX_SDHC_SD_100HZ_MAX_OTAP		15U
#define XLNX_SDHC_SD_200HZ_MAX_ITAP		30U
#define XLNX_SDHC_SD_50HZ_MAX_ITAP		120U
#define XLNX_SDHC_SD_100HZ_MAX_ITAP		60U
#define XLNX_SDHC_EMMC_200HZ_MAX_OTAP		32U
#define XLNX_SDHC_EMMC_50HZ_MAX_OTAP		16U
#define XLNX_SDHC_EMMC_50HZ_MAX_ITAP		32U

/* Constant dll clock frequency select */
#define XLNX_SDHC_FREQSEL_200M_170M	0x0U
#define XLNX_SDHC_FREQSEL_170M_140M	0x1U
#define XLNX_SDHC_FREQSEL_140M_110M	0x2U
#define XLNX_SDHC_FREQSEL_110M_80M	0x3U
#define XLNX_SDHC_FREQSEL_80M_50M	0x4U
#define XLNX_SDHC_200_FREQ		200U
#define XLNX_SDHC_170_FREQ		170U
#define XLNX_SDHC_140_FREQ		140U
#define XLNX_SDHC_110_FREQ		110U
#define XLNX_SDHC_80_FREQ		80U

#define XLNX_SDHC_KHZ_TO_MHZ		1000000U

#define XLNX_SDHC_DAT_PRESENT_SEL_MASK	BIT(5)

#define XLNX_SDHC_TXFR_INTR_EN_MASK	0x8023U
#define XLNX_SDHC_DAT_LINE_TIMEOUT	0xEU

#define XLNX_SDHC_SD_SLOT		0x0U
#define XLNX_SDHC_EMMC_SLOT		0X1U

struct reg_base {
	volatile uint32_t sdma_sysaddr;  /**< SDMA System Address */
	volatile uint16_t block_size;    /**< Block Size */
	volatile uint16_t block_count;   /**< Block Count */
	volatile uint32_t argument;      /**< Argument */
	volatile uint16_t transfer_mode; /**< Transfer Mode */
	volatile uint16_t cmd;           /**< Command */

	volatile uint32_t resp_0;               /**< Response Register 0 */
	volatile uint32_t resp_1;               /**< Response Register 1 */
	volatile uint32_t resp_2;               /**< Response Register 2 */
	volatile uint32_t resp_3;               /**< Response Register 3 */
	volatile uint32_t data_port;            /**< Buffer Data Port */
	volatile uint32_t present_state;        /**< Present State */
	volatile uint8_t host_ctrl1;            /**< Host Control 1 */
	volatile uint8_t power_ctrl;            /**< Power Control */
	volatile uint8_t block_gap_ctrl;        /**< Block Gap Control */
	volatile uint8_t wake_up_ctrl;          /**< Wakeup Control */
	volatile uint16_t clock_ctrl;           /**< Clock Control */
	volatile uint8_t timeout_ctrl;          /**< Timeout Control */
	volatile uint8_t sw_reset;              /**< Software Reset */
	volatile uint16_t normal_int_stat;      /**< Normal Interrupt Status */
	volatile uint16_t err_int_stat;         /**< Error Interrupt Status */
	volatile uint16_t normal_int_stat_en;   /**< Normal Interrupt Status Enable */
	volatile uint16_t err_int_stat_en;      /**< Error Interrupt Status Enable */
	volatile uint16_t normal_int_signal_en; /**< Normal Interrupt Signal Enable */
	volatile uint16_t err_int_signal_en;    /**< Error Interrupt Signal Enable */
	volatile uint16_t auto_cmd_err_stat;    /**< Auto CMD Error Status */
	volatile uint16_t host_ctrl2;           /**< Host Control 2 */
	volatile uint64_t capabilities;         /**< Capabilities */

	volatile uint64_t max_current_cap;        /**< Max Current Capabilities */
	volatile uint16_t force_err_autocmd_stat; /**< Force Event for Auto CMD Error Status*/
	volatile uint16_t force_err_int_stat;     /**< Force Event for Error Interrupt Status */
	volatile uint8_t adma_err_stat;           /**< ADMA Error Status */
	volatile uint8_t reserved0[3];
	volatile uint64_t adma_sys_addr; /**< ADMA System Address */
	volatile uint16_t preset_val_0;   /**< Preset Value 0 */
	volatile uint16_t preset_val_1;   /**< Preset Value 1 */
	volatile uint16_t preset_val_2;   /**< Preset Value 2 */
	volatile uint16_t preset_val_3;   /**< Preset Value 3 */
	volatile uint16_t preset_val_4;   /**< Preset Value 4 */
	volatile uint16_t preset_val_5;   /**< Preset Value 5 */
	volatile uint16_t preset_val_6;   /**< Preset Value 6 */
	volatile uint16_t preset_val_7;   /**< Preset Value 7 */
	volatile uint32_t boot_timeout;   /**< Boot Timeout */
	volatile uint16_t reserved1[58];
	volatile uint32_t reserved2[5];
	volatile uint16_t slot_intr_stat;     /**< Slot Interrupt Status */
	volatile uint16_t host_cntrl_version; /**< Host Controller Version */
	volatile uint32_t reserved4[64];
	volatile uint32_t cq_ver;            /**< Command Queue Version */
	volatile uint32_t cq_cap;            /**< Command Queue Capabilities */
	volatile uint32_t cq_cfg;            /**< Command Queue Configuration */
	volatile uint32_t cq_ctrl;           /**< Command Queue Control */
	volatile uint32_t cq_intr_stat;      /**< Command Queue Interrupt Status */
	volatile uint32_t cq_intr_stat_en;   /**< Command Queue Interrupt Status Enable */
	volatile uint32_t cq_intr_sig_en;    /**< Command Queue Interrupt Signal Enable */
	volatile uint32_t cq_intr_coalesc;   /**< Command Queue Interrupt Coalescing */
	volatile uint32_t cq_tdlba;          /**< Command Queue Task Desc List Base Addr */
	volatile uint32_t cq_tdlba_upr;      /**< Command Queue Task Desc List Base Addr Upr */
	volatile uint32_t cq_task_db;        /**< Command Queue Task DoorBell */
	volatile uint32_t cq_task_db_notify; /**< Command Queue Task DoorBell Notify */
	volatile uint32_t cq_dev_qstat;      /**< Command Queue Device queue status */
	volatile uint32_t cq_dev_pend_task;  /**< Command Queue Device pending tasks */
	volatile uint32_t cq_task_clr;       /**< Command Queue Task Clr */
	volatile uint32_t reserved6;
	volatile uint32_t cq_ssc1;  /**< Command Queue Send Status Configuration 1 */
	volatile uint32_t cq_ssc2;  /**< Command Queue Send Status Configuration 2 */
	volatile uint32_t cq_crdct; /**< Command response for direct command */
	volatile uint32_t reserved7;
	volatile uint32_t cq_rmem;  /**< Command response mode error mask */
	volatile uint32_t cq_terri; /**< Command Queue Task Error Information */
	volatile uint32_t cq_cri;   /**< Command Queue Command response index */
	volatile uint32_t cq_cra;   /**< Command Queue Command response argument */
	volatile uint32_t cq_cerri; /**< Command Queue Command Error Information */
	volatile uint32_t reserved8[3];
	volatile uint32_t phy_ctrl1; /**< Configuring phyctrl */
	volatile uint32_t phy_ctrl2; /**< Configuring phyctrl and DLL */
	volatile uint32_t bist_ctrl; /**< BIST test control */
	volatile uint32_t bist_stat; /**< BIST test status */
	volatile uint32_t hs200_tap; /**< HS200 Tap Delay Select */
	volatile uint32_t reserved3[15261];
	volatile uint32_t itap_dly; /**< Input Tap Delay Select */
	volatile uint32_t otap_dly; /**< Output Tap Delay Select */
} __packed;
#endif /* __XLNX_SDHC_H__ */
