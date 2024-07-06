/*
 * Copyright (c) 2022 Andes Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/syscon.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>

/*
 * SMU(System Management Unit) Registers for hwinfo driver
 */

/* Register offset*/
#define SMU_SYSTEMVER	0x00
#define SMU_WRSR	0x10

/* Wake-Up and Reset Status Register bitmask */
#define SMU_WRSR_APOR	BIT(0)
#define SMU_WRSR_MPOR	BIT(1)
#define SMU_WRSR_HW	BIT(2)
#define SMU_WRSR_WDT	BIT(3)
#define SMU_WRSR_SW	BIT(4)

#define ANDES_RESET_STATUS_MASK	BIT_MASK(5)

static const struct device *const syscon_dev =
			DEVICE_DT_GET(DT_NODELABEL(syscon));

int z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	int ret = 0;
	uint8_t id[3];
	uint32_t ver;

	if (!device_is_ready(syscon_dev)) {
		return -ENODEV;
	}

	ret = syscon_read_reg(syscon_dev, SMU_SYSTEMVER, &ver);
	if (ret < 0) {
		return ret;
	}

	if (length > sizeof(id)) {
		length = sizeof(id);
	}

	sys_put_le24(ver, id);

	memcpy(buffer, id, length);

	return length;
}

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	int ret = 0;
	uint32_t reason, flags = 0;

	if (!device_is_ready(syscon_dev)) {
		return -ENODEV;
	}

	ret = syscon_read_reg(syscon_dev, SMU_WRSR, &reason);
	if (ret < 0) {
		return ret;
	}

	if (reason & SMU_WRSR_APOR) {
		flags |= RESET_POR;
	}
	if (reason & SMU_WRSR_MPOR) {
		flags |= RESET_POR;
	}
	if (reason & SMU_WRSR_HW) {
		flags |= RESET_PIN;
	}
	if (reason & SMU_WRSR_WDT) {
		flags |= RESET_WATCHDOG;
	}
	if (reason & SMU_WRSR_SW) {
		flags |= RESET_SOFTWARE;
	}

	*cause = flags;

	return 0;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	int ret = 0;
	uint32_t reason;

	if (!device_is_ready(syscon_dev)) {
		return -ENODEV;
	}

	ret = syscon_write_reg(syscon_dev, SMU_WRSR, ANDES_RESET_STATUS_MASK);
	if (ret < 0) {
		return ret;
	}

	do {
		ret = syscon_read_reg(syscon_dev, SMU_WRSR, &reason);
	} while ((reason & ANDES_RESET_STATUS_MASK) && (!ret));

	return ret;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = (RESET_PIN
		| RESET_WATCHDOG
		| RESET_SOFTWARE
		| RESET_POR);

	return 0;
}
