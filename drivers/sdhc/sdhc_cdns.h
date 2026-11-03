/*
 * Copyright (C) 2023 Intel Corporation
 * Copyright (C) 2026 Altera Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SDHC_SDHC_CDNS_H
#define ZEPHYR_DRIVERS_SDHC_SDHC_CDNS_H

#include <stdio.h>

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include "sdhc_standard_core.h"

/*
 * The SRS slot registers follow the SD Host Controller specification: their
 * offsets and version 3.00 fields come from sdhc_standard_core.h. Only what
 * the standard core does not describe is defined here.
 */

/* Host Control 2 fields of the specification version 4.00 */
#define SDHC_CDNS_HOST_CTRL2_HV4E BIT(12) /* Host Version 4 Enable */
#define SDHC_CDNS_HOST_CTRL2_A64B BIT(13) /* 64-bit Addressing     */

/* Error Interrupt field of the specification version 4.10 */
#define SDHC_CDNS_ERROR_INT_ERSP BIT(11) /* Response Error */

/*
 * Transfer Mode and Command share one 32-bit word and writing the Command
 * half issues the command, so both are written at once.
 */
#define SDHC_CDNS_COMMAND_SHIFT 16

/* Response type, CRC check and index check encoding of the Command register */
#define SDHC_CDNS_RESP_NONE SDHC_REG_COMMAND_RTS_NONE

#define SDHC_CDNS_RESP_R1B                                                                         \
	(SDHC_REG_COMMAND_RTS_48_BUSY | SDHC_REG_COMMAND_CRCCE_MASK | SDHC_REG_COMMAND_CICE_MASK)

#define SDHC_CDNS_RESP_R1                                                                          \
	(SDHC_REG_COMMAND_RTS_48 | SDHC_REG_COMMAND_CRCCE_MASK | SDHC_REG_COMMAND_CICE_MASK)

#define SDHC_CDNS_RESP_R2 (SDHC_REG_COMMAND_RTS_136 | SDHC_REG_COMMAND_CRCCE_MASK)

#define SDHC_CDNS_RESP_R3 SDHC_REG_COMMAND_RTS_48

#define SDHC_CDNS_RESP_R6                                                                          \
	(SDHC_REG_COMMAND_RTS_48 | SDHC_REG_COMMAND_CRCCE_MASK | SDHC_REG_COMMAND_CICE_MASK)

/* Bit map to update response type */
#define SDHC_CDNS_CRC_LEFT_SHIFT  0x8U
#define SDHC_CDNS_CRC_RIGHT_SHIFT 0x18U

#define SDHC_CDNS_BLOCK_SIZE_512 0x200U

/* Data Timeout Counter Value: TMCLK * 2^27 */
#define SDHC_CDNS_TIMEOUT_CTRL_DTCV 0x0EU

/* Voltages of the bus IO lines supply */
#define SDHC_CDNS_VQMMC_3_3_UV 3300000
#define SDHC_CDNS_VQMMC_3_0_UV 3000000
#define SDHC_CDNS_VQMMC_1_8_UV 1800000

/* Time for the signalling level to settle after a switch, in ms */
#define SDHC_CDNS_SIGNAL_SETTLE_TIME_MS 5U
/* Time for the bus IO lines supply to be stable at 1.8 V, in ms */
#define SDHC_CDNS_1_8_REG_STABLE_TIME_MS 200U

/* Bit map for ADMA2 attribute */
#define SDHC_CDNS_ADMA2_DESC_TRAN  BIT(5)
#define SDHC_CDNS_ADMA2_DESC_END   BIT(1)
#define SDHC_CDNS_ADMA2_DESC_VALID BIT(0)

/*
 * The normal and error interrupt registers are accessed in pairs, as one
 * 32-bit word at the offset of the normal interrupt register: normal
 * interrupts in bits 15:0, error interrupts in bits 31:16. The status bits
 * are write-1-to-clear and the 16-bit accessors of the standard core
 * read-modify-write the whole word, which would clear the other half.
 */
#define SDHC_CDNS_INT_ERROR_SHIFT 16

#define SDHC_CDNS_INT_CC   SDHC_REG_NORMAL_INT_CC_MASK   /* Command Complete  */
#define SDHC_CDNS_INT_TC   SDHC_REG_NORMAL_INT_TC_MASK   /* Transfer Complete */
#define SDHC_CDNS_INT_EINT SDHC_REG_NORMAL_INT_EINT_MASK /* Error Interrupt   */

/* Error interrupts the driver enables */
#define SDHC_CDNS_ERR_INT_MASK                                                                     \
	((uint32_t)(SDHC_CDNS_ERROR_INT_ERSP | SDHC_REG_ERROR_INT_ADMAE_MASK |                     \
		    SDHC_REG_ERROR_INT_ACE_MASK | SDHC_REG_ERROR_INT_CLE_MASK |                    \
		    SDHC_REG_ERROR_INT_DEB_MASK | SDHC_REG_ERROR_INT_DCRC_MASK |                   \
		    SDHC_REG_ERROR_INT_DTE_MASK | SDHC_REG_ERROR_INT_CIE_MASK |                    \
		    SDHC_REG_ERROR_INT_CEB_MASK | SDHC_REG_ERROR_INT_CCRC_MASK |                   \
		    SDHC_REG_ERROR_INT_CTE_MASK)                                                   \
	 << SDHC_CDNS_INT_ERROR_SHIFT)

/* Interrupts the driver handles: command complete, transfer complete, errors */
#define SDHC_CDNS_TXFR_INTR_EN_MASK                                                                \
	(SDHC_CDNS_INT_CC | SDHC_CDNS_INT_TC | SDHC_CDNS_ERR_INT_MASK)

/* Host Version 4 requires an 8-byte aligned ADMA2 descriptor table */
#if defined(CONFIG_DCACHE_LINE_SIZE) && (CONFIG_DCACHE_LINE_SIZE > 8)
#define SDHC_CDNS_DESC_ALIGN CONFIG_DCACHE_LINE_SIZE
#else
#define SDHC_CDNS_DESC_ALIGN 8
#endif

/**
 * @brief ADMA2 descriptor table structure (Host Version 4).
 *
 * 128-bit descriptor with 64-bit addressing, 64-bit descriptor with 32-bit
 * addressing.
 */
#ifdef CONFIG_64BIT
typedef struct {
	/**< attrs of descriptor */
	uint16_t attr;
	/**< Length of current dma transfer max 64kb */
	uint16_t len;
	/**< source/destination addr for current dma transfer */
	uint64_t addr;
	/**< reserved, must be zero */
	uint32_t reserved;
} __packed adma2_descriptor;
#else
typedef struct {
	/**< attrs of descriptor */
	uint16_t attr;
	/**< Length of current dma transfer max 64kb */
	uint16_t len;
	/**< source/destination addr for current dma transfer */
	uint32_t addr;
} __packed adma2_descriptor;
#endif

/**
 * @brief Holds device private data.
 */
struct sdhc_cdns_data {
	/**< Standard host data, mapping of the slot registers set; must be first */
	struct sdhc_standard_host_common_data standard;
	/**< Mapped address of the host registers set */
	mm_reg_t host_base;
	/**< Current I/O settings of SDHC */
	struct sdhc_io host_io;
	/**< SDHC IRQ events */
	struct k_event irq_event;
	/**< transfer mode and data direction (Transfer Mode register) */
	uint16_t transfermode;
	/**< SD host controller timing mode */
	uint32_t timing_mode;
	/**< Bounce buffer for small DMA reads */
	uint8_t read_bounce_buffer[CONFIG_SDHC_BUFFER_ALIGNMENT]
		__aligned(CONFIG_SDHC_BUFFER_ALIGNMENT);
	/**< ADMA descriptor table */
	adma2_descriptor adma2_desc[MAX(1, CONFIG_SDHC_CDNS_DESC_SIZE)]
		__aligned(SDHC_CDNS_DESC_ALIGN);
};

/**
 * @brief Holds SDHC configuration data.
 */
struct sdhc_cdns_config {
	/**< Standard host config, slot registers set and sdhc.yaml properties; must be first */
	struct sdhc_standard_host_common_config standard;
	/**< Physical address of the host registers set */
	uintptr_t host_phys;
	/**< Size of the host registers set */
	size_t host_size;
	/**< Pointer to the device structure representing the clock bus */
	const struct device *clock_dev;
	/**< device structure representing the clock ID */
	clock_control_subsys_t clock_id;
	/**< Callback to the device interrupt configuration api */
	void (*irq_config_fn)(const struct device *dev);
	/**< Reset lines toggled at init, in devicetree order */
	const struct reset_dt_spec *resets;
	/**< Number of entries in resets */
	size_t num_resets;
	/**< Card detection pin available or not */
	bool broken_cd;
	/**< Supply of the bus IO lines, NULL if the board does not switch it */
	const struct device *vqmmc;
	/**< Indicates if driver  has a separate "host" MMIO region. */
	bool has_host;
	/* Cadence specific quirks */
	const struct sdhc_cdns_quirks *const quirks;
	/* Cadence specific quirks config */
	const void *quirk_config;
};

/* Vendor quirks per driver instance */
struct sdhc_cdns_quirks {
	/* One-time PHY bring-up at device init, before any mode is selected. */
	int (*phy_init)(const struct device *dev);
	/* Apply PHY/HRS tuning for a negotiated SDHC_TIMING_* mode. */
	int (*phy_config)(const struct device *dev);
};

/* clang-format off */
#define SDHC_CDNS_QUIRK_CONFIG(dev)                                                                \
	(((const struct sdhc_cdns_config *)(dev)->config)->quirk_config)

#if DT_HAS_COMPAT_STATUS_OKAY(cdns_sd6hc)
#include "sdhc_cdns6.h"
#endif

#define SDHC_CDNS_HAS_QUIRK(n)                                                                     \
	DT_NODE_VENDOR_HAS_IDX(DT_DRV_INST(n), 1)

#define SDHC_CDNS_QUIRK_GET(n) COND_CODE_1(SDHC_CDNS_HAS_QUIRK(n),                                 \
			(&sdhc_cdns_quirks_##n),                                                   \
			(NULL))

#define SDHC_CDNS_QUIRK_CONFIG_GET(n)                                                              \
	COND_CODE_1(SDHC_CDNS_HAS_QUIRK(n),                                                        \
			(&sdhc_cdns_quirk_config_##n),                                             \
			(NULL))

#define CDNS_QUIRK_FUNC_DEFINE(fname)                                                              \
	static inline int sdhc_cdns_quirk_##fname(const struct device *dev)                        \
	{                                                                                          \
		const struct sdhc_cdns_config *const config = dev->config;                         \
		const struct sdhc_cdns_quirks *const quirks = config->quirks;                      \
	                                                                                           \
		if (quirks != NULL && quirks->fname != NULL) {                                     \
			return config->quirks->fname(dev);                                         \
		}                                                                                  \
		return 0;                                                                          \
	}

CDNS_QUIRK_FUNC_DEFINE(phy_init)
CDNS_QUIRK_FUNC_DEFINE(phy_config)
/* clang-format on */

#endif /* ZEPHYR_DRIVERS_SDHC_SDHC_CDNS_H */
