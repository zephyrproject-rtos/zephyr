/* disk_access_usdhc.c - NXP USDHC driver*/

/*
 * Copyright (c) 2019 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <disk/disk_access.h>
#include <drivers/gpio.h>
#include <sys/byteorder.h>
#include <soc.h>
#include <drivers/clock_control.h>

#include "disk_access_sdhc.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(usdhc, LOG_LEVEL_INF);

enum usdhc_cmd_type {
	USDHC_CMD_TYPE_NORMAL = 0U,
	/*!< Normal command */
	USDHC_CMD_TYPE_SUSPEND = 1U,
	/*!< Suspend command */
	USDHC_CMD_TYPE_RESUME = 2U,
	/*!< Resume command */
	USDHC_CMD_TYPE_ABORT = 3U,
	/*!< Abort command */
	USDHC_CMD_TYPE_EMPTY = 4U,
	/*!< Empty command */
};

enum usdhc_status_flag {
	USDHC_CMD_INHIBIT_FLAG =
		USDHC_PRES_STATE_CIHB_MASK,
	/*!< Command inhibit */
	USDHC_DATA_INHIBIT_FLAG =
		USDHC_PRES_STATE_CDIHB_MASK,
	/*!< Data inhibit */
	USDHC_DATA_LINE_ACTIVE_FLAG =
		USDHC_PRES_STATE_DLA_MASK,
	/*!< Data line active */
	USDHC_SD_CLK_STATUS_FLAG =
		USDHC_PRES_STATE_SDSTB_MASK,
	/*!< SD bus clock stable */
	USDHC_WRITE_ACTIVE_FLAG =
		USDHC_PRES_STATE_WTA_MASK,
	/*!< Write transfer active */
	USDHC_READ_ACTIVE_FLAG =
		USDHC_PRES_STATE_RTA_MASK,
	/*!< Read transfer active */
	USDHC_BUF_WRITE_ENABLE_FLAG =
		USDHC_PRES_STATE_BWEN_MASK,
	/*!< Buffer write enable */
	USDHC_BUF_READ_ENABLE_FLAG =
		USDHC_PRES_STATE_BREN_MASK,
	/*!< Buffer read enable */
	USDHC_RETUNING_REQ_FLAG =
		USDHC_PRES_STATE_RTR_MASK,
	/*!< re-tuning request flag ,only used for SDR104 mode */
	USDHC_DELAY_SETTING_DONE_FLAG =
		USDHC_PRES_STATE_TSCD_MASK,
	/*!< delay setting finished flag */
	USDHC_CARD_INSERTED_FLAG =
		USDHC_PRES_STATE_CINST_MASK,
	/*!< Card inserted */
	USDHC_CMD_LINE_LEVEL_FLAG =
		USDHC_PRES_STATE_CLSL_MASK,
	/*!< Command line signal level */
	USDHC_DATA0_LINE_LEVEL_FLAG =
		1U << USDHC_PRES_STATE_DLSL_SHIFT,
	/*!< Data0 line signal level */
	USDHC_DATA1_LINE_LEVEL_FLAG =
		1U << (USDHC_PRES_STATE_DLSL_SHIFT + 1U),
	/*!< Data1 line signal level */
	USDHC_DATA2_LINE_LEVEL_FLAG =
		1U << (USDHC_PRES_STATE_DLSL_SHIFT + 2U),
	/*!< Data2 line signal level */
	USDHC_DATA3_LINE_LEVEL_FLAG =
		1U << (USDHC_PRES_STATE_DLSL_SHIFT + 3U),
	/*!< Data3 line signal level */
	USDHC_DATA4_LINE_LEVEL_FLAG =
		1U << (USDHC_PRES_STATE_DLSL_SHIFT + 4U),
	/*!< Data4 line signal level */
	USDHC_DATA5_LINE_LEVEL_FLAG =
		1U << (USDHC_PRES_STATE_DLSL_SHIFT + 5U),
	/*!< Data5 line signal level */
	USDHC_DATA6_LINE_LEVEL_FLAG =
		1U << (USDHC_PRES_STATE_DLSL_SHIFT + 6U),
	/*!< Data6 line signal level */
	USDHC_DATA7_LINE_LEVEL_FLAG =
		(int)(1U << (USDHC_PRES_STATE_DLSL_SHIFT + 7U)),
	/*!< Data7 line signal level */
};

enum usdhc_transfer_flag {
	USDHC_ENABLE_DMA_FLAG =
		USDHC_MIX_CTRL_DMAEN_MASK,
	/*!< Enable DMA */

	USDHC_CMD_TYPE_SUSPEND_FLAG =
		(USDHC_CMD_XFR_TYP_CMDTYP(1U)),
	/*!< Suspend command */
	USDHC_CMD_TYPE_RESUME_FLAG =
		(USDHC_CMD_XFR_TYP_CMDTYP(2U)),
	/*!< Resume command */
	USDHC_CMD_TYPE_ABORT_FLAG =
		(USDHC_CMD_XFR_TYP_CMDTYP(3U)),
	/*!< Abort command */

	USDHC_BLOCK_COUNT_FLAG =
		USDHC_MIX_CTRL_BCEN_MASK,
	/*!< Enable block count */
	USDHC_AUTO_CMD12_FLAG =
		USDHC_MIX_CTRL_AC12EN_MASK,
	/*!< Enable auto CMD12 */
	USDHC_DATA_READ_FLAG =
		USDHC_MIX_CTRL_DTDSEL_MASK,
	/*!< Enable data read */
	USDHC_MULTIPLE_BLOCK_FLAG =
		USDHC_MIX_CTRL_MSBSEL_MASK,
	/*!< Multiple block data read/write */
	USDHC_AUTO_CMD23FLAG =
		USDHC_MIX_CTRL_AC23EN_MASK,
	/*!< Enable auto CMD23 */
	USDHC_RSP_LEN_136_FLAG =
		USDHC_CMD_XFR_TYP_RSPTYP(1U),
	/*!< 136 bit response length */
	USDHC_RSP_LEN_48_FLAG =
		USDHC_CMD_XFR_TYP_RSPTYP(2U),
	/*!< 48 bit response length */
	USDHC_RSP_LEN_48_BUSY_FLAG =
		USDHC_CMD_XFR_TYP_RSPTYP(3U),
	/*!< 48 bit response length with busy status */

	USDHC_CRC_CHECK_FLAG =
		USDHC_CMD_XFR_TYP_CCCEN_MASK,
	/*!< Enable CRC check */
	USDHC_IDX_CHECK_FLAG =
		USDHC_CMD_XFR_TYP_CICEN_MASK,
	/*!< Enable index check */
	USDHC_DATA_PRESENT_FLAG =
		USDHC_CMD_XFR_TYP_DPSEL_MASK,
	/*!< Data present flag */
};

enum usdhc_int_status_flag {
	USDHC_INT_CMD_DONE_FLAG =
		USDHC_INT_STATUS_CC_MASK,
	/*!< Command complete */
	USDHC_INT_DATA_DONE_FLAG =
		USDHC_INT_STATUS_TC_MASK,
	/*!< Data complete */
	USDHC_INT_BLK_GAP_EVENT_FLAG =
		USDHC_INT_STATUS_BGE_MASK,
	/*!< Block gap event */
	USDHC_INT_DMA_DONE_FLAG =
		USDHC_INT_STATUS_DINT_MASK,
	/*!< DMA interrupt */
	USDHC_INT_BUF_WRITE_READY_FLAG =
		USDHC_INT_STATUS_BWR_MASK,
	/*!< Buffer write ready */
	USDHC_INT_BUF_READ_READY_FLAG =
		USDHC_INT_STATUS_BRR_MASK,
	/*!< Buffer read ready */
	USDHC_INT_CARD_INSERTED_FLAG =
		USDHC_INT_STATUS_CINS_MASK,
	/*!< Card inserted */
	USDHC_INT_CARD_REMOVED_FLAG =
		USDHC_INT_STATUS_CRM_MASK,
	/*!< Card removed */
	USDHC_INT_CARD_INTERRUPT_FLAG =
		USDHC_INT_STATUS_CINT_MASK,
	/*!< Card interrupt */

	USDHC_INT_RE_TUNING_EVENT_FLAG =
		USDHC_INT_STATUS_RTE_MASK,
	/*!< Re-Tuning event,only for SD3.0 SDR104 mode */
	USDHC_INT_TUNING_PASS_FLAG =
		USDHC_INT_STATUS_TP_MASK,
	/*!< SDR104 mode tuning pass flag */
	USDHC_INT_TUNING_ERR_FLAG =
		USDHC_INT_STATUS_TNE_MASK,
	/*!< SDR104 tuning error flag */

	USDHC_INT_CMD_TIMEOUT_FLAG =
		USDHC_INT_STATUS_CTOE_MASK,
	/*!< Command timeout error */
	USDHC_INT_CMD_CRC_ERR_FLAG =
		USDHC_INT_STATUS_CCE_MASK,
	/*!< Command CRC error */
	USDHC_INT_CMD_ENDBIT_ERR_FLAG =
		USDHC_INT_STATUS_CEBE_MASK,
	/*!< Command end bit error */
	USDHC_INT_CMD_IDX_ERR_FLAG =
		USDHC_INT_STATUS_CIE_MASK,
	/*!< Command index error */
	USDHC_INT_DATA_TIMEOUT_FLAG =
		USDHC_INT_STATUS_DTOE_MASK,
	/*!< Data timeout error */
	USDHC_INT_DATA_CRC_ERR_FLAG =
		USDHC_INT_STATUS_DCE_MASK,
	/*!< Data CRC error */
	USDHC_INT_DATA_ENDBIT_ERR_FLAG =
		USDHC_INT_STATUS_DEBE_MASK,
	/*!< Data end bit error */
	USDHC_INT_AUTO_CMD12_ERR_FLAG =
		USDHC_INT_STATUS_AC12E_MASK,
	/*!< Auto CMD12 error */
	USDHC_INT_DMA_ERR_FLAG =
		USDHC_INT_STATUS_DMAE_MASK,
	/*!< DMA error */

	USDHC_INT_CMD_ERR_FLAG =
		(USDHC_INT_CMD_TIMEOUT_FLAG |
		USDHC_INT_CMD_CRC_ERR_FLAG |
		USDHC_INT_CMD_ENDBIT_ERR_FLAG |
		USDHC_INT_CMD_IDX_ERR_FLAG),
	/*!< Command error */
	USDHC_INT_DATA_ERR_FLAG =
		(USDHC_INT_DATA_TIMEOUT_FLAG |
		USDHC_INT_DATA_CRC_ERR_FLAG |
		USDHC_INT_DATA_ENDBIT_ERR_FLAG |
		USDHC_INT_AUTO_CMD12_ERR_FLAG),
	/*!< Data error */
	USDHC_INT_ERR_FLAG =
		(USDHC_INT_CMD_ERR_FLAG |
		USDHC_INT_DATA_ERR_FLAG |
		USDHC_INT_DMA_ERR_FLAG),
	/*!< All error */
	USDHC_INT_DATA_FLAG =
		(USDHC_INT_DATA_DONE_FLAG |
		USDHC_INT_DMA_DONE_FLAG |
		USDHC_INT_BUF_WRITE_READY_FLAG |
		USDHC_INT_BUF_READ_READY_FLAG |
		USDHC_INT_DATA_ERR_FLAG |
		USDHC_INT_DMA_ERR_FLAG),
	/*!< Data interrupts */
	USDHC_INT_CMD_FLAG =
		(USDHC_INT_CMD_DONE_FLAG |
		USDHC_INT_CMD_ERR_FLAG),
	/*!< Command interrupts */
	USDHC_INT_CARD_DETECT_FLAG =
		(USDHC_INT_CARD_INSERTED_FLAG |
		USDHC_INT_CARD_REMOVED_FLAG),
	/*!< Card detection interrupts */
	USDHC_INT_SDR104_TUNING_FLAG =
		(USDHC_INT_RE_TUNING_EVENT_FLAG |
		USDHC_INT_TUNING_PASS_FLAG |
		USDHC_INT_TUNING_ERR_FLAG),

	USDHC_INT_ALL_FLAGS =
		(USDHC_INT_BLK_GAP_EVENT_FLAG |
		USDHC_INT_CARD_INTERRUPT_FLAG |
		USDHC_INT_CMD_FLAG |
		USDHC_INT_DATA_FLAG |
		USDHC_INT_ERR_FLAG |
		USDHC_INT_SDR104_TUNING_FLAG),
	/*!< All flags mask */
};

enum usdhc_data_bus_width {
	USDHC_DATA_BUS_WIDTH_1BIT = 0U,
	/*!< 1-bit mode */
	USDHC_DATA_BUS_WIDTH_4BIT = 1U,
	/*!< 4-bit mode */
	USDHC_DATA_BUS_WIDTH_8BIT = 2U,
	/*!< 8-bit mode */
};

#define USDHC_MAX_BLOCK_COUNT	\
	(USDHC_BLK_ATT_BLKCNT_MASK >>	\
	USDHC_BLK_ATT_BLKCNT_SHIFT)

struct usdhc_cmd {
	u32_t index;	/*cmd idx*/
	u32_t argument;	/*cmd arg*/
	enum usdhc_cmd_type cmd_type;
	enum sdhc_rsp_type rsp_type;
	u32_t response[4U];
	u32_t rsp_err_flags;
	u32_t flags;
};

struct usdhc_data {
	bool cmd12;
	/* Enable auto CMD12 */
	bool cmd23;
	/* Enable auto CMD23 */
	bool ignore_err;
	/* Enable to ignore error event
	 * to read/write all the data
	 */
	bool data_enable;
	u8_t data_type;
	/* this is used to distinguish
	 * the normal/tuning/boot data
	 */
	u32_t block_size;
	/* Block size
	 */
	u32_t block_count;
	/* Block count
	 */
	u32_t *rx_data;
	/* Buffer to save data read
	 */
	const u32_t *tx_data;
	/* Data buffer to write
	 */
};

enum usdhc_dma_mode {
	USDHC_DMA_SIMPLE = 0U,
	/* external DMA
	 */
	USDHC_DMA_ADMA1 = 1U,
	/* ADMA1 is selected
	 */
	USDHC_DMA_ADMA2 = 2U,
	/* ADMA2 is selected
	 */
	USDHC_EXT_DMA = 3U,
	/* external dma mode select
	 */
};

enum usdhc_burst_len {
	USDHC_INCR_BURST_LEN = 0x01U,
	/* enable burst len for INCR
	 */
	USDHC_INCR4816_BURST_LEN = 0x02U,
	/* enable burst len for INCR4/INCR8/INCR16
	 */
	USDHC_INCR4816_BURST_LEN_WRAP = 0x04U,
	/* enable burst len for INCR4/8/16 WRAP
	 */
};

struct usdhc_adma_config {
	enum usdhc_dma_mode dma_mode;
	/* DMA mode
	 */
	enum usdhc_burst_len burst_len;
	/* burst len config
	 */
	u32_t *adma_table;
	/* ADMA table address,
	 * can't be null if transfer way is ADMA1/ADMA2
	 */
	u32_t adma_table_words;
	/* ADMA table length united as words,
	 * can't be 0 if transfer way is ADMA1/ADMA2
	 */
};

struct usdhc_context {
	bool cmd_only;
	struct usdhc_cmd cmd;
	struct usdhc_data data;
	struct usdhc_adma_config dma_cfg;
};

enum usdhc_endian_mode {
	USDHC_BIG_ENDIAN = 0U,
	/* Big endian mode
	 */
	USDHC_HALF_WORD_BIG_ENDIAN = 1U,
	/* Half word big endian mode
	 */
	USDHC_LITTLE_ENDIAN = 2U,
	/* Little endian mode
	 */
};

struct usdhc_config {
	USDHC_Type *base;
	u32_t data_timeout;
	/* Data timeout value
	 */
	enum usdhc_endian_mode endian;
	/* Endian mode
	 */
	u8_t read_watermark;
	/* Watermark level for DMA read operation.
	 * Available range is 1 ~ 128.
	 */
	u8_t write_watermark;
	/* Watermark level for DMA write operation.
	 * Available range is 1 ~ 128.
	 */
	u8_t read_burst_len;
	/* Read burst len
	 */
	u8_t write_burst_len;
	/* Write burst len
	 */
	u32_t src_clk_hz;
};

struct usdhc_capability {
	u32_t max_blk_len;
	u32_t max_blk_cnt;
	u32_t host_flags;
};

enum host_detect_type {
	SD_DETECT_GPIO_CD,
	/* sd card detect by CD pin through GPIO
	 */
	SD_DETECT_HOST_CD,
	/* sd card detect by CD pin through host
	 */
	SD_DETECT_HOST_DATA3,
	/* sd card detect by DAT3 pin through host
	 */
};

struct usdhc_client_info {
	u32_t busclk_hz;
	u32_t relative_addr;
	u32_t version;
	u32_t card_flags;
	u32_t raw_cid[4U];
	u32_t raw_csd[4U];
	u32_t raw_scr[2U];
	u32_t raw_ocr;
	struct sd_cid cid;
	struct sd_csd csd;
	struct sd_scr scr;
	u32_t sd_block_count;
	u32_t sd_block_size;
	enum sd_timing_mode sd_timing;
	enum sd_driver_strength driver_strength;
	enum sd_max_current max_current;
	enum sd_voltage voltage;
};

struct usdhc_board_config {
	struct device *pwr_gpio;
	u32_t pwr_pin;
	int pwr_flags;

	struct device *detect_gpio;
	u32_t detect_pin;
	struct gpio_callback detect_cb;
};

struct usdhc_priv {
	bool host_ready;
	u8_t status;
	u8_t nusdhc;

	struct usdhc_board_config board_cfg;

	enum host_detect_type detect_type;
	bool inserted;

	struct device *clock_dev;
	clock_control_subsys_t clock_sys;

	struct usdhc_config host_config;
	struct usdhc_capability host_capability;

	struct usdhc_client_info card_info;

	struct usdhc_context op_context;
};

enum usdhc_xfer_data_type {
	USDHC_XFER_NORMAL = 0U,
	/* transfer normal read/write data
	 */
	USDHC_XFER_TUNING = 1U,
	/* transfer tuning data
	 */
	USDHC_XFER_BOOT = 2U,
	/* transfer boot data
	 */
	USDHC_XFER_BOOT_CONTINUOUS = 3U,
	/* transfer boot data continuous
	 */
};

#define USDHC_ADMA1_ADDRESS_ALIGN (4096U)
#define USDHC_ADMA1_LENGTH_ALIGN (4096U)
#define USDHC_ADMA2_ADDRESS_ALIGN (4U)
#define USDHC_ADMA2_LENGTH_ALIGN (4U)

#define USDHC_ADMA2_DESCRIPTOR_LENGTH_SHIFT (16U)
#define USDHC_ADMA2_DESCRIPTOR_LENGTH_MASK (0xFFFFU)
#define USDHC_ADMA2_DESCRIPTOR_MAX_LENGTH_PER_ENTRY	\
	(USDHC_ADMA2_DESCRIPTOR_LENGTH_MASK - 3U)

#define SWAP_WORD_BYTE_SEQUENCE(x) (__REV(x))
#define SWAP_HALF_WROD_BYTE_SEQUENCE(x) (__REV16(x))

#define SDMMCHOST_NOT_SUPPORT 0U

#define CARD_BUS_FREQ_50MHZ (0U)
#define CARD_BUS_FREQ_100MHZ0 (1U)
#define CARD_BUS_FREQ_100MHZ1 (2U)
#define CARD_BUS_FREQ_200MHZ (3U)

#define CARD_BUS_STRENGTH_0 (0U)
#define CARD_BUS_STRENGTH_1 (1U)
#define CARD_BUS_STRENGTH_2 (2U)
#define CARD_BUS_STRENGTH_3 (3U)
#define CARD_BUS_STRENGTH_4 (4U)
#define CARD_BUS_STRENGTH_5 (5U)
#define CARD_BUS_STRENGTH_6 (6U)
#define CARD_BUS_STRENGTH_7 (7U)

enum usdhc_adma_flag {
	USDHC_ADMA_SINGLE_FLAG = 0U,
	USDHC_ADMA_MUTI_FLAG = 1U,
};

enum usdhc_adma2_descriptor_flag {
	USDHC_ADMA2_VALID_FLAG = (1U << 0U),
	/* Valid flag
	 */
	USDHC_ADMA2_END_FLAG = (1U << 1U),
	/* End flag
	 */
	USDHC_ADMA2_INT_FLAG = (1U << 2U),
	/* Interrupt flag
	 */
	USDHC_ADMA2_ACTIVITY1_FLAG = (1U << 4U),
	/* Activity 1 mask
	 */
	USDHC_ADMA2_ACTIVITY2_FLAG = (1U << 5U),
	/* Activity 2 mask
	 */

	USDHC_ADMA2_NOP_FLAG =
		(USDHC_ADMA2_VALID_FLAG),
	/* No operation
	 */
	USDHC_ADMA2_RESERVED_FLAG =
		(USDHC_ADMA2_ACTIVITY1_FLAG |
		USDHC_ADMA2_VALID_FLAG),
	/* Reserved
	 */
	USDHC_ADMA2_XFER_FLAG =
		(USDHC_ADMA2_ACTIVITY2_FLAG |
		USDHC_ADMA2_VALID_FLAG),
	/* Transfer type
	 */
	USDHC_ADMA2_LINK_FLAG =
		(USDHC_ADMA2_ACTIVITY1_FLAG |
		USDHC_ADMA2_ACTIVITY2_FLAG |
		USDHC_ADMA2_VALID_FLAG),
	/* Link type
	 */
};

struct usdhc_adma2_descriptor {
	u32_t attribute;
	/*!< The control and status field */
	const u32_t *address;
	/*!< The address field */
};

enum usdhc_card_flag {
	USDHC_HIGH_CAPACITY_FLAG =
		(1U << 1U),
	/* Support high capacity
	 */
	USDHC_4BIT_WIDTH_FLAG =
		(1U << 2U),
	/* Support 4-bit data width
	 */
	USDHC_SDHC_FLAG =
		(1U << 3U),
	/* Card is SDHC
	 */
	USDHC_SDXC_FLAG =
		(1U << 4U),
	/* Card is SDXC
	 */
	USDHC_VOL_1_8V_FLAG =
		(1U << 5U),
	/* card support 1.8v voltage
	 */
	USDHC_SET_BLK_CNT_CMD23_FLAG =
		(1U << 6U),
	/* card support cmd23 flag
	 */
	USDHC_SPEED_CLASS_CONTROL_CMD_FLAG =
		(1U << 7U),
	/* card support speed class control flag
	 */
};

enum usdhc_capability_flag {
	USDHC_SUPPORT_ADMA_FLAG =
		USDHC_HOST_CTRL_CAP_ADMAS_MASK,
	/*!< Support ADMA */
	USDHC_SUPPORT_HIGHSPEED_FLAG =
		USDHC_HOST_CTRL_CAP_HSS_MASK,
	/*!< Support high-speed */
	USDHC_SUPPORT_DMA_FLAG =
		USDHC_HOST_CTRL_CAP_DMAS_MASK,
	/*!< Support DMA */
	USDHC_SUPPORT_SUSPEND_RESUME_FLAG =
		USDHC_HOST_CTRL_CAP_SRS_MASK,
	/*!< Support suspend/resume */
	USDHC_SUPPORT_V330_FLAG =
		USDHC_HOST_CTRL_CAP_VS33_MASK,
	/*!< Support voltage 3.3V */
	USDHC_SUPPORT_V300_FLAG =
		USDHC_HOST_CTRL_CAP_VS30_MASK,
	/*!< Support voltage 3.0V */
	USDHC_SUPPORT_V180_FLAG =
		USDHC_HOST_CTRL_CAP_VS18_MASK,
	/*!< Support voltage 1.8V */
	/* Put additional two flags in
	 * HTCAPBLT_MBL's position.
	 */
	USDHC_SUPPORT_4BIT_FLAG =
		(USDHC_HOST_CTRL_CAP_MBL_SHIFT << 0U),
	/*!< Support 4 bit mode */
	USDHC_SUPPORT_8BIT_FLAG =
		(USDHC_HOST_CTRL_CAP_MBL_SHIFT << 1U),
	/*!< Support 8 bit mode */
	/* sd version 3.0 new feature */
	USDHC_SUPPORT_DDR50_FLAG =
		USDHC_HOST_CTRL_CAP_DDR50_SUPPORT_MASK,
	/*!< support DDR50 mode */

#if defined(FSL_FEATURE_USDHC_HAS_SDR104_MODE) &&\
	(!FSL_FEATURE_USDHC_HAS_SDR104_MODE)
	USDHC_SUPPORT_SDR104_FLAG = 0,
	/*!< not support SDR104 mode */
#else
	USDHC_SUPPORT_SDR104_FLAG =
		USDHC_HOST_CTRL_CAP_SDR104_SUPPORT_MASK,
	/*!< support SDR104 mode */
#endif
#if defined(FSL_FEATURE_USDHC_HAS_SDR50_MODE) &&\
	(!FSL_FEATURE_USDHC_HAS_SDR50_MODE)
	USDHC_SUPPORT_SDR50_FLAG = 0U,
	/*!< not support SDR50 mode */
#else
	USDHC_SUPPORT_SDR50_FLAG =
		USDHC_HOST_CTRL_CAP_SDR50_SUPPORT_MASK,
	/*!< support SDR50 mode */
#endif
};

#define NXP_SDMMC_MAX_VOLTAGE_RETRIES (1000U)

#define CARD_DATA0_STATUS_MASK USDHC_DATA0_LINE_LEVEL_FLAG
#define CARD_DATA1_STATUS_MASK USDHC_DATA1_LINE_LEVEL_FLAG
#define CARD_DATA2_STATUS_MASK USDHC_DATA2_LINE_LEVEL_FLAG
#define CARD_DATA3_STATUS_MASK USDHC_DATA3_LINE_LEVEL_FLAG
#define CARD_DATA0_NOT_BUSY USDHC_DATA0_LINE_LEVEL_FLAG

#define SDHC_STANDARD_TUNING_START (10U)
/*!< standard tuning start point */
#define SDHC_TUINIG_STEP (2U)
/*!< standard tuning step */
#define SDHC_RETUNING_TIMER_COUNT (0U)
/*!< Re-tuning timer */

#define USDHC_MAX_DVS	\
	((USDHC_SYS_CTRL_DVS_MASK >>	\
	USDHC_SYS_CTRL_DVS_SHIFT) + 1U)
#define USDHC_MAX_CLKFS	\
	((USDHC_SYS_CTRL_SDCLKFS_MASK >>	\
	USDHC_SYS_CTRL_SDCLKFS_SHIFT) + 1U)
#define USDHC_PREV_DVS(x) ((x) -= 1U)
#define USDHC_PREV_CLKFS(x, y) ((x) >>= (y))

#define SDMMCHOST_SUPPORT_SDR104_FREQ SD_CLOCK_208MHZ

#define USDHC_ADMA_TABLE_WORDS (8U)
#define USDHC_ADMA2_ADDR_ALIGN (4U)
#define USDHC_READ_BURST_LEN (8U)
#define USDHC_WRITE_BURST_LEN (8U)
#define USDHC_DATA_TIMEOUT (0xFU)

#define USDHC_READ_WATERMARK_LEVEL (0x80U)
#define USDHC_WRITE_WATERMARK_LEVEL (0x80U)

enum usdhc_reset {
	USDHC_RESET_ALL =
		USDHC_SYS_CTRL_RSTA_MASK,
	/*!< Reset all except card detection */
	USDHC_RESET_CMD =
		USDHC_SYS_CTRL_RSTC_MASK,
	/*!< Reset command line */
	USDHC_RESET_DATA =
		USDHC_SYS_CTRL_RSTD_MASK,
	/*!< Reset data line */

#if defined(FSL_FEATURE_USDHC_HAS_SDR50_MODE) &&\
	(!FSL_FEATURE_USDHC_HAS_SDR50_MODE)
	USDHC_RESET_TUNING = 0U,
	/*!< no reset tuning circuit bit */
#else
	USDHC_RESET_TUNING = USDHC_SYS_CTRL_RSTT_MASK,
	/*!< reset tuning circuit */
#endif

	USDHC_RESETS_All =
		(USDHC_RESET_ALL |
		USDHC_RESET_CMD |
		USDHC_RESET_DATA |
		USDHC_RESET_TUNING),
	/*!< All reset types */
};

#define HOST_CARD_INSERT_CD_LEVEL (0U)

static void usdhc_millsec_delay(unsigned int cycles_to_wait)
{
	unsigned int start = z_timer_cycle_get_32();

	while (z_timer_cycle_get_32() - start < (cycles_to_wait * 1000))
		;
}

u32_t g_usdhc_boot_dummy __aligned(64);
u32_t g_usdhc_rx_dummy[2048] __aligned(64);

static int usdhc_adma2_descriptor_cfg(
	u32_t *adma_table, u32_t adma_table_words,
	const u32_t *data_addr, u32_t data_size, u32_t flags)
{
	u32_t min_entries, start_entry = 0U;
	u32_t max_entries = (adma_table_words * sizeof(u32_t)) /
		sizeof(struct usdhc_adma2_descriptor);
	struct usdhc_adma2_descriptor *adma2_addr =
		(struct usdhc_adma2_descriptor *)(adma_table);
	u32_t i, dma_buf_len = 0U;

	if ((u32_t)data_addr % USDHC_ADMA2_ADDRESS_ALIGN) {
		return -EIO;
	}
	/* Add non aligned access support.
	 */
	if (data_size % sizeof(u32_t)) {
		/* make the data length as word-aligned */
		data_size += sizeof(u32_t) - (data_size % sizeof(u32_t));
	}

	/* Check if ADMA descriptor's number is enough. */
	if (!(data_size % USDHC_ADMA2_DESCRIPTOR_MAX_LENGTH_PER_ENTRY)) {
		min_entries = data_size /
			USDHC_ADMA2_DESCRIPTOR_MAX_LENGTH_PER_ENTRY;
	} else {
		min_entries = ((data_size /
			USDHC_ADMA2_DESCRIPTOR_MAX_LENGTH_PER_ENTRY) + 1U);
	}
	/* calcucate the start entry for multiple descriptor mode,
	 * ADMA engine is not stop, so update the descriptor
	 * data address and data size is enough
	 */
	if (flags == USDHC_ADMA_MUTI_FLAG) {
		for (i = 0U; i < max_entries; i++) {
			if (!(adma2_addr[i].attribute & USDHC_ADMA2_VALID_FLAG))
				break;
		}
		start_entry = i;
		/* add one entry for dummy entry */
		min_entries += 1U;
	}

	if ((min_entries + start_entry) > max_entries) {
		return -EIO;
	}

	for (i = start_entry; i < (min_entries + start_entry); i++) {
		if (data_size > USDHC_ADMA2_DESCRIPTOR_MAX_LENGTH_PER_ENTRY) {
			dma_buf_len =
				USDHC_ADMA2_DESCRIPTOR_MAX_LENGTH_PER_ENTRY;
		} else {
			dma_buf_len = (data_size == 0U ? sizeof(u32_t) :
				data_size);
			/* adma don't support 0 data length transfer
			 * descriptor
			 */
		}

		/* Each descriptor for ADMA2 is 64-bit in length */
		adma2_addr[i].address = (data_size == 0U) ?
			&g_usdhc_boot_dummy : data_addr;
		adma2_addr[i].attribute = (dma_buf_len <<
			USDHC_ADMA2_DESCRIPTOR_LENGTH_SHIFT);
		adma2_addr[i].attribute |=
			(data_size == 0U) ? 0U :
				(USDHC_ADMA2_XFER_FLAG | USDHC_ADMA2_INT_FLAG);
		data_addr += (dma_buf_len / sizeof(u32_t));

		if (data_size != 0U)
			data_size -= dma_buf_len;
	}

	/* add a dummy valid ADMA descriptor for multiple descriptor mode,
	 * this is useful when transfer boot data, the ADMA
	 * engine will not stop at block gap
	 */
	if (flags == USDHC_ADMA_MUTI_FLAG) {
		adma2_addr[start_entry + 1U].attribute |= USDHC_ADMA2_XFER_FLAG;
	} else {
		adma2_addr[i - 1U].attribute |= USDHC_ADMA2_END_FLAG;
		/* set the end bit */
	}

	return 0;
}

static int usdhc_Internal_dma_cfg(struct usdhc_priv *priv,
	struct usdhc_adma_config *dma_cfg,
	const u32_t *data_addr)
{
	USDHC_Type *base = priv->host_config.base;
	bool cmd23 = priv->op_context.data.cmd23;

	if (dma_cfg->dma_mode == USDHC_DMA_SIMPLE) {
		/* check DMA data buffer address align or not */
		if (((u32_t)data_addr % USDHC_ADMA2_ADDRESS_ALIGN) != 0U) {
			return -EIO;
		}
		/* in simple DMA mode if use auto CMD23,
		 * address should load to ADMA addr,
		 * and block count should load to DS_ADDR
		 */
		if (cmd23)
			base->ADMA_SYS_ADDR = (u32_t)data_addr;
		else
			base->DS_ADDR = (u32_t)data_addr;
	} else {
		/* When use ADMA, disable simple DMA */
		base->DS_ADDR = 0U;
		base->ADMA_SYS_ADDR = (u32_t)(dma_cfg->adma_table);
	}

	/* select DMA mode and config the burst length */
	base->PROT_CTRL &= ~(USDHC_PROT_CTRL_DMASEL_MASK |
		USDHC_PROT_CTRL_BURST_LEN_EN_MASK);
	base->PROT_CTRL |= USDHC_PROT_CTRL_DMASEL(dma_cfg->dma_mode) |
		USDHC_PROT_CTRL_BURST_LEN_EN(dma_cfg->burst_len);
	/* enable DMA */
	base->MIX_CTRL |= USDHC_MIX_CTRL_DMAEN_MASK;

	return 0;
}


static int usdhc_adma_table_cfg(struct usdhc_priv *priv, u32_t flags)
{
	int error = -EIO;
	struct usdhc_data *data = &priv->op_context.data;
	struct usdhc_adma_config *dma_cfg = &priv->op_context.dma_cfg;
	u32_t boot_dummy_off = data->data_type == USDHC_XFER_BOOT_CONTINUOUS ?
		sizeof(u32_t) : 0U;
	const u32_t *data_addr = (const u32_t *)((u32_t)((!data->rx_data) ?
		data->tx_data : data->rx_data) + boot_dummy_off);
	u32_t data_size = data->block_size * data->block_count - boot_dummy_off;

	switch (dma_cfg->dma_mode) {
	case USDHC_DMA_SIMPLE:
		error = 0;
	break;

	case USDHC_DMA_ADMA1:
		error = -EINVAL;
	break;

	case USDHC_DMA_ADMA2:
		error = usdhc_adma2_descriptor_cfg(dma_cfg->adma_table,
			dma_cfg->adma_table_words, data_addr, data_size, flags);
	break;
	default:
		return -EINVAL;
	}

	/* for internal dma, internal DMA configurations should not update
	 * the configurations when continuous transfer the
	 * boot data, only the DMA descriptor need update
	 */
	if ((!error) && (data->data_type != USDHC_XFER_BOOT_CONTINUOUS)) {
		error = usdhc_Internal_dma_cfg(priv, dma_cfg, data_addr);
	}

	return error;
}

static int usdhc_data_xfer_cfg(struct usdhc_priv *priv,
	bool en_dma)
{
	USDHC_Type *base = priv->host_config.base;
	u32_t mix_ctrl = base->MIX_CTRL;
	struct usdhc_data *data = NULL;
	u32_t *flag = &priv->op_context.cmd.flags;

	if (!priv->op_context.cmd_only)
		data = &priv->op_context.data;

	if (data != NULL) {
		if (data->data_type == USDHC_XFER_BOOT_CONTINUOUS) {
			/* clear stop at block gap request */
			base->PROT_CTRL &= ~USDHC_PROT_CTRL_SABGREQ_MASK;
			/* continuous transfer data */
			base->PROT_CTRL |= USDHC_PROT_CTRL_CREQ_MASK;
			return 0;
		}

		/* check data inhibit flag */
		if (base->PRES_STATE & USDHC_DATA_INHIBIT_FLAG)
			return -EBUSY;
		/* check transfer block count */
		if ((data->block_count > USDHC_MAX_BLOCK_COUNT) ||
			(!data->tx_data && !data->rx_data))
			return -EINVAL;

		/* config mix parameter */
		mix_ctrl &= ~(USDHC_MIX_CTRL_MSBSEL_MASK |
			USDHC_MIX_CTRL_BCEN_MASK |
			USDHC_MIX_CTRL_DTDSEL_MASK |
			USDHC_MIX_CTRL_AC12EN_MASK);

		if (data->rx_data) {
			mix_ctrl |= USDHC_MIX_CTRL_DTDSEL_MASK;
		}

		if (data->block_count > 1U) {
			mix_ctrl |= USDHC_MIX_CTRL_MSBSEL_MASK |
				USDHC_MIX_CTRL_BCEN_MASK;
			/* auto command 12 */
			if (data->cmd12) {
				mix_ctrl |= USDHC_MIX_CTRL_AC12EN_MASK;
			}
		}

		/* auto command 23, auto send set block count cmd before
		 * multiple read/write
		 */
		if ((data->cmd23)) {
			mix_ctrl |= USDHC_MIX_CTRL_AC23EN_MASK;
			base->VEND_SPEC2 |=
				USDHC_VEND_SPEC2_ACMD23_ARGU2_EN_MASK;
			/* config the block count to DS_ADDR */
			base->DS_ADDR = data->block_count;
		} else {
			mix_ctrl &= ~USDHC_MIX_CTRL_AC23EN_MASK;
			base->VEND_SPEC2 &=
				(~USDHC_VEND_SPEC2_ACMD23_ARGU2_EN_MASK);
		}

		if (data->data_type != USDHC_XFER_BOOT) {
			/* config data block size/block count */
			base->BLK_ATT =
				((base->BLK_ATT & ~(USDHC_BLK_ATT_BLKSIZE_MASK |
				USDHC_BLK_ATT_BLKCNT_MASK)) |
				(USDHC_BLK_ATT_BLKSIZE(data->block_size) |
				USDHC_BLK_ATT_BLKCNT(data->block_count)));
		} else {
			mix_ctrl |= USDHC_MIX_CTRL_MSBSEL_MASK |
				USDHC_MIX_CTRL_BCEN_MASK;
			base->PROT_CTRL |=
				USDHC_PROT_CTRL_RD_DONE_NO_8CLK_MASK;
		}

		/* data present flag */
		*flag |= USDHC_DATA_PRESENT_FLAG;
		/* Disable useless interrupt */
		if (en_dma) {
			base->INT_SIGNAL_EN &=
				~(USDHC_INT_BUF_WRITE_READY_FLAG |
				USDHC_INT_BUF_READ_READY_FLAG |
				USDHC_INT_DMA_DONE_FLAG);
			base->INT_STATUS_EN &=
				~(USDHC_INT_BUF_WRITE_READY_FLAG |
				USDHC_INT_BUF_READ_READY_FLAG |
				USDHC_INT_DMA_DONE_FLAG);
		} else {
			base->INT_SIGNAL_EN |=
				USDHC_INT_BUF_WRITE_READY_FLAG |
				USDHC_INT_BUF_READ_READY_FLAG;
			base->INT_STATUS_EN |=
				USDHC_INT_BUF_WRITE_READY_FLAG |
				USDHC_INT_BUF_READ_READY_FLAG;
		}
	} else {
		/* clear data flags */
		mix_ctrl &= ~(USDHC_MIX_CTRL_MSBSEL_MASK |
					USDHC_MIX_CTRL_BCEN_MASK |
					USDHC_MIX_CTRL_DTDSEL_MASK |
					USDHC_MIX_CTRL_AC12EN_MASK |
					USDHC_MIX_CTRL_AC23EN_MASK);

		if (base->PRES_STATE & USDHC_CMD_INHIBIT_FLAG)
			return -EBUSY;
	}

	/* config the mix parameter */
	base->MIX_CTRL = mix_ctrl;

	return 0;
}

static void usdhc_send_cmd(USDHC_Type *base, struct usdhc_cmd *command)
{
	u32_t xfer_type = base->CMD_XFR_TYP;
	u32_t flags = command->flags;

	if (!(base->PRES_STATE & USDHC_CMD_INHIBIT_FLAG)
		&& (command->cmd_type != USDHC_CMD_TYPE_EMPTY)) {
		/* Define the flag corresponding to each response type. */
		switch (command->rsp_type) {
		case SDHC_RSP_TYPE_NONE:
			break;
		case SDHC_RSP_TYPE_R1: /* Response 1 */
		case SDHC_RSP_TYPE_R5: /* Response 5 */
		case SDHC_RSP_TYPE_R6: /* Response 6 */
		case SDHC_RSP_TYPE_R7: /* Response 7 */
			flags |= (USDHC_RSP_LEN_48_FLAG |
					USDHC_CRC_CHECK_FLAG |
					USDHC_IDX_CHECK_FLAG);
		break;

		case SDHC_RSP_TYPE_R1b: /* Response 1 with busy */
		case SDHC_RSP_TYPE_R5b: /* Response 5 with busy */
			flags |= (USDHC_RSP_LEN_48_BUSY_FLAG |
					USDHC_CRC_CHECK_FLAG |
					USDHC_IDX_CHECK_FLAG);
		break;

		case SDHC_RSP_TYPE_R2: /* Response 2 */
			flags |= (USDHC_RSP_LEN_136_FLAG |
				USDHC_CRC_CHECK_FLAG);
		break;

		case SDHC_RSP_TYPE_R3: /* Response 3 */
		case SDHC_RSP_TYPE_R4: /* Response 4 */
			flags |= (USDHC_RSP_LEN_48_FLAG);
		break;

		default:
			break;
		}

		if (command->cmd_type == USDHC_CMD_TYPE_ABORT)
			flags |= USDHC_CMD_TYPE_ABORT_FLAG;

		/* config cmd index */
		xfer_type &= ~(USDHC_CMD_XFR_TYP_CMDINX_MASK |
					USDHC_CMD_XFR_TYP_CMDTYP_MASK |
					USDHC_CMD_XFR_TYP_CICEN_MASK |
					USDHC_CMD_XFR_TYP_CCCEN_MASK |
					USDHC_CMD_XFR_TYP_RSPTYP_MASK |
					USDHC_CMD_XFR_TYP_DPSEL_MASK);

		xfer_type |=
			(((command->index << USDHC_CMD_XFR_TYP_CMDINX_SHIFT) &
				USDHC_CMD_XFR_TYP_CMDINX_MASK) |
				((flags) & (USDHC_CMD_XFR_TYP_CMDTYP_MASK |
				USDHC_CMD_XFR_TYP_CICEN_MASK |
				USDHC_CMD_XFR_TYP_CCCEN_MASK |
				USDHC_CMD_XFR_TYP_RSPTYP_MASK |
				USDHC_CMD_XFR_TYP_DPSEL_MASK)));

		/* config the command xfertype and argument */
		base->CMD_ARG = command->argument;
		base->CMD_XFR_TYP = xfer_type;
	}

	if (command->cmd_type == USDHC_CMD_TYPE_EMPTY) {
		/* disable CMD done interrupt for empty command */
		base->INT_SIGNAL_EN &= ~USDHC_INT_SIGNAL_EN_CCIEN_MASK;
	}
}

static int usdhc_cmd_rsp(struct usdhc_priv *priv)
{
	u32_t i;
	USDHC_Type *base = priv->host_config.base;
	struct usdhc_cmd *cmd = &priv->op_context.cmd;

	if (cmd->rsp_type != SDHC_RSP_TYPE_NONE) {
		cmd->response[0U] = base->CMD_RSP0;
		if (cmd->rsp_type == SDHC_RSP_TYPE_R2) {
			cmd->response[1U] = base->CMD_RSP1;
			cmd->response[2U] = base->CMD_RSP2;
			cmd->response[3U] = base->CMD_RSP3;

			i = 4U;
			/* R3-R2-R1-R0(lowest 8 bit is invalid bit)
			 * has the same format
			 * as R2 format in SD specification document
			 * after removed internal CRC7 and end bit.
			 */
			do {
				cmd->response[i - 1U] <<= 8U;
				if (i > 1U) {
					cmd->response[i - 1U] |=
						((cmd->response[i - 2U] &
						0xFF000000U) >> 24U);
				}
				i--;
			} while (i);
		}
	}
	/* check response error flag */
	if ((cmd->rsp_err_flags) &&
		((cmd->rsp_type == SDHC_RSP_TYPE_R1) ||
		(cmd->rsp_type == SDHC_RSP_TYPE_R1b) ||
		(cmd->rsp_type == SDHC_RSP_TYPE_R6) ||
		(cmd->rsp_type == SDHC_RSP_TYPE_R5))) {
		if (((cmd->rsp_err_flags) & (cmd->response[0U])))
			return -EIO;
	}

	return 0;
}

static int usdhc_wait_cmd_done(struct usdhc_priv *priv,
	bool poll_cmd)
{
	int error = 0;
	u32_t int_status = 0U;
	USDHC_Type *base = priv->host_config.base;

	/* check if need polling command done or not */
	if (poll_cmd) {
		/* Wait command complete or USDHC encounters error. */
		while (!(int_status & (USDHC_INT_CMD_DONE_FLAG |
			USDHC_INT_CMD_ERR_FLAG))) {
			int_status = base->INT_STATUS;
		}

		if ((int_status & USDHC_INT_TUNING_ERR_FLAG) ||
			(int_status & USDHC_INT_CMD_ERR_FLAG)) {
			error = -EIO;
		}
		/* Receive response when command completes successfully. */
		if (!error) {
			error = usdhc_cmd_rsp(priv);
		} else {
			LOG_ERR("CMD%d Polling ERROR\r\n",
				priv->op_context.cmd.index);
		}

		base->INT_STATUS = (USDHC_INT_CMD_DONE_FLAG |
				USDHC_INT_CMD_ERR_FLAG |
				USDHC_INT_TUNING_ERR_FLAG);
	}

	return error;
}

static inline void usdhc_write_data(USDHC_Type *base, u32_t data)
{
	base->DATA_BUFF_ACC_PORT = data;
}

static inline u32_t usdhc_read_data(USDHC_Type *base)
{
	return base->DATA_BUFF_ACC_PORT;
}

static u32_t usdhc_read_data_port(struct usdhc_priv *priv,
	u32_t xfered_words)
{
	USDHC_Type *base = priv->host_config.base;
	struct usdhc_data *data = &priv->op_context.data;
	u32_t i, total_words, remaing_words;
	/* The words can be read at this time. */
	u32_t watermark = ((base->WTMK_LVL & USDHC_WTMK_LVL_RD_WML_MASK) >>
		USDHC_WTMK_LVL_RD_WML_SHIFT);

	/* If DMA is enable, do not need to polling data port */
	if (!(base->MIX_CTRL & USDHC_MIX_CTRL_DMAEN_MASK)) {
		/*Add non aligned access support.*/
		if (data->block_size % sizeof(u32_t)) {
			data->block_size +=
				sizeof(u32_t) -
				(data->block_size % sizeof(u32_t));
			/* make the block size as word-aligned */
		}

		total_words = ((data->block_count * data->block_size) /
			sizeof(u32_t));

		if (watermark >= total_words) {
			remaing_words = total_words;
		} else if ((watermark < total_words) &&
			((total_words - xfered_words) >= watermark)) {
			remaing_words = watermark;
		} else {
			remaing_words = (total_words - xfered_words);
		}

		i = 0U;
		while (i < remaing_words) {
			data->rx_data[xfered_words++] = usdhc_read_data(base);
			i++;
		}
	}

	return xfered_words;
}

static int usdhc_read_data_port_sync(struct usdhc_priv *priv)
{
	USDHC_Type *base = priv->host_config.base;
	struct usdhc_data *data = &priv->op_context.data;
	u32_t total_words;
	u32_t xfered_words = 0U, int_status = 0U;
	int error = 0;

	if (data->block_size % sizeof(u32_t)) {
		data->block_size +=
			sizeof(u32_t) -
			(data->block_size % sizeof(u32_t));
	}

	total_words =
		((data->block_count * data->block_size) /
		sizeof(u32_t));

	while ((!error) && (xfered_words < total_words)) {
		while (!(int_status & (USDHC_INT_BUF_READ_READY_FLAG |
			USDHC_INT_DATA_ERR_FLAG |
			USDHC_INT_TUNING_ERR_FLAG)))
			int_status = base->INT_STATUS;

		/* during std tuning process, software do not need to read data,
		 * but wait BRR is enough
		 */
		if ((data->data_type == USDHC_XFER_TUNING) &&
			(int_status & USDHC_INT_BUF_READ_READY_FLAG)) {
			base->INT_STATUS = USDHC_INT_BUF_READ_READY_FLAG |
				USDHC_INT_TUNING_PASS_FLAG;

			return 0;
		} else if ((int_status & USDHC_INT_TUNING_ERR_FLAG)) {
			base->INT_STATUS = USDHC_INT_TUNING_ERR_FLAG;
			/* if tuning error occur ,return directly */
			error = -EIO;
		} else if ((int_status & USDHC_INT_DATA_ERR_FLAG)) {
			if (!(data->ignore_err))
				error = -EIO;
			/* clear data error flag */
			base->INT_STATUS = USDHC_INT_DATA_ERR_FLAG;
		}

		if (!error) {
			xfered_words = usdhc_read_data_port(priv, xfered_words);
			/* clear buffer read ready */
			base->INT_STATUS = USDHC_INT_BUF_READ_READY_FLAG;
			int_status = 0U;
		}
	}

	/* Clear data complete flag after the last read operation. */
	base->INT_STATUS = USDHC_INT_DATA_DONE_FLAG;

	return error;
}

static u32_t usdhc_write_data_port(struct usdhc_priv *priv,
	u32_t xfered_words)
{
	USDHC_Type *base = priv->host_config.base;
	struct usdhc_data *data = &priv->op_context.data;
	u32_t i, total_words, remaing_words;
	/* Words can be wrote at this time. */
	u32_t watermark = ((base->WTMK_LVL & USDHC_WTMK_LVL_WR_WML_MASK) >>
		USDHC_WTMK_LVL_WR_WML_SHIFT);

	/* If DMA is enable, do not need to polling data port */
	if (!(base->MIX_CTRL & USDHC_MIX_CTRL_DMAEN_MASK)) {
		if (data->block_size % sizeof(u32_t)) {
			data->block_size +=
				sizeof(u32_t) -
				(data->block_size % sizeof(u32_t));
		}

		total_words =
			((data->block_count * data->block_size) /
			sizeof(u32_t));

		if (watermark >= total_words) {
			remaing_words = total_words;
		} else if ((watermark < total_words) &&
			((total_words - xfered_words) >= watermark)) {
			remaing_words = watermark;
		} else {
			remaing_words = (total_words - xfered_words);
		}

		i = 0U;
		while (i < remaing_words) {
			usdhc_write_data(base, data->tx_data[xfered_words++]);
			i++;
		}
	}

	return xfered_words;
}

static status_t usdhc_write_data_port_sync(struct usdhc_priv *priv)
{
	USDHC_Type *base = priv->host_config.base;
	struct usdhc_data *data = &priv->op_context.data;
	u32_t total_words;
	u32_t xfered_words = 0U, int_status = 0U;
	int error = 0;

	if (data->block_size % sizeof(u32_t)) {
		data->block_size +=
			sizeof(u32_t) - (data->block_size % sizeof(u32_t));
	}

	total_words = (data->block_count * data->block_size) / sizeof(u32_t);

	while ((!error) && (xfered_words < total_words)) {
		while (!(int_status & (USDHC_INT_BUF_WRITE_READY_FLAG |
				USDHC_INT_DATA_ERR_FLAG |
				USDHC_INT_TUNING_ERR_FLAG))) {
			int_status = base->INT_STATUS;
		}

		if (int_status & USDHC_INT_TUNING_ERR_FLAG) {
			base->INT_STATUS = USDHC_INT_TUNING_ERR_FLAG;
			/* if tuning error occur ,return directly */
			return -EIO;
		} else if (int_status & USDHC_INT_DATA_ERR_FLAG) {
			if (!(data->ignore_err))
				error = -EIO;
			/* clear data error flag */
			base->INT_STATUS = USDHC_INT_DATA_ERR_FLAG;
		}

		if (!error) {
			xfered_words = usdhc_write_data_port(priv,
				xfered_words);
			/* clear buffer write ready */
			base->INT_STATUS = USDHC_INT_BUF_WRITE_READY_FLAG;
			int_status = 0U;
		}
	}

	/* Wait write data complete or data transfer error
	 * after the last writing operation.
	 */
	while (!(int_status & (USDHC_INT_DATA_DONE_FLAG |
		USDHC_INT_DATA_ERR_FLAG))) {
		int_status = base->INT_STATUS;
	}

	if (int_status & USDHC_INT_DATA_ERR_FLAG) {
		if (!(data->ignore_err))
			error = -EIO;
	}
	base->INT_STATUS = USDHC_INT_DATA_DONE_FLAG |
		USDHC_INT_DATA_ERR_FLAG;

	return error;
}

static int usdhc_data_sync_xfer(struct usdhc_priv *priv, bool en_dma)
{
	int error = 0;
	u32_t int_status = 0U;
	USDHC_Type *base = priv->host_config.base;
	struct usdhc_data *data = &priv->op_context.data;

	if (en_dma) {
		/* Wait data complete or USDHC encounters error. */
		while (!((int_status &
			(USDHC_INT_DATA_DONE_FLAG | USDHC_INT_DATA_ERR_FLAG |
			USDHC_INT_CMD_ERR_FLAG | USDHC_INT_TUNING_ERR_FLAG)))) {
			int_status = base->INT_STATUS;
		}

		if (int_status & USDHC_INT_TUNING_ERR_FLAG) {
			error = -EIO;
		} else if ((int_status & (USDHC_INT_DATA_ERR_FLAG |
			USDHC_INT_DMA_ERR_FLAG))) {
			if ((!(data->ignore_err)) ||
				(int_status &
				USDHC_INT_DATA_TIMEOUT_FLAG)) {
				error = -EIO;
			}
		}
		/* load dummy data */
		if ((data->data_type == USDHC_XFER_BOOT_CONTINUOUS) && (!error))
			*(data->rx_data) = g_usdhc_boot_dummy;

		base->INT_STATUS = (USDHC_INT_DATA_DONE_FLAG |
			USDHC_INT_DATA_ERR_FLAG |
			USDHC_INT_DMA_ERR_FLAG |
			USDHC_INT_TUNING_PASS_FLAG |
			USDHC_INT_TUNING_ERR_FLAG);
	} else {
		if (data->rx_data) {
			error = usdhc_read_data_port_sync(priv);
		} else {
			error = usdhc_write_data_port_sync(priv);
		}
	}
	return error;
}

static int usdhc_xfer(struct usdhc_priv *priv)
{
	int error = -EIO;
	struct usdhc_data *data = NULL;
	bool en_dma = true, execute_tuning;
	USDHC_Type *base = priv->host_config.base;

	if (!priv->op_context.cmd_only) {
		data = &priv->op_context.data;
		if (data->data_type == USDHC_XFER_TUNING)
			execute_tuning = true;
		else
			execute_tuning = false;
	} else {
		execute_tuning = false;
	}

	/*check re-tuning request*/
	if ((base->INT_STATUS & USDHC_INT_RE_TUNING_EVENT_FLAG)) {
		base->INT_STATUS = USDHC_INT_RE_TUNING_EVENT_FLAG;
		return -EAGAIN;
	}

	/* Update ADMA descriptor table according to different DMA mode
	 * (no DMA, ADMA1, ADMA2).
	 */

	if (data && (!execute_tuning) && priv->op_context.dma_cfg.adma_table)
		error = usdhc_adma_table_cfg(priv,
			(data->data_type & USDHC_XFER_BOOT) ?
			USDHC_ADMA_MUTI_FLAG : USDHC_ADMA_SINGLE_FLAG);

	/* if the DMA descriptor configure fail or not needed , disable it */
	if (error) {
		en_dma = false;
		/* disable DMA, using polling mode in this situation */
		base->MIX_CTRL &= ~USDHC_MIX_CTRL_DMAEN_MASK;
		base->PROT_CTRL &= ~USDHC_PROT_CTRL_DMASEL_MASK;
	}

	/* config the data transfer parameter */
	error = usdhc_data_xfer_cfg(priv, en_dma);
	if (error)
		return error;
	/* send command first */
	usdhc_send_cmd(base, &priv->op_context.cmd);
	/* wait command done */
	error = usdhc_wait_cmd_done(priv, (data == NULL) ||
		(data->data_type == USDHC_XFER_NORMAL));
	/* wait transfer data finish */
	if (data && (!error)) {
		return usdhc_data_sync_xfer(priv, en_dma);
	}

	return error;
}

static inline void usdhc_select_1_8_vol(USDHC_Type *base, bool en_1_8_v)
{
	if (en_1_8_v)
		base->VEND_SPEC |= USDHC_VEND_SPEC_VSELECT_MASK;
	else
		base->VEND_SPEC &= ~USDHC_VEND_SPEC_VSELECT_MASK;
}

static inline void usdhc_force_clk_on(USDHC_Type *base, bool on)
{
	if (on)
		base->VEND_SPEC |= USDHC_VEND_SPEC_FRC_SDCLK_ON_MASK;
	else
		base->VEND_SPEC &= ~USDHC_VEND_SPEC_FRC_SDCLK_ON_MASK;
}

static void usdhc_tuning(USDHC_Type *base, u32_t start, u32_t step, bool enable)
{
	u32_t tuning_ctrl = 0U;

	if (enable) {
		/* feedback clock */
		base->MIX_CTRL |= USDHC_MIX_CTRL_FBCLK_SEL_MASK;
		/* config tuning start and step */
		tuning_ctrl = base->TUNING_CTRL;
		tuning_ctrl &= ~(USDHC_TUNING_CTRL_TUNING_START_TAP_MASK |
			USDHC_TUNING_CTRL_TUNING_STEP_MASK);
		tuning_ctrl |= (USDHC_TUNING_CTRL_TUNING_START_TAP(start) |
			USDHC_TUNING_CTRL_TUNING_STEP(step) |
			USDHC_TUNING_CTRL_STD_TUNING_EN_MASK);
		base->TUNING_CTRL = tuning_ctrl;

		/* excute tuning */
		base->AUTOCMD12_ERR_STATUS |=
			(USDHC_AUTOCMD12_ERR_STATUS_EXECUTE_TUNING_MASK |
			USDHC_AUTOCMD12_ERR_STATUS_SMP_CLK_SEL_MASK);
	} else {
		/* disable the standard tuning */
		base->TUNING_CTRL &= ~USDHC_TUNING_CTRL_STD_TUNING_EN_MASK;
		/* clear excute tuning */
		base->AUTOCMD12_ERR_STATUS &=
			~(USDHC_AUTOCMD12_ERR_STATUS_EXECUTE_TUNING_MASK |
			USDHC_AUTOCMD12_ERR_STATUS_SMP_CLK_SEL_MASK);
	}
}

int usdhc_adjust_tuning_timing(USDHC_Type *base, u32_t delay)
{
	u32_t clk_tune_ctrl = 0U;

	clk_tune_ctrl = base->CLK_TUNE_CTRL_STATUS;

	clk_tune_ctrl &= ~USDHC_CLK_TUNE_CTRL_STATUS_DLY_CELL_SET_PRE_MASK;

	clk_tune_ctrl |= USDHC_CLK_TUNE_CTRL_STATUS_DLY_CELL_SET_PRE(delay);

	/* load the delay setting */
	base->CLK_TUNE_CTRL_STATUS = clk_tune_ctrl;
	/* check delat setting error */
	if (base->CLK_TUNE_CTRL_STATUS &
		(USDHC_CLK_TUNE_CTRL_STATUS_PRE_ERR_MASK |
		USDHC_CLK_TUNE_CTRL_STATUS_NXT_ERR_MASK))
		return -EIO;

	return 0;
}

static inline void usdhc_set_retuning_timer(USDHC_Type *base, u32_t counter)
{
	base->HOST_CTRL_CAP &= ~USDHC_HOST_CTRL_CAP_TIME_COUNT_RETUNING_MASK;
	base->HOST_CTRL_CAP |= USDHC_HOST_CTRL_CAP_TIME_COUNT_RETUNING(counter);
}

static inline void usdhc_set_bus_width(USDHC_Type *base,
	enum usdhc_data_bus_width width)
{
	base->PROT_CTRL = ((base->PROT_CTRL & ~USDHC_PROT_CTRL_DTW_MASK) |
		USDHC_PROT_CTRL_DTW(width));
}

static int usdhc_execute_tuning(struct usdhc_priv *priv)
{
	bool tuning_err = true;
	int ret;
	USDHC_Type *base = priv->host_config.base;

	/* enable the standard tuning */
	usdhc_tuning(base, SDHC_STANDARD_TUNING_START, SDHC_TUINIG_STEP, true);

	while (true) {
		/* send tuning block */
		ret = usdhc_xfer(priv);
		if (ret) {
			return ret;
		}
		usdhc_millsec_delay(10);

		/*wait excute tuning bit clear*/
		if ((base->AUTOCMD12_ERR_STATUS &
			USDHC_AUTOCMD12_ERR_STATUS_EXECUTE_TUNING_MASK)) {
			continue;
		}

		/* if tuning error , re-tuning again */
		if ((base->CLK_TUNE_CTRL_STATUS &
			(USDHC_CLK_TUNE_CTRL_STATUS_NXT_ERR_MASK |
			USDHC_CLK_TUNE_CTRL_STATUS_PRE_ERR_MASK)) &&
			tuning_err) {
			tuning_err = false;
			/* enable the standard tuning */
			usdhc_tuning(base, SDHC_STANDARD_TUNING_START,
				SDHC_TUINIG_STEP, true);
			usdhc_adjust_tuning_timing(base,
				SDHC_STANDARD_TUNING_START);
		} else {
			break;
		}
	}

	/* delay to wait the host controller stable */
	usdhc_millsec_delay(1000);

	/* check tuning result*/
	if (!(base->AUTOCMD12_ERR_STATUS &
		USDHC_AUTOCMD12_ERR_STATUS_SMP_CLK_SEL_MASK)) {
		return -EIO;
	}

	usdhc_set_retuning_timer(base, SDHC_RETUNING_TIMER_COUNT);

	return 0;
}

static int usdhc_vol_switch(struct usdhc_priv *priv)
{
	USDHC_Type *base = priv->host_config.base;
	int retry = 0xffff;

	while (base->PRES_STATE &
		(CARD_DATA1_STATUS_MASK | CARD_DATA2_STATUS_MASK |
		CARD_DATA3_STATUS_MASK | CARD_DATA0_NOT_BUSY)) {
		retry--;
		if (retry <= 0) {
			return -EACCES;
		}
	}

	/* host switch to 1.8V */
	usdhc_select_1_8_vol(base, true);

	usdhc_millsec_delay(20000U);

	/*enable force clock on*/
	usdhc_force_clk_on(base, true);
	/* dealy 1ms,not exactly correct when use while */
	usdhc_millsec_delay(20000U);
	/*disable force clock on*/
	usdhc_force_clk_on(base, false);

	/* check data line and cmd line status */
	retry = 0xffff;
	while (!(base->PRES_STATE &
		(CARD_DATA1_STATUS_MASK | CARD_DATA2_STATUS_MASK |
		CARD_DATA3_STATUS_MASK | CARD_DATA0_NOT_BUSY))) {
		retry--;
		if (retry <= 0) {
			return -EBUSY;
		}
	}

	return 0;
}

static inline void usdhc_op_ctx_init(struct usdhc_priv *priv,
	bool cmd_only, u8_t cmd_idx, u32_t arg, enum sdhc_rsp_type rsp_type)
{
	struct usdhc_cmd *cmd = &priv->op_context.cmd;
	struct usdhc_data *data = &priv->op_context.data;

	priv->op_context.cmd_only = cmd_only;

	memset((char *)cmd, 0, sizeof(struct usdhc_cmd));
	memset((char *)data, 0, sizeof(struct usdhc_data));

	cmd->index = cmd_idx;
	cmd->argument = arg;
	cmd->rsp_type = rsp_type;
}

static int usdhc_select_fun(struct usdhc_priv *priv,
	u32_t group, u32_t function)
{
	u32_t *fun_status;
	u16_t fun_grp_info[6U] = {0};
	u32_t current_fun_status = 0U, arg;
	struct usdhc_cmd *cmd = &priv->op_context.cmd;
	struct usdhc_data *data = &priv->op_context.data;
	int ret;

	/* check if card support CMD6 */
	if ((priv->card_info.version <= SD_SPEC_VER1_0) ||
		(!(priv->card_info.csd.cmd_class & SD_CMD_CLASS_SWITCH))) {
		return -EINVAL;
	}

	/* Check if card support high speed mode. */
	arg = (SD_SWITCH_CHECK << 31U | 0x00FFFFFFU);
	arg &= ~((u32_t)(0xFU) << (group * 4U));
	arg |= (function << (group * 4U));
	usdhc_op_ctx_init(priv, 0, SDHC_SWITCH, arg, SDHC_RSP_TYPE_R1);

	data->block_size = 64U;
	data->block_count = 1U;
	data->rx_data = &g_usdhc_rx_dummy[0];
	ret = usdhc_xfer(priv);
	if (ret || (cmd->response[0U] & SDHC_R1ERR_All_FLAG))
		return -EIO;

	fun_status = data->rx_data;

	/* Switch function status byte sequence
	 * from card is big endian(MSB first).
	 */
	switch (priv->host_config.endian) {
	case USDHC_LITTLE_ENDIAN:
		fun_status[0U] = SWAP_WORD_BYTE_SEQUENCE(fun_status[0U]);
		fun_status[1U] = SWAP_WORD_BYTE_SEQUENCE(fun_status[1U]);
		fun_status[2U] = SWAP_WORD_BYTE_SEQUENCE(fun_status[2U]);
		fun_status[3U] = SWAP_WORD_BYTE_SEQUENCE(fun_status[3U]);
		fun_status[4U] = SWAP_WORD_BYTE_SEQUENCE(fun_status[4U]);
		break;
	case USDHC_BIG_ENDIAN:
		break;
	case USDHC_HALF_WORD_BIG_ENDIAN:
		fun_status[0U] = SWAP_HALF_WROD_BYTE_SEQUENCE(fun_status[0U]);
		fun_status[1U] = SWAP_HALF_WROD_BYTE_SEQUENCE(fun_status[1U]);
		fun_status[2U] = SWAP_HALF_WROD_BYTE_SEQUENCE(fun_status[2U]);
		fun_status[3U] = SWAP_HALF_WROD_BYTE_SEQUENCE(fun_status[3U]);
		fun_status[4U] = SWAP_HALF_WROD_BYTE_SEQUENCE(fun_status[4U]);
		break;
	default:
		return -ENOTSUP;
	}

	fun_grp_info[5U] = (u16_t)fun_status[0U];
	fun_grp_info[4U] = (u16_t)(fun_status[1U] >> 16U);
	fun_grp_info[3U] = (u16_t)(fun_status[1U]);
	fun_grp_info[2U] = (u16_t)(fun_status[2U] >> 16U);
	fun_grp_info[1U] = (u16_t)(fun_status[2U]);
	fun_grp_info[0U] = (u16_t)(fun_status[3U] >> 16U);
	current_fun_status = ((fun_status[3U] & 0xFFU) << 8U) |
		(fun_status[4U] >> 24U);

	/* check if function is support */
	if (((fun_grp_info[group] & (1 << function)) == 0U) ||
		((current_fun_status >>
		(group * 4U)) & 0xFU) != function) {
		return -ENOTSUP;
	}

	/* Switch to high speed mode. */
	usdhc_op_ctx_init(priv, 0, SDHC_SWITCH, arg, SDHC_RSP_TYPE_R1);

	data->block_size = 64U;
	data->block_count = 1U;
	data->rx_data = &g_usdhc_rx_dummy[0];

	cmd->argument = (SD_SWITCH_SET << 31U | 0x00FFFFFFU);
	cmd->argument &= ~((u32_t)(0xFU) << (group * 4U));
	cmd->argument |= (function << (group * 4U));

	ret = usdhc_xfer(priv);
	if (ret || (cmd->response[0U] & SDHC_R1ERR_All_FLAG))
		return -EIO;
	/* Switch function status byte sequence
	 * from card is big endian(MSB first).
	 */
	switch (priv->host_config.endian) {
	case USDHC_LITTLE_ENDIAN:
		fun_status[3U] = SWAP_WORD_BYTE_SEQUENCE(fun_status[3U]);
		fun_status[4U] = SWAP_WORD_BYTE_SEQUENCE(fun_status[4U]);
		break;
	case USDHC_BIG_ENDIAN:
		break;
	case USDHC_HALF_WORD_BIG_ENDIAN:
		fun_status[3U] = SWAP_HALF_WROD_BYTE_SEQUENCE(fun_status[3U]);
		fun_status[4U] = SWAP_HALF_WROD_BYTE_SEQUENCE(fun_status[4U]);
		break;
	default:
		return -ENOTSUP;
	}
	/* According to the "switch function status[bits 511~0]" return
	 * by switch command in mode "set function":
	 * -check if group 1 is successfully changed to function 1 by checking
	 * if bits 379~376 equal value 1;
	 */
	current_fun_status = ((fun_status[3U] & 0xFFU) << 8U) |
		(fun_status[4U] >> 24U);

	if (((current_fun_status >>
		(group * 4U)) & 0xFU) != function) {
		return -EINVAL;
	}

	return 0;
}

u32_t usdhc_set_sd_clk(USDHC_Type *base, u32_t src_clk_hz, u32_t sd_clk_hz)
{
	u32_t total_div = 0U;
	u32_t divisor = 0U;
	u32_t prescaler = 0U;
	u32_t sysctl = 0U;
	u32_t nearest_freq = 0U;

	assert(src_clk_hz != 0U);
	assert((sd_clk_hz != 0U) && (sd_clk_hz <= src_clk_hz));

	/* calculate total divisor first */
	total_div = src_clk_hz / sd_clk_hz;
	if (total_div > (USDHC_MAX_CLKFS * USDHC_MAX_DVS)) {
		return 0U;
	}

	if (total_div) {
		/* calculate the divisor (src_clk_hz / divisor) <= sd_clk_hz */
		if ((src_clk_hz / total_div) > sd_clk_hz)
			total_div++;

		/* divide the total divisor to div and prescaler */
		if (total_div > USDHC_MAX_DVS) {
			prescaler = total_div / USDHC_MAX_DVS;
			/* prescaler must be a value which equal 2^n and
			 * smaller than SDHC_MAX_CLKFS
			 */
			while (((USDHC_MAX_CLKFS % prescaler) != 0U) ||
				(prescaler == 1U))
				prescaler++;
			/* calculate the divisor */
			divisor = total_div / prescaler;
			/* fine tuning the divisor until
			 * divisor * prescaler >= total_div
			 */
			while ((divisor * prescaler) < total_div) {
				divisor++;
				if (divisor > USDHC_MAX_DVS) {
					if ((prescaler <<= 1U) >
						USDHC_MAX_CLKFS) {
						return 0;
					}
				divisor = total_div / prescaler;
				}
			}
		} else {
			/* in this situation , divsior and SDCLKFS
			 * can generate same clock
			 * use SDCLKFS
			 */
			if (((total_div % 2U) != 0U) & (total_div != 1U)) {
				divisor = total_div;
				prescaler = 1U;
			} else {
				divisor = 1U;
				prescaler = total_div;
			}
		}
		nearest_freq = src_clk_hz / (divisor == 0U ? 1U : divisor) /
			prescaler;
	} else {
		/* in this condition , src_clk_hz = busClock_Hz, */
		/* in DDR mode , set SDCLKFS to 0, divisor = 0, actually the
		 * totoal divider = 2U
		 */
		divisor = 0U;
		prescaler = 0U;
		nearest_freq = src_clk_hz;
	}

	/* calculate the value write to register */
	if (divisor != 0U) {
		USDHC_PREV_DVS(divisor);
	}
	/* calculate the value write to register */
	if (prescaler != 0U) {
		USDHC_PREV_CLKFS(prescaler, 1U);
	}

	/* Set the SD clock frequency divisor, SD clock frequency select,
	 * data timeout counter value.
	 */
	sysctl = base->SYS_CTRL;
	sysctl &= ~(USDHC_SYS_CTRL_DVS_MASK |
		USDHC_SYS_CTRL_SDCLKFS_MASK);
	sysctl |= (USDHC_SYS_CTRL_DVS(divisor) |
		USDHC_SYS_CTRL_SDCLKFS(prescaler));
	base->SYS_CTRL = sysctl;

	/* Wait until the SD clock is stable. */
	while (!(base->PRES_STATE & USDHC_PRES_STATE_SDSTB_MASK)) {
		;
	}

	return nearest_freq;
}

static void usdhc_enable_ddr_mode(USDHC_Type *base,
	bool enable, u32_t nibble_pos)
{
	u32_t prescaler = (base->SYS_CTRL & USDHC_SYS_CTRL_SDCLKFS_MASK) >>
		USDHC_SYS_CTRL_SDCLKFS_SHIFT;

	if (enable) {
		base->MIX_CTRL &= ~USDHC_MIX_CTRL_NIBBLE_POS_MASK;
		base->MIX_CTRL |= (USDHC_MIX_CTRL_DDR_EN_MASK |
			USDHC_MIX_CTRL_NIBBLE_POS(nibble_pos));
		prescaler >>= 1U;
	} else {
		base->MIX_CTRL &= ~USDHC_MIX_CTRL_DDR_EN_MASK;

		if (prescaler == 0U) {
			prescaler += 1U;
		} else {
			prescaler <<= 1U;
		}
	}

	base->SYS_CTRL = (base->SYS_CTRL & (~USDHC_SYS_CTRL_SDCLKFS_MASK)) |
		USDHC_SYS_CTRL_SDCLKFS(prescaler);
}

static int usdhc_select_bus_timing(struct usdhc_priv *priv)
{
	int error = -EIO;

	if (priv->card_info.voltage != SD_VOL_1_8_V) {
		/* Switch the card to high speed mode */
		if (priv->host_capability.host_flags &
			USDHC_SUPPORT_HIGHSPEED_FLAG) {
			/* group 1, function 1 ->high speed mode*/
			error = usdhc_select_fun(priv, SD_GRP_TIMING_MODE,
				SD_TIMING_SDR25_HIGH_SPEED_MODE);
			/* If the result isn't "switching to
			 * high speed mode(50MHZ)
			 * successfully or card doesn't support
			 * high speed
			 * mode". Return failed status.
			 */
			if (!error) {
				priv->card_info.sd_timing =
					SD_TIMING_SDR25_HIGH_SPEED_MODE;
				priv->card_info.busclk_hz =
					usdhc_set_sd_clk(priv->host_config.base,
						priv->host_config.src_clk_hz,
						SD_CLOCK_50MHZ);
			} else if (error == -ENOTSUP) {
				/* if not support high speed,
				 * keep the card work at default mode
				 */
				return 0;
			}
		} else {
			/* if not support high speed,
			 * keep the card work at default mode
			 */
			return 0;
		}
	} else if ((USDHC_SUPPORT_SDR104_FLAG !=
		SDMMCHOST_NOT_SUPPORT) ||
		(USDHC_SUPPORT_SDR50_FLAG != SDMMCHOST_NOT_SUPPORT) ||
		(USDHC_SUPPORT_DDR50_FLAG != SDMMCHOST_NOT_SUPPORT)) {
		/* card is in UHS_I mode */
		switch (priv->card_info.sd_timing) {
			/* if not select timing mode,
			 * sdmmc will handle it automatically
			 */
		case SD_TIMING_SDR12_DFT_MODE:
		case SD_TIMING_SDR104_MODE:
			error = usdhc_select_fun(priv, SD_GRP_TIMING_MODE,
				SD_TIMING_SDR104_MODE);
			if (!error) {
				priv->card_info.sd_timing =
					SD_TIMING_SDR104_MODE;
				priv->card_info.busclk_hz =
					usdhc_set_sd_clk(priv->host_config.base,
						priv->host_config.src_clk_hz,
						SDMMCHOST_SUPPORT_SDR104_FREQ);
				break;
			}
		case SD_TIMING_DDR50_MODE:
			error = usdhc_select_fun(priv, SD_GRP_TIMING_MODE,
				SD_TIMING_DDR50_MODE);
			if (!error) {
				priv->card_info.sd_timing =
					SD_TIMING_DDR50_MODE;
				priv->card_info.busclk_hz =
					usdhc_set_sd_clk(
						priv->host_config.base,
						priv->host_config.src_clk_hz,
						SD_CLOCK_50MHZ);
				usdhc_enable_ddr_mode(
					priv->host_config.base, true, 0U);
			}
			break;
		case SD_TIMING_SDR50_MODE:
			error = usdhc_select_fun(priv,
				SD_GRP_TIMING_MODE,
				SD_TIMING_SDR50_MODE);
			if (!error) {
				priv->card_info.sd_timing =
					SD_TIMING_SDR50_MODE;
				priv->card_info.busclk_hz =
					usdhc_set_sd_clk(
						priv->host_config.base,
						priv->host_config.src_clk_hz,
						SD_CLOCK_100MHZ);
			}
			break;
		case SD_TIMING_SDR25_HIGH_SPEED_MODE:
			error = usdhc_select_fun(priv, SD_GRP_TIMING_MODE,
				SD_TIMING_SDR25_HIGH_SPEED_MODE);
			if (!error) {
				priv->card_info.sd_timing =
					SD_TIMING_SDR25_HIGH_SPEED_MODE;
				priv->card_info.busclk_hz =
					usdhc_set_sd_clk(
						priv->host_config.base,
						priv->host_config.src_clk_hz,
						SD_CLOCK_50MHZ);
			}
			break;

		default:
			break;
		}
	}

	/* SDR50 and SDR104 mode need tuning */
	if ((priv->card_info.sd_timing == SD_TIMING_SDR50_MODE) ||
		(priv->card_info.sd_timing == SD_TIMING_SDR104_MODE)) {
		struct usdhc_cmd *cmd = &priv->op_context.cmd;
		struct usdhc_data *data = &priv->op_context.data;

		/* config IO strength in IOMUX*/
		if (priv->card_info.sd_timing == SD_TIMING_SDR50_MODE) {
			imxrt_usdhc_pinmux(priv->nusdhc, false,
				CARD_BUS_FREQ_100MHZ1,
				CARD_BUS_STRENGTH_7);
		} else {
			imxrt_usdhc_pinmux(priv->nusdhc, false,
				CARD_BUS_FREQ_200MHZ,
				CARD_BUS_STRENGTH_7);
		}
		/* execute tuning */
		priv->op_context.cmd_only = 0;

		memset((char *)cmd, 0, sizeof(struct usdhc_cmd));
		memset((char *)data, 0, sizeof(struct usdhc_data));

		cmd->index = SDHC_SEND_TUNING_BLOCK;
		cmd->rsp_type = SDHC_RSP_TYPE_R1;

		data->block_size = 64;
		data->block_count = 1U;
		data->rx_data = &g_usdhc_rx_dummy[0];
		data->data_type = USDHC_XFER_TUNING;
		error = usdhc_execute_tuning(priv);
		if (error)
			return error;
	} else {
		/* set default IO strength to 4 to cover card adapter driver
		 * strength difference
		 */
		imxrt_usdhc_pinmux(priv->nusdhc, false,
			CARD_BUS_FREQ_100MHZ1,
			CARD_BUS_STRENGTH_4);
	}

	return error;
}

static int usdhc_write_sector(void *bus_data, const u8_t *buf, u32_t sector,
		     u32_t count)
{
	struct usdhc_priv *priv = bus_data;
	struct usdhc_cmd *cmd = &priv->op_context.cmd;
	struct usdhc_data *data = &priv->op_context.data;

	memset((char *)cmd, 0, sizeof(struct usdhc_cmd));
	memset((char *)data, 0, sizeof(struct usdhc_data));

	priv->op_context.cmd_only = 0;
	cmd->index = SDHC_WRITE_MULTIPLE_BLOCK;
	data->block_size = priv->card_info.sd_block_size;
	data->block_count = count;
	data->tx_data = (const u32_t *)buf;
	data->cmd12 = true;
	if (data->block_count == 1U) {
		cmd->index = SDHC_WRITE_BLOCK;
	}
	cmd->argument = sector;
	if (!(priv->card_info.card_flags & SDHC_HIGH_CAPACITY_FLAG)) {
		cmd->argument *= priv->card_info.sd_block_size;
	}
	cmd->rsp_type = SDHC_RSP_TYPE_R1;
	cmd->rsp_err_flags = SDHC_R1ERR_All_FLAG;

	return usdhc_xfer(priv);
}

static int usdhc_read_sector(void *bus_data, u8_t *buf, u32_t sector,
		     u32_t count)
{
	struct usdhc_priv *priv = bus_data;
	struct usdhc_cmd *cmd = &priv->op_context.cmd;
	struct usdhc_data *data = &priv->op_context.data;

	memset((char *)cmd, 0, sizeof(struct usdhc_cmd));
	memset((char *)data, 0, sizeof(struct usdhc_data));

	priv->op_context.cmd_only = 0;
	cmd->index = SDHC_READ_MULTIPLE_BLOCK;
	data->block_size = priv->card_info.sd_block_size;
	data->block_count = count;
	data->rx_data = (u32_t *)buf;
	data->cmd12 = true;

	if (data->block_count == 1U) {
		cmd->index = SDHC_READ_SINGLE_BLOCK;
	}

	cmd->argument = sector;
	if (!(priv->card_info.card_flags & SDHC_HIGH_CAPACITY_FLAG)) {
		cmd->argument *= priv->card_info.sd_block_size;
	}

	cmd->rsp_type = SDHC_RSP_TYPE_R1;
	cmd->rsp_err_flags = SDHC_R1ERR_All_FLAG;

	return usdhc_xfer(priv);
}

static bool usdhc_set_sd_active(USDHC_Type *base)
{
	u32_t timeout = 0xffff;

	base->SYS_CTRL |= USDHC_SYS_CTRL_INITA_MASK;
	/* Delay some time to wait card become active state. */
	while ((base->SYS_CTRL & USDHC_SYS_CTRL_INITA_MASK) ==
		USDHC_SYS_CTRL_INITA_MASK) {
		if (!timeout) {
			break;
		}
		timeout--;
	}

	return ((!timeout) ? false : true);
}

static void usdhc_get_host_capability(USDHC_Type *base,
	struct usdhc_capability *capability)
{
	u32_t host_cap;
	u32_t max_blk_len;

	host_cap = base->HOST_CTRL_CAP;

	/* Get the capability of USDHC. */
	max_blk_len = ((host_cap & USDHC_HOST_CTRL_CAP_MBL_MASK) >>
		USDHC_HOST_CTRL_CAP_MBL_SHIFT);
	capability->max_blk_len = (512U << max_blk_len);
	/* Other attributes not in HTCAPBLT register. */
	capability->max_blk_cnt = USDHC_MAX_BLOCK_COUNT;
	capability->host_flags = (host_cap & (USDHC_SUPPORT_ADMA_FLAG |
		USDHC_SUPPORT_HIGHSPEED_FLAG | USDHC_SUPPORT_DMA_FLAG |
		USDHC_SUPPORT_SUSPEND_RESUME_FLAG | USDHC_SUPPORT_V330_FLAG));
	capability->host_flags |= (host_cap & USDHC_SUPPORT_V300_FLAG);
	capability->host_flags |= (host_cap & USDHC_SUPPORT_V180_FLAG);
	capability->host_flags |=
		(host_cap & (USDHC_SUPPORT_DDR50_FLAG |
			USDHC_SUPPORT_SDR104_FLAG |
			USDHC_SUPPORT_SDR50_FLAG));
	/* USDHC support 4/8 bit data bus width. */
	capability->host_flags |= (USDHC_SUPPORT_4BIT_FLAG |
		USDHC_SUPPORT_8BIT_FLAG);
}

static bool usdhc_hw_reset(USDHC_Type *base, u32_t mask, u32_t timeout)
{
	base->SYS_CTRL |= (mask & (USDHC_SYS_CTRL_RSTA_MASK |
		USDHC_SYS_CTRL_RSTC_MASK | USDHC_SYS_CTRL_RSTD_MASK));
	/* Delay some time to wait reset success. */
	while ((base->SYS_CTRL & mask)) {
		if (!timeout) {
			break;
		}
		timeout--;
	}

	return ((!timeout) ? false : true);
}

static void usdhc_host_hw_init(USDHC_Type *base,
	const struct usdhc_config *config)
{
	u32_t proctl, sysctl, wml;
	u32_t int_mask;

	assert(config);
	assert((config->write_watermark >= 1U) &&
		(config->write_watermark <= 128U));
	assert((config->read_watermark >= 1U) &&
		(config->read_watermark <= 128U));
	assert(config->write_burst_len <= 16U);

	/* Reset USDHC. */
	usdhc_hw_reset(base, USDHC_RESET_ALL, 100U);

	proctl = base->PROT_CTRL;
	wml = base->WTMK_LVL;
	sysctl = base->SYS_CTRL;

	proctl &= ~(USDHC_PROT_CTRL_EMODE_MASK | USDHC_PROT_CTRL_DMASEL_MASK);
	/* Endian mode*/
	proctl |= USDHC_PROT_CTRL_EMODE(config->endian);

	/* Watermark level */
	wml &= ~(USDHC_WTMK_LVL_RD_WML_MASK |
			USDHC_WTMK_LVL_WR_WML_MASK |
			USDHC_WTMK_LVL_RD_BRST_LEN_MASK |
			USDHC_WTMK_LVL_WR_BRST_LEN_MASK);
	wml |= (USDHC_WTMK_LVL_RD_WML(config->read_watermark) |
			USDHC_WTMK_LVL_WR_WML(config->write_watermark) |
			USDHC_WTMK_LVL_RD_BRST_LEN(config->read_burst_len) |
			USDHC_WTMK_LVL_WR_BRST_LEN(config->write_burst_len));

	/* config the data timeout value */
	sysctl &= ~USDHC_SYS_CTRL_DTOCV_MASK;
	sysctl |= USDHC_SYS_CTRL_DTOCV(config->data_timeout);

	base->SYS_CTRL = sysctl;
	base->WTMK_LVL = wml;
	base->PROT_CTRL = proctl;

	/* disable internal DMA and DDR mode */
	base->MIX_CTRL &= ~(USDHC_MIX_CTRL_DMAEN_MASK |
		USDHC_MIX_CTRL_DDR_EN_MASK);

	int_mask = (USDHC_INT_CMD_FLAG | USDHC_INT_CARD_DETECT_FLAG |
		USDHC_INT_DATA_FLAG | USDHC_INT_SDR104_TUNING_FLAG |
		USDHC_INT_BLK_GAP_EVENT_FLAG);

	base->INT_STATUS_EN |= int_mask;

}

static void usdhc_cd_gpio_cb(struct device *dev,
				  struct gpio_callback *cb, u32_t pins)
{
	struct usdhc_board_config *board_cfg =
		CONTAINER_OF(cb, struct usdhc_board_config, detect_cb);

	gpio_pin_disable_callback(dev, board_cfg->detect_pin);

}

static int usdhc_cd_gpio_init(struct device *detect_gpio,
	u32_t pin, struct gpio_callback *callback)
{
	int ret;

	ret = gpio_pin_configure(detect_gpio, pin,
		GPIO_DIR_IN | GPIO_INT_DOUBLE_EDGE);
	if (ret)
		return ret;

	gpio_init_callback(callback, usdhc_cd_gpio_cb, BIT(pin));

	return gpio_add_callback(detect_gpio, callback);
}

static void usdhc_host_reset(struct usdhc_priv *priv)
{
	USDHC_Type *base = priv->host_config.base;

	usdhc_select_1_8_vol(base, false);
	usdhc_enable_ddr_mode(base, false, 0);
	usdhc_tuning(base, SDHC_STANDARD_TUNING_START, SDHC_TUINIG_STEP, false);
#if FSL_FEATURE_USDHC_HAS_HS400_MODE
#error Not implemented!
	/* Disable HS400 mode */
	/* Disable DLL */
#endif
}

static int usdhc_app_host_cmd(struct usdhc_priv *priv, int retry,
	u32_t arg, u8_t app_cmd, u32_t app_arg, enum sdhc_rsp_type rsp_type,
	enum sdhc_rsp_type app_rsp_type, bool app_cmd_only)
{
	struct usdhc_cmd *cmd = &priv->op_context.cmd;
	int ret;

APP_CMD_XFER_AGAIN:
	priv->op_context.cmd_only = 1;
	cmd->index = SDHC_APP_CMD;
	cmd->argument = arg;
	cmd->rsp_type = rsp_type;
	ret = usdhc_xfer(priv);
	retry--;
	if (ret && retry > 0) {
		goto APP_CMD_XFER_AGAIN;
	}

	priv->op_context.cmd_only = app_cmd_only;
	cmd->index = app_cmd;
	cmd->argument = app_arg;
	cmd->rsp_type = app_rsp_type;
	ret = usdhc_xfer(priv);
	if (ret && retry > 0) {
		goto APP_CMD_XFER_AGAIN;
	}

	return ret;
}

static int usdhc_sd_init(struct usdhc_priv *priv)
{
	USDHC_Type *base = priv->host_config.base;
	u32_t app_cmd_41_arg = 0U;
	int ret, retry;
	struct usdhc_cmd *cmd = &priv->op_context.cmd;
	struct usdhc_data *data = &priv->op_context.data;

	if (!priv->host_ready) {
		return -ENODEV;
	}

	/* reset variables */
	priv->card_info.card_flags = 0U;
	/* set DATA bus width 1bit at beginning*/
	usdhc_set_bus_width(base, USDHC_DATA_BUS_WIDTH_1BIT);
	/*set card freq to 400KHZ at begging*/
	priv->card_info.busclk_hz =
		usdhc_set_sd_clk(base, priv->host_config.src_clk_hz,
			SDMMC_CLOCK_400KHZ);
	/* send card active */
	ret = usdhc_set_sd_active(base);
	if (ret == false) {
		return -EIO;
	}

	/* Get host capability. */
	usdhc_get_host_capability(base, &priv->host_capability);

	/* card go idle */
	usdhc_op_ctx_init(priv, 1, SDHC_GO_IDLE_STATE, 0, SDHC_RSP_TYPE_NONE);

	ret = usdhc_xfer(priv);
	if (ret) {
		return ret;
	}

	if (USDHC_SUPPORT_V330_FLAG != SDMMCHOST_NOT_SUPPORT) {
		app_cmd_41_arg |= (SD_OCR_VDD32_33FLAG | SD_OCR_VDD33_34FLAG);
		priv->card_info.voltage = SD_VOL_3_3_V;
	} else if (USDHC_SUPPORT_V300_FLAG != SDMMCHOST_NOT_SUPPORT) {
		app_cmd_41_arg |= SD_OCR_VDD29_30FLAG;
		priv->card_info.voltage = SD_VOL_3_3_V;
	}

	/* allow user select the work voltage, if not select,
	 * sdmmc will handle it automatically
	 */
	if (USDHC_SUPPORT_V180_FLAG != SDMMCHOST_NOT_SUPPORT) {
		app_cmd_41_arg |= SD_OCR_SWITCH_18_REQ_FLAG;
	}

	/* Check card's supported interface condition. */
	usdhc_op_ctx_init(priv, 1, SDHC_SEND_IF_COND,
		SDHC_VHS_3V3 | SDHC_CHECK, SDHC_RSP_TYPE_R7);

	retry = 10;
	while (retry) {
		ret = usdhc_xfer(priv);
		if (!ret) {
			if ((cmd->response[0U] & 0xFFU) != SDHC_CHECK) {
				ret = -ENOTSUP;
			} else {
				break;
			}
		}
		retry--;
	}

	if (!ret) {
		/* SDHC or SDXC card */
		app_cmd_41_arg |= SD_OCR_HOST_CAP_FLAG;
		priv->card_info.card_flags |= USDHC_SDHC_FLAG;
	} else {
		/* SDSC card */
		LOG_ERR("USDHC SDSC not implemented yet!\r\n");
		return -ENOTSUP;
	}

	/* Set card interface condition according to SDHC capability and
	 * card's supported interface condition.
	 */
APP_SEND_OP_COND_AGAIN:
	usdhc_op_ctx_init(priv, 1, 0, 0, SDHC_RSP_TYPE_NONE);
	ret = usdhc_app_host_cmd(priv, NXP_SDMMC_MAX_VOLTAGE_RETRIES, 0,
		SDHC_APP_SEND_OP_COND, app_cmd_41_arg,
		SDHC_RSP_TYPE_R1, SDHC_RSP_TYPE_R3, 1);
	if (ret) {
		LOG_ERR("APP Condition CMD failed:%d\r\n", ret);
		return ret;
	}
	if (cmd->response[0U] & SD_OCR_PWR_BUSY_FLAG) {
		/* high capacity check */
		if (cmd->response[0U] & SD_OCR_CARD_CAP_FLAG) {
			priv->card_info.card_flags |= SDHC_HIGH_CAPACITY_FLAG;
		}
		/* 1.8V support */
		if (cmd->response[0U] & SD_OCR_SWITCH_18_ACCEPT_FLAG) {
			priv->card_info.card_flags |= SDHC_1800MV_FLAG;
		}
		priv->card_info.raw_ocr = cmd->response[0U];
	} else {
		goto APP_SEND_OP_COND_AGAIN;
	}

	/* check if card support 1.8V */
	if ((priv->card_info.card_flags & USDHC_VOL_1_8V_FLAG)) {
		usdhc_op_ctx_init(priv, 1, SDHC_VOL_SWITCH,
			0, SDHC_RSP_TYPE_R1);

		ret = usdhc_xfer(priv);
		if (!ret) {
			ret = usdhc_vol_switch(priv);
		}
		if (ret) {
			LOG_ERR("Voltage switch failed: %d\r\n", ret);
			return ret;
		}
		priv->card_info.voltage = SD_VOL_1_8_V;
	}

	/* Initialize card if the card is SD card. */
	usdhc_op_ctx_init(priv, 1, SDHC_ALL_SEND_CID, 0, SDHC_RSP_TYPE_R2);

	ret = usdhc_xfer(priv);
	if (!ret) {
		memcpy(priv->card_info.raw_cid, cmd->response,
			sizeof(priv->card_info.raw_cid));
		sdhc_decode_cid(&priv->card_info.cid,
			priv->card_info.raw_cid);
	} else {
		LOG_ERR("All send CID CMD failed: %d\r\n", ret);
		return ret;
	}

	usdhc_op_ctx_init(priv, 1, SDHC_SEND_RELATIVE_ADDR,
		0, SDHC_RSP_TYPE_R6);

	ret = usdhc_xfer(priv);
	if (!ret) {
		priv->card_info.relative_addr = (cmd->response[0U] >> 16U);
	} else {
		LOG_ERR("Send relative address CMD failed: %d\r\n", ret);
		return ret;
	}

	usdhc_op_ctx_init(priv, 1, SDHC_SEND_CSD,
		(priv->card_info.relative_addr << 16U), SDHC_RSP_TYPE_R2);

	ret = usdhc_xfer(priv);
	if (!ret) {
		memcpy(priv->card_info.raw_csd, cmd->response,
			sizeof(priv->card_info.raw_csd));
		sdhc_decode_csd(&priv->card_info.csd, priv->card_info.raw_csd,
			&priv->card_info.sd_block_count,
			&priv->card_info.sd_block_size);
	} else {
		LOG_ERR("Send CSD CMD failed: %d\r\n", ret);
		return ret;
	}

	usdhc_op_ctx_init(priv, 1, SDHC_SELECT_CARD,
		priv->card_info.relative_addr << 16U,
		SDHC_RSP_TYPE_R1);

	ret = usdhc_xfer(priv);
	if (ret || (cmd->response[0U] & SDHC_R1ERR_All_FLAG)) {
		LOG_ERR("Select card CMD failed: %d\r\n", ret);
		return -EIO;
	}

	usdhc_op_ctx_init(priv, 0, 0, 0, SDHC_RSP_TYPE_NONE);
	data->block_size = 8;
	data->block_count = 1;
	data->rx_data = &priv->card_info.raw_scr[0];
	ret = usdhc_app_host_cmd(priv, 1, (priv->card_info.relative_addr << 16),
		SDHC_APP_SEND_SCR, 0,
		SDHC_RSP_TYPE_R1, SDHC_RSP_TYPE_R1, 0);

	if (ret) {
		LOG_ERR("Send SCR following APP CMD failed: %d\r\n", ret);
		return ret;
	}

	switch (priv->host_config.endian) {
	case USDHC_LITTLE_ENDIAN:
		priv->card_info.raw_scr[0] =
			SWAP_WORD_BYTE_SEQUENCE(priv->card_info.raw_scr[0]);
		priv->card_info.raw_scr[1] =
			SWAP_WORD_BYTE_SEQUENCE(priv->card_info.raw_scr[1]);
	break;
	case USDHC_BIG_ENDIAN:
	break;
	case USDHC_HALF_WORD_BIG_ENDIAN:
		priv->card_info.raw_scr[0U] =
			SWAP_HALF_WROD_BYTE_SEQUENCE(
				priv->card_info.raw_scr[0U]);
		priv->card_info.raw_scr[1U] =
			SWAP_HALF_WROD_BYTE_SEQUENCE(
				priv->card_info.raw_scr[1U]);
	break;
	default:
		return -EINVAL;
	}

	sdhc_decode_scr(&priv->card_info.scr, priv->card_info.raw_scr,
		&priv->card_info.version);
	if (priv->card_info.scr.sd_width & 0x4U) {
		priv->card_info.card_flags |=
			USDHC_4BIT_WIDTH_FLAG;
	}
	/* speed class control cmd */
	if (priv->card_info.scr.cmd_support & 0x01U) {
		priv->card_info.card_flags |=
			USDHC_SPEED_CLASS_CONTROL_CMD_FLAG;
	}
	/* set block count cmd */
	if (priv->card_info.scr.cmd_support & 0x02U) {
		priv->card_info.card_flags |=
			USDHC_SET_BLK_CNT_CMD23_FLAG;
	}

	/* Set to max frequency in non-high speed mode. */
	priv->card_info.busclk_hz = usdhc_set_sd_clk(base,
		priv->host_config.src_clk_hz, SD_CLOCK_25MHZ);

	/* Set to 4-bit data bus mode. */
	if ((priv->host_capability.host_flags & USDHC_SUPPORT_4BIT_FLAG) &&
		(priv->card_info.card_flags & USDHC_4BIT_WIDTH_FLAG)) {
		usdhc_op_ctx_init(priv, 1, 0, 0, SDHC_RSP_TYPE_NONE);

		ret = usdhc_app_host_cmd(priv, 1,
				(priv->card_info.relative_addr << 16),
				SDHC_APP_SET_BUS_WIDTH, 2,
				SDHC_RSP_TYPE_R1, SDHC_RSP_TYPE_R1, 1);

		if (ret) {
			LOG_ERR("Set bus width failed: %d\r\n", ret);
			return ret;
		}
		usdhc_set_bus_width(base, USDHC_DATA_BUS_WIDTH_4BIT);
	}

	/* set sd card driver strength */
	ret = usdhc_select_fun(priv, SD_GRP_DRIVER_STRENGTH_MODE,
		priv->card_info.driver_strength);
	if (ret) {
		LOG_ERR("Set SD driver strehgth failed: %d\r\n", ret);
		return ret;
	}

	/* set sd card current limit */
	ret = usdhc_select_fun(priv, SD_GRP_CURRENT_LIMIT_MODE,
		priv->card_info.max_current);
	if (ret) {
		LOG_ERR("Set SD current limit failed: %d\r\n", ret);
		return ret;
	}

	/* set block size */
	usdhc_op_ctx_init(priv, 1, SDHC_SET_BLOCK_SIZE,
		priv->card_info.sd_block_size, SDHC_RSP_TYPE_R1);

	ret = usdhc_xfer(priv);
	if (ret || cmd->response[0U] & SDHC_R1ERR_All_FLAG) {
		LOG_ERR("Set block size failed: %d\r\n", ret);
		return -EIO;
	}

	/* select bus timing */
	ret = usdhc_select_bus_timing(priv);
	if (ret) {
		LOG_ERR("Select bus timing failed: %d\r\n", ret);
		return ret;
	}

	retry = 10;
	ret = -EIO;
	while (ret && retry >= 0) {
		ret = usdhc_read_sector(priv, (u8_t *)g_usdhc_rx_dummy, 0, 1);
		if (!ret) {
			break;
		}
		retry--;
	}

	if (ret) {
		LOG_ERR("USDHC bus device initalization failed!\r\n");
	}

	return ret;
}

struct usdhc_priv g_usdhc_priv1 __aligned(64);
struct usdhc_priv g_usdhc_priv2 __aligned(64);


static K_MUTEX_DEFINE(z_usdhc_init_lock);

static int usdhc_board_access_init(struct usdhc_priv *priv)
{
	int ret;
	u32_t gpio_level;

	if (priv->nusdhc == 0) {
#ifdef DT_INST_0_NXP_IMX_USDHC_PWR_GPIOS_CONTROLLER
		priv->board_cfg.pwr_gpio =
			device_get_binding(
				DT_INST_0_NXP_IMX_USDHC_PWR_GPIOS_CONTROLLER);
		if (!priv->board_cfg.pwr_gpio) {
			return -ENODEV;
		}
		priv->board_cfg.pwr_pin =
			DT_INST_0_NXP_IMX_USDHC_PWR_GPIOS_PIN;
		priv->board_cfg.pwr_flags =
			DT_INST_0_NXP_IMX_USDHC_PWR_GPIOS_FLAGS;
#endif
#ifdef DT_INST_0_NXP_IMX_USDHC_CD_GPIOS_CONTROLLER
		priv->detect_type = SD_DETECT_GPIO_CD;
		priv->board_cfg.detect_gpio =
			device_get_binding(
				DT_INST_0_NXP_IMX_USDHC_CD_GPIOS_CONTROLLER);
		if (!priv->board_cfg.detect_gpio) {
			return -ENODEV;
		}
		priv->board_cfg.detect_pin =
			DT_INST_0_NXP_IMX_USDHC_CD_GPIOS_PIN;
#endif

	} else if (priv->nusdhc == 1) {
#ifdef DT_INST_1_NXP_IMX_USDHC_PWR_GPIOS_CONTROLLER
		priv->board_cfg.pwr_gpio =
			device_get_binding(
				DT_INST_1_NXP_IMX_USDHC_PWR_GPIOS_CONTROLLER);
		if (!priv->board_cfg.pwr_gpio) {
			return -ENODEV;
		}
		priv->board_cfg.pwr_pin =
			DT_INST_1_NXP_IMX_USDHC_PWR_GPIOS_PIN;
		priv->board_cfg.pwr_flags =
			DT_INST_1_NXP_IMX_USDHC_PWR_GPIOS_FLAGS;
#endif
#ifdef DT_INST_1_NXP_IMX_USDHC_CD_GPIOS_CONTROLLER
		priv->detect_type = SD_DETECT_GPIO_CD;
		priv->board_cfg.detect_gpio =
			device_get_binding(
				DT_INST_1_NXP_IMX_USDHC_CD_GPIOS_CONTROLLER);
		if (!priv->board_cfg.detect_gpio) {
			return -ENODEV;
		}
		priv->board_cfg.detect_pin =
			DT_INST_1_NXP_IMX_USDHC_CD_GPIOS_PIN;
#endif
	} else {
		return -ENODEV;
	}

	if (priv->board_cfg.pwr_gpio) {
		ret = gpio_pin_configure(priv->board_cfg.pwr_gpio,
				priv->board_cfg.pwr_pin,
				priv->board_cfg.pwr_flags);
		if (ret) {
			return ret;
		}

		/* 100ms delay to make sure SD card is stable,
		 * maybe could be shorter
		 */
		k_busy_wait(100000);
		if (priv->board_cfg.pwr_flags & (GPIO_DIR_OUT)) {
			ret = gpio_pin_write(priv->board_cfg.pwr_gpio,
				priv->board_cfg.pwr_pin, 1);
			if (ret) {
				return ret;
			}
		}
	}

	if (!priv->board_cfg.detect_gpio) {
		LOG_INF("USDHC detection other than GPIO not implemented!\r\n");
		return 0;
	}

	ret = usdhc_cd_gpio_init(priv->board_cfg.detect_gpio,
			priv->board_cfg.detect_pin,
			&priv->board_cfg.detect_cb);
	if (ret) {
		return ret;
	}
	ret = gpio_pin_read(priv->board_cfg.detect_gpio,
			priv->board_cfg.detect_pin,
			&gpio_level);
	if (ret) {
		return ret;
	}

	if (gpio_level != HOST_CARD_INSERT_CD_LEVEL) {
		priv->inserted = false;
		LOG_ERR("NO SD inserted!\r\n");

		return -ENODEV;
	}

	priv->inserted = true;
	LOG_INF("SD inserted!\r\n");

	return 0;
}

static int usdhc_access_init(const struct device *dev)
{
	struct usdhc_priv *priv = dev->driver_data;
	int ret;

	(void)k_mutex_lock(&z_usdhc_init_lock, K_FOREVER);

	memset((char *)priv, 0, sizeof(struct usdhc_priv));
#ifdef DT_INST_0_NXP_IMX_USDHC_LABEL
	if (!strcmp(dev->config->name, DT_INST_0_NXP_IMX_USDHC_LABEL)) {
		priv->host_config.base =
			(USDHC_Type *)DT_INST_0_NXP_IMX_USDHC_BASE_ADDRESS;
		priv->nusdhc = 0;
		priv->clock_dev = device_get_binding(
			DT_INST_0_NXP_IMX_USDHC_CLOCK_CONTROLLER);
		if (priv->clock_dev == NULL) {
			return -EINVAL;
		}
		priv->clock_sys =
			(clock_control_subsys_t)
			DT_INST_0_NXP_IMX_USDHC_CLOCK_NAME;
	}
#endif

#ifdef DT_INST_1_NXP_IMX_USDHC_LABEL
	if (!strcmp(dev->config->name, DT_INST_1_NXP_IMX_USDHC_LABEL)) {
		priv->host_config.base =
			(USDHC_Type *)DT_INST_1_NXP_IMX_USDHC_BASE_ADDRESS;
		priv->nusdhc = 1;
		priv->clock_dev = device_get_binding(
			DT_INST_1_NXP_IMX_USDHC_CLOCK_CONTROLLER);
		if (priv->clock_dev == NULL) {
			return -EINVAL;
		}
		priv->clock_sys =
			(clock_control_subsys_t)
			DT_INST_1_NXP_IMX_USDHC_CLOCK_NAME;
	}
#endif

	if (!priv->host_config.base) {
		k_mutex_unlock(&z_usdhc_init_lock);

		return -ENODEV;
	}

	if (clock_control_get_rate(priv->clock_dev, priv->clock_sys,
				   &priv->host_config.src_clk_hz)) {
		return -EINVAL;
	}

	ret = usdhc_board_access_init(priv);
	if (ret) {
		k_mutex_unlock(&z_usdhc_init_lock);

		return ret;
	}

	priv->host_config.data_timeout = USDHC_DATA_TIMEOUT;
	priv->host_config.endian = USDHC_LITTLE_ENDIAN;
	priv->host_config.read_watermark = USDHC_READ_WATERMARK_LEVEL;
	priv->host_config.write_watermark = USDHC_WRITE_WATERMARK_LEVEL;
	priv->host_config.read_burst_len = USDHC_READ_BURST_LEN;
	priv->host_config.write_burst_len = USDHC_WRITE_BURST_LEN;

	priv->op_context.dma_cfg.dma_mode = USDHC_DMA_ADMA2;
	priv->op_context.dma_cfg.burst_len = USDHC_INCR_BURST_LEN;
	/*No DMA used for this Version*/
	priv->op_context.dma_cfg.adma_table = 0;
	priv->op_context.dma_cfg.adma_table_words = USDHC_ADMA_TABLE_WORDS;
	usdhc_host_hw_init(priv->host_config.base, &priv->host_config);
	priv->host_ready = 1;

	usdhc_host_reset(priv);
	ret = usdhc_sd_init(priv);
	k_mutex_unlock(&z_usdhc_init_lock);

	return ret;
}

static int disk_usdhc_access_status(struct disk_info *disk)
{
	struct device *dev = disk->dev;
	struct usdhc_priv *priv = dev->driver_data;

	return priv->status;
}

static int disk_usdhc_access_read(struct disk_info *disk, u8_t *buf,
				 u32_t sector, u32_t count)
{
	struct device *dev = disk->dev;
	struct usdhc_priv *priv = dev->driver_data;

	LOG_DBG("sector=%u count=%u", sector, count);

	return usdhc_read_sector(priv, buf, sector, count);
}

static int disk_usdhc_access_write(struct disk_info *disk, const u8_t *buf,
				  u32_t sector, u32_t count)
{
	struct device *dev = disk->dev;
	struct usdhc_priv *priv = dev->driver_data;

	LOG_DBG("sector=%u count=%u", sector, count);

	return usdhc_write_sector(priv, buf, sector, count);
}

static int disk_usdhc_access_ioctl(struct disk_info *disk, u8_t cmd, void *buf)
{
	struct device *dev = disk->dev;
	struct usdhc_priv *priv = dev->driver_data;
	int err;

	err = sdhc_map_disk_status(priv->status);
	if (err != 0) {
		return err;
	}

	switch (cmd) {
	case DISK_IOCTL_CTRL_SYNC:
		break;
	case DISK_IOCTL_GET_SECTOR_COUNT:
		*(u32_t *)buf = priv->card_info.sd_block_count;
		break;
	case DISK_IOCTL_GET_SECTOR_SIZE:
		*(u32_t *)buf = priv->card_info.sd_block_size;
		break;
	case DISK_IOCTL_GET_ERASE_BLOCK_SZ:
		*(u32_t *)buf = priv->card_info.sd_block_size;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int disk_usdhc_access_init(struct disk_info *disk)
{
	struct device *dev = disk->dev;
	struct usdhc_priv *priv = dev->driver_data;

	if (priv->status == DISK_STATUS_OK) {
		/* Called twice, don't re-init. */
		return 0;
	}

	return usdhc_access_init(dev);
}

static const struct disk_operations usdhc_disk_ops = {
	.init = disk_usdhc_access_init,
	.status = disk_usdhc_access_status,
	.read = disk_usdhc_access_read,
	.write = disk_usdhc_access_write,
	.ioctl = disk_usdhc_access_ioctl,
};

static struct disk_info usdhc_disk = {
	.name = CONFIG_DISK_SDHC_VOLUME_NAME,
	.ops = &usdhc_disk_ops,
};

static int disk_usdhc_init(struct device *dev)
{
	struct usdhc_priv *priv = dev->driver_data;

	priv->status = DISK_STATUS_UNINIT;

	usdhc_disk.dev = dev;

	return disk_access_register(&usdhc_disk);
}

#ifdef CONFIG_DISK_ACCESS_USDHC1
static struct usdhc_priv usdhc_priv_1;
#ifdef DT_INST_0_NXP_IMX_USDHC_LABEL
DEVICE_AND_API_INIT(usdhc_dev1,
		DT_INST_0_NXP_IMX_USDHC_LABEL, disk_usdhc_init,
		&usdhc_priv_1, NULL, APPLICATION,
		CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		NULL);
#else
#error No USDHC1 slot on board.
#endif
#endif

#ifdef CONFIG_DISK_ACCESS_USDHC2
static struct usdhc_priv usdhc_priv_2;
#ifdef DT_INST_1_NXP_IMX_USDHC_LABEL
DEVICE_AND_API_INIT(usdhc_dev2,
		DT_INST_1_NXP_IMX_USDHC_LABEL, disk_usdhc_init,
		usdhc_priv_2, NULL, APPLICATION,
		CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		NULL);
#else
#error No USDHC2 slot on board.
#endif
#endif

