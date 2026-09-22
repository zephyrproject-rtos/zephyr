/*
 * Copyright (c) 2023 Piotr Dymacz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/byteorder.h>

#include <driverlib/sys_ctrl.h>
#include <inc/hw_ccfg.h>
#include <inc/hw_fcfg1.h>
#include <inc/hw_memmap.h>

#ifdef CONFIG_HWINFO_CC13XX_CC26XX_USE_BLE_MAC
#define CC13XX_CC26XX_DEVID_SIZE	6
#else
#define CC13XX_CC26XX_DEVID_SIZE	8
#endif

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	uint8_t *mac;

	if (IS_ENABLED(CONFIG_HWINFO_CC13XX_CC26XX_USE_BLE_MAC)) {
		if (IS_ENABLED(CONFIG_HWINFO_CC13XX_CC26XX_ALWAYS_USE_FACTORY_DEFAULT) ||
		    sys_read32(CCFG_BASE + CCFG_O_IEEE_BLE_0) == 0xFFFFFFFF ||
		    sys_read32(CCFG_BASE + CCFG_O_IEEE_BLE_1) == 0xFFFFFFFF) {
			mac = (uint8_t *)(FCFG1_BASE + FCFG1_O_MAC_BLE_0);
		} else {
			mac = (uint8_t *)(CCFG_BASE + CCFG_O_IEEE_BLE_0);
		}
	} else if (IS_ENABLED(CONFIG_HWINFO_CC13XX_CC26XX_USE_IEEE_MAC)) {
		if (IS_ENABLED(CONFIG_HWINFO_CC13XX_CC26XX_ALWAYS_USE_FACTORY_DEFAULT) ||
		    sys_read32(CCFG_BASE + CCFG_O_IEEE_MAC_0) == 0xFFFFFFFF ||
		    sys_read32(CCFG_BASE + CCFG_O_IEEE_MAC_1) == 0xFFFFFFFF) {
			mac = (uint8_t *)(FCFG1_BASE + FCFG1_O_MAC_15_4_0);
		} else {
			mac = (uint8_t *)(CCFG_BASE + CCFG_O_IEEE_MAC_0);
		}
	}

	if (length > CC13XX_CC26XX_DEVID_SIZE) {
		length = CC13XX_CC26XX_DEVID_SIZE;
	}

	/* Provide device ID (MAC) in big endian */
	sys_memcpy_swap(buffer, mac, length);

	return length;
}

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	uint32_t reset_src;

	reset_src = SysCtrlResetSourceGet();

	switch (reset_src) {
	case RSTSRC_PWR_ON:
		*cause = RESET_POR;
		break;
	case RSTSRC_PIN_RESET:
		*cause = RESET_PIN;
		break;
	case RSTSRC_VDDS_LOSS:
		__fallthrough;
	case RSTSRC_VDDR_LOSS:
		*cause = RESET_BROWNOUT;
		break;
	case RSTSRC_CLK_LOSS:
		*cause = RESET_CLOCK;
		break;
	case RSTSRC_SYSRESET:
		*cause = RESET_SOFTWARE;
		break;
	}

	return 0;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	return -ENOSYS;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = (RESET_PIN
		      | RESET_SOFTWARE
		      | RESET_BROWNOUT
		      | RESET_POR
		      | RESET_CLOCK);

	return 0;
}
