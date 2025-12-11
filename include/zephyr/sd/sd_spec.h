/*
 * Copyright 2022-2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * SD card specification
 */

#ifndef ZEPHYR_SUBSYS_SD_SPEC_H_
#define ZEPHYR_SUBSYS_SD_SPEC_H_

#include <stdint.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SD specification command opcodes
 *
 * SD specification command opcodes. Note that some command opcodes are
 * specific to SDIO cards, or cards running in SPI mode instead of native SD
 * mode.
 */
enum sd_opcode {
	SD_GO_IDLE_STATE = 0,
	MMC_SEND_OP_COND = 1,
	SD_ALL_SEND_CID = 2,
	SD_SEND_RELATIVE_ADDR = 3,
	MMC_SEND_RELATIVE_ADDR = 3,
	SDIO_SEND_OP_COND = 5, /* SDIO cards only */
	SD_SWITCH = 6,
	SD_SELECT_CARD = 7,
	SD_SEND_IF_COND = 8,
	MMC_SEND_EXT_CSD = 8,
	SD_SEND_CSD = 9,
	SD_SEND_CID = 10,
	SD_VOL_SWITCH = 11,
	SD_STOP_TRANSMISSION = 12,
	SD_SEND_STATUS = 13,
	MMC_CHECK_BUS_TEST = 14,
	SD_GO_INACTIVE_STATE = 15,
	SD_SET_BLOCK_SIZE = 16,
	SD_READ_SINGLE_BLOCK = 17,
	SD_READ_MULTIPLE_BLOCK = 18,
	SD_SEND_TUNING_BLOCK = 19,
	MMC_SEND_BUS_TEST = 19,
	MMC_SEND_TUNING_BLOCK = 21,
	SD_SET_BLOCK_COUNT = 23,
	SD_WRITE_SINGLE_BLOCK = 24,
	SD_WRITE_MULTIPLE_BLOCK = 25,
	SD_ERASE_BLOCK_START = 32,
	SD_ERASE_BLOCK_END = 33,
	SD_ERASE_BLOCK_OPERATION = 38,
	SDIO_RW_DIRECT = 52,
	SDIO_RW_EXTENDED = 53,
	SD_APP_CMD = 55,
	SD_SPI_READ_OCR = 58, /* SPI mode only */
	SD_SPI_CRC_ON_OFF = 59, /* SPI mode only */
};

/**
 * @brief SD application command opcodes.
 *
 * all application command opcodes must be prefixed with a CMD55 command
 * to inform the SD card the next command is an application-specific one.
 */
enum sd_app_cmd {
	SD_APP_SET_BUS_WIDTH = 6,
	SD_APP_SEND_STATUS = 13,
	SD_APP_SEND_NUM_WRITTEN_BLK = 22,
	SD_APP_SET_WRITE_BLK_ERASE_CNT = 23,
	SD_APP_SEND_OP_COND = 41,
	SD_APP_CLEAR_CARD_DETECT = 42,
	SD_APP_SEND_SCR = 51,
};

/**
 * @brief Native SD mode R1 response status flags
 *
 * Native response flags for SD R1 response, used to check for error in command.
 */
enum sd_r1_status {
	/* Bits 0-2 reserved */
	SD_R1_AUTH_ERR = BIT(3),
	/* Bit 4 reserved for SDIO */
	SD_R1_APP_CMD = BIT(5),
	SD_R1_FX_EVENT = BIT(6),
	/* Bit 7 reserved */
	SD_R1_RDY_DATA = BIT(8),
	SD_R1_CUR_STATE = (0xFU << 9),
	SD_R1_ERASE_RESET = BIT(13),
	SD_R1_ECC_DISABLED = BIT(14),
	SD_R1_ERASE_SKIP = BIT(15),
	SD_R1_CSD_OVERWRITE = BIT(16),
	/* Bits 17-18 reserved */
	SD_R1_ERR = BIT(19),
	SD_R1_CC_ERR = BIT(20),
	SD_R1_ECC_FAIL = BIT(21),
	SD_R1_ILLEGAL_CMD = BIT(22),
	SD_R1_CRC_ERR = BIT(23),
	SD_R1_UNLOCK_FAIL = BIT(24),
	SD_R1_CARD_LOCKED = BIT(25),
	SD_R1_WP_VIOLATION = BIT(26),
	SD_R1_ERASE_PARAM = BIT(27),
	SD_R1_ERASE_SEQ_ERR = BIT(28),
	SD_R1_BLOCK_LEN_ERR = BIT(29),
	SD_R1_ADDR_ERR = BIT(30),
	SD_R1_OUT_OF_RANGE = BIT(31),
	SD_R1_ERR_FLAGS = (SD_R1_AUTH_ERR |
			SD_R1_ERASE_SKIP |
			SD_R1_CSD_OVERWRITE |
			SD_R1_ERR |
			SD_R1_CC_ERR |
			SD_R1_ECC_FAIL |
			SD_R1_ILLEGAL_CMD |
			SD_R1_CRC_ERR |
			SD_R1_UNLOCK_FAIL |
			SD_R1_WP_VIOLATION |
			SD_R1_ERASE_PARAM |
			SD_R1_ERASE_SEQ_ERR |
			SD_R1_BLOCK_LEN_ERR |
			SD_R1_ADDR_ERR |
			SD_R1_OUT_OF_RANGE),
	SD_R1ERR_NONE = 0,
};

#define SD_R1_CURRENT_STATE(x) (((x) & SD_R1_CUR_STATE) >> 9U)

/**
 * @brief SD current state values
 *
 * SD current state values, contained in R1 response data.
 */
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

/**
 * @brief SPI SD mode R1 response status flags
 *
 * SPI mode R1 response flags. Used to check for error in SD SPI mode command.
 */
enum sd_spi_r1_error_flag {
	SD_SPI_R1PARAMETER_ERR = BIT(6),
	SD_SPI_R1ADDRESS_ERR = BIT(5),
	SD_SPI_R1ERASE_SEQ_ERR = BIT(4),
	SD_SPI_R1CMD_CRC_ERR = BIT(3),
	SD_SPI_R1ILLEGAL_CMD_ERR = BIT(2),
	SD_SPI_R1ERASE_RESET = BIT(1),
	SD_SPI_R1IDLE_STATE = BIT(0),
};

/**
 * @brief SPI SD mode R2 response status flags
 *
 * SPI mode R2 response flags. Sent in response to SEND_STATUS command. Used
 * to check status of SD card.
 */
enum sd_spi_r2_status {
	SDHC_SPI_R2_CARD_LOCKED = BIT(8),
	SDHC_SPI_R2_UNLOCK_FAIL = BIT(9),
	SDHC_SPI_R2_ERR = BIT(10),
	SDHC_SPI_R2_CC_ERR = BIT(11),
	SDHC_SPI_R2_ECC_FAIL = BIT(12),
	SDHC_SPI_R2_WP_VIOLATION = BIT(13),
	SDHC_SPI_R2_ERASE_PARAM = BIT(14),
	SDHC_SPI_R2_OUT_OF_RANGE = BIT(15),
};

/* Byte length of SD SPI mode command */
#define SD_SPI_CMD_SIZE 6
#define SD_SPI_CMD_BODY_SIZE (SD_SPI_CMD_SIZE - 1)
/* Byte length of CRC16 appended to data blocks in SPI mode */
#define SD_SPI_CRC16_SIZE 2

/* SPI Command flags */
#define SD_SPI_START 0x80
#define SD_SPI_TX 0x40
#define SD_SPI_CMD 0x3F

/* SPI Data block tokens */
#define SD_SPI_TOKEN_SINGLE 0xFE
#define SD_SPI_TOKEN_MULTI_WRITE 0xFC
#define SD_SPI_TOKEN_STOP_TRAN 0xFD

/* SPI Data block responses */
#define SD_SPI_RESPONSE_ACCEPTED 0x05
#define SD_SPI_RESPONSE_CRC_ERR 0x0B
#define SD_SPI_RESPONSE_WRITE_ERR 0x0C

/* Masks used in SD interface condition query (CMD8) */
#define SD_IF_COND_VHS_MASK (0x0F << 8)
#define SD_IF_COND_VHS_3V3 BIT(8)
#define SD_IF_COND_CHECK 0xAA

/**
 * @brief SD response types
 *
 * SD response types. Note that SPI mode has difference response types than
 * cards in native SD mode.
 */
enum sd_rsp_type {
	/* Native response types (lower 4 bits) */
	SD_RSP_TYPE_NONE = 0U,
	SD_RSP_TYPE_R1 = 1U,
	SD_RSP_TYPE_R1b = 2U,
	SD_RSP_TYPE_R2 = 3U,
	SD_RSP_TYPE_R3 = 4U,
	SD_RSP_TYPE_R4 = 5U,
	SD_RSP_TYPE_R5 = 6U,
	SD_RSP_TYPE_R5b = 7U,
	SD_RSP_TYPE_R6 = 8U,
	SD_RSP_TYPE_R7 = 9U,
	/* SPI response types (bits [7:4]) */
	SD_SPI_RSP_TYPE_R1 = (1U << 4),
	SD_SPI_RSP_TYPE_R1b = (2U << 4),
	SD_SPI_RSP_TYPE_R2 = (3U << 4),
	SD_SPI_RSP_TYPE_R3 = (4U << 4),
	SD_SPI_RSP_TYPE_R4 = (5U << 4),
	SD_SPI_RSP_TYPE_R5 = (6U << 4),
	SD_SPI_RSP_TYPE_R7 = (7U << 4),
};

/**
 * @brief SD support flags
 *
 * flags used by SD subsystem to determine support for SD card features.
 */
enum sd_support_flag {
	SD_HIGH_CAPACITY_FLAG = BIT(1),
	SD_4BITS_WIDTH = BIT(2),
	SD_SDHC_FLAG = BIT(3),
	SD_SDXC_FLAG = BIT(4),
	SD_1800MV_FLAG = BIT(5),
	SD_3000MV_FLAG = BIT(6),
	SD_CMD23_FLAG = BIT(7),
	SD_SPEED_CLASS_CONTROL_FLAG = BIT(8),
	SD_MEM_PRESENT_FLAG = BIT(9),
};


/**
 * @brief SD OCR bit flags
 *
 * bit flags present in SD OCR response. Used to determine status and
 * supported functions of SD card.
 */
enum sd_ocr_flag {
	/** Power up busy status */
	SD_OCR_PWR_BUSY_FLAG = BIT(31),
	/** Card capacity status */
	SD_OCR_HOST_CAP_FLAG = BIT(30),
	/** Card capacity status */
	SD_OCR_CARD_CAP_FLAG = SD_OCR_HOST_CAP_FLAG,
	/** Switch to 1.8V request */
	SD_OCR_SWITCH_18_REQ_FLAG = BIT(24),
	/** Switch to 1.8V accepted */
	SD_OCR_SWITCH_18_ACCEPT_FLAG = SD_OCR_SWITCH_18_REQ_FLAG,
	/** VDD 2.7-2.8 */
	SD_OCR_VDD27_28FLAG = BIT(15),
	/** VDD 2.8-2.9 */
	SD_OCR_VDD28_29FLAG = BIT(16),
	/** VDD 2.9-3.0 */
	SD_OCR_VDD29_30FLAG = BIT(17),
	/** VDD 3.0-3.1 */
	SD_OCR_VDD30_31FLAG = BIT(18),
	/** VDD 3.1-3.2 */
	SD_OCR_VDD31_32FLAG = BIT(19),
	/** VDD 3.2-3.3 */
	SD_OCR_VDD32_33FLAG = BIT(20),
	/** VDD 3.3-3.4 */
	SD_OCR_VDD33_34FLAG = BIT(21),
	/** VDD 3.4-3.5 */
	SD_OCR_VDD34_35FLAG = BIT(22),
	/** VDD 3.5-3.6 */
	SD_OCR_VDD35_36FLAG = BIT(23),
};

/**
 * @brief MMC OCR bit flags
 *
 * bit flags present in MMC OCR response. Used to determine status and
 * supported functions of MMC card.
 */
enum mmc_ocr_flag {
	MMC_OCR_VDD170_195FLAG = BIT(7),
	MMC_OCR_VDD20_26FLAG = 0x7F << 8,
	MMC_OCR_VDD27_36FLAG = 0x1FF << 15,
	MMC_OCR_SECTOR_MODE = BIT(30),
	MMC_OCR_PWR_BUSY_FLAG = BIT(31)
};

#define SDIO_OCR_IO_NUMBER_SHIFT 28
/* Lower 24 bits hold SDIO I/O OCR */
#define SDIO_IO_OCR_MASK 0xFFFFFF

/**
 * @brief SDIO OCR bit flags
 *
 * bit flags present in SDIO OCR response. Used to determine status and
 * supported functions of SDIO card.
 */
enum sdio_ocr_flag {
	SDIO_OCR_IO_READY_FLAG = BIT(31),
	SDIO_OCR_IO_NUMBER = (7U << 28U), /*!< Number of io function */
	SDIO_OCR_MEM_PRESENT_FLAG = BIT(27), /*!< Memory present flag */
	SDIO_OCR_180_VOL_FLAG = BIT(24),	/*!< Switch to 1.8v signalling */
	SDIO_OCR_VDD20_21FLAG = BIT(8),  /*!< VDD 2.0-2.1 */
	SDIO_OCR_VDD21_22FLAG = BIT(9),  /*!< VDD 2.1-2.2 */
	SDIO_OCR_VDD22_23FLAG = BIT(10), /*!< VDD 2.2-2.3 */
	SDIO_OCR_VDD23_24FLAG = BIT(11), /*!< VDD 2.3-2.4 */
	SDIO_OCR_VDD24_25FLAG = BIT(12), /*!< VDD 2.4-2.5 */
	SDIO_OCR_VDD25_26FLAG = BIT(13), /*!< VDD 2.5-2.6 */
	SDIO_OCR_VDD26_27FLAG = BIT(14), /*!< VDD 2.6-2.7 */
	SDIO_OCR_VDD27_28FLAG = BIT(15), /*!< VDD 2.7-2.8 */
	SDIO_OCR_VDD28_29FLAG = BIT(16), /*!< VDD 2.8-2.9 */
	SDIO_OCR_VDD29_30FLAG = BIT(17), /*!< VDD 2.9-3.0 */
	SDIO_OCR_VDD30_31FLAG = BIT(18), /*!< VDD 2.9-3.0 */
	SDIO_OCR_VDD31_32FLAG = BIT(19), /*!< VDD 3.0-3.1 */
	SDIO_OCR_VDD32_33FLAG = BIT(20), /*!< VDD 3.1-3.2 */
	SDIO_OCR_VDD33_34FLAG = BIT(21), /*!< VDD 3.2-3.3 */
	SDIO_OCR_VDD34_35FLAG = BIT(22), /*!< VDD 3.3-3.4 */
	SDIO_OCR_VDD35_36FLAG = BIT(23), /*!< VDD 3.4-3.5 */
};


/**
 * @brief SD switch arguments
 *
 * SD CMD6 can either check or set a function. Bitfields are used to indicate
 * feature support when checking a function, and when setting a function an
 * integer value is used to select it.
 */
enum sd_switch_arg {
	/** SD switch mode 0: check function */
	SD_SWITCH_CHECK = 0U,
	/** SD switch mode 1: set function */
	SD_SWITCH_SET = 1U,
};

/**
 * @brief SD switch group numbers
 *
 * SD CMD6 has multiple function groups it can check/set. These indicies are
 * used to determine which group CMD6 will interact with.
 */
enum sd_group_num {
	/** access mode group */
	SD_GRP_TIMING_MODE = 0U,
	/** command system group */
	SD_GRP_CMD_SYS_MODE = 1U,
	/** driver strength group */
	SD_GRP_DRIVER_STRENGTH_MODE = 2U,
	/** current limit group */
	SD_GRP_CURRENT_LIMIT_MODE = 3U,
};

/* Maximum data rate possible for SD high speed cards */
enum hs_max_data_rate {
	HS_UNSUPPORTED = 0,
	HS_MAX_DTR = MHZ(50),
};

/* Maximum data rate possible for SD uhs cards */
enum uhs_max_data_rate {
	UHS_UNSUPPORTED = 0,
	UHS_SDR12_MAX_DTR = MHZ(25),
	UHS_SDR25_MAX_DTR = MHZ(50),
	UHS_SDR50_MAX_DTR = MHZ(100),
	UHS_SDR104_MAX_DTR = MHZ(208),
	UHS_DDR50_MAX_DTR = MHZ(50),
};

/**
 * @brief SD bus speed support bit flags
 *
 * Bit flags indicating support for SD bus speeds.
 * these bit flags are provided as a response to CMD6 by the card
 */
enum sd_bus_speed {
	UHS_SDR12_BUS_SPEED = BIT(0),
	DEFAULT_BUS_SPEED = BIT(0),
	HIGH_SPEED_BUS_SPEED = BIT(1),
	UHS_SDR25_BUS_SPEED = BIT(1),
	UHS_SDR50_BUS_SPEED = BIT(2),
	UHS_SDR104_BUS_SPEED = BIT(3),
	UHS_DDR50_BUS_SPEED = BIT(4),
};

/**
 * @brief SD timing mode function selection values.
 *
 * sent to the card via CMD6 to select a card speed, and used by SD host
 * controller to identify timing of card.
 */
enum sd_timing_mode {
	/** Default Mode */
	SD_TIMING_DEFAULT = 0U,
	/** SDR12 mode */
	SD_TIMING_SDR12 = 0U,
	/** High speed mode */
	SD_TIMING_HIGH_SPEED = 1U,
	/** SDR25 mode */
	SD_TIMING_SDR25 = 1U,
	/** SDR50 mode*/
	SD_TIMING_SDR50 = 2U,
	/** SDR104 mode */
	SD_TIMING_SDR104 = 3U,
	/** DDR50 mode */
	SD_TIMING_DDR50 = 4U,
};

/**
 * @brief SD host controller clock speed
 *
 * Controls the SD host controller clock speed on the SD bus.
 */
enum sdhc_clock_speed {
	SDMMC_CLOCK_400KHZ = KHZ(400),
	SD_CLOCK_25MHZ = MHZ(25),
	SD_CLOCK_50MHZ = MHZ(50),
	SD_CLOCK_100MHZ = MHZ(100),
	SD_CLOCK_208MHZ = MHZ(208),
	MMC_CLOCK_26MHZ = MHZ(26),
	MMC_CLOCK_52MHZ = MHZ(52),
	MMC_CLOCK_DDR52 = MHZ(52),
	MMC_CLOCK_HS200 = MHZ(200),
	MMC_CLOCK_HS400 = MHZ(200), /* Same clock freq as HS200, just DDR */
};

/**
 * @brief SD current setting values
 *
 * Used with CMD6 to inform the card what its maximum current draw is.
 */
enum sd_current_setting {
	SD_SET_CURRENT_200MA = 0,
	SD_SET_CURRENT_400MA = 1,
	SD_SET_CURRENT_600MA = 2,
	SD_SET_CURRENT_800MA = 3,
};

/**
 * @brief SD current support bitfield
 *
 * Used with CMD6 to determine the maximum current the card will draw.
 */
enum sd_current_limit {
	/** default current limit */
	SD_MAX_CURRENT_200MA = BIT(0),
	/** current limit to 400MA */
	SD_MAX_CURRENT_400MA = BIT(1),
	/** current limit to 600MA */
	SD_MAX_CURRENT_600MA = BIT(2),
	/** current limit to 800MA */
	SD_MAX_CURRENT_800MA = BIT(3),
};

/**
 * @brief SD driver types
 *
 * Used with CMD6 to determine the driver type the card should use.
 */
enum sd_driver_type {
	SD_DRIVER_TYPE_B = 0x1,
	SD_DRIVER_TYPE_A = 0x2,
	SD_DRIVER_TYPE_C = 0x4,
	SD_DRIVER_TYPE_D = 0x8,
};

/**
 * @brief SD switch drive type selection
 *
 * These values are used to select the preferred driver type for an SD card.
 */
enum sd_driver_strength {
	/** default driver strength*/
	SD_DRV_STRENGTH_TYPEB = 0U,
	/** driver strength TYPE A */
	SD_DRV_STRENGTH_TYPEA = 1U,
	/** driver strength TYPE C */
	SD_DRV_STRENGTH_TYPEC = 2U,
	/** driver strength TYPE D */
	SD_DRV_STRENGTH_TYPED = 3U,
};

/**
 * @brief SD switch capabilities
 *
 * records switch capabilities for SD card. These capabilities are set
 * and queried via CMD6
 */
struct sd_switch_caps {
	enum hs_max_data_rate hs_max_dtr;
	enum uhs_max_data_rate uhs_max_dtr;
	enum sd_bus_speed bus_speed;
	enum sd_driver_type sd_drv_type;
	enum sd_current_limit sd_current_limit;
};


#define SD_PRODUCT_NAME_BYTES 5

/**
 * @brief SD card identification register
 *
 * Layout of SD card identification register
 */
struct sd_cid {
	/** Manufacturer ID [127:120] */
	uint8_t manufacturer;
	/** OEM/Application ID [119:104] */
	uint16_t application;
	/** Product name [103:64] */
	uint8_t name[SD_PRODUCT_NAME_BYTES];
	/** Product revision [63:56] */
	uint8_t version;
	/** Product serial number [55:24] */
	uint32_t ser_num;
	/** Manufacturing date [19:8] */
	uint16_t date;
};

/**
 * @brief SD card specific data register.
 *
 * Card specific data register. contains additional data about SD card.
 */
struct sd_csd {
	/** CSD structure [127:126] */
	uint8_t csd_structure;
	/** Data read access-time-1 [119:112] */
	uint8_t read_time1;
	/** Data read access-time-2 in clock cycles (NSAC*100) [111:104] */
	uint8_t read_time2;
	/** Maximum data transfer rate [103:96] */
	uint8_t xfer_rate;
	/** Card command classes [95:84] */
	uint16_t cmd_class;
	/** Maximum read data block length [83:80] */
	uint8_t read_blk_len;
	/** Flags in _sd_csd_flag */
	uint16_t flags;
	/** Device size [73:62] */
	uint32_t device_size;
	/** Maximum read current at VDD min [61:59] */
	uint8_t read_current_min;
	/** Maximum read current at VDD max [58:56] */
	uint8_t read_current_max;
	/** Maximum write current at VDD min [55:53] */
	uint8_t write_current_min;
	/** Maximum write current at VDD max [52:50] */
	uint8_t write_current_max;
	/** Device size multiplier [49:47] */
	uint8_t dev_size_mul;
	/** Erase sector size [45:39] */
	uint8_t erase_size;
	/** Write protect group size [38:32] */
	uint8_t write_prtect_size;
	/** Write speed factor [28:26] */
	uint8_t write_speed_factor;
	/** Maximum write data block length [25:22] */
	uint8_t write_blk_len;
	/** File format [11:10] */
	uint8_t file_fmt;
};

/**
 * @brief MMC Maximum Frequency
 *
 * Max freq in MMC csd
 */
enum mmc_csd_freq {
	MMC_MAXFREQ_100KHZ = 0U << 0U,
	MMC_MAXFREQ_1MHZ = 1U << 0U,
	MMC_MAXFREQ_10MHZ = 2U << 0U,
	MMC_MAXFREQ_100MHZ = 3U << 0U,
	MMC_MAXFREQ_MULT_10 = 1U << 3U,
	MMC_MAXFREQ_MULT_12 = 2U << 3U,
	MMC_MAXFREQ_MULT_13 = 3U << 3U,
	MMC_MAXFREQ_MULT_15 = 4U << 3U,
	MMC_MAXFREQ_MULT_20 = 5U << 3U,
	MMC_MAXFREQ_MULT_26 = 6U << 3U,
	MMC_MAXFREQ_MULT_30 = 7U << 3U,
	MMC_MAXFREQ_MULT_35 = 8U << 3U,
	MMC_MAXFREQ_MULT_40 = 9U << 3U,
	MMC_MAXFREQ_MULT_45 = 0xAU << 3U,
	MMC_MAXFREQ_MULT_52 = 0xBU << 3U,
	MMC_MAXFREQ_MULT_55 = 0xCU << 3U,
	MMC_MAXFREQ_MULT_60 = 0xDU << 3U,
	MMC_MAXFREQ_MULT_70 = 0xEU << 3U,
	MMC_MAXFREQ_MULT_80 = 0xFU << 3u
};

/**
 * @brief MMC Timing Modes
 *
 * MMC Timing Mode, encoded in EXT_CSD
 */
enum mmc_timing_mode {
	MMC_LEGACY_TIMING = 0U,
	MMC_HS_TIMING = 1U,
	MMC_HS200_TIMING = 2U,
	MMC_HS400_TIMING = 3U
};

/**
 * @brief MMC Driver Strengths
 *
 * Encoded in EXT_CSD
 */
enum mmc_driver_strengths {
	mmc_driv_type0 = 0U,
	mmc_driv_type1 = 1U,
	mmc_driv_type2 = 2U,
	mmc_driv_type3 = 3U,
	mmc_driv_type4 = 4U
};


/**
 * @brief MMC Device Type
 *
 * Encoded in EXT_CSD
 */
struct mmc_device_type {
	bool MMC_HS400_DDR_1200MV;
	bool MMC_HS400_DDR_1800MV;
	bool MMC_HS200_SDR_1200MV;
	bool MMC_HS200_SDR_1800MV;
	bool MMC_HS_DDR_1200MV;
	bool MMC_HS_DDR_1800MV;
	bool MMC_HS_52_DV;
	bool MMC_HS_26_DV;
};

/**
 * @brief CSD Revision
 *
 * Value of CSD rev in EXT_CSD
 */
enum mmc_ext_csd_rev {
	MMC_5_1 = 8U,
	MMC_5_0 = 7U,
	MMC_4_5 = 6U,
	MMC_4_4 = 5U,
	MMC_4_3 = 3U,
	MMC_4_2 = 2U,
	MMC_4_1 = 1U,
	MMC_4_0 = 0U
};

/**
 * @brief MMC extended card specific data register
 *
 * Extended card specific data register.
 * Contains additional additional data about MMC card.
 */
struct mmc_ext_csd {
	/** Sector Count [215:212] */
	uint32_t sec_count;
	/** Bus Width Mode [183] */
	uint8_t bus_width;
	/** High Speed Timing Mode [185] */
	enum mmc_timing_mode hs_timing;
	/** Device Type [196] */
	struct mmc_device_type device_type;
	/** Extended CSD Revision [192] */
	enum mmc_ext_csd_rev rev;
	/** Selected power class [187]*/
	uint8_t power_class;
	/** Driver strengths [197] */
	uint8_t mmc_driver_strengths;
	/** Power class information for HS200 at VCC!=1.95V [237] */
	uint8_t pwr_class_200MHZ_VCCQ195;
	/** Power class information for HS400 [253] */
	uint8_t pwr_class_HS400;
	/** Size of eMMC cache [252:249] */
	uint32_t cache_size;
};

/**
 * @brief SD card specific data flags
 *
 * flags used in decoding the SD card specific data register
 */
enum sd_csd_flag {
	/** Partial blocks for read allowed [79:79] */
	SD_CSD_READ_BLK_PARTIAL_FLAG = BIT(0),
	/** Write block misalignment [78:78] */
	SD_CSD_WRITE_BLK_MISALIGN_FLAG = BIT(1),
	/** Read block misalignment [77:77] */
	SD_CSD_READ_BLK_MISALIGN_FLAG = BIT(2),
	/** DSR implemented [76:76] */
	SD_CSD_DSR_IMPLEMENTED_FLAG = BIT(3),
	/** Erase single block enabled [46:46] */
	SD_CSD_ERASE_BLK_EN_FLAG = BIT(4),
	/** Write protect group enabled [31:31] */
	SD_CSD_WRITE_PROTECT_GRP_EN_FLAG = BIT(5),
	/** Partial blocks for write allowed [21:21] */
	SD_CSD_WRITE_BLK_PARTIAL_FLAG = BIT(6),
	/** File format group [15:15] */
	SD_CSD_FILE_FMT_GRP_FLAG = BIT(7),
	/** Copy flag [14:14] */
	SD_CSD_COPY_FLAG = BIT(8),
	/** Permanent write protection [13:13] */
	SD_CSD_PERMANENT_WRITE_PROTECT_FLAG = BIT(9),
	/** Temporary write protection [12:12] */
	SD_CSD_TMP_WRITE_PROTECT_FLAG = BIT(10),
};

/**
 * @brief SD card configuration register
 *
 * Even more SD card data.
 */
struct sd_scr {
	/** SCR Structure [63:60] */
	uint8_t scr_structure;
	/** SD memory card specification version [59:56] */
	uint8_t sd_spec;
	/** SCR flags in _sd_scr_flag */
	uint16_t flags;
	/** Security specification supported [54:52] */
	uint8_t sd_sec;
	/** Data bus widths supported [51:48] */
	uint8_t sd_width;
	/** Extended security support [46:43] */
	uint8_t sd_ext_sec;
	/** Command support bits [33:32] 33-support CMD23, 32-support cmd20*/
	uint8_t cmd_support;
	/** reserved for manufacturer usage [31:0] */
	uint32_t rsvd;
};

/**
 * @brief SD card configuration register
 *
 * flags used in decoding the SD card configuration register
 */
enum sd_scr_flag {
	/** Data status after erases [55:55] */
	SD_SCR_DATA_STATUS_AFTER_ERASE = BIT(0),
	/** Specification version 3.00 or higher [47:47]*/
	SD_SCR_SPEC3 = BIT(1),
};

/**
 * @brief SD specification version
 *
 * SD spec version flags used in decoding the SD card configuration register
 */
enum sd_spec_version {
	/** SD card version 1.0-1.01 */
	SD_SPEC_VER1_0 = BIT(0),
	/** SD card version 1.10 */
	SD_SPEC_VER1_1 = BIT(1),
	/** SD card version 2.00 */
	SD_SPEC_VER2_0 = BIT(2),
	/** SD card version 3.0 */
	SD_SPEC_VER3_0 = BIT(3),
};


#define SDMMC_DEFAULT_BLOCK_SIZE 512
#define MMC_EXT_CSD_BYTES 512
/**
 * @brief SDIO function number
 *
 * SDIO function number used to select function when performing I/O on SDIO card
 */
enum sdio_func_num {
	SDIO_FUNC_NUM_0 = 0,
	SDIO_FUNC_NUM_1 = 1,
	SDIO_FUNC_NUM_2 = 2,
	SDIO_FUNC_NUM_3 = 3,
	SDIO_FUNC_NUM_4 = 4,
	SDIO_FUNC_NUM_5 = 5,
	SDIO_FUNC_NUM_6 = 6,
	SDIO_FUNC_NUM_7 = 7,
	SDIO_FUNC_MEMORY = 8,
};

/**
 * @brief SDIO I/O direction
 *
 * SDIO I/O direction (read or write)
 */
enum sdio_io_dir {
	SDIO_IO_READ = 0,
	SDIO_IO_WRITE = 1,
};

#define SDIO_CMD_ARG_RW_SHIFT 31		/*!< read/write flag shift */
#define SDIO_CMD_ARG_FUNC_NUM_SHIFT 28	/*!< function number shift */
#define SDIO_DIRECT_CMD_ARG_RAW_SHIFT 27	/*!< direct raw flag shift */
#define SDIO_CMD_ARG_REG_ADDR_SHIFT 9	/*!< direct reg addr shift */
#define SDIO_CMD_ARG_REG_ADDR_MASK 0x1FFFF	/*!< direct reg addr mask */
#define SDIO_DIRECT_CMD_DATA_MASK 0xFF	/*!< data mask */

#define SDIO_EXTEND_CMD_ARG_BLK_SHIFT 27	/*!< extended write block mode */
#define SDIO_EXTEND_CMD_ARG_OP_CODE_SHIFT 26	/*!< op code (increment address) */

/**
 * @brief Card common control register definitions
 *
 * Card common control registers, present on all SDIO cards
 */
#define SDIO_CCCR_CCCR 0x00 /*!< SDIO CCCR revision register */
#define SDIO_CCCR_CCCR_REV_MASK 0x0F
#define SDIO_CCCR_CCCR_REV_SHIFT 0x0
#define SDIO_CCCR_CCCR_REV_1_00 0x0 /*!< CCCR/FBR Version 1.00 */
#define SDIO_CCCR_CCCR_REV_1_10 0x1 /*!< CCCR/FBR Version 1.10 */
#define SDIO_CCCR_CCCR_REV_2_00 0x2 /*!< CCCR/FBR Version 2.00 */
#define SDIO_CCCR_CCCR_REV_3_00 0x3 /*!< CCCR/FBR Version 3.00 */

#define SDIO_CCCR_SD 0x01 /*!< SD spec version  register */
#define SDIO_CCCR_SD_SPEC_MASK 0x0F
#define SDIO_CCCR_SD_SPEC_SHIFT 0x0

#define SDIO_CCCR_IO_EN 0x02 /*!< SDIO IO Enable register */

#define SDIO_CCCR_IO_RD 0x03 /*!< SDIO IO Ready register */

#define SDIO_CCCR_INT_EN 0x04 /*!< SDIO Interrupt enable register */

#define SDIO_CCCR_INT_P 0x05 /*!< SDIO Interrupt pending register */

#define SDIO_CCCR_ABORT 0x06 /*!< SDIO IO abort register */

#define SDIO_CCCR_BUS_IF 0x07 /*!< SDIO bus interface control register */
#define SDIO_CCCR_BUS_IF_WIDTH_MASK 0x3 /*!< SDIO bus width setting mask */
#define SDIO_CCCR_BUS_IF_WIDTH_1_BIT 0x00 /*!< 1 bit SDIO bus setting */
#define SDIO_CCCR_BUS_IF_WIDTH_4_BIT 0x02 /*!< 4 bit SDIO bus setting */
#define SDIO_CCCR_BUS_IF_WIDTH_8_BIT 0x03 /*!< 8 bit SDIO bus setting */

#define SDIO_CCCR_CAPS 0x08 /*!< SDIO card capabilities */
#define SDIO_CCCR_CAPS_SDC BIT(0) /*!< support CMD52 while data transfer */
#define SDIO_CCCR_CAPS_SMB BIT(1) /*!< support multiple block transfer */
#define SDIO_CCCR_CAPS_SRW BIT(2) /*!< support read wait control */
#define SDIO_CCCR_CAPS_SBS BIT(3) /*!< support bus control */
#define SDIO_CCCR_CAPS_S4MI BIT(4) /*!< support block gap interrupt */
#define SDIO_CCCR_CAPS_E4MI BIT(5) /*!< enable block gap interrupt */
#define SDIO_CCCR_CAPS_LSC BIT(6) /*!< low speed card */
#define SDIO_CCCR_CAPS_BLS BIT(7) /*!< low speed card with 4 bit support */

#define SDIO_CCCR_CIS 0x09 /*!< SDIO CIS tuples pointer */

#define SDIO_CCCR_SPEED	0x13 /*!< SDIO bus speed select */
#define SDIO_CCCR_SPEED_SHS BIT(0) /*!< high speed support */
#define SDIO_CCCR_SPEED_MASK 0xE /*!< bus speed select mask*/
#define SDIO_CCCR_SPEED_SHIFT 0x1 /*!< bus speed select shift */
#define SDIO_CCCR_SPEED_SDR12 0x0 /*!< select SDR12 */
#define SDIO_CCCR_SPEED_HS 0x1 /*!< select High speed mode */
#define SDIO_CCCR_SPEED_SDR25 0x1 /*!< select SDR25 */
#define SDIO_CCCR_SPEED_SDR50 0x2 /*!< select SDR50 */
#define SDIO_CCCR_SPEED_SDR104 0x3 /*!< select SDR104 */
#define SDIO_CCCR_SPEED_DDR50 0x4 /*!< select DDR50 */

#define SDIO_CCCR_UHS 0x14 /*!< SDIO UHS support */
#define SDIO_CCCR_UHS_SDR50 BIT(0) /*!< SDR50 support */
#define SDIO_CCCR_UHS_SDR104 BIT(1) /*!< SDR104 support */
#define SDIO_CCCR_UHS_DDR50 BIT(2) /*!< DDR50 support */

#define SDIO_CCCR_DRIVE_STRENGTH 0x15 /*!< SDIO drive strength */
#define SDIO_CCCR_DRIVE_STRENGTH_A BIT(0) /*!< drive type A */
#define SDIO_CCCR_DRIVE_STRENGTH_C BIT(1) /*!< drive type C */
#define SDIO_CCCR_DRIVE_STRENGTH_D BIT(2) /*!< drive type D */

#define SDIO_FBR_BASE(n) ((n) * 0x100) /*!< Get function base register addr */

#define SDIO_FBR_CIS 0x09 /*!< SDIO function base register CIS pointer */
#define SDIO_FBR_CSA 0x0C /*!< SDIO function base register CSA pointer */
#define SDIO_FBR_BLK_SIZE 0x10 /*!< SDIO function base register block size */


#define SDIO_MAX_IO_NUMS 7 /*!< Maximum number of I/O functions for SDIO */

#define SDIO_TPL_CODE_NULL 0x00 /*!< NULL CIS tuple code */
#define SDIO_TPL_CODE_MANIFID 0x20 /*!< manufacturer ID CIS tuple code */
#define SDIO_TPL_CODE_FUNCID 0x21 /*!< function ID CIS tuple code */
#define SDIO_TPL_CODE_FUNCE 0x22 /*!< function extension CIS tuple code */
#define SDIO_TPL_CODE_END 0xFF /*!< End CIS tuple code */

/**
 * @brief Card common control register flags
 *
 * flags to indicate capabilities supported by an SDIO card, read from the CCCR
 * registers
 */
enum sdio_cccr_flags {
	SDIO_SUPPORT_HS = BIT(0),
	SDIO_SUPPORT_SDR50 = BIT(1),
	SDIO_SUPPORT_SDR104 = BIT(2),
	SDIO_SUPPORT_DDR50 = BIT(3),
	SDIO_SUPPORT_4BIT_LS_BUS = BIT(4),
	SDIO_SUPPORT_MULTIBLOCK = BIT(5),
};

/**
 * @brief SDIO common CIS tuple properties
 *
 * CIS tuple properties. Note that additional properties exist for
 * functions 1-7, but we do not read this data as the stack does not utilize it.
 */
struct sdio_cis {
	/* Manufacturer ID string tuple */
	uint16_t manf_id; /*!< manufacturer ID */
	uint16_t manf_code; /*!< manufacturer code */
	/* Function identification tuple */
	uint8_t func_id; /*!< sdio device class function id */
	/* Function extension table */
	uint16_t max_blk_size; /*!< Max transfer block size */
	uint8_t max_speed; /*!< Max transfer speed */
	uint16_t rdy_timeout; /*!< I/O ready timeout */
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_SD_SPEC_H_ */
