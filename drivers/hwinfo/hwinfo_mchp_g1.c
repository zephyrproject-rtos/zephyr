/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hwinfo_mchp_g1, LOG_LEVEL_ERR);

#if DT_HAS_COMPAT_STATUS_OKAY(microchip_hwinfo_g1)
#define HWINFO_INST DT_COMPAT_GET_ANY_STATUS_OKAY(microchip_hwinfo_g1)

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	/*
	 * structure to hold a hardware unique identifier.
	 *
	 * This structure contains a 128-bit data array of four 32-bit unsigned integers,
	 * representing the unique hardware ID.
	 */
	struct hwinfo_id {
		uint32_t id[4];
	} dev_id;

	dev_id.id[0] = sys_cpu_to_be32(*(const uint32_t *)DT_REG_ADDR_BY_IDX(HWINFO_INST, 0));
	dev_id.id[1] = sys_cpu_to_be32(*(const uint32_t *)DT_REG_ADDR_BY_IDX(HWINFO_INST, 1));
	dev_id.id[2] = sys_cpu_to_be32(*(const uint32_t *)DT_REG_ADDR_BY_IDX(HWINFO_INST, 2));
	dev_id.id[3] = sys_cpu_to_be32(*(const uint32_t *)DT_REG_ADDR_BY_IDX(HWINFO_INST, 3));

	if (length > sizeof(dev_id.id)) {
		LOG_INF("Device ID size is 16 bytes");
		length = sizeof(dev_id.id);
	}

	memcpy(buffer, dev_id.id, length);

	return length;
}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(microchip_hwinfo_g1) */

#if DT_HAS_COMPAT_STATUS_OKAY(microchip_rstc_g1_reset)
#define RSTC_INST  DT_COMPAT_GET_ANY_STATUS_OKAY(microchip_rstc_g1_reset)

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = RESET_POR | RESET_BROWNOUT | RESET_PIN | RESET_WATCHDOG | RESET_SOFTWARE |
		     RESET_USER | RESET_LOW_POWER_WAKE;

	return 0;
}

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	volatile uint8_t *rcause_reg = (uint8_t *)(DT_REG_ADDR(RSTC_INST));
	uint8_t rcause = *rcause_reg;
	uint32_t result = 0;

	if (cause == NULL) {
		LOG_ERR("Invalid argument: NULL pointer passed");
		return -EINVAL;
	}

	if ((rcause & RSTC_RCAUSE_POR_Msk) != 0) {
		result |= RESET_POR;
	}
	if ((rcause & RSTC_RCAUSE_BODCORE_Msk) != 0) {
		result |= RESET_BROWNOUT;
	}
	if ((rcause & RSTC_RCAUSE_BODVDD_Msk) != 0) {
		result |= RESET_BROWNOUT;
	}
	if ((rcause & RSTC_RCAUSE_EXT_Msk) != 0) {
		result |= RESET_PIN | RESET_USER;
	}
	if ((rcause & RSTC_RCAUSE_WDT_Msk) != 0) {
		result |= RESET_WATCHDOG;
	}
	if ((rcause & RSTC_RCAUSE_SYST_Msk) != 0) {
		result |= RESET_SOFTWARE;
	}
	if ((rcause & RSTC_RCAUSE_BACKUP_Msk) != 0) {
		result |= RESET_LOW_POWER_WAKE;
	}

	*cause = result;

	return 0;
}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(microchip_rstc_g1_reset) */
