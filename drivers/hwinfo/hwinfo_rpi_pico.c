/*
 * Copyright (c) 2022 Yonatan Schachter
 * Copyright (c) 2024 Andrew Featherstone
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/drivers/hwinfo.h>
#include <hardware/flash.h>
#if defined(CONFIG_SOC_SERIES_RP2040)
#include <hardware/structs/vreg_and_chip_reset.h>
#else
#include <zephyr/sys/byteorder.h>
#include <pico/bootrom.h>
#include <hardware/structs/powman.h>
#endif

#define FLASH_RUID_DATA_BYTES 8

#if defined(CONFIG_SOC_SERIES_RP2040)
#define HAD_RUN_BIT BIT(VREG_AND_CHIP_RESET_CHIP_RESET_HAD_RUN_LSB)
#define HAD_PSM_RESTART_BIT BIT(VREG_AND_CHIP_RESET_CHIP_RESET_HAD_PSM_RESTART_LSB)
#define HAD_POR_BIT BIT(VREG_AND_CHIP_RESET_CHIP_RESET_HAD_POR_LSB)
#else
#define HAD_RUN_BIT BIT(POWMAN_CHIP_RESET_HAD_RUN_LOW_LSB)
#define HAD_PSM_RESTART_BIT BIT(POWMAN_CHIP_RESET_HAD_DP_RESET_REQ_LSB)
#define HAD_POR_BIT BIT(POWMAN_CHIP_RESET_HAD_POR_LSB)
#endif

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	uint8_t id[FLASH_RUID_DATA_BYTES];

#if CONFIG_HWINFO_RPI_PICO_CHIP_ID
	rom_get_sys_info_fn get_sys_info = (rom_get_sys_info_fn)
					rom_func_lookup_inline(ROM_FUNC_GET_SYS_INFO);
	/* Words: CHIP_INFO, PACKAGE_SEL, DEVICE_ID, WAFER_ID */
	uint32_t words[4];

	int n = get_sys_info(words, ARRAY_SIZE(words), SYS_INFO_CHIP_INFO);
	/* CHIP_INFO returns 4 words */
	__ASSERT(n == ARRAY_SIZE(words), "Failed to get chip info");

	/* Use DEVICE_ID + WAFER_ID, like BootROM uses for its USB ID */
	sys_put_be(id, &words[2], 2 * sizeof(uint32_t));
#else
	uint32_t key;

	/*
	 * flash_get_unique_id temporarily disables XIP to query the
	 * flash for its ID. If the CPU is interrupted while XIP is
	 * disabled, it will halt. Therefore, interrupts must be disabled
	 * before fetching the ID.
	 */
	key = irq_lock();
	flash_get_unique_id(id);
	irq_unlock(key);
#endif /* defined(CONFIG_SOC_SERIES_RP2350) */

	if (length > sizeof(id)) {
		length = sizeof(id);
	}
	memcpy(buffer, id, length);

	return length;
}

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	uint32_t flags = 0;
#if defined(CONFIG_SOC_SERIES_RP2040)
	uint32_t reset_register = vreg_and_chip_reset_hw->chip_reset;
#else
	uint32_t reset_register = powman_hw->chip_reset;
#endif

	if (reset_register & HAD_POR_BIT) {
		flags |= RESET_POR;
	}

	if (reset_register & HAD_RUN_BIT) {
		flags |= RESET_PIN;
	}

	if (reset_register & HAD_PSM_RESTART_BIT) {
		flags |= RESET_DEBUG;
	}
#if defined(CONFIG_SOC_SERIES_RP2350)
	if (reset_register & POWMAN_CHIP_RESET_HAD_BOR_BITS) {
		flags |= RESET_BROWNOUT;
	}

	if (reset_register & (POWMAN_CHIP_RESET_HAD_HZD_SYS_RESET_REQ_BITS |
			      POWMAN_CHIP_RESET_HAD_RESCUE_BITS)) {
		flags |= RESET_DEBUG;
	}

	if (reset_register & POWMAN_CHIP_RESET_HAD_GLITCH_DETECT_BITS) {
		flags |= RESET_SECURITY;
	}

	if (reset_register & (POWMAN_CHIP_RESET_HAD_WATCHDOG_RESET_RSM_BITS |
			      POWMAN_CHIP_RESET_HAD_WATCHDOG_RESET_SWCORE_BITS |
			      POWMAN_CHIP_RESET_HAD_WATCHDOG_RESET_POWMAN_BITS |
			      POWMAN_CHIP_RESET_HAD_WATCHDOG_RESET_POWMAN_ASYNC_BITS)) {
		flags |= RESET_WATCHDOG;
	}
#endif

	*cause = flags;
	return 0;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	/* The reset register can't be modified, nothing to do. */

	return -ENOSYS;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = RESET_PIN | RESET_DEBUG | RESET_POR;
#if defined(CONFIG_SOC_SERIES_RP2350)
	*supported |= RESET_BROWNOUT | RESET_WATCHDOG | RESET_SECURITY;
#endif

	return 0;
}
