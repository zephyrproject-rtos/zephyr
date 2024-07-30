/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_DISK_EMMC_HOST_H_
#define ZEPHYR_DRIVERS_DISK_EMMC_HOST_H_

/* Bit map for command Register */
#define EMMC_HOST_CMD_RESP_TYPE_LOC    0
#define EMMC_HOST_CMD_CRC_CHECK_EN_LOC 3
#define EMMC_HOST_CMD_IDX_CHECK_EN_LOC 4
#define EMMC_HOST_CMD_DATA_PRESENT_LOC 5
#define EMMC_HOST_CMD_TYPE_LOC         6
#define EMMC_HOST_CMD_INDEX_LOC        8

/* Bit map for Transfer Mode Register */
#define EMMC_HOST_XFER_DMA_EN_LOC          0
#define EMMC_HOST_XFER_BLOCK_CNT_EN_LOC    1
#define EMMC_HOST_XFER_AUTO_CMD_EN_LOC     2
#define EMMC_HOST_XFER_DATA_DIR_LOC        4
#define EMMC_HOST_XFER_MULTI_BLOCK_SEL_LOC 5

#define EMMC_HOST_XFER_DMA_EN_MASK          0x01
#define EMMC_HOST_XFER_BLOCK_CNT_EN_MASK    0x01
#define EMMC_HOST_XFER_AUTO_CMD_EN_MASK     0x03
#define EMMC_HOST_XFER_DATA_DIR_MASK        0x01
#define EMMC_HOST_XFER_MULTI_BLOCK_SEL_MASK 0x01

/* Bit map for Block Size and GAP Register */
#define EMMC_HOST_BLOCK_SIZE_LOC    0
#define EMMC_HOST_BLOCK_SIZE_MASK   0xFFF
#define EMMC_HOST_DMA_BUF_SIZE_LOC  12
#define EMMC_HOST_DMA_BUF_SIZE_MASK 0x07
#define EMMC_HOST_BLOCK_GAP_LOC     3
#define EMMC_HOST_BLOCK_GAP_MASK    0x01

#define EMMC_HOST_ADMA_BUFF_ADD_LOC   32
#define EMMC_HOST_ADMA_BUFF_LEN_LOC   16
#define EMMC_HOST_ADMA_BUFF_LINK_NEXT (0x3 << 4)
#define EMMC_HOST_ADMA_BUFF_LINK_LAST (0x2 << 4)
#define EMMC_HOST_ADMA_INTR_EN        BIT(2)
#define EMMC_HOST_ADMA_BUFF_LAST      BIT(1)
#define EMMC_HOST_ADMA_BUFF_VALID     BIT(0)

/* Bit Map and length details for Clock Control Register */
#define EMMC_HOST_CLK_SDCLCK_FREQ_SEL_LOC       8
#define EMMC_HOST_CLK_SDCLCK_FREQ_SEL_UPPER_LOC 6

#define EMMC_HOST_CLK_SDCLCK_FREQ_SEL_MASK       0xFF
#define EMMC_HOST_CLK_SDCLCK_FREQ_SEL_UPPER_MASK 0x03

/* Bit Map for Host Control 1 Register */
#define EMMC_HOST_CTRL1_DAT_WIDTH_LOC     1
#define EMMC_HOST_CTRL1_DMA_SEL_LOC       3
#define EMMC_HOST_CTRL1_EXT_DAT_WIDTH_LOC 5

#define EMMC_HOST_CTRL1_DMA_SEL_MASK       0x03
#define EMMC_HOST_CTRL1_EXT_DAT_WIDTH_MASK 0x01
#define EMMC_HOST_CTRL1_DAT_WIDTH_MASK     0x01

/** Constants Software Reset register */
#define EMMC_HOST_SW_RESET_REG_ALL  BIT(0)
#define EMMC_HOST_SW_RESET_REG_CMD  BIT(1)
#define EMMC_HOST_SW_RESET_REG_DATA BIT(2)

#define EMMC_HOST_RESPONSE_SIZE      4
#define EMMC_HOST_OCR_BUSY_BIT       BIT(31)
#define EMMC_HOST_OCR_CAPACITY_MASK  0x40000000U
#define EMMC_HOST_DUAL_VOLTAGE_RANGE 0x40FF8080U
#define EMMC_HOST_BLOCK_SIZE         512

#define EMMC_HOST_RCA_SHIFT                16
#define EMMC_HOST_EXTCSD_SEC_COUNT         53
#define EMMC_HOST_EXTCSD_GENERIC_CMD6_TIME 62
#define EMMC_HOST_EXTCSD_BUS_WIDTH_ADDR    0xB7
#define EMMC_HOST_EXTCSD_HS_TIMING_ADDR    0xB9
#define EMMC_HOST_BUS_SPEED_HIGHSPEED      1

#define EMMC_HOST_CMD_COMPLETE_RETRY 10000
#define EMMC_HOST_XFR_COMPLETE_RETRY 2000000

#define EMMC_HOST_CMD1_RETRY_TIMEOUT 1000
#define EMMC_HOST_CMD6_TIMEOUT_MULT  10

#define EMMC_HOST_NORMAL_INTR_MASK     0x3f
#define EMMC_HOST_ERROR_INTR_MASK      0x13ff
#define EMMC_HOST_NORMAL_INTR_MASK_CLR 0x60ff

#define EMMC_HOST_POWER_CTRL_SD_BUS_POWER    0x1
#define EMMC_HOST_POWER_CTRL_SD_BUS_VOLT_SEL 0x5

#define EMMC_HOST_UHSMODE_SDR12  0x0
#define EMMC_HOST_UHSMODE_SDR25  0x1
#define EMMC_HOST_UHSMODE_SDR50  0x2
#define EMMC_HOST_UHSMODE_SDR104 0x3
#define EMMC_HOST_UHSMODE_DDR50  0x4
#define EMMC_HOST_UHSMODE_HS400  0x5

#define EMMC_HOST_CTRL2_1P8V_SIG_EN       1
#define EMMC_HOST_CTRL2_1P8V_SIG_LOC      3
#define EMMC_HOST_CTRL2_UHS_MODE_SEL_LOC  0
#define EMMC_HOST_CTRL2_UHS_MODE_SEL_MASK 0x07

/* Event/command status */
#define EMMC_HOST_CMD_COMPLETE   BIT(0)
#define EMMC_HOST_XFER_COMPLETE  BIT(1)
#define EMMC_HOST_BLOCK_GAP_INTR BIT(2)
#define EMMC_HOST_DMA_INTR       BIT(3)
#define EMMC_HOST_BUF_WR_READY   BIT(4)
#define EMMC_HOST_BUF_RD_READY   BIT(5)

#define EMMC_HOST_CMD_TIMEOUT_ERR  BIT(0)
#define EMMC_HOST_CMD_CRC_ERR      BIT(1)
#define EMMC_HOST_CMD_END_BIT_ERR  BIT(2)
#define EMMC_HOST_CMD_IDX_ERR      BIT(3)
#define EMMC_HOST_DATA_TIMEOUT_ERR BIT(4)
#define EMMC_HOST_DATA_CRC_ERR     BIT(5)
#define EMMC_HOST_DATA_END_BIT_ERR BIT(6)
#define EMMC_HOST_CUR_LMT_ERR      BIT(7)
#define EMMC_HOST_DMA_TXFR_ERR     BIT(12)
#define EMMC_HOST_ERR_STATUS       0xFFF

/** PState register bits */
#define EMMC_HOST_PSTATE_CMD_INHIBIT     BIT(0)
#define EMMC_HOST_PSTATE_DAT_INHIBIT     BIT(1)
#define EMMC_HOST_PSTATE_DAT_LINE_ACTIVE BIT(2)

#define EMMC_HOST_PSTATE_WR_DMA_ACTIVE BIT(8)
#define EMMC_HOST_PSTATE_RD_DMA_ACTIVE BIT(9)

#define EMMC_HOST_PSTATE_BUF_READ_EN  BIT(11)
#define EMMC_HOST_PSTATE_BUF_WRITE_EN BIT(10)

#define EMMC_HOST_PSTATE_CARD_INSERTED BIT(16)

#define EMMC_HOST_MAX_TIMEOUT 0xe
#define EMMC_HOST_MSEC_DELAY  1000

/** Constants for Clock Control register */
#define EMMC_HOST_INTERNAL_CLOCK_EN     BIT(0)
#define EMMC_HOST_INTERNAL_CLOCK_STABLE BIT(1)
#define EMMC_HOST_SD_CLOCK_EN           BIT(2)

/** Clock frequency */
#define EMMC_HOST_CLK_FREQ_400K 0.4
#define EMMC_HOST_CLK_FREQ_25M  25
#define EMMC_HOST_CLK_FREQ_50M  50
#define EMMC_HOST_CLK_FREQ_100M 100
#define EMMC_HOST_CLK_FREQ_200M 200

#define EMMC_HOST_TUNING_SUCCESS BIT(7)
#define EMMC_HOST_START_TUNING   BIT(6)

#define EMMC_HOST_VOL_3_3_V_SUPPORT BIT(24)
#define EMMC_HOST_VOL_3_3_V_SELECT  (7 << 1)
#define EMMC_HOST_VOL_3_0_V_SUPPORT BIT(25)
#define EMMC_HOST_VOL_3_0_V_SELECT  (6 << 1)
#define EMMC_HOST_VOL_1_8_V_SUPPORT BIT(26)
#define EMMC_HOST_VOL_1_8_V_SELECT  (5 << 1)

#define EMMC_HOST_CMD_WAIT_TIMEOUT_US    3000
#define EMMC_HOST_CMD_CMPLETE_TIMEOUT_US 9000
#define EMMC_HOST_XFR_CMPLETE_TIMEOUT_US 1000
#define EMMC_HOST_SDMA_BOUNDARY          0x0
#define EMMC_HOST_RCA_ADDRESS            0x2

#define EMMC_HOST_RESP_MASK (0xFF000000U)

#define EMMC_HOST_SET_RESP(resp0, resp1) (resp0 >> 1) | ((resp1 & 1) << 30)

#define SET_BITS(reg, pos, bit_width, val)                                                         \
	reg &= ~(bit_width << pos);                                                                \
	reg |= ((val & bit_width) << pos)

/* get value from certain bit
 */
#define GET_BITS(reg_name, start, width) ((reg_name) & (((1 << (width)) - 1) << (start)))

#define ERR_INTR_STATUS_EVENT(reg_bits) reg_bits << 16

#define ADDRESS_32BIT_MASK 0xFFFFFFFF

struct emmc_reg {
	volatile uint32_t sdma_sysaddr;  /**< SDMA System Address */
	volatile uint16_t block_size;    /**< Block Size */
	volatile uint16_t block_count;   /**< Block Count */
	volatile uint32_t argument;      /**< Argument */
	volatile uint16_t transfer_mode; /**< Transfer Mode */
	volatile uint16_t cmd;           /**< Command */

	volatile uint32_t resp_01;              /**< Response Register 0 & 1 */
	volatile uint16_t resp_2;               /**< Response Register 2*/
	volatile uint16_t resp_3;               /**< Response Register 3 */
	volatile uint16_t resp_4;               /**< Response Register 4 */
	volatile uint16_t resp_5;               /**< Response Register 5 */
	volatile uint16_t resp_6;               /**< Response Register 6 */
	volatile uint16_t resp_7;               /**< Response Register 7 */
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
	volatile uint8_t reserved[3];
	volatile uint32_t adma_sys_addr1; /**< ADMA System Address1 */
	volatile uint32_t adma_sys_addr2; /**< ADMA System Address2 */
	volatile uint16_t preset_val_0;   /**< Preset Value 0 */
	volatile uint16_t preset_val_1;   /**< Preset Value 1 */
	volatile uint16_t preset_val_2;   /**< Preset Value 2 */
	volatile uint16_t preset_val_3;   /**< Preset Value 3 */
	volatile uint16_t preset_val_4;   /**< Preset Value 4 */
	volatile uint16_t preset_val_5;   /**< Preset Value 5 */
	volatile uint16_t preset_val_6;   /**< Preset Value 6 */
	volatile uint16_t preset_val_7;   /**< Preset Value 7 */
	volatile uint32_t boot_timeout;   /**< Boot Timeout */
	volatile uint16_t preset_val_8;   /**< Preset Value 8 */
	volatile uint16_t reserved3;
	volatile uint16_t vendor_reg; /**< Vendor Enhanced strobe */
	volatile uint16_t reserved4[56];
	volatile uint32_t reserved5[4];
	volatile uint16_t slot_intr_stat;     /**< Slot Interrupt Status */
	volatile uint16_t host_cntrl_version; /**< Host Controller Version */
	volatile uint32_t reserved6[64];
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
	volatile uint32_t reserved7;
	volatile uint32_t cq_ssc1;  /**< Command Queue Send Status Configuration 1 */
	volatile uint32_t cq_ssc2;  /**< Command Queue Send Status Configuration 2 */
	volatile uint32_t cq_crdct; /**< Command response for direct command */
	volatile uint32_t reserved8;
	volatile uint32_t cq_rmem;  /**< Command response mode error mask */
	volatile uint32_t cq_terri; /**< Command Queue Task Error Information */
	volatile uint32_t cq_cri;   /**< Command Queue Command response index */
	volatile uint32_t cq_cra;   /**< Command Queue Command response argument */
	volatile uint32_t reserved9[425];
};

enum emmc_sw_reset {
	EMMC_HOST_SW_RESET_DATA_LINE = 0,
	EMMC_HOST_SW_RESET_CMD_LINE,
	EMMC_HOST_SW_RESET_ALL
};

enum emmc_cmd_type {
	EMMC_HOST_CMD_NORMAL = 0,
	EMMC_HOST_CMD_SUSPEND,
	EMMC_HOST_CMD_RESUME,
	EMMC_HOST_CMD_ABORT,
};

enum emmc_response_type {
	EMMC_HOST_RESP_NONE = 0,
	EMMC_HOST_RESP_LEN_136,
	EMMC_HOST_RESP_LEN_48,
	EMMC_HOST_RESP_LEN_48B,
	EMMC_HOST_INVAL_HOST_RESP_LEN,
};

struct emmc_cmd_config {
	struct sdhc_command *sdhc_cmd;
	uint32_t cmd_idx;
	enum emmc_cmd_type cmd_type;
	bool data_present;
	bool idx_check_en;
	bool crc_check_en;
};

struct resp {
	uint64_t resp_48bit;
};
#endif /* _EMMC_HOST_HC_H_ */
