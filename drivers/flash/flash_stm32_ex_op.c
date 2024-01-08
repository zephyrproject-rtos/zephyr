/*
 * Copyright (c) 2023 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash/stm32_flash_api_extensions.h>
#include <zephyr/kernel.h>

#ifdef CONFIG_USERSPACE
#include <zephyr/syscall.h>
#include <zephyr/internal/syscall_handler.h>
#endif

#include <soc.h>
#include "flash_stm32.h"

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
int flash_stm32_ex_op_rdp(const struct device *dev, const uintptr_t in,
			  void *out)
{
	const struct flash_stm32_ex_op_rdp *request =
		(const struct flash_stm32_ex_op_rdp *)in;
	struct flash_stm32_ex_op_rdp *result =
		(struct flash_stm32_ex_op_rdp *)out;

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
			rc = flash_stm32_update_rdp(dev, request->enable,
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
		rc2 = flash_stm32_get_rdp(dev, &result->enable,
					  &result->permanent);
		if (!rc) {
			rc = rc2;
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
