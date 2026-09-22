/*
 * Copyright (c) 2025 Aleksandr Senin <al@meshium.net>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/hwinfo.h>
#include <string.h>

#include <soc.h>
#include <gd32_regs.h>
#include <zephyr/sys/byteorder.h>

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	uint32_t uid_words[3];

	const volatile uint32_t *uid = (const volatile uint32_t *)GD32_UID_BASE;
	size_t out_len = sizeof(uid_words);

	uid_words[0] = sys_cpu_to_be32(uid[0]);
	uid_words[1] = sys_cpu_to_be32(uid[1]);
	uid_words[2] = sys_cpu_to_be32(uid[2]);

	if (length < out_len) {
		out_len = length;
	}

	memcpy(buffer, uid_words, out_len);
	return out_len;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	uint32_t sup = 0U;

#if defined(RCU_RSTSCK_EPRSTF)
	sup |= RESET_PIN;
#endif
#if defined(RCU_RSTSCK_SWRSTF)
	sup |= RESET_SOFTWARE;
#endif
#if defined(RCU_RSTSCK_BORRSTF)
	sup |= RESET_BROWNOUT;
#endif
#if defined(RCU_RSTSCK_PORRSTF)
	sup |= RESET_POR;
#endif
#if defined(RCU_RSTSCK_WWDGTRSTF) || defined(RCU_RSTSCK_FWDGTRSTF)
	sup |= RESET_WATCHDOG;
#endif
#if defined(RCU_RSTSCK_LPRSTF)
	sup |= RESET_LOW_POWER_WAKE;
#endif
#if defined(RCU_RSTSCK_OBLRSTF)
	sup |= RESET_FLASH;
#endif
#if defined(RCU_RSTSCK_V11RSTF) || defined(RCU_RSTSCK_V12RSTF)
	sup |= RESET_POR;
#endif
#if defined(RCU_RSTSCK_LOCKUPRSTF)
	sup |= RESET_CPU_LOCKUP;
#endif
#if defined(RCU_RSTSCK_LVDRSTF)
	sup |= RESET_BROWNOUT;
#endif
#if defined(RCU_RSTSCK_LOHRSTF)
	sup |= RESET_CLOCK;
#endif
#if defined(RCU_RSTSCK_LOPRSTF)
	sup |= RESET_PLL;
#endif
#if defined(RCU_RSTSCK_ECCRSTF)
	sup |= RESET_PARITY;
#endif

	*supported = sup;

	return 0;
}

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	uint32_t flags = 0U;
	uint32_t rsts = RCU_RSTSCK;

#if defined(RCU_RSTSCK_EPRSTF)
	if (rsts & RCU_RSTSCK_EPRSTF) {
		flags |= RESET_PIN;
	}
#endif
#if defined(RCU_RSTSCK_WWDGTRSTF)
	if (rsts & RCU_RSTSCK_WWDGTRSTF) {
		flags |= RESET_WATCHDOG;
	}
#endif
#if defined(RCU_RSTSCK_FWDGTRSTF)
	if (rsts & RCU_RSTSCK_FWDGTRSTF) {
		flags |= RESET_WATCHDOG;
	}
#endif
#if defined(RCU_RSTSCK_SWRSTF)
	if (rsts & RCU_RSTSCK_SWRSTF) {
		flags |= RESET_SOFTWARE;
	}
#endif
#if defined(RCU_RSTSCK_BORRSTF)
	if (rsts & RCU_RSTSCK_BORRSTF) {
		flags |= RESET_BROWNOUT;
	}
#endif
#if defined(RCU_RSTSCK_PORRSTF)
	if (rsts & RCU_RSTSCK_PORRSTF) {
		flags |= RESET_POR;
	}
#endif
#if defined(RCU_RSTSCK_LPRSTF)
	if (rsts & RCU_RSTSCK_LPRSTF) {
		flags |= RESET_LOW_POWER_WAKE;
	}
#endif
#if defined(RCU_RSTSCK_OBLRSTF)
	if (rsts & RCU_RSTSCK_OBLRSTF) {
		flags |= RESET_FLASH;
	}
#endif
#if defined(RCU_RSTSCK_V11RSTF)
	if (rsts & RCU_RSTSCK_V11RSTF) {
		flags |= RESET_POR;
	}
#endif
#if defined(RCU_RSTSCK_V12RSTF)
	if (rsts & RCU_RSTSCK_V12RSTF) {
		flags |= RESET_POR;
	}
#endif
#if defined(RCU_RSTSCK_LOCKUPRSTF)
	if (rsts & RCU_RSTSCK_LOCKUPRSTF) {
		flags |= RESET_CPU_LOCKUP;
	}
#endif
#if defined(RCU_RSTSCK_LVDRSTF)
	if (rsts & RCU_RSTSCK_LVDRSTF) {
		flags |= RESET_BROWNOUT;
	}
#endif
#if defined(RCU_RSTSCK_LOHRSTF)
	if (rsts & RCU_RSTSCK_LOHRSTF) {
		flags |= RESET_CLOCK;
	}
#endif
#if defined(RCU_RSTSCK_LOPRSTF)
	if (rsts & RCU_RSTSCK_LOPRSTF) {
		flags |= RESET_PLL;
	}
#endif
#if defined(RCU_RSTSCK_ECCRSTF)
	if (rsts & RCU_RSTSCK_ECCRSTF) {
		flags |= RESET_PARITY;
	}
#endif

	*cause = flags;
	return 0;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	/* Writing 1 to RSTFC clears all reset flags */
#if defined(RCU_RSTSCK_RSTFC)
	RCU_RSTSCK |= RCU_RSTSCK_RSTFC;
	return 0;
#else
	return -ENOSYS;
#endif
}
