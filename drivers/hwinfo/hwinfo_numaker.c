/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/byteorder.h>
#include <string.h>

struct numaker_uid {
	uint32_t id[3];
};

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	struct numaker_uid dev_id;
	bool was_reg_locked = (SYS->REGLCTL == 0);
	bool was_fmc_opened = (FMC->ISPCTL & FMC_ISPCTL_ISPEN_Msk);

	if (was_reg_locked) {
		SYS_UnlockReg();
	}
	if (!was_fmc_opened) {
		FMC_Open();
	}

	dev_id.id[0] = sys_cpu_to_be32(FMC_ReadUID(0));
	dev_id.id[1] = sys_cpu_to_be32(FMC_ReadUID(1));
	dev_id.id[2] = sys_cpu_to_be32(FMC_ReadUID(2));

	length = MIN(length, sizeof(dev_id.id));
	memcpy(buffer, dev_id.id, length);

	if (!was_fmc_opened) {
		FMC_Close();
	}
	if (was_reg_locked) {
		SYS_LockReg();
	}

	return length;
}
