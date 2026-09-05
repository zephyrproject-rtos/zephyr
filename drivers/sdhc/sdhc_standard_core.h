/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SDHC_SDHC_STANDARD_CORE_H_
#define ZEPHYR_DRIVERS_SDHC_SDHC_STANDARD_CORE_H_

#include <zephyr/drivers/sdhc.h>
#include "sdhc_common.h"

/**
 * @name SD host standard registers
 *
 * SD Specifications Part A2
 * SD Host Controller Simplified Specification
 * Version 3.00
 * @{
 */
#define SDHC_REG_SDMA_SYS_ADDR_ARG2 0x000 /**< SDMA System Address / Argument 2 Register (u32) */
#define SDHC_REG_BLOCK_SIZE         0x004 /**< Block Size Register (u16) */
#define SDHC_REG_BLOCK_COUNT        0x006 /**< Block Count Register (u16) */
#define SDHC_REG_ARGUMENT1          0x008 /**< Argument 1 Register (u32) */
#define SDHC_REG_TRANSFER_MODE      0x00C /**< Transfer Mode Register (u16) */
#define SDHC_REG_COMMAND            0x00E /**< Command Register (u16) */
#define SDHC_REG_RESPONSE_0_1       0x010 /**< Response Register (u32) */
#define SDHC_REG_RESPONSE_2_3       0x014 /**< Response Register (u32) */
#define SDHC_REG_RESPONSE_4_5       0x018 /**< Response Register (u32) */
#define SDHC_REG_RESPONSE_6_7       0x01C /**< Response Register (u32) */
#define SDHC_REG_BUF_DATA_PORT      0x020 /**< Buffer Data Port Register (u32) */
#define SDHC_REG_PRESENT_STATE      0x024 /**< Present State Register (u32) */
#define SDHC_REG_HOST_CTRL1         0x028 /**< Host Control 1 Register (u8) */
#define SDHC_REG_POWER_CTRL         0x029 /**< Power Control Register (u8) */
#define SDHC_REG_BLOCK_GAP_CTRL     0x02A /**< Block Gap Control Register (u8) */
#define SDHC_REG_WAKEUP_CTRL        0x02B /**< Wakeup Control Register (u8) */
#define SDHC_REG_CLOCK_CTRL         0x02C /**< Clock Control Register (u16) */
#define SDHC_REG_TIMEOUT_CTRL       0x02E /**< Timeout Control Register (u8) */
#define SDHC_REG_SW_RESET           0x02F /**< Software Reset Register (u8) */
#define SDHC_REG_NORMAL_INT_STAT    0x030 /**< Normal Interrupt Status Register (u16) */
#define SDHC_REG_ERROR_INT_STAT     0x032 /**< Error Interrupt Status Register (u16) */
#define SDHC_REG_NORMAL_INT_STAT_EN 0x034 /**< Normal Interrupt Status Enable Register (u16) */
#define SDHC_REG_ERROR_INT_STAT_EN  0x036 /**< Error Interrupt Status Enable Register (u16) */
#define SDHC_REG_NORMAL_INT_SIG_EN  0x038 /**< Normal Interrupt Signal Enable Register (u16) */
#define SDHC_REG_ERROR_INT_SIG_EN   0x03A /**< Error Interrupt Signal Enable Register (u16) */
#define SDHC_REG_AUTO_CMD_ERR_STAT  0x03C /**< Auto CMD Error Status Register (u16) */
#define SDHC_REG_HOST_CTRL2         0x03E /**< Host Control 2 Register (u16) */
#define SDHC_REG_CAPABILITIES       0x040 /**< Capabilities Register (u64) */
#define SDHC_REG_MAX_CURRENT_CAP    0x048 /**< Maximum Current Capabilities Register (u64) */
#define SDHC_REG_FORCE_EVT_AUTO_CMD_ERR 0x050 /**< Force Event - Auto CMD Error Status (u16) */
#define SDHC_REG_FORCE_EVT_ERR_INT_STAT 0x052 /**< Force Event - Error Interrupt Status (u16) */
#define SDHC_REG_ADMA_ERR_STAT          0x054 /**< ADMA Error Status Register (u8) */
#define SDHC_REG_ADMA_SYS_ADDR          0x058 /**< ADMA System Address Register (u64) */
#define SDHC_REG_PRESET_INIT            0x060 /**< Preset Value for Initialization (u16) */
#define SDHC_REG_PRESET_DEFAULT         0x062 /**< Preset Value for Default Speed (u16) */
#define SDHC_REG_PRESET_HIGH_SPEED      0x064 /**< Preset Value for High Speed (u16) */
#define SDHC_REG_PRESET_SDR12           0x066 /**< Preset Value for SDR12 (u16) */
#define SDHC_REG_PRESET_SDR25           0x068 /**< Preset Value for SDR25 (u16) */
#define SDHC_REG_PRESET_SDR50           0x06A /**< Preset Value for SDR50 (u16) */
#define SDHC_REG_PRESET_SDR104          0x06C /**< Preset Value for SDR104 (u16) */
#define SDHC_REG_PRESET_DDR50           0x06E /**< Preset Value for DDR50 (u16) */
#define SDHC_REG_SHARED_BUS_CTRL        0x0E0 /**< Shared Bus Control Register (u32) */
#define SDHC_REG_SLOT_INT_STAT          0x0FC /**< Slot Interrupt Status Register (u16) */
#define SDHC_REG_HOST_CTRL_VER          0x0FE /**< Host Controller Version Register (u16) */
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

/**
 * @brief Callback table for the SDHC standard host controller.
 *
 * This structure holds function pointers that a host driver may provide to
 * customize or extend the behavior of the standard host core.  The table is
 * expected to grow as new optional extension points are added to the core;
 * any entry left NULL causes the core to use its built-in default behavior
 * for that operation.
 *
 * Currently the table only contains I/O accessor hooks, enabled by
 * @kconfig{CONFIG_SDHC_STANDARD_CORE_IO_ACCESSORS}.  These allow a host
 * driver to override the default memory-mapped register access, which is
 * useful when:
 * - The host controller uses non-standard register layouts or offsets that
 *   differ from the SD Host Controller Specification.
 * - Register access requires additional side effects (e.g., lock acquisition,
 *   byte-swapping, or indirect addressing through a vendor-specific bridge).
 * - The platform cannot perform direct MMIO access at the width implied by
 *   the function signature.
 */
#ifdef CONFIG_SDHC_STANDARD_CORE_IO_ACCESSORS
struct sdhc_standard_host_ops {
	/** @brief Read an 8-bit register. See @ref sdhc_standard_read8. */
	uint8_t (*read8)(const struct device *dev, uint32_t reg);

	/** @brief Write an 8-bit register. See @ref sdhc_standard_write8. */
	void (*write8)(const struct device *dev, uint32_t reg, uint8_t value);

	/** @brief Read a 16-bit register. See @ref sdhc_standard_read16. */
	uint16_t (*read16)(const struct device *dev, uint32_t reg);

	/** @brief Write a 16-bit register. See @ref sdhc_standard_write16. */
	void (*write16)(const struct device *dev, uint32_t reg, uint16_t value);

	/** @brief Read a 32-bit register. See @ref sdhc_standard_read32. */
	uint32_t (*read32)(const struct device *dev, uint32_t reg);

	/** @brief Write a 32-bit register. See @ref sdhc_standard_write32. */
	void (*write32)(const struct device *dev, uint32_t reg, uint32_t value);

	/** @brief Read a 64-bit register. See @ref sdhc_standard_read64. */
	uint64_t (*read64)(const struct device *dev, uint32_t reg);

	/** @brief Write a 64-bit register. See @ref sdhc_standard_write64. */
	void (*write64)(const struct device *dev, uint32_t reg, uint64_t value);
};
#endif

/**
 * @brief Compile-time configuration for an SDHC standard host controller.
 *
 * Must be the first field of every driver-specific config structure so that
 * the standard host core can safely cast @c dev->config to this type.
 */
struct sdhc_standard_host_common_config {
	DEVICE_MMIO_NAMED_ROM(sdhc_base);
	/** @brief Device tree properties. */
	struct sdhc_common_config common;
#ifdef CONFIG_SDHC_STANDARD_CORE_IO_ACCESSORS
	/** @brief Optional I/O accessor hook table. */
	struct sdhc_standard_host_ops *ops;
#endif
};

/**
 * @brief Runtime data for an SDHC standard host controller.
 *
 * Must be the first field of every driver-specific data structure so that
 * the standard host core can safely cast @c dev->data to this type.
 */
struct sdhc_standard_host_common_data {
	DEVICE_MMIO_NAMED_RAM(sdhc_base);
	/** @brief Host properties derived from device tree and hardware capabilities. */
	struct sdhc_host_props props;
};

/** @cond INTERNAL_HIDDEN */
#define DEV_CFG(dev)  ((const struct sdhc_standard_host_common_config *)(dev)->config)
#define DEV_DATA(dev) ((struct sdhc_standard_host_common_data *)(dev)->data)
/** @endcond */

/**
 * @brief Initializer fragment for the ops field of
 *        @ref sdhc_standard_host_common_config.
 *
 * @param o Pointer to the @ref sdhc_standard_host_ops instance, or NULL.
 */
#ifdef CONFIG_SDHC_STANDARD_CORE_IO_ACCESSORS
#define SDHC_STANDARD_COMMON_CONFIG_OPS_INIT(o) .ops = (o),
#else
#define SDHC_STANDARD_COMMON_CONFIG_OPS_INIT(o)
#endif

/**
 * @brief Initializer for @ref sdhc_standard_host_common_config from a DT instance.
 *
 * @param n DT instance number.
 * @param o Pointer to @ref sdhc_standard_host_ops, or NULL if no I/O
 *          overrides are needed. Ignored when
 *          @kconfig{CONFIG_SDHC_STANDARD_CORE_IO_ACCESSORS} is disabled.
 */
#define SDHC_STANDARD_COMMON_CONFIG_DT_INST_INIT(n, o) \
	{                                                               \
		DEVICE_MMIO_NAMED_ROM_INIT(sdhc_base, DT_DRV_INST(n)),  \
		.common = SDHC_COMMON_CONFIG_DT_INST_INIT(n),           \
		SDHC_STANDARD_COMMON_CONFIG_OPS_INIT(o)}

/**
 * @brief Get the CPU-side base address of the host controller register block.
 *
 * Returns the virtual address established by @ref sdhc_standard_host_init.
 * Valid only after initialization is complete.
 *
 * @param dev Pointer to the host controller device.
 * @return Virtual base address of the MMIO register block.
 */
#define SDHC_STANDARD_DEV_MMIO_GET(dev) DEVICE_MMIO_NAMED_GET(dev, sdhc_base)

/**
 * @defgroup sdhc_standard_host_io SDHC Standard Host Low-Level I/O Primitives
 * @{
 *
 * @brief Low-level register I/O primitives for the SDHC standard host controller.
 *
 * @note Prefer the higher-level SDHC APIs (command submission, data transfer,
 * clock/bus-width control, etc.) over these primitives wherever possible.
 * Direct register access bypasses any state tracking maintained by the driver
 * and should only be used when implementing new low-level features or
 * performing platform-specific bring-up and debugging.
 */

/**
 * @brief Read an 8-bit register from the host controller.
 *
 * @param dev Pointer to the standard host device.
 * @param reg  Register offset in bytes from the controller base address.
 *
 * @return The 8-bit value read from the register.
 */
uint8_t sdhc_standard_read8(const struct device *dev, uint32_t reg);

/**
 * @brief Write an 8-bit value to a host controller register.
 *
 * @param dev Pointer to the standard host device.
 * @param reg   Register offset in bytes from the controller base address.
 * @param value Value to write.
 */
void sdhc_standard_write8(const struct device *dev, uint32_t reg, uint8_t value);

/**
 * @brief Read a 16-bit register from the host controller.
 *
 * @param dev Pointer to the standard host device.
 * @param reg  Register offset in bytes from the controller base address.
 *
 * @return The 16-bit value read from the register.
 */
uint16_t sdhc_standard_read16(const struct device *dev, uint32_t reg);

/**
 * @brief Write a 16-bit value to a host controller register.
 *
 * @param dev Pointer to the standard host device.
 * @param reg   Register offset in bytes from the controller base address.
 * @param value Value to write.
 */
void sdhc_standard_write16(const struct device *dev, uint32_t reg, uint16_t value);

/**
 * @brief Read a 32-bit register from the host controller.
 *
 * @param dev Pointer to the standard host device.
 * @param reg  Register offset in bytes from the controller base address.
 *
 * @return The 32-bit value read from the register.
 */
uint32_t sdhc_standard_read32(const struct device *dev, uint32_t reg);

/**
 * @brief Write a 32-bit value to a host controller register.
 *
 * @param dev Pointer to the standard host device.
 * @param reg   Register offset in bytes from the controller base address.
 * @param value Value to write.
 */
void sdhc_standard_write32(const struct device *dev, uint32_t reg, uint32_t value);

/**
 * @brief Read a 64-bit register from the host controller.
 *
 * @param dev Pointer to the standard host device.
 * @param reg  Register offset in bytes from the controller base address.
 *
 * @return The 64-bit value read from the register.
 */
uint64_t sdhc_standard_read64(const struct device *dev, uint32_t reg);

/**
 * @brief Write a 64-bit value to a host controller register.
 *
 * @param dev Pointer to the standard host device.
 * @param reg   Register offset in bytes from the controller base address.
 * @param value Value to write.
 */
void sdhc_standard_write64(const struct device *dev, uint32_t reg, uint64_t value);

/**
 * @}
 */

/**
 * @brief Initialize the SDHC standard host controller.
 *
 * Performs the following initialization steps:
 * 1. Maps the controller's MMIO register block into the CPU address space.
 * 2. Populates host properties from the device tree configuration.
 * 3. Reads and parses the hardware capability registers.
 *
 * Must be called from the driver's @c init callback before invoking any
 * other standard host core API.
 *
 * @param dev Pointer to the host controller device.
 */
void sdhc_standard_host_init(const struct device *dev);
#endif /* ZEPHYR_DRIVERS_SDHC_SDHC_STANDARD_CORE_H_ */
