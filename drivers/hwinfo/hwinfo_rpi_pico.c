/*
 * Copyright (c) 2022 Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/drivers/hwinfo.h>
#include <hardware/flash.h>
#include <hardware/structs/vreg_and_chip_reset.h>

#define FLASH_RUID_DATA_BYTES 8

#define HAD_RUN_BIT BIT(VREG_AND_CHIP_RESET_CHIP_RESET_HAD_RUN_LSB)
#define HAD_PSM_RESTART_BIT BIT(VREG_AND_CHIP_RESET_CHIP_RESET_HAD_PSM_RESTART_LSB)
#define HAD_POR_BIT BIT(VREG_AND_CHIP_RESET_CHIP_RESET_HAD_POR_LSB)

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	uint8_t id[FLASH_RUID_DATA_BYTES];
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

	if (length > sizeof(id)) {
		length = sizeof(id);
	}
	memcpy(buffer, id, length);

	return length;
}

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	uint32_t flags = 0;
	uint32_t reset_register = vreg_and_chip_reset_hw->chip_reset;

	if (reset_register & HAD_POR_BIT) {
		flags |= RESET_POR;
	}

	if (reset_register & HAD_RUN_BIT) {
		flags |= RESET_PIN;
	}

	if (reset_register & HAD_PSM_RESTART_BIT) {
		flags |= RESET_DEBUG;
	}

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

	return 0;
}
