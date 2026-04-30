/*
 * Copyright (c) 2026 Alex Fabre
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for the STM32 system bootloader host driver.
 *
 * The driver is standalone: it exposes the AN3155 command set through its
 * own typed API, so callers interact with the target at the protocol's own
 * level of abstraction (enter, get_info, read_memory, mass_erase, go, ...).
 *
 * A companion flash adapter driver (@c st,stm32-bootloader-flash) can
 * optionally expose one or more target memory regions as Zephyr flash
 * devices by delegating to the functions declared below. Users choose:
 *
 *  - Full bootloader control:  @c DEVICE_DT_GET(<bootloader-node>) and use
 *                              the API in this header.
 *  - Flash-style programming:  @c DEVICE_DT_GET(<flash-adapter-child>) and
 *                              use @c flash_read / @c flash_write /
 *                              @c flash_erase / @c stream_flash / etc.
 *
 * The adapter does not manage sessions — callers must @c enter the
 * bootloader device before any @c flash_* call on a child adapter and
 * @c exit when done.
 *
 * @defgroup stm32_bootloader STM32 system bootloader
 * @ingroup misc_interfaces
 * @{
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MISC_STM32_BOOTLOADER_H_
#define ZEPHYR_INCLUDE_DRIVERS_MISC_STM32_BOOTLOADER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum data bytes per single Read/Write Memory command (AN3155). */
#define STM32_BOOTLOADER_MAX_DATA_SIZE 256

/** Information returned by the target bootloader on the Get / Get Version /
 *  Get ID commands.
 */
struct stm32_bootloader_info {
	/** Bootloader version, BCD-packed (e.g. 0x31 = v3.1). */
	uint8_t bl_version;
	/** Number of supported commands reported by Get. */
	uint8_t num_cmds;
	/** Supported command bytes (truncated to 16 — enough for AN3155). */
	uint8_t cmds[16];
	/** Option byte 1 (from Get Version). */
	uint8_t option1;
	/** Option byte 2 (from Get Version). */
	uint8_t option2;
	/** Target Product ID (from Get ID). */
	uint16_t pid;
	/** True if Extended Erase (0x44) is supported, else Standard Erase (0x43). */
	bool has_extended_erase;
};

/** @cond INTERNAL_HIDDEN */

typedef int (*stm32_bootloader_enter_t)(const struct device *dev);
typedef int (*stm32_bootloader_exit_t)(const struct device *dev, bool reset_target);
typedef int (*stm32_bootloader_go_t)(const struct device *dev, uint32_t addr);
typedef int (*stm32_bootloader_get_info_t)(const struct device *dev,
					   struct stm32_bootloader_info *info);
typedef int (*stm32_bootloader_read_memory_t)(const struct device *dev, uint32_t addr, void *buf,
					      size_t len);
typedef int (*stm32_bootloader_write_memory_t)(const struct device *dev, uint32_t addr,
					       const void *buf, size_t len);
typedef int (*stm32_bootloader_erase_pages_t)(const struct device *dev, const uint16_t *pages,
					      size_t num_pages);
typedef int (*stm32_bootloader_mass_erase_t)(const struct device *dev);

struct stm32_bootloader_driver_api {
	stm32_bootloader_enter_t enter;
	stm32_bootloader_exit_t exit;
	stm32_bootloader_go_t go;
	stm32_bootloader_get_info_t get_info;
	stm32_bootloader_read_memory_t read_memory;
	stm32_bootloader_write_memory_t write_memory;
	stm32_bootloader_erase_pages_t erase_pages;
	stm32_bootloader_mass_erase_t mass_erase;
};

/** @endcond */

/**
 * @brief Enter the target STM32's system bootloader.
 *
 * Asserts BOOT pin(s) and pulses NRST if the device tree declares them,
 * synchronises with the target over the transport bus, and caches the
 * target's capabilities (so later @c erase_pages / @c mass_erase calls
 * select the right erase command automatically).
 *
 * Must be called before any @c read_memory / @c write_memory /
 * @c erase_pages / @c mass_erase / @c go on this device, and before any
 * flash_* call on a companion @c st,stm32-bootloader-flash child node.
 *
 * @return 0 on success, negative errno on failure.
 */
static inline int stm32_bootloader_enter(const struct device *dev)
{
	const struct stm32_bootloader_driver_api *api = DEVICE_API_GET(stm32_bootloader, dev);

	return api->enter(dev);
}

/**
 * @brief Exit the bootloader and restore the transport configuration.
 *
 * @param dev          Bootloader device.
 * @param reset_target If true, pulse NRST to hard-reset the target.
 *                     Requires an @c nrst-gpios in the device tree.
 */
static inline int stm32_bootloader_exit(const struct device *dev, bool reset_target)
{
	const struct stm32_bootloader_driver_api *api = DEVICE_API_GET(stm32_bootloader, dev);

	return api->exit(dev, reset_target);
}

/**
 * @brief Issue the Go command, starting execution at @p addr on the target.
 *
 * BOOT pin(s) are de-asserted so the target boots from flash on the next
 * reset. @ref stm32_bootloader_exit() should still be called afterwards
 * to restore the host's transport configuration.
 */
static inline int stm32_bootloader_go(const struct device *dev, uint32_t addr)
{
	const struct stm32_bootloader_driver_api *api = DEVICE_API_GET(stm32_bootloader, dev);

	return api->go(dev, addr);
}

/**
 * @brief Return the cached target bootloader information.
 *
 * The information is fetched during @ref stm32_bootloader_enter(); this
 * call just copies it out. Returns @c -ENOTCONN if @c enter has not run.
 */
static inline int stm32_bootloader_get_info(const struct device *dev,
					    struct stm32_bootloader_info *info)
{
	const struct stm32_bootloader_driver_api *api = DEVICE_API_GET(stm32_bootloader, dev);

	return api->get_info(dev, info);
}

/**
 * @brief Read from any addressable memory on the target.
 *
 * @p addr is an absolute address on the target's memory map (e.g.
 * 0x08000000 for main flash, 0x1FFF7800 for option bytes on some parts,
 * 0x20000000 for SRAM). The driver chunks the request into AN3155
 * Read Memory commands as needed.
 */
static inline int stm32_bootloader_read_memory(const struct device *dev, uint32_t addr, void *buf,
					       size_t len)
{
	const struct stm32_bootloader_driver_api *api = DEVICE_API_GET(stm32_bootloader, dev);

	return api->read_memory(dev, addr, buf, len);
}

/**
 * @brief Write to any addressable memory on the target.
 *
 * AN3155 requires 4-byte aligned addresses and lengths; the driver
 * validates this at the boundary.
 */
static inline int stm32_bootloader_write_memory(const struct device *dev, uint32_t addr,
						const void *buf, size_t len)
{
	const struct stm32_bootloader_driver_api *api = DEVICE_API_GET(stm32_bootloader, dev);

	return api->write_memory(dev, addr, buf, len);
}

/**
 * @brief Erase specific pages/sectors on the target.
 *
 * The driver auto-selects Standard Erase (0x43) or Extended Erase (0x44)
 * based on the cached target capabilities.
 */
static inline int stm32_bootloader_erase_pages(const struct device *dev, const uint16_t *pages,
					       size_t num_pages)
{
	const struct stm32_bootloader_driver_api *api = DEVICE_API_GET(stm32_bootloader, dev);

	return api->erase_pages(dev, pages, num_pages);
}

/**
 * @brief Erase the target's entire flash in a single command.
 *
 * Much faster on large targets than erasing every page individually.
 * The driver auto-selects Standard or Extended Erase.
 */
static inline int stm32_bootloader_mass_erase(const struct device *dev)
{
	const struct stm32_bootloader_driver_api *api = DEVICE_API_GET(stm32_bootloader, dev);

	return api->mass_erase(dev);
}

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_MISC_STM32_BOOTLOADER_H_ */
