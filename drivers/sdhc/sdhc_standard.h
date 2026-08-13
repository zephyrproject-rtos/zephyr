/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SDHC_SDHC_STANDARD_H_
#define ZEPHYR_DRIVERS_SDHC_SDHC_STANDARD_H_

#include <zephyr/drivers/sdhc.h>

/**
 * @name SD host standard registers
 *
 * SD Specifications Part A2
 * SD Host Controller Simplified Specification
 * Version 3.00
 * @{
 */
#define SDHC_REG_SDMA_SYS_ADDR_ARG2     0x000 /**< SDMA System Address / Argument 2 Register */
#define SDHC_REG_BLOCK_SIZE             0x004 /**< Block Size Register */
#define SDHC_REG_BLOCK_COUNT            0x006 /**< Block Count Register */
#define SDHC_REG_ARGUMENT1              0x008 /**< Argument 1 Register */
#define SDHC_REG_TRANSFER_MODE          0x00C /**< Transfer Mode Register */
#define SDHC_REG_COMMAND                0x00E /**< Command Register */
#define SDHC_REG_RESPONSE               0x010 /**< Response Register */
#define SDHC_REG_BUF_DATA_PORT          0x020 /**< Buffer Data Port Register */
#define SDHC_REG_PRESENT_STATE          0x024 /**< Present State Register */
#define SDHC_REG_HOST_CTRL1             0x028 /**< Host Control 1 Register */
#define SDHC_REG_POWER_CTRL             0x029 /**< Power Control Register */
#define SDHC_REG_BLOCK_GAP_CTRL         0x02A /**< Block Gap Control Register */
#define SDHC_REG_WAKEUP_CTRL            0x02B /**< Wakeup Control Register */
#define SDHC_REG_CLOCK_CTRL             0x02C /**< Clock Control Register */
#define SDHC_REG_TIMEOUT_CTRL           0x02E /**< Timeout Control Register */
#define SDHC_REG_SW_RESET               0x02F /**< Software Reset Register */
#define SDHC_REG_NORMAL_INT_STAT        0x030 /**< Normal Interrupt Status Register */
#define SDHC_REG_ERROR_INT_STAT         0x032 /**< Error Interrupt Status Register */
#define SDHC_REG_NORMAL_INT_STAT_EN     0x034 /**< Normal Interrupt Status Enable Register */
#define SDHC_REG_ERROR_INT_STAT_EN      0x036 /**< Error Interrupt Status Enable Register */
#define SDHC_REG_NORMAL_INT_SIG_EN      0x038 /**< Normal Interrupt Signal Enable Register */
#define SDHC_REG_ERROR_INT_SIG_EN       0x03A /**< Error Interrupt Signal Enable Register */
#define SDHC_REG_AUTO_CMD_ERR_STAT      0x03C /**< Auto CMD Error Status Register */
#define SDHC_REG_HOST_CTRL2             0x03E /**< Host Control 2 Register */
#define SDHC_REG_CAPABILITIES           0x040 /**< Capabilities Register */
#define SDHC_REG_MAX_CURRENT_CAP        0x048 /**< Maximum Current Capabilities Register */
#define SDHC_REG_FORCE_EVT_AUTO_CMD_ERR 0x050 /**< Force Event Register - Auto CMD Error Status */
#define SDHC_REG_FORCE_EVT_ERR_INT_STAT 0x052 /**< Force Event Register - Error Interrupt Status */
#define SDHC_REG_ADMA_ERR_STAT          0x054 /**< ADMA Error Status Register */
#define SDHC_REG_ADMA_SYS_ADDR          0x058 /**< ADMA System Address Register */
#define SDHC_REG_PRESET_INIT            0x060 /**< Preset Value for Initialization */
#define SDHC_REG_PRESET_DEFAULT         0x062 /**< Preset Value for Default Speed */
#define SDHC_REG_PRESET_HIGH_SPEED      0x064 /**< Preset Value for High Speed */
#define SDHC_REG_PRESET_SDR12           0x066 /**< Preset Value for SDR12 */
#define SDHC_REG_PRESET_SDR25           0x068 /**< Preset Value for SDR25 */
#define SDHC_REG_PRESET_SDR50           0x06A /**< Preset Value for SDR50 */
#define SDHC_REG_PRESET_SDR104          0x06C /**< Preset Value for SDR104 */
#define SDHC_REG_PRESET_DDR50           0x06E /**< Preset Value for DDR50 */
#define SDHC_REG_SHARED_BUS_CTRL        0x0E0 /**< Shared Bus Control Register */
#define SDHC_REG_SLOT_INT_STAT          0x0FC /**< Slot Interrupt Status Register */
#define SDHC_REG_HOST_CTRL_VER          0x0FE /**< Host Controller Version Register */
/** @} */

/**
 * @name Capabilities Register Bits
 * @{
 */
#define SDHC_REG_CAPABILITIES_TCF_MASK     0x3f
#define SDHC_REG_CAPABILITIES_TCF_SHIFT    0
#define SDHC_REG_CAPABILITIES_TCU_MASK     BIT64(7)
#define SDHC_REG_CAPABILITIES_BCF_MASK     (0xffULL << 8)
#define SDHC_REG_CAPABILITIES_BCF_SHIFT    8
#define SDHC_REG_CAPABILITIES_MBL_MASK     (0x3ULL << 16)
#define SDHC_REG_CAPABILITIES_MBL_SHIFT    16
#define SDHC_REG_CAPABILITIES_8B_MASK      BIT64(18)
#define SDHC_REG_CAPABILITIES_ADMA2_MASK   BIT64(19)
#define SDHC_REG_CAPABILITIES_HSS_MASK     BIT64(21)
#define SDHC_REG_CAPABILITIES_SDMA_MASK    BIT64(22)
#define SDHC_REG_CAPABILITIES_SRS_MASK     BIT64(23)
#define SDHC_REG_CAPABILITIES_VS33_MASK    BIT64(24)
#define SDHC_REG_CAPABILITIES_VS30_MASK    BIT64(25)
#define SDHC_REG_CAPABILITIES_VS18_MASK    BIT64(26)
#define SDHC_REG_CAPABILITIES_64B_MASK     BIT64(28)
#define SDHC_REG_CAPABILITIES_AIS_MASK     BIT64(29)
#define SDHC_REG_CAPABILITIES_ST_MASK      (0x3ULL << 30)
#define SDHC_REG_CAPABILITIES_ST_SHIFT     30
#define SDHC_REG_CAPABILITIES_SDR50_MASK   BIT64(32)
#define SDHC_REG_CAPABILITIES_SDR104_MASK  BIT64(33)
#define SDHC_REG_CAPABILITIES_DDR50_MASK   BIT64(34)
#define SDHC_REG_CAPABILITIES_DTA_MASK     BIT64(36)
#define SDHC_REG_CAPABILITIES_DTC_MASK     BIT64(37)
#define SDHC_REG_CAPABILITIES_DTD_MASK     BIT64(38)
#define SDHC_REG_CAPABILITIES_TCRT_MASK    (0xfULL << 40)
#define SDHC_REG_CAPABILITIES_TCRT_SHIFT   40
#define SDHC_REG_CAPABILITIES_UTSDR50_MASK BIT64(45)
#define SDHC_REG_CAPABILITIES_RTM_MASK     (0x3ULL << 46)
#define SDHC_REG_CAPABILITIES_RTM_SHIFT    46
#define SDHC_REG_CAPABILITIES_CM_MASK      (0xffULL << 48)
#define SDHC_REG_CAPABILITIES_CM_SHIFT     48
/** @} */

struct sdhc_standard_host {
	const struct device *dev;

	uint8_t (*read8)(const struct device *dev, uint32_t reg);
	void (*write8)(const struct device *dev, uint32_t reg, uint8_t value);
	uint16_t (*read16)(const struct device *dev, uint32_t reg);
	void (*write16)(const struct device *dev, uint32_t reg, uint16_t value);
	uint32_t (*read32)(const struct device *dev, uint32_t reg);
	void (*write32)(const struct device *dev, uint32_t reg, uint32_t value);
	uint64_t (*read64)(const struct device *dev, uint32_t reg);
	void (*write64)(const struct device *dev, uint32_t reg, uint64_t value);
};

uint8_t sdhc_standard_read8(struct sdhc_standard_host *host, uint32_t reg);
void sdhc_standard_write8(struct sdhc_standard_host *host, uint32_t reg, uint8_t value);
uint16_t sdhc_standard_read16(struct sdhc_standard_host *host, uint32_t reg);
void sdhc_standard_write16(struct sdhc_standard_host *host, uint32_t reg, uint16_t value);
uint32_t sdhc_standard_read32(struct sdhc_standard_host *host, uint32_t reg);
void sdhc_standard_write32(struct sdhc_standard_host *host, uint32_t reg, uint32_t value);
uint64_t sdhc_standard_read64(struct sdhc_standard_host *host, uint32_t reg);
void sdhc_standard_write64(struct sdhc_standard_host *host, uint32_t reg, uint64_t value);

void sdhc_standard_capabilities_init(struct sdhc_standard_host *host, struct sdhc_host_caps *caps);

#endif /* ZEPHYR_DRIVERS_SDHC_SDHC_STANDARD_H_ */
