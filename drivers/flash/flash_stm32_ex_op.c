/*
 * Copyright (c) 2023 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash/stm32_flash_api_extensions.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_USERSPACE
#include <zephyr/syscall.h>
#include <zephyr/internal/syscall_handler.h>
#endif

#include <soc.h>
#include "flash_stm32.h"

LOG_MODULE_REGISTER(flash_stm32_ex_op, CONFIG_FLASH_LOG_LEVEL);

#if defined(CONFIG_FLASH_STM32_WRITE_PROTECT)
int flash_stm32_ex_op_sector_wp(const struct device *dev, const uintptr_t in,
				void *out)
{
	const struct flash_stm32_ex_op_sector_wp_in *request =
		(const struct flash_stm32_ex_op_sector_wp_in *)in;
	struct flash_stm32_ex_op_sector_wp_out *result =
		(struct flash_stm32_ex_op_sector_wp_out *)out;
	uint32_t change_mask;
	int rc = 0, rc2 = 0;
#ifdef CONFIG_USERSPACE
	bool syscall_trap = z_syscall_trap();
#endif

	if (request != NULL) {
#ifdef CONFIG_USERSPACE
		struct flash_stm32_ex_op_sector_wp_in in_copy;

		if (syscall_trap) {
			K_OOPS(k_usermode_from_copy(&in_copy, request,
						sizeof(in_copy)));
			request = &in_copy;
		}
#endif
		change_mask = request->enable_mask;

		if (!IS_ENABLED(
			    CONFIG_FLASH_STM32_WRITE_PROTECT_DISABLE_PREVENTION)) {
			change_mask |= request->disable_mask;
		}

		rc = flash_stm32_option_bytes_lock(dev, false);
		if (rc == 0) {
			rc = flash_stm32_update_wp_sectors(
				dev, change_mask, request->enable_mask);
		}

		rc2 = flash_stm32_option_bytes_lock(dev, true);
		if (!rc) {
			rc = rc2;
		}
	}

	if (result != NULL) {
#ifdef CONFIG_USERSPACE
		struct flash_stm32_ex_op_sector_wp_out out_copy;

		if (syscall_trap) {
			result = &out_copy;
		}
#endif
		rc2 = flash_stm32_get_wp_sectors(dev, &result->protected_mask);
		if (!rc) {
			rc = rc2;
		}

#ifdef CONFIG_USERSPACE
		if (syscall_trap) {
			K_OOPS(k_usermode_to_copy(out, result, sizeof(out_copy)));
		}
#endif
	}

	return rc;
}
#endif /* CONFIG_FLASH_STM32_WRITE_PROTECT */

#if defined(CONFIG_FLASH_STM32_READOUT_PROTECTION)
int flash_stm32_ex_op_update_rdp(const struct device *dev, bool enable,
			   bool permanent)
{
	uint8_t current_level, target_level;

	current_level = flash_stm32_get_rdp_level(dev);
	target_level = current_level;

	/*
	 * 0xAA = RDP level 0 (no protection)
	 * 0xCC = RDP level 2 (permanent protection)
	 * others = RDP level 1 (protection active)
	 */
	switch (current_level) {
	case FLASH_STM32_RDP2:
		if (!enable || !permanent) {
			LOG_DBG("RDP level 2 is permanent and can't be changed!");
			return -ENOTSUP;
		}
		break;
	case FLASH_STM32_RDP0:
		if (enable) {
			target_level = FLASH_STM32_RDP1;
			if (permanent) {
#if defined(CONFIG_FLASH_STM32_READOUT_PROTECTION_PERMANENT_ALLOW)
				target_level = FLASH_STM32_RDP2;
#else
				LOG_DBG("Permanent readout protection (RDP "
					"level 0 -> 2) not allowed");
				return -ENOTSUP;
#endif
			}
		}
		break;
	default: /* FLASH_STM32_RDP1 */
		if (enable && permanent) {
#if defined(CONFIG_FLASH_STM32_READOUT_PROTECTION_PERMANENT_ALLOW)
			target_level = FLASH_STM32_RDP2;
#else
			LOG_DBG("Permanent readout protection (RDP "
				"level 1 -> 2) not allowed");
			return -ENOTSUP;
#endif
		}
		if (!enable) {
#if defined(CONFIG_FLASH_STM32_READOUT_PROTECTION_DISABLE_ALLOW)
			target_level = FLASH_STM32_RDP0;
#else
			LOG_DBG("Disabling readout protection (RDP "
				"level 1 -> 0) not allowed");
			return -EACCES;
#endif
		}
	}

	/* Update RDP level if needed */
	if (current_level != target_level) {
		LOG_INF("RDP changed from 0x%02x to 0x%02x", current_level,
			target_level);

		flash_stm32_set_rdp_level(dev, target_level);
	}
	return 0;
}

int flash_stm32_ex_op_rdp(const struct device *dev, const uintptr_t in,
			  void *out)
{
	const struct flash_stm32_ex_op_rdp *request =
		(const struct flash_stm32_ex_op_rdp *)in;
	struct flash_stm32_ex_op_rdp *result =
		(struct flash_stm32_ex_op_rdp *)out;
	uint8_t current_level;

#ifdef CONFIG_USERSPACE
	struct flash_stm32_ex_op_rdp copy;
	bool syscall_trap = z_syscall_trap();
#endif
	int rc = 0, rc2 = 0;

	if (request != NULL) {
#ifdef CONFIG_USERSPACE
		if (syscall_trap) {
			K_OOPS(k_usermode_from_copy(&copy, request, sizeof(copy)));
			request = &copy;
		}
#endif
		rc = flash_stm32_option_bytes_lock(dev, false);
		if (rc == 0) {
			rc = flash_stm32_ex_op_update_rdp(dev, request->enable,
						    request->permanent);
		}

		rc2 = flash_stm32_option_bytes_lock(dev, true);
		if (!rc) {
			rc = rc2;
		}
	}

	if (result != NULL) {
#ifdef CONFIG_USERSPACE
		if (syscall_trap) {
			result = &copy;
		}
#endif

		current_level = flash_stm32_get_rdp_level(dev);

		/*
		 * 0xAA = RDP level 0 (no protection)
		 * 0xCC = RDP level 2 (permanent protection)
		 * others = RDP level 1 (protection active)
		 */
		switch (current_level) {
		case FLASH_STM32_RDP2:
			result->enable = true;
			result->permanent = true;
			break;
		case FLASH_STM32_RDP0:
			result->enable = false;
			result->permanent = false;
			break;
		default: /* FLASH_STM32_RDP1 */
			result->enable = true;
			result->permanent = false;
		}

#ifdef CONFIG_USERSPACE
		if (syscall_trap) {
			K_OOPS(k_usermode_to_copy(out, result, sizeof(copy)));
		}
#endif
	}

	return rc;
}
#endif /* CONFIG_FLASH_STM32_READOUT_PROTECTION */
