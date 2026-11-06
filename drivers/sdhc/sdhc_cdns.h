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
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

/*
 * SRS slot registers. Only the registers and fields the driver programs
 * are listed.
 */

/* SRS01 - Block Size / Block Count */
#define SDHC_CDNS_BLOCK_SIZE     0x04
#define SDHC_CDNS_BLOCK_SIZE_512 0x200U

#define SDHC_CDNS_BLOCK_SIZE_BCCT_POS  16
#define SDHC_CDNS_BLOCK_SIZE_BCCT_MASK GENMASK(31, 16) /* Block Count for Current Transfer */

/* SRS02 - Argument1 */
#define SDHC_CDNS_ARGUMENT 0x08

/* SRS03 - Command / Transfer Mode */
#define SDHC_CDNS_XFER_MODE 0x0C

#define SDHC_CDNS_XFER_MODE_CIDX_POS  24
#define SDHC_CDNS_XFER_MODE_CIDX_MASK GENMASK(29, 24) /* Command Index             */
#define SDHC_CDNS_XFER_MODE_DPS       BIT(21)         /* Data Present Select       */
#define SDHC_CDNS_XFER_MODE_CICE      BIT(20)         /* Cmd Index Check Enable    */
#define SDHC_CDNS_XFER_MODE_CRCCE     BIT(19)         /* Cmd CRC Check Enable      */
#define SDHC_CDNS_XFER_MODE_RTS_MASK  GENMASK(17, 16) /* Response Type Select      */
#define SDHC_CDNS_CMD_RESP_L48_MASK   BIT(17)         /* Response length 48        */
#define SDHC_CDNS_CMD_RESP_L136_MASK  BIT(16)         /* Response length 136       */
#define SDHC_CDNS_XFER_MODE_MSBS      BIT(5)          /* Multi/Single Block Select */
#define SDHC_CDNS_XFER_MODE_DTDS      BIT(4)          /* Data Transfer Dir Select  */
#define SDHC_CDNS_XFER_MODE_CMD12_EN  BIT(2)          /* Auto CMD12 Enable         */
#define SDHC_CDNS_XFER_MODE_BCE       BIT(1)          /* Block Count Enable        */
#define SDHC_CDNS_XFER_MODE_DMAE      BIT(0)          /* DMA Enable                */

/* Bit map for Transfer Mode */
#define SDHC_CDNS_RESP_NONE 0

#define SDHC_CDNS_RESP_R1B                                                                        \
	SDHC_CDNS_XFER_MODE_RTS_MASK | SDHC_CDNS_XFER_MODE_CRCCE | SDHC_CDNS_XFER_MODE_CICE

#define SDHC_CDNS_RESP_R1                                                                         \
	SDHC_CDNS_CMD_RESP_L48_MASK | SDHC_CDNS_XFER_MODE_CRCCE | SDHC_CDNS_XFER_MODE_CICE

#define SDHC_CDNS_RESP_R2 SDHC_CDNS_CMD_RESP_L136_MASK | SDHC_CDNS_XFER_MODE_CRCCE

#define SDHC_CDNS_RESP_R3 SDHC_CDNS_CMD_RESP_L48_MASK

#define SDHC_CDNS_RESP_R6                                                                         \
	SDHC_CDNS_CMD_RESP_L48_MASK | SDHC_CDNS_XFER_MODE_CRCCE | SDHC_CDNS_XFER_MODE_CICE

/* SRS04..SRS07 - Response 0..3 (read-only) */
#define SDHC_CDNS_RESPONSE0 0x10
#define SDHC_CDNS_RESPONSE1 0x14
#define SDHC_CDNS_RESPONSE2 0x18
#define SDHC_CDNS_RESPONSE3 0x1C

/* Bit map to update response type */
#define SDHC_CDNS_CRC_LEFT_SHIFT  0x8U
#define SDHC_CDNS_CRC_RIGHT_SHIFT 0x18U

/* SRS09 - Present State */
#define SDHC_CDNS_PRESENT_STATE 0x24

#define SDHC_CDNS_PRESENT_STATE_DATSL1_D0 BIT(20) /* DAT[0] Line Signal Level */
#define SDHC_CDNS_PRESENT_STATE_CI        BIT(16) /* Card Inserted            */
#define SDHC_CDNS_PRESENT_STATE_CIDAT     BIT(1)  /* Cmd Inhibit (DAT)        */
#define SDHC_CDNS_PRESENT_STATE_CICMD     BIT(0)  /* Cmd Inhibit (CMD)        */

/* SRS10 - Host Control 1 (General / Power / Block-Gap / Wake-Up) */
#define SDHC_CDNS_HOST_CTRL 0x28

#define SDHC_CDNS_HOST_CTRL_BVS_POS     9
#define SDHC_CDNS_HOST_CTRL_BVS_MASK    GENMASK(11, 9) /* Bus Voltage Select       */
#define SDHC_CDNS_HOST_CTRL_BP          BIT(8)         /* Bus Power                */
#define SDHC_CDNS_HOST_CTRL_EDTW        BIT(5)         /* Extended Data Xfer Width */
#define SDHC_CDNS_HOST_CTRL_DMASEL_POS  3
#define SDHC_CDNS_HOST_CTRL_DMASEL_MASK GENMASK(4, 3)  /* DMA Select               */
#define SDHC_CDNS_HOST_CTRL_HSE         BIT(2)         /* High Speed Enable        */
#define SDHC_CDNS_HOST_CTRL_DTW         BIT(1)         /* Data Transfer Width      */

/* Bit map for Host Control 1 Bus Voltage Select */
#define SDHC_CDNS_HOST_VOL_3_3_V_SELECT (0x7U << SDHC_CDNS_HOST_CTRL_BVS_POS)
#define SDHC_CDNS_HOST_VOL_3_0_V_SELECT (0x6U << SDHC_CDNS_HOST_CTRL_BVS_POS)
#define SDHC_CDNS_HOST_VOL_1_8_V_SELECT (0x5U << SDHC_CDNS_HOST_CTRL_BVS_POS)

/* Bit map for Host Control 1 DMA Select */
#define SDHC_CDNS_HOST_CTRL_DMASEL_ADMA2_32 (0x2 << SDHC_CDNS_HOST_CTRL_DMASEL_POS)

/* Bit map for ADMA2 attribute */
#define SDHC_CDNS_ADMA2_DESC_TRAN  BIT(5)
#define SDHC_CDNS_ADMA2_DESC_END   BIT(1)
#define SDHC_CDNS_ADMA2_DESC_VALID BIT(0)

/* SRS11 - Clock Control / Timeout Control / Software Reset */
#define SDHC_CDNS_CLOCK_CTRL 0x2C

#define SDHC_CDNS_CLOCK_CTRL_SRFA       BIT(24)         /* Software Reset for All     */
#define SDHC_CDNS_CLOCK_CTRL_DTCV_POS   16
#define SDHC_CDNS_CLOCK_CTRL_DTCV_MASK  GENMASK(19, 16) /* Data Timeout Counter Value */
#define SDHC_CDNS_CLOCK_CTRL_SDCFSL_POS 8               /* SDCLK Freq Select (low)    */
#define SDHC_CDNS_CLOCK_CTRL_SDCFSH_POS 6               /* SDCLK Freq Select (high)   */
#define SDHC_CDNS_CLOCK_CTRL_SDCE       BIT(2)          /* SD Clock Enable            */
#define SDHC_CDNS_CLOCK_CTRL_ICS        BIT(1)          /* Internal Clock Stable      */
#define SDHC_CDNS_CLOCK_CTRL_ICE        BIT(0)          /* Internal Clock Enable      */

#define SDHC_CDNS_CLOCK_CTRL_DTCV 0x0E

/* SRS12 - Error / Normal Interrupt Status */
#define SDHC_CDNS_INT_STATUS 0x30

#define SDHC_CDNS_INT_STATUS_EINT BIT(15) /* Error Interrupt   */
#define SDHC_CDNS_INT_STATUS_TC   BIT(1)  /* Transfer Complete */
#define SDHC_CDNS_INT_STATUS_CC   BIT(0)  /* Command Complete  */

/* SRS13 - Error / Normal Interrupt Status Enable */
#define SDHC_CDNS_INT_ENABLE 0x34

#define SDHC_CDNS_INT_ENABLE_ERSP_SE  BIT(27) /* Error Response        */
#define SDHC_CDNS_INT_ENABLE_EADMA_SE BIT(25) /* ADMA Error            */
#define SDHC_CDNS_INT_ENABLE_EAC_SE   BIT(24) /* Auto CMD Error        */
#define SDHC_CDNS_INT_ENABLE_ECL_SE   BIT(23) /* Current Limit Error   */
#define SDHC_CDNS_INT_ENABLE_EDEB_SE  BIT(22) /* Data End Bit Error    */
#define SDHC_CDNS_INT_ENABLE_EDCRC_SE BIT(21) /* Data CRC Error        */
#define SDHC_CDNS_INT_ENABLE_EDT_SE   BIT(20) /* Data Timeout Error    */
#define SDHC_CDNS_INT_ENABLE_ECI_SE   BIT(19) /* Command Index Error   */
#define SDHC_CDNS_INT_ENABLE_ECEB_SE  BIT(18) /* Command End Bit Error */
#define SDHC_CDNS_INT_ENABLE_ECCRC_SE BIT(17) /* Command CRC Error     */
#define SDHC_CDNS_INT_ENABLE_ECT_SE   BIT(16) /* Command Timeout Error */

/* SRS14 - Error / Normal Interrupt Signal Enable */
#define SDHC_CDNS_INT_SIGNAL_ENABLE 0x38

/* SRS15 - Auto CMD Error Status / Host Control 2 */
#define SDHC_CDNS_AUTO_CMD_HOST_CTRL2 0x3C

#define SDHC_CDNS_HOST_CTRL2_A64B     BIT(29)         /* 64-bit Addressing     */
#define SDHC_CDNS_HOST_CTRL2_HV4E     BIT(28)         /* Host Version 4 Enable */
#define SDHC_CDNS_HOST_CTRL2_V18SE    BIT(19)         /* 1.8V Signaling Enable */
#define SDHC_CDNS_HOST_CTRL2_UMS_POS  16
#define SDHC_CDNS_HOST_CTRL2_UMS_MASK GENMASK(18, 16) /* UHS Mode Select       */

/* Bit map for Host Control 2 UHS Mode Select */
#define SDHC_CDNS_UHS_SPEED_MODE_SDR25 (0x1U << SDHC_CDNS_HOST_CTRL2_UMS_POS)
#define SDHC_CDNS_UHS_SPEED_MODE_SDR12 (0x0U << SDHC_CDNS_HOST_CTRL2_UMS_POS)

/* SRS16 - Capabilities */
#define SDHC_CDNS_CAPS1 0x40

#define SDHC_CDNS_CAPS1_VS18         BIT(26)         /* Voltage Support 1.8V      */
#define SDHC_CDNS_CAPS1_VS30         BIT(25)         /* Voltage Support 3.0V      */
#define SDHC_CDNS_CAPS1_VS33         BIT(24)         /* Voltage Support 3.3V      */
#define SDHC_CDNS_CAPS1_DMAS         BIT(22)         /* SDMA Support              */
#define SDHC_CDNS_CAPS1_HSS          BIT(21)         /* High Speed Support        */
#define SDHC_CDNS_CAPS1_ADMA2S       BIT(19)         /* ADMA2 Support             */
#define SDHC_CDNS_CAPS1_EDS8         BIT(18)         /* 8-bit Embedded Support    */
#define SDHC_CDNS_CAPS1_MBL_POS      16
#define SDHC_CDNS_CAPS1_MBL_MASK     GENMASK(17, 16) /* Max Block Length          */
#define SDHC_CDNS_CAPS1_BCSDCLK_POS  8
#define SDHC_CDNS_CAPS1_BCSDCLK_MASK GENMASK(15, 8)  /* Base Clock Freq for SDCLK */

/* SRS18 - Max Current 1 Capabilities */
#define SDHC_CDNS_MAX_CURRENT1 0x48

#define SDHC_CDNS_MAX_CURRENT1_MC18_POS  16
#define SDHC_CDNS_MAX_CURRENT1_MC18_MASK GENMASK(23, 16) /* Max Current for 1.8V */
#define SDHC_CDNS_MAX_CURRENT1_MC30_POS  8
#define SDHC_CDNS_MAX_CURRENT1_MC30_MASK GENMASK(15, 8)  /* Max Current for 3.0V */
#define SDHC_CDNS_MAX_CURRENT1_MC33_POS  0
#define SDHC_CDNS_MAX_CURRENT1_MC33_MASK GENMASK(7, 0)   /* Max Current for 3.3V */

/* SRS22, SRS23 - ADMA System Address */
#define SDHC_CDNS_ADMA_SYS_ADDR1 0x58
#define SDHC_CDNS_ADMA_SYS_ADDR2 0x5C

/* Error interrupts enabled in SRS13 */
#define SDHC_CDNS_ERR_INT_MASK                                                                     \
	(SDHC_CDNS_INT_ENABLE_ERSP_SE | SDHC_CDNS_INT_ENABLE_EADMA_SE |                            \
	 SDHC_CDNS_INT_ENABLE_EAC_SE | SDHC_CDNS_INT_ENABLE_ECL_SE |                               \
	 SDHC_CDNS_INT_ENABLE_EDEB_SE | SDHC_CDNS_INT_ENABLE_EDCRC_SE |                            \
	 SDHC_CDNS_INT_ENABLE_EDT_SE | SDHC_CDNS_INT_ENABLE_ECI_SE |                               \
	 SDHC_CDNS_INT_ENABLE_ECEB_SE | SDHC_CDNS_INT_ENABLE_ECCRC_SE |                            \
	 SDHC_CDNS_INT_ENABLE_ECT_SE)

/* Interrupts the driver handles: command complete, transfer complete, errors */
#define SDHC_CDNS_TXFR_INTR_EN_MASK                                                                \
	(SDHC_CDNS_INT_STATUS_CC | SDHC_CDNS_INT_STATUS_TC | SDHC_CDNS_ERR_INT_MASK)

/* Host Version 4 requires an 8-byte aligned ADMA2 descriptor table */
#if defined(CONFIG_DCACHE_LINE_SIZE) && (CONFIG_DCACHE_LINE_SIZE > 8)
#define SDHC_CDNS_DESC_ALIGN CONFIG_DCACHE_LINE_SIZE
#else
#define SDHC_CDNS_DESC_ALIGN 8
#endif

/* Required by DEVICE_MMIO_NAMED_* macros */
#define DEV_CFG(dev)  ((const struct sdhc_cdns_config *)(dev)->config)
#define DEV_DATA(dev) ((struct sdhc_cdns_data *)(dev)->data)

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
	/* MMIO mapping information for SDHC slot registers set */
	DEVICE_MMIO_NAMED_RAM(slot);
	/* MMIO mapping information for SDHC host registers set */
	DEVICE_MMIO_NAMED_RAM(host);
	/**< Current I/O settings of SDHC */
	struct sdhc_io host_io;
	/**< Supported properties of SDHC */
	struct sdhc_host_props props;
	/**< SDHC IRQ events */
	struct k_event irq_event;
	/**< transfer mode and data direction (lower 16 bits of SDHC_CDNS_XFER_MODE) */
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
	/* MMIO mapping information for SDHC slot registers set */
	DEVICE_MMIO_NAMED_ROM(slot);
	/* MMIO mapping information for SDHC host registers set */
	DEVICE_MMIO_NAMED_ROM(host);
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
	/**< delay given to card to power up or down fully */
	uint16_t powerdelay;
	/**< 1.8V card voltage not supported by the board */
	bool no_1_8_v;
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
