/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Macro definitions
 */

/** "SDHI" in ASCII, used to determine if channel is open. */
#define SDHI_PRV_OPEN (0x53444849U)

/* Create a bitmask of access errors. */
#define SDHI_PRV_CARD_CMD_ERR (1U << 16) /* Command error */
#define SDHI_PRV_CARD_CRC_ERR (1U << 17) /* CRC error */
#define SDHI_PRV_CARD_END_ERR (1U << 18) /* End bit error */
#define SDHI_PRV_CARD_DTO     (1U << 19) /* Data Timeout */
#define SDHI_PRV_CARD_ILW     (1U << 20) /* Illegal write address */
#define SDHI_PRV_CARD_ILR     (1U << 21) /* Illegal read address */
#define SDHI_PRV_CARD_RSPT    (1U << 22) /* Response timeout */
#define SDHI_PRV_CARD_ILA_ERR (1U << 31) /* Illegal access */

#define SDHI_PRV_ACCESS_ERROR_MASK                                                                 \
	(SDHI_PRV_CARD_CMD_ERR | SDHI_PRV_CARD_CRC_ERR | SDHI_PRV_CARD_END_ERR |                   \
	 SDHI_PRV_CARD_DTO | SDHI_PRV_CARD_ILW | SDHI_PRV_CARD_ILR | SDHI_PRV_CARD_RSPT |          \
	 SDHI_PRV_CARD_ILA_ERR)

/* The clock register can be accessed 8 SD clock cycles after the last command completes. */

/* SD_INFO1 */
#define SDHI_PRV_SDHI_INFO1_RESPONSE_END       (1U << 0)  /* Response End */
#define SDHI_PRV_SDHI_INFO1_ACCESS_END         (1U << 2)  /* Access End */
#define SDHI_PRV_SDHI_INFO1_CARD_REMOVED       (1U << 3)  /* Card Removed */
#define SDHI_PRV_SDHI_INFO1_CARD_INSERTED      (1U << 4)  /* Card Inserted */
#define SDHI_PRV_SDHI_INFO1_CARD_DAT3_REMOVED  (1U << 8)  /* Card Removed */
#define SDHI_PRV_SDHI_INFO1_CARD_DAT3_INSERTED (1U << 9)  /* Card Inserted */
#define SDHI_PRV_SDHI_INFO2_CARD_CMD_ERR       (1U << 0)  /* Command error */
#define SDHI_PRV_SDHI_INFO2_CARD_CRC_ERR       (1U << 1)  /* CRC error */
#define SDHI_PRV_SDHI_INFO2_CARD_END_ERR       (1U << 2)  /* End bit error */
#define SDHI_PRV_SDHI_INFO2_CARD_DTO           (1U << 3)  /* Data Timeout */
#define SDHI_PRV_SDHI_INFO2_CARD_ILW           (1U << 4)  /* Illegal write address */
#define SDHI_PRV_SDHI_INFO2_CARD_ILR           (1U << 5)  /* Illegal read address */
#define SDHI_PRV_SDHI_INFO2_CARD_RSPT          (1U << 6)  /* Response timeout */
#define SDHI_PRV_SDHI_INFO2_CARD_BRE           (1U << 8)  /* Buffer read enable */
#define SDHI_PRV_SDHI_INFO2_CARD_BWE           (1U << 9)  /* Buffer write enable */
#define SDHI_PRV_SDHI_INFO2_CARD_ILA_ERR       (1U << 15) /* Illegal access */

#define SDHI_PRV_SDHI_INFO2_MASK                                                                   \
	((SDHI_PRV_SDHI_INFO2_CARD_CMD_ERR | SDHI_PRV_SDHI_INFO2_CARD_CRC_ERR |                    \
	  SDHI_PRV_SDHI_INFO2_CARD_END_ERR | SDHI_PRV_SDHI_INFO2_CARD_DTO |                        \
	  SDHI_PRV_SDHI_INFO2_CARD_ILW | SDHI_PRV_SDHI_INFO2_CARD_ILR |                            \
	  SDHI_PRV_SDHI_INFO2_CARD_BRE | SDHI_PRV_SDHI_INFO2_CARD_BWE |                            \
	  SDHI_PRV_SDHI_INFO2_CARD_RSPT | SDHI_PRV_SDHI_INFO2_CARD_ILA_ERR))

#define SDHI_PRV_SDHI_INFO1_ACCESS_MASK                                                            \
	((SDHI_PRV_SDHI_INFO1_RESPONSE_END | SDHI_PRV_SDHI_INFO1_ACCESS_END))
#define SDHI_PRV_SDHI_INFO1_CARD_MASK                                                              \
	((SDHI_PRV_SDHI_INFO1_CARD_REMOVED | SDHI_PRV_SDHI_INFO1_CARD_INSERTED |                   \
	  SDHI_PRV_SDHI_INFO1_CARD_DAT3_REMOVED | SDHI_PRV_SDHI_INFO1_CARD_DAT3_INSERTED))
#define SDHI_PRV_SDHI_INFO1_CARD_REMOVED_MASK                                                      \
	((SDHI_PRV_SDHI_INFO1_CARD_REMOVED | SDHI_PRV_SDHI_INFO1_CARD_DAT3_REMOVED))
#define SDHI_PRV_SDHI_INFO1_CARD_INSERTED_MASK                                                     \
	((SDHI_PRV_SDHI_INFO1_CARD_INSERTED | SDHI_PRV_SDHI_INFO1_CARD_DAT3_INSERTED))

/* Clear all masks to enable interrupts by all sources.
 * Do not set BREM or BWEM when using DMA/DTC. This driver always uses DMA or DTC.
 */
#define SDHI_PRV_SDHI_INFO2_MASK_CMD_SEND (0x00007C80U)

/* The relationship of the SD Clock Control Register SD_CLK_CTRL CLKSEL to the division of the
 * source PCLK b7            b0 1 1 1 1 1 1 1 1: PCLK 0 0 0 0 0 0 0 0: PCLK/2 0 0 0 0 0 0 0 1:
 * PCLK/4 0 0 0 0 0 0 1 0: PCLK/8 0 0 0 0 0 1 0 0: PCLK/16 0 0 0 0 1 0 0 0: PCLK/32 0 0 0 1 0 0 0 0:
 * PCLK/64 0 0 1 0 0 0 0 0: PCLK/128 0 1 0 0 0 0 0 0: PCLK/256 1 0 0 0 0 0 0 0: PCLK/512. Other
 * settings are prohibited.
 */
#define SDHI_PRV_MAX_CLOCK_DIVISION_SHIFT (9U) /* 512 (2^9) is max clock division supported */

#define SDHI_PRV_CLK_CTRL_DIV_INVALID (0xFFU)

/* Delay up to 250 ms per sector before timing out waiting for response or response timeout flag. */

/* Delay up to 10 ms before timing out waiting for response or response timeout flag. */
#define SDHI_PRV_RESPONSE_TIMEOUT_US (10000U)

/* Delay up to 5 seconds before timing out waiting for busy after updating bus width or high speed
 * status for eMMC.
 */
#define SDHI_PRV_BUSY_TIMEOUT_US (5000000U)

/* Delay up to 500 ms before timing out waiting for data or data timeout flag. */
#define SDHI_PRV_DATA_TIMEOUT_US (500000U)

/* Delay up to 100 ms before timing out waiting for access end flag after receiving data during
 * initialization.
 */
#define SDHI_PRV_ACCESS_TIMEOUT_US (100000U)

/* 400 kHz maximum clock required for initialization. */
#define SDHI_PRV_INIT_MAX_CLOCK_RATE_HZ  (400000U)
#define SDHI_PRV_BITS_PER_COMMAD         (48U)
#define SDHI_PRV_BITS_PER_RESPONSE       (48U)
#define SDHI_PRV_CLOCKS_BETWEEN_COMMANDS (8U)
#define SDHI_PRV_MIN_CYCLES_PER_COMMAND_RESPONSE                                                   \
	((SDHI_PRV_BITS_PER_COMMAD + SDHI_PRV_BITS_PER_RESPONSE) + SDHI_PRV_CLOCKS_BETWEEN_COMMANDS)
#define SDHI_PRV_INIT_ONE_SECOND_TIMEOUT_ITERATIONS                                                \
	(SDHI_PRV_INIT_MAX_CLOCK_RATE_HZ / SDHI_PRV_MIN_CYCLES_PER_COMMAND_RESPONSE)

#define SDHI_PRV_SDIO_REG_HIGH_SPEED (0x13U) SDIO High Speed register address
#define SDHI_PRV_SDIO_REG_HIGH_SPEED_BIT_EHS                                                       \
	(1U << 1) Enable high speed bit of SDIO high speed register
#define SDHI_PRV_SDIO_REG_HIGH_SPEED_BIT_SHS                                                       \
	(1U << 0) Support high speed bit of SDIO high speed register
#define SDHI_PRV_CSD_REG_CCC_CLASS_10_BIT                                                          \
	((1U << 10)) CCC_CLASS bit is set if the card supports high speed

/* SDIO maximum bytes allows in writeIoExt() and readIoExt(). */
#define SDHI_PRV_SDIO_EXT_MAX_BYTES (512U)

/* SDIO maximum blocks allows in writeIoExt() and readIoExt(). */
#define SDHI_PRV_SDIO_EXT_MAX_BLOCKS (511U)

/* Masks for CMD53 argument. */
#define SDHI_PRV_SDIO_CMD52_CMD53_COUNT_MASK    (0x1FFU)
#define SDHI_PRV_SDIO_CMD52_CMD53_FUNCTION_MASK (0x7U)
#define SDHI_PRV_SDIO_CMD52_CMD53_ADDRESS_MASK  (0x1FFFFU)

/* Startup delay in milliseconds. */

#define SDHI_PRV_SD_OPTION_DEFAULT    (0x40E0U)
#define SDHI_PRV_SD_OPTION_WIDTH8_BIT (13)

#define SDHI_PRV_BUS_WIDTH_1_BIT (4U)

#define SDHI_PRV_SDIO_INFO1_MASK_IRQ_DISABLE       (0xC006U)
#define SDHI_PRV_SDIO_INFO1_IRQ_CLEAR              (0xFFFF3FFEU)
#define SDHI_PRV_SDIO_INFO1_TRANSFER_COMPLETE_MASK (0xC000)
#define SDHI_PRV_SD_INFO2_MASK_BREM_BWEM_MASK      (0x300U)
#define SDHI_PRV_EMMC_BUS_WIDTH_INDEX              (183U)
#define SDHI_PRV_BYTES_PER_KILOBYTE                (1024)
#define SDHI_PRV_SECTOR_COUNT_IN_EXT_CSD           (0xFFFU)
#define SDHI_PRV_SD_CLK_CTRL_DEFAULT               (0x20U)

#define SDHI_PRV_SD_INFO2_CBSY_SDD0MON_IDLE_MASK (0x4080)
#define SDHI_PRV_SD_INFO2_CBSY_SDD0MON_IDLE_VAL  (0x80)
#define SDHI_PRV_SD_CLK_CTRLEN_TIMEOUT           (8U * 512U)
#define SDHI_PRV_SD_INFO1_MASK_MASK_ALL          (0x31DU)
#define SDHI_PRV_SD_INFO1_MASK_CD_ENABLE         (0x305U)
#define SDHI_PRV_SD_STOP_SD_SECCNT_ENABLE        (0x100U)
#define SDHI_PRV_SD_DMAEN_DMAEN_SET              (0x2U)

#define SDHI_PRV_SDHI_PRV_SD_CLK_CTRL_CLKCTRLEN_MASK    (1U << 9)
#define SDHI_PRV_SDHI_PRV_SD_CLK_CTRL_CLKEN_MASK        (1U << 8)
#define SDHI_PRV_SDHI_PRV_SD_CLK_AUTO_CLOCK_ENABLE_MASK (0x300U)

#define SDHI_PRV_ACCESS_BIT   (2U)
#define SDHI_PRV_RESPONSE_BIT (0U)

struct sdmmc_ra_event {
	volatile bool transfer_completed;
	volatile bool transfer_error;
};
