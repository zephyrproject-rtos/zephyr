/*
 * Copyright (c) 2024 Ambiq
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <am_mcu_apollo.h>
#include <zephyr/drivers/hwinfo.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>

int z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{

	struct ambiq_hwinfo {
		/* Ambiq Chip ID0 */
		uint32_t chip_id_0;
		/* Ambiq Chip ID1 */
		uint32_t chip_id_1;
		/* Ambiq Factory Trim Revision */
		/* Can be used in Ambiq HAL for additional code support  */
		uint32_t factory_trim_version;
	};

	struct ambiq_hwinfo dev_hw_info = {0};

	/* Contains the HAL hardware information about the device. */
	am_hal_mcuctrl_device_t mcu_ctrl_device;

	am_hal_mram_info_read(1, AM_REG_INFO1_TRIM_REV_O / 4, 1, &dev_hw_info.factory_trim_version);
	am_hal_mcuctrl_info_get(AM_HAL_MCUCTRL_INFO_DEVICEID, &mcu_ctrl_device);

	dev_hw_info.chip_id_0 = mcu_ctrl_device.ui32ChipID0;
	dev_hw_info.chip_id_1 = mcu_ctrl_device.ui32ChipID1;

	if (length > sizeof(dev_hw_info)) {
		length = sizeof(dev_hw_info);
	}

	dev_hw_info.chip_id_0 = BSWAP_32(dev_hw_info.chip_id_0);
	dev_hw_info.chip_id_1 = BSWAP_32(dev_hw_info.chip_id_1);
	dev_hw_info.factory_trim_version = BSWAP_32(dev_hw_info.factory_trim_version);
	memcpy(buffer, &dev_hw_info, length);

	return length;
}

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	uint32_t flags = 0;
	uint32_t reset_status = 0;
	am_hal_reset_status_t status = {0};

	/* Print out reset status register upon entry */
	am_hal_reset_status_get(&status);
	reset_status = status.eStatus;

	/* EXTERNAL PIN */
	if (reset_status & AM_HAL_RESET_STATUS_EXTERNAL) {
		flags |= RESET_PIN;
	}

	/* POWER CYCLE */
	if (reset_status & AM_HAL_RESET_STATUS_POR) {
		flags |= RESET_POR;
	}

	/* BROWNOUT DETECTOR */
	if (reset_status & AM_HAL_RESET_STATUS_BOD) {
		flags |= RESET_BROWNOUT;
	}

	/* SOFTWARE POR */
	if (reset_status & AM_HAL_RESET_STATUS_SWPOR) {
		flags |= RESET_SOFTWARE;
	}

	/* SOFTWARE POI */
	if (reset_status & AM_HAL_RESET_STATUS_SWPOI) {
		flags |= RESET_SOFTWARE;
	}

	/* DEBUGGER */
	if (reset_status & AM_HAL_RESET_STATUS_DEBUGGER) {
		flags |= RESET_DEBUG;
	}

	/* WATCHDOG */
	if (reset_status & AM_HAL_RESET_STATUS_WDT) {
		flags |= RESET_WATCHDOG;
	}

	/* BOUNREG */
	if (reset_status & AM_HAL_RESET_STATUS_BOUNREG) {
		flags |= RESET_HARDWARE;
	}

	/* BOCORE */
	if (reset_status & AM_HAL_RESET_STATUS_BOCORE) {
		flags |= RESET_HARDWARE;
	}

	/* BOMEM */
	if (reset_status & AM_HAL_RESET_STATUS_BOMEM) {
		flags |= RESET_HARDWARE;
	}

	/* BOHPMEM */
	if (reset_status & AM_HAL_RESET_STATUS_BOHPMEM) {
		flags |= RESET_HARDWARE;
	}

	*cause = flags;
	return 0;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	/* SBL maintains the RSTGEN->STAT register in
	 * INFO1 space even upon clearing RSTGEN->STAT
	 * register.
	 * - INFO1_RESETSTATUS
	 */

	return -ENOSYS;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = RESET_PIN
			| RESET_SOFTWARE
			| RESET_POR
			| RESET_WATCHDOG
			| RESET_HARDWARE
			| RESET_BROWNOUT;
	return 0;
}
