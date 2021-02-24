/*
 * Copyright (c) 2021 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/hwinfo.h>
#include <soc.h>
#include <string.h>

/* host module instances for hardware info */
static struct mswc_reg *const inst_mswc = (struct mswc_reg *)
	DT_REG_ADDR_BY_NAME(DT_INST(0, nuvoton_npcx_host_sub), mswc);

/**
 * @brief Receive 8-bit value that identifies a family of devices with similar
 *        functionality.
 *
 * @retval value that identifies a family of devices with similar functionality.
 */
static uint8_t npcx_hwinfo_family_id(void)
{
	return inst_mswc->SID_CR;
}

/**
 * @brief Receive 8-bit value that identifies a device group of the family.
 *
 * @retval value that identifies a device group of the family.
 */
static uint8_t npcx_hwinfo_chip_id(void)
{
	return inst_mswc->SRID_CR;
}

/**
 * @brief Receive 8-bit value that identifies a specific device of a group.
 *
 * @retval value that identifies a specific device of a group.
 */
static uint8_t npcx_hwinfo_device_id(void)
{
	return inst_mswc->DEVICE_ID_CR;
}

/**
 * @brief Receive 8-bit value that identifies the device revision.
 *
 * @retval value that identifies the device revision.
 */
static uint8_t npcx_hwinfo_revision(void)
{
	return inst_mswc->CHPREV_CR;
}

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	uint8_t chip_info[4];

	chip_info[0] = npcx_hwinfo_family_id();
	chip_info[1] = npcx_hwinfo_chip_id();
	chip_info[2] = npcx_hwinfo_device_id();
	chip_info[3] = npcx_hwinfo_revision();

	if (length > sizeof(chip_info)) {
		length = sizeof(chip_info);
	}

	memcpy(buffer, chip_info, length);

	return length;
}
