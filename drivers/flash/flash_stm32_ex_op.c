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

int flash_stm32_option_bytes_lock(const struct device *dev, bool enable)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);

#if defined(FLASH_OPTCR_OPTLOCK) /* F2, F4, F7 or H7 */
	if (enable) {
		regs->OPTCR |= FLASH_OPTCR_OPTLOCK;
	} else if (regs->OPTCR & FLASH_OPTCR_OPTLOCK) {
		regs->OPTKEYR = FLASH_OPT_KEY1;
		regs->OPTKEYR = FLASH_OPT_KEY2;
	}
#else
	int rc;

	/* Unlock CR/PECR/NSCR register if needed. */
	if (!enable) {
		rc = flash_stm32_cr_lock(dev, false);
		if (rc) {
			return rc;
		}
	}

#if defined(FLASH_CR_OPTWRE)	  /* F0, F1 and F3 */
	if (enable) {
		regs->CR &= ~FLASH_CR_OPTWRE;
	} else if (!(regs->CR & FLASH_CR_OPTWRE)) {
		regs->OPTKEYR = FLASH_OPTKEY1;
		regs->OPTKEYR = FLASH_OPTKEY2;
	}
#elif defined(FLASH_CR_OPTLOCK)	  /* G0, G4, L4, WB and WL */
	if (enable) {
		regs->CR |= FLASH_CR_OPTLOCK;
	} else if (regs->CR & FLASH_CR_OPTLOCK) {
		regs->OPTKEYR = FLASH_OPTKEY1;
		regs->OPTKEYR = FLASH_OPTKEY2;
	}
#elif defined(FLASH_PECR_OPTLOCK) /* L0 and L1 */
	if (enable) {
		regs->PECR |= FLASH_PECR_OPTLOCK;
	} else if (regs->PECR & FLASH_PECR_OPTLOCK) {
		regs->OPTKEYR = FLASH_OPTKEY1;
		regs->OPTKEYR = FLASH_OPTKEY2;
	}
#elif defined(FLASH_NSCR_OPTLOCK) /* L5 and U5 */
	if (enable) {
		regs->NSCR |= FLASH_NSCR_OPTLOCK;
	} else if (regs->NSCR & FLASH_NSCR_OPTLOCK) {
		regs->OPTKEYR = FLASH_OPTKEY1;
		regs->OPTKEYR = FLASH_OPTKEY2;
	}
#elif defined(FLASH_NSCR1_OPTLOCK) /* WBA */
	if (enable) {
		regs->NSCR1 |= FLASH_NSCR1_OPTLOCK;
	} else if (regs->NSCR1 & FLASH_NSCR1_OPTLOCK) {
		regs->OPTKEYR = FLASH_OPTKEY1;
		regs->OPTKEYR = FLASH_OPTKEY2;
	}
#endif
	/* Lock CR/PECR/NSCR register if needed. */
	if (enable) {
		rc = flash_stm32_cr_lock(dev, true);
		if (rc) {
			return rc;
		}
	}
#endif

	if (enable) {
		LOG_DBG("Option bytes locked");
	} else {
		LOG_DBG("Option bytes unlocked");
	}

	return 0;
}

#if defined(CONFIG_FLASH_STM32_WRITE_PROTECT)
int flash_stm32_ex_op_sector_wp(const struct device *dev, const uintptr_t in,
				void *out)
{
	const struct flash_stm32_ex_op_sector_wp_in *request =
		(const struct flash_stm32_ex_op_sector_wp_in *)in;
	struct flash_stm32_ex_op_sector_wp_out *result =
		(struct flash_stm32_ex_op_sector_wp_out *)out;
	uint64_t change_mask;
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

int flash_stm32_ex_op(const struct device *dev, uint16_t code,
			     const uintptr_t in, void *out)
{
	int rv = -ENOTSUP;

	flash_stm32_sem_take(dev);

	switch (code) {
#if defined(CONFIG_FLASH_STM32_WRITE_PROTECT)
	case FLASH_STM32_EX_OP_SECTOR_WP:
		rv = flash_stm32_ex_op_sector_wp(dev, in, out);
		break;
#endif /* CONFIG_FLASH_STM32_WRITE_PROTECT */
#if defined(CONFIG_FLASH_STM32_READOUT_PROTECTION)
	case FLASH_STM32_EX_OP_RDP:
		rv = flash_stm32_ex_op_rdp(dev, in, out);
		break;
#endif /* CONFIG_FLASH_STM32_READOUT_PROTECTION */
#if defined(CONFIG_FLASH_STM32_BLOCK_REGISTERS)
	case FLASH_STM32_EX_OP_BLOCK_OPTION_REG:
		rv = flash_stm32_option_bytes_disable(dev);
		break;
	case FLASH_STM32_EX_OP_BLOCK_CONTROL_REG:
		rv = flash_stm32_control_register_disable(dev);
		break;
#endif /* CONFIG_FLASH_STM32_BLOCK_REGISTERS */
#if defined(CONFIG_FLASH_STM32_OPTION_BYTES) && ( \
		defined(CONFIG_DT_HAS_ST_STM32F4_FLASH_CONTROLLER_ENABLED) || \
		defined(CONFIG_DT_HAS_ST_STM32F7_FLASH_CONTROLLER_ENABLED) || \
		defined(CONFIG_DT_HAS_ST_STM32G0_FLASH_CONTROLLER_ENABLED) || \
		defined(CONFIG_DT_HAS_ST_STM32G4_FLASH_CONTROLLER_ENABLED) || \
		defined(CONFIG_DT_HAS_ST_STM32L4_FLASH_CONTROLLER_ENABLED))
	case FLASH_STM32_EX_OP_OPTB_READ:
		if (out == NULL) {
			rv = -EINVAL;
			break;
		}

		*(uint32_t *)out = flash_stm32_option_bytes_read(dev);
		rv = 0;

		break;
	case FLASH_STM32_EX_OP_OPTB_WRITE:
		int rv2;

		rv = flash_stm32_option_bytes_lock(dev, false);
		if (rv > 0) {
			break;
		}

		rv2 = flash_stm32_option_bytes_write(dev, UINT32_MAX, (uint32_t)in);
		/* returned later, we always re-lock */

		rv = flash_stm32_option_bytes_lock(dev, true);
		if (rv > 0) {
			break;
		}

		rv = rv2;

		break;
#endif
	}

	flash_stm32_sem_give(dev);

	return rv;
}
