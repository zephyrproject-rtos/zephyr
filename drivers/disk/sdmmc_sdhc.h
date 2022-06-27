/* disc_access_sdhc.h - header SDHC*/

/*
 * Copyright (c) 2019 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DISK_DRIVER_SDMMC_H_
#define ZEPHYR_INCLUDE_DISK_DRIVER_SDMMC_H_

#include <zephyr/drivers/spi.h>

#define SDMMC_CLOCK_400KHZ (400000U)
#define SD_CLOCK_25MHZ (25000000U)
#define SD_CLOCK_50MHZ (50000000U)
#define SD_CLOCK_100MHZ (100000000U)
#define SD_CLOCK_208MHZ (208000000U)
#define MMC_CLOCK_26MHZ (26000000U)
#define MMC_CLOCK_52MHZ (52000000U)
#define MMC_CLOCK_DDR52 (52000000U)
#define MMC_CLOCK_HS200 (200000000U)
#define MMC_CLOCK_HS400 (400000000U)

/* Command IDs */
#define SDHC_GO_IDLE_STATE 0
#define SDHC_ALL_SEND_CID 2
#define SDHC_SEND_RELATIVE_ADDR 3
#define SDHC_SWITCH 6
#define SDHC_SELECT_CARD 7
#define SDHC_SEND_IF_COND 8
#define SDHC_SEND_CSD 9
#define SDHC_SEND_CID 10
#define SDHC_VOL_SWITCH 11
#define SDHC_STOP_TRANSMISSION 12
#define SDHC_SEND_STATUS 13
#define SDHC_GO_INACTIVE_STATE 15
#define SDHC_SET_BLOCK_SIZE 16
#define SDHC_READ_SINGLE_BLOCK 17
#define SDHC_READ_MULTIPLE_BLOCK 18
#define SDHC_SEND_TUNING_BLOCK 19
#define SDHC_SET_BLOCK_COUNT 23
#define SDHC_WRITE_BLOCK 24
#define SDHC_WRITE_MULTIPLE_BLOCK 25
#define SDHC_ERASE_BLOCK_START 32
#define SDHC_ERASE_BLOCK_END 33
#define SDHC_ERASE_BLOCK_OPERATION 38
#define SDHC_APP_CMD 55
#define SDHC_READ_OCR 58
#define SDHC_CRC_ON_OFF 59

enum sdhc_app_ext_cmd {
	SDHC_APP_SET_BUS_WIDTH = 6,
	SDHC_APP_SEND_STATUS = 13,
	SDHC_APP_SEND_NUM_WRITTEN_BLK = 22,
	SDHC_APP_SET_WRITE_BLK_ERASE_CNT = 23,
	SDHC_APP_SEND_OP_COND = 41,
	SDHC_APP_CLEAR_CARD_DETECT = 42,
	SDHC_APP_SEND_SCR = 51
};

#define SDHC_SEND_OP_COND SDHC_APP_SEND_OP_COND

/* R1 response status */
#define SDHC_R1_IDLE 0x01
#define SDHC_R1_ERASE_RESET 0x02
#define SDHC_R1_ILLEGAL_COMMAND 0x04
#define SDHC_R1_COM_CRC 0x08
#define SDHC_R1_ERASE_SEQ 0x10
#define SDHC_R1_ADDRESS 0x20
#define SDHC_R1_PARAMETER 0x40

#define SDHC_CMD_SIZE 6
#define SDHC_CMD_BODY_SIZE (SDHC_CMD_SIZE - 1)
#define SDHC_CRC16_SIZE 2

/* Command flags */
#define SDHC_START 0x80
#define SDHC_TX 0x40

/* Fields in various card registers */
#define SDHC_HCS (1 << 30)
#define SDHC_CCS (1 << 30)
#define SDHC_BUSY (1 << 31)
#define SDHC_VHS_MASK (0x0F << 8)
#define SDHC_VHS_3V3 (1 << 8)
#define SDHC_CHECK 0xAA
#define SDHC_CSD_SIZE 16
#define SDHC_CSD_V1 0
#define SDHC_CSD_V2 1

/* Data block tokens */
#define SDHC_TOKEN_SINGLE 0xFE
#define SDHC_TOKEN_MULTI_WRITE 0xFC
#define SDHC_TOKEN_STOP_TRAN 0xFD

/* Data block responses */
#define SDHC_RESPONSE_ACCEPTED 0x05
#define SDHC_RESPONSE_CRC_ERR 0x0B
#define SDHC_RESPONSE_WRITE_ERR 0x0E

#define SDHC_MIN_TRIES 20
#define SDHC_RETRY_DELAY 20
/* Time to wait for the card to initialise */
#define SDHC_INIT_TIMEOUT 5000
/* Time to wait for the card to respond or come ready */
#define SDHC_READY_TIMEOUT 500

enum sdhc_rsp_type {
	SDHC_RSP_TYPE_NONE = 0U,
	SDHC_RSP_TYPE_R1 = 1U,
	SDHC_RSP_TYPE_R1b = 2U,
	SDHC_RSP_TYPE_R2 = 3U,
	SDHC_RSP_TYPE_R3 = 4U,
	SDHC_RSP_TYPE_R4 = 5U,
	SDHC_RSP_TYPE_R5 = 6U,
	SDHC_RSP_TYPE_R5b = 7U,
	SDHC_RSP_TYPE_R6 = 8U,
	SDHC_RSP_TYPE_R7 = 9U,
};

enum sdhc_bus_width {
	SDHC_BUS_WIDTH1BIT = 0U,
	SDHC_BUS_WIDTH4BIT = 1U,
};

enum sdhc_flag {
	SDHC_HIGH_CAPACITY_FLAG = (1U << 1U),
	SDHC_4BITS_WIDTH = (1U << 2U),
	SDHC_SDHC_FLAG = (1U << 3U),
	SDHC_SDXC_FLAG = (1U << 4U),
	SDHC_1800MV_FLAG = (1U << 5U),
	SDHC_CMD23_FLAG = (1U << 6U),
	SDHC_SPEED_CLASS_CONTROL_FLAG = (1U << 7U),
};

enum sdhc_r1_error_flag {
	SDHC_R1OUTOF_RANGE_ERR = (1U << 31U),
	SDHC_R1ADDRESS_ERR = (1U << 30U),
	SDHC_R1BLK_LEN_ERR = (1U << 29U),
	SDHC_R1ERASE_SEQ_ERR = (1U << 28U),
	SDHC_R1ERASE_PARAMETER_ERR = (1U << 27U),
	SDHC_R1WRITE_PROTECTION_ERR = (1U << 26U),
	SDHC_R1CARD_LOCKED_ERR = (1U << 25U),
	SDHC_R1LOCK_UNLOCK_ERR = (1U << 24U),
	SDHC_R1CMD_CRC_ERR = (1U << 23U),
	SDHC_R1ILLEGAL_CMD_ERR = (1U << 22U),
	SDHC_R1ECC_ERR = (1U << 21U),
	SDHC_R1CARD_CONTROL_ERR = (1U << 20U),
	SDHC_R1ERR = (1U << 19U),
	SDHC_R1CID_CSD_OVERWRITE_ERR = (1U << 16U),
	SDHC_R1WRITE_PROTECTION_ERASE_SKIP = (1U << 15U),
	SDHC_R1CARD_ECC_DISABLED = (1U << 14U),
	SDHC_R1ERASE_RESET = (1U << 13U),
	SDHC_R1READY_FOR_DATA = (1U << 8U),
	SDHC_R1SWITCH_ERR = (1U << 7U),
	SDHC_R1APP_CMD = (1U << 5U),
	SDHC_R1AUTH_SEQ_ERR = (1U << 3U),

	SDHC_R1ERR_All_FLAG =
		(SDHC_R1OUTOF_RANGE_ERR |
		SDHC_R1ADDRESS_ERR |
		SDHC_R1BLK_LEN_ERR |
		SDHC_R1ERASE_SEQ_ERR |
		SDHC_R1ERASE_PARAMETER_ERR |
		SDHC_R1WRITE_PROTECTION_ERR |
		SDHC_R1CARD_LOCKED_ERR |
		SDHC_R1LOCK_UNLOCK_ERR |
		SDHC_R1CMD_CRC_ERR |
		SDHC_R1ILLEGAL_CMD_ERR |
		SDHC_R1ECC_ERR |
		SDHC_R1CARD_CONTROL_ERR |
		SDHC_R1ERR |
		SDHC_R1CID_CSD_OVERWRITE_ERR |
		SDHC_R1AUTH_SEQ_ERR),

	SDHC_R1ERR_NONE = 0,
};

#define SD_R1_CURRENT_STATE(x) (((x)&0x00001E00U) >> 9U)

enum sd_r1_current_state {
	SDMMC_R1_IDLE = 0U,
	SDMMC_R1_READY = 1U,
	SDMMC_R1_IDENTIFY = 2U,
	SDMMC_R1_STANDBY = 3U,
	SDMMC_R1_TRANSFER = 4U,
	SDMMC_R1_SEND_DATA = 5U,
	SDMMC_R1_RECIVE_DATA = 6U,
	SDMMC_R1_PROGRAM = 7U,
	SDMMC_R1_DISCONNECT = 8U,
};

enum sd_ocr_flag {
	SD_OCR_PWR_BUSY_FLAG = (1U << 31U),
	/*!< Power up busy status */
	SD_OCR_HOST_CAP_FLAG = (1U << 30U),
	/*!< Card capacity status */
	SD_OCR_CARD_CAP_FLAG = SD_OCR_HOST_CAP_FLAG,
	/*!< Card capacity status */
	SD_OCR_SWITCH_18_REQ_FLAG = (1U << 24U),
	/*!< Switch to 1.8V request */
	SD_OCR_SWITCH_18_ACCEPT_FLAG = SD_OCR_SWITCH_18_REQ_FLAG,
	/*!< Switch to 1.8V accepted */
	SD_OCR_VDD27_28FLAG = (1U << 15U),
	/*!< VDD 2.7-2.8 */
	SD_OCR_VDD28_29FLAG = (1U << 16U),
	/*!< VDD 2.8-2.9 */
	SD_OCR_VDD29_30FLAG = (1U << 17U),
	/*!< VDD 2.9-3.0 */
	SD_OCR_VDD30_31FLAG = (1U << 18U),
	/*!< VDD 2.9-3.0 */
	SD_OCR_VDD31_32FLAG = (1U << 19U),
	/*!< VDD 3.0-3.1 */
	SD_OCR_VDD32_33FLAG = (1U << 20U),
	/*!< VDD 3.1-3.2 */
	SD_OCR_VDD33_34FLAG = (1U << 21U),
	/*!< VDD 3.2-3.3 */
	SD_OCR_VDD34_35FLAG = (1U << 22U),
	/*!< VDD 3.3-3.4 */
	SD_OCR_VDD35_36FLAG = (1U << 23U),
	/*!< VDD 3.4-3.5 */
};

#define SD_PRODUCT_NAME_BYTES (5U)

struct sd_cid {
	uint8_t manufacturer;
	/*!< Manufacturer ID [127:120] */
	uint16_t application;
	/*!< OEM/Application ID [119:104] */
	uint8_t name[SD_PRODUCT_NAME_BYTES];
	/*!< Product name [103:64] */
	uint8_t version;
	/*!< Product revision [63:56] */
	uint32_t ser_num;
	/*!< Product serial number [55:24] */
	uint16_t date;
	/*!< Manufacturing date [19:8] */
};

struct sd_csd {
	uint8_t csd_structure;
	/*!< CSD structure [127:126] */
	uint8_t read_time1;
	/*!< Data read access-time-1 [119:112] */
	uint8_t read_time2;
	/*!< Data read access-time-2 in clock cycles (NSAC*100) [111:104] */
	uint8_t xfer_rate;
	/*!< Maximum data transfer rate [103:96] */
	uint16_t cmd_class;
	/*!< Card command classes [95:84] */
	uint8_t read_blk_len;
	/*!< Maximum read data block length [83:80] */
	uint16_t flags;
	/*!< Flags in _sd_csd_flag */
	uint32_t device_size;
	/*!< Device size [73:62] */
	uint8_t read_current_min;
	/*!< Maximum read current at VDD min [61:59] */
	uint8_t read_current_max;
	/*!< Maximum read current at VDD max [58:56] */
	uint8_t write_current_min;
	/*!< Maximum write current at VDD min [55:53] */
	uint8_t write_current_max;
	/*!< Maximum write current at VDD max [52:50] */
	uint8_t dev_size_mul;
	/*!< Device size multiplier [49:47] */

	uint8_t erase_size;
	/*!< Erase sector size [45:39] */
	uint8_t write_prtect_size;
	/*!< Write protect group size [38:32] */
	uint8_t write_speed_factor;
	/*!< Write speed factor [28:26] */
	uint8_t write_blk_len;
	/*!< Maximum write data block length [25:22] */
	uint8_t file_fmt;
	/*!< File format [11:10] */
};

struct sd_scr {
	uint8_t scr_structure;
	/*!< SCR Structure [63:60] */
	uint8_t sd_spec;
	/*!< SD memory card specification version [59:56] */
	uint16_t flags;
	/*!< SCR flags in _sd_scr_flag */
	uint8_t sd_sec;
	/*!< Security specification supported [54:52] */
	uint8_t sd_width;
	/*!< Data bus widths supported [51:48] */
	uint8_t sd_ext_sec;
	/*!< Extended security support [46:43] */
	uint8_t cmd_support;
	/*!< Command support bits [33:32] 33-support CMD23, 32-support cmd20*/
	uint32_t rsvd;
	/*!< reserved for manufacturer usage [31:0] */
};

enum sd_timing_mode {
	SD_TIMING_SDR12_DFT_MODE = 0U,
	/*!< Identification mode & SDR12 */
	SD_TIMING_SDR25_HIGH_SPEED_MODE = 1U,
	/*!< High speed mode & SDR25 */
	SD_TIMING_SDR50_MODE = 2U,
	/*!< SDR50 mode*/
	SD_TIMING_SDR104_MODE = 3U,
	/*!< SDR104 mode */
	SD_TIMING_DDR50_MODE = 4U,
	/*!< DDR50 mode */
};

/*! @brief SD card current limit */
enum sd_max_current {
	SD_MAX_CURRENT_200MA = 0U,
	/*!< default current limit */
	SD_MAX_CURRENT_400MA = 1U,
	/*!< current limit to 400MA */
	SD_MAX_CURRENT_600MA = 2U,
	/*!< current limit to 600MA */
	SD_MAX_CURRENT_800MA = 3U,
	/*!< current limit to 800MA */
};

enum sd_voltage {
	SD_VOL_NONE = 0U,
	/*!< indicate current voltage setting is not set by user*/
	SD_VOL_3_3_V = 1U,
	/*!< card operation voltage around 3.3v */
	SD_VOL_3_0_V = 2U,
	/*!< card operation voltage around 3.0v */
	SD_VOL_1_8_V = 3U,
	/*!< card operation voltage around 1.8v */
};

#define SDMMC_DEFAULT_BLOCK_SIZE (512U)

struct sd_data_op {
	uint32_t start_block;
	uint32_t block_size;
	uint32_t block_count;
	uint32_t *buf;
};

enum sd_switch_arg {
	SD_SWITCH_CHECK = 0U,
	/*!< SD switch mode 0: check function */
	SD_SWITCH_SET = 1U,
	/*!< SD switch mode 1: set function */
};

enum sd_group_num {
	SD_GRP_TIMING_MODE = 0U,
	/*!< access mode group*/
	SD_GRP_CMD_SYS_MODE = 1U,
	/*!< command system group*/
	SD_GRP_DRIVER_STRENGTH_MODE = 2U,
	/*!< driver strength group*/
	SD_GRP_CURRENT_LIMIT_MODE = 3U,
	/*!< current limit group*/
};

enum sd_driver_strength {
	SD_DRV_STRENGTH_TYPEB = 0U,
	/*!< default driver strength*/
	SD_DRV_STRENGTH_TYPEA = 1U,
	/*!< driver strength TYPE A */
	SD_DRV_STRENGTH_TYPEC = 2U,
	/*!< driver strength TYPE C */
	SD_DRV_STRENGTH_TYPED = 3U,
	/*!< driver strength TYPE D */
};

enum sd_csd_flag {
	SD_CSD_READ_BLK_PARTIAL_FLAG = (1U << 0U),
	/*!< Partial blocks for read allowed [79:79] */
	SD_CSD_WRITE_BLK_MISALIGN_FLAG = (1U << 1U),
	/*!< Write block misalignment [78:78] */
	SD_CSD_READ_BLK_MISALIGN_FLAG = (1U << 2U),
	/*!< Read block misalignment [77:77] */
	SD_CSD_DSR_IMPLEMENTED_FLAG = (1U << 3U),
	/*!< DSR implemented [76:76] */
	SD_CSD_ERASE_BLK_EN_FLAG = (1U << 4U),
	/*!< Erase single block enabled [46:46] */
	SD_CSD_WRITE_PROTECT_GRP_EN_FLAG = (1U << 5U),
	/*!< Write protect group enabled [31:31] */
	SD_CSD_WRITE_BLK_PARTIAL_FLAG = (1U << 6U),
	/*!< Partial blocks for write allowed [21:21] */
	SD_CSD_FILE_FMT_GRP_FLAG = (1U << 7U),
	/*!< File format group [15:15] */
	SD_CSD_COPY_FLAG = (1U << 8U),
	/*!< Copy flag [14:14] */
	SD_CSD_PERMANENT_WRITE_PROTECT_FLAG = (1U << 9U),
	/*!< Permanent write protection [13:13] */
	SD_CSD_TMP_WRITE_PROTECT_FLAG = (1U << 10U),
	/*!< Temporary write protection [12:12] */
};

enum sd_scr_flag {
	SD_SCR_DATA_STATUS_AFTER_ERASE = (1U << 0U),
	/*!< Data status after erases [55:55] */
	SD_SCR_SPEC3 = (1U << 1U),
	/*!< Specification version 3.00 or higher [47:47]*/
};

enum sd_spec_version {
	SD_SPEC_VER1_0 = (1U << 0U),
	/*!< SD card version 1.0-1.01 */
	SD_SPEC_VER1_1 = (1U << 1U),
	/*!< SD card version 1.10 */
	SD_SPEC_VER2_0 = (1U << 2U),
	/*!< SD card version 2.00 */
	SD_SPEC_VER3_0 = (1U << 3U),
	/*!< SD card version 3.0 */
};

enum sd_command_class {
	SD_CMD_CLASS_BASIC = (1U << 0U),
	/*!< Card command class 0 */
	SD_CMD_CLASS_BLOCK_READ = (1U << 2U),
	/*!< Card command class 2 */
	SD_CMD_CLASS_BLOCK_WRITE = (1U << 4U),
	/*!< Card command class 4 */
	SD_CMD_CLASS_ERASE = (1U << 5U),
	/*!< Card command class 5 */
	SD_CMD_CLASS_WRITE_PROTECT = (1U << 6U),
	/*!< Card command class 6 */
	SD_CMD_CLASS_LOCKCARD = (1U << 7U),
	/*!< Card command class 7 */
	SD_CMD_CLASS_APP_SPECIFIC = (1U << 8U),
	/*!< Card command class 8 */
	SD_CMD_CLASS_IO_MODE = (1U << 9U),
	/*!< Card command class 9 */
	SD_CMD_CLASS_SWITCH = (1U << 10U),
	/*!< Card command class 10 */
};

struct sdhc_retry {
	uint32_t end;
	int16_t tries;
	uint16_t sleep;
};

struct sdhc_flag_map {
	uint8_t mask;
	uint8_t err;
};

/* The SD protocol requires sending ones while reading but Zephyr
 * defaults to writing zeros.
 */
static const uint8_t sdhc_ones[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

BUILD_ASSERT(sizeof(sdhc_ones) % SDHC_CSD_SIZE == 0);
BUILD_ASSERT(SDMMC_DEFAULT_BLOCK_SIZE % sizeof(sdhc_ones) == 0);

/* Maps R1 response flags to error codes */
static const struct sdhc_flag_map sdhc_r1_flags[] = {
	{SDHC_R1_PARAMETER, EFAULT},	   {SDHC_R1_ADDRESS, EFAULT},
	{SDHC_R1_ILLEGAL_COMMAND, EINVAL}, {SDHC_R1_COM_CRC, EILSEQ},
	{SDHC_R1_ERASE_SEQ, EIO},	  {SDHC_R1_ERASE_RESET, EIO},
	{SDHC_R1_IDLE, ECONNRESET},	{0, 0},
};

/* Maps disk status flags to error codes */
static const struct sdhc_flag_map sdhc_disk_status_flags[] = {
	{DISK_STATUS_UNINIT, ENODEV},
	{DISK_STATUS_NOMEDIA, ENOENT},
	{DISK_STATUS_WR_PROTECT, EROFS},
	{0, 0},
};

/* Maps data block flags to error codes */
static const struct sdhc_flag_map sdhc_data_response_flags[] = {
	{SDHC_RESPONSE_WRITE_ERR, EIO},
	{SDHC_RESPONSE_CRC_ERR, EILSEQ},
	{SDHC_RESPONSE_ACCEPTED, 0},
	/* Unrecognised value */
	{0, EPROTO},
};

/* Returns true if an error code is retryable at the disk layer */
static inline bool sdhc_is_retryable(int err)
{
	switch (err) {
	case 0:
		return false;
	case -EILSEQ:
	case -EIO:
	case -ETIMEDOUT:
		return true;
	default:
		return false;
	}
}

/* Maps a flag based error code into a Zephyr errno */
static inline int sdhc_map_flags(const struct sdhc_flag_map *map, int flags)
{
	if (flags < 0) {
		return flags;
	}

	for (; map->mask != 0U; map++) {
		if ((flags & map->mask) == map->mask) {
			return -map->err;
		}
	}

	return -map->err;
}

/* Converts disk status into an error code */
static inline int sdhc_map_disk_status(int status)
{
	return sdhc_map_flags(sdhc_disk_status_flags, status);
}

/* Converts the R1 response flags into an error code */
static inline int sdhc_map_r1_status(int status)
{
	return sdhc_map_flags(sdhc_r1_flags, status);
}

/* Converts an early stage idle mode R1 code into an error code */
static inline int sdhc_map_r1_idle_status(int status)
{
	if (status < 0) {
		return status;
	}

	if (status == SDHC_R1_IDLE) {
		return 0;
	}

	return sdhc_map_r1_status(status);
}

/* Converts the data block response flags into an error code */
static inline int sdhc_map_data_status(int status)
{
	return sdhc_map_flags(sdhc_data_response_flags, status);
}

/* Initialises a retry helper */
static inline void sdhc_retry_init(struct sdhc_retry *retry, uint32_t timeout,
			    uint16_t sleep)
{
	retry->end = k_uptime_get_32() + timeout;
	retry->tries = 0;
	retry->sleep = sleep;
}

/* Called at the end of a retry loop.  Returns if the minimum try
 * count and timeout has passed.  Delays/yields on retry.
 */
static inline bool sdhc_retry_ok(struct sdhc_retry *retry)
{
	int32_t remain = retry->end - k_uptime_get_32();

	if (retry->tries < SDHC_MIN_TRIES) {
		retry->tries++;
		if (retry->sleep != 0U) {
			k_msleep(retry->sleep);
		}

		return true;
	}

	if (remain >= 0) {
		if (retry->sleep > 0) {
			k_msleep(retry->sleep);
		} else {
			k_yield();
		}

		return true;
	}

	return false;
}

static inline void sdhc_decode_csd(struct sd_csd *csd,
	uint32_t *raw_csd, uint32_t *blk_cout, uint32_t *blk_size)
{
	uint32_t tmp_blk_cout, tmp_blk_size;

	csd->csd_structure = (uint8_t)((raw_csd[3U] &
		0xC0000000U) >> 30U);
	csd->read_time1 = (uint8_t)((raw_csd[3U] &
		0xFF0000U) >> 16U);
	csd->read_time2 = (uint8_t)((raw_csd[3U] &
		0xFF00U) >> 8U);
	csd->xfer_rate = (uint8_t)(raw_csd[3U] &
		0xFFU);
	csd->cmd_class = (uint16_t)((raw_csd[2U] &
		0xFFF00000U) >> 20U);
	csd->read_blk_len = (uint8_t)((raw_csd[2U] &
		0xF0000U) >> 16U);
	if (raw_csd[2U] & 0x8000U)
		csd->flags |= SD_CSD_READ_BLK_PARTIAL_FLAG;
	if (raw_csd[2U] & 0x4000U)
		csd->flags |= SD_CSD_READ_BLK_PARTIAL_FLAG;
	if (raw_csd[2U] & 0x2000U)
		csd->flags |= SD_CSD_READ_BLK_MISALIGN_FLAG;
	if (raw_csd[2U] & 0x1000U)
		csd->flags |= SD_CSD_DSR_IMPLEMENTED_FLAG;

	switch (csd->csd_structure) {
	case 0:
		csd->device_size = (uint32_t)((raw_csd[2U] &
			0x3FFU) << 2U);
		csd->device_size |= (uint32_t)((raw_csd[1U] &
			0xC0000000U) >> 30U);
		csd->read_current_min = (uint8_t)((raw_csd[1U] &
			0x38000000U) >> 27U);
		csd->read_current_max = (uint8_t)((raw_csd[1U] &
			0x7000000U) >> 24U);
		csd->write_current_min = (uint8_t)((raw_csd[1U] &
			0xE00000U) >> 20U);
		csd->write_current_max = (uint8_t)((raw_csd[1U] &
			0x1C0000U) >> 18U);
		csd->dev_size_mul = (uint8_t)((raw_csd[1U] &
			0x38000U) >> 15U);

		/* Get card total block count and block size. */
		tmp_blk_cout = ((csd->device_size + 1U) <<
			(csd->dev_size_mul + 2U));
		tmp_blk_size = (1U << (csd->read_blk_len));
		if (tmp_blk_size != SDMMC_DEFAULT_BLOCK_SIZE) {
			tmp_blk_cout = (tmp_blk_cout * tmp_blk_size);
			tmp_blk_size = SDMMC_DEFAULT_BLOCK_SIZE;
			tmp_blk_cout = (tmp_blk_cout / tmp_blk_size);
		}
		if (blk_cout)
			*blk_cout = tmp_blk_cout;
		if (blk_size)
			*blk_size = tmp_blk_size;
		break;
	case 1:
		tmp_blk_size = SDMMC_DEFAULT_BLOCK_SIZE;

		csd->device_size = (uint32_t)((raw_csd[2U] &
			0x3FU) << 16U);
		csd->device_size |= (uint32_t)((raw_csd[1U] &
			0xFFFF0000U) >> 16U);

		tmp_blk_cout = ((csd->device_size + 1U) * 1024U);
		if (blk_cout)
			*blk_cout = tmp_blk_cout;
		if (blk_size)
			*blk_size = tmp_blk_size;
		break;
	default:
		break;
	}

	if ((uint8_t)((raw_csd[1U] & 0x4000U) >> 14U))
		csd->flags |= SD_CSD_ERASE_BLK_EN_FLAG;
	csd->erase_size = (uint8_t)((raw_csd[1U] &
		0x3F80U) >> 7U);
	csd->write_prtect_size = (uint8_t)(raw_csd[1U] &
		0x7FU);
	csd->write_speed_factor = (uint8_t)((raw_csd[0U] &
		0x1C000000U) >> 26U);
	csd->write_blk_len = (uint8_t)((raw_csd[0U] &
		0x3C00000U) >> 22U);
	if ((uint8_t)((raw_csd[0U] & 0x200000U) >> 21U))
		csd->flags |= SD_CSD_WRITE_BLK_PARTIAL_FLAG;
	if ((uint8_t)((raw_csd[0U] & 0x8000U) >> 15U))
		csd->flags |= SD_CSD_FILE_FMT_GRP_FLAG;
	if ((uint8_t)((raw_csd[0U] & 0x4000U) >> 14U))
		csd->flags |= SD_CSD_COPY_FLAG;
	if ((uint8_t)((raw_csd[0U] & 0x2000U) >> 13U))
		csd->flags |=
			SD_CSD_PERMANENT_WRITE_PROTECT_FLAG;
	if ((uint8_t)((raw_csd[0U] & 0x1000U) >> 12U))
		csd->flags |=
			SD_CSD_TMP_WRITE_PROTECT_FLAG;
	csd->file_fmt = (uint8_t)((raw_csd[0U] & 0xC00U) >> 10U);
}

static inline void sdhc_decode_scr(struct sd_scr *scr,
	uint32_t *raw_scr, uint32_t *version)
{
	uint32_t tmp_version = 0;

	scr->scr_structure = (uint8_t)((raw_scr[0U] & 0xF0000000U) >> 28U);
	scr->sd_spec = (uint8_t)((raw_scr[0U] & 0xF000000U) >> 24U);
	if ((uint8_t)((raw_scr[0U] & 0x800000U) >> 23U))
		scr->flags |= SD_SCR_DATA_STATUS_AFTER_ERASE;
	scr->sd_sec = (uint8_t)((raw_scr[0U] & 0x700000U) >> 20U);
	scr->sd_width = (uint8_t)((raw_scr[0U] & 0xF0000U) >> 16U);
	if ((uint8_t)((raw_scr[0U] & 0x8000U) >> 15U))
		scr->flags |= SD_SCR_SPEC3;
	scr->sd_ext_sec = (uint8_t)((raw_scr[0U] & 0x7800U) >> 10U);
	scr->cmd_support = (uint8_t)(raw_scr[0U] & 0x3U);
	scr->rsvd = raw_scr[1U];
	/* Get specification version. */
	switch (scr->sd_spec) {
	case 0U:
		tmp_version = SD_SPEC_VER1_0;
		break;
	case 1U:
		tmp_version = SD_SPEC_VER1_1;
		break;
	case 2U:
		tmp_version = SD_SPEC_VER2_0;
		if (scr->flags & SD_SCR_SPEC3) {
			tmp_version = SD_SPEC_VER3_0;
		}
		break;
	default:
		break;
	}

	if (version && tmp_version)
		*version = tmp_version;
}

static inline void sdhc_decode_cid(struct sd_cid *cid,
	uint32_t *raw_cid)
{
	cid->manufacturer = (uint8_t)((raw_cid[3U] & 0xFF000000U) >> 24U);
	cid->application = (uint16_t)((raw_cid[3U] & 0xFFFF00U) >> 8U);

	cid->name[0U] = (uint8_t)((raw_cid[3U] & 0xFFU));
	cid->name[1U] = (uint8_t)((raw_cid[2U] & 0xFF000000U) >> 24U);
	cid->name[2U] = (uint8_t)((raw_cid[2U] & 0xFF0000U) >> 16U);
	cid->name[3U] = (uint8_t)((raw_cid[2U] & 0xFF00U) >> 8U);
	cid->name[4U] = (uint8_t)((raw_cid[2U] & 0xFFU));

	cid->version = (uint8_t)((raw_cid[1U] & 0xFF000000U) >> 24U);

	cid->ser_num = (uint32_t)((raw_cid[1U] & 0xFFFFFFU) << 8U);
	cid->ser_num |= (uint32_t)((raw_cid[0U] & 0xFF000000U) >> 24U);

	cid->date = (uint16_t)((raw_cid[0U] & 0xFFF00U) >> 8U);
}

#endif /*ZEPHYR_INCLUDE_DISK_DRIVER_SDMMC_H_*/
