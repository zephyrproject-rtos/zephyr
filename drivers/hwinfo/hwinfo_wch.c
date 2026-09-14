/*
 * SPDX-FileCopyrightText: Copyright Michael Hope <michaelh@juju.nz>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/hwinfo.h>
#include <string.h>

#include <hal_ch32fun.h>

#define DT_DRV_COMPAT wch_esig

/* Define locally as ch32fun uses different names for ESIG and the ESIG registers on each variant */
#define HWINFO_WCH_UID_OFFSET 0x08
#define HWINFO_WCH_UID_SIZE   12

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	const uint8_t *uid = (const uint8_t *)DT_INST_REG_ADDR(0) + HWINFO_WCH_UID_OFFSET;

	length = MIN(length, HWINFO_WCH_UID_SIZE);
	memcpy(buffer, uid, length);

	return length;
}

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	RCC_TypeDef *regs = (RCC_TypeDef *)DT_REG_ADDR(DT_INST(0, wch_rcc));
	uint32_t status = regs->RSTSCKR;

	*cause = 0;
	if ((status & (RCC_IWDGRSTF | RCC_WWDGRSTF)) != 0) {
		*cause |= RESET_WATCHDOG;
	}

	if ((status & RCC_PINRSTF) != 0) {
		*cause |= RESET_PIN;
	}

	if ((status & RCC_PORRSTF) != 0) {
		*cause |= RESET_POR;
	}

	if ((status & RCC_SFTRSTF) != 0) {
		*cause |= RESET_SOFTWARE;
	}

#if defined(RCC_LPWRRSTF)
	if ((status & RCC_LPWRRSTF) != 0) {
		*cause |= RESET_BROWNOUT;
	}
#endif

	return 0;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	RCC_TypeDef *regs = (RCC_TypeDef *)DT_REG_ADDR(DT_INST(0, wch_rcc));

	regs->RSTSCKR |= RCC_RMVF;

	return 0;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = RESET_WATCHDOG | RESET_PIN | RESET_POR | RESET_SOFTWARE
#if defined(RCC_LPWRRSTF)
		     | RESET_BROWNOUT
#endif
		;

	return 0;
}
