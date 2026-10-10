/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_p3t1755

#include <zephyr/drivers/sensor/emul_sensor_regmap.h>

/*
 * P3T1755 datasheet Rev. 1.3, table 13 and section 7.5.2.
 * https://www.nxp.com/docs/en/data-sheet/P3T1755.pdf
 */
#define P3T1755_REGS(R) \
	R(TEMP, 0x00, .flags = EMUL_SENSOR_REG_RO) \
	R(CONF, 0x01, .bytes = 1, .reset = 0x28) \
	/* 75 degC and 80 degC */ \
	R(TLOW, 0x02, .reset = 0x4B00) \
	R(THIGH, 0x03, .reset = 0x5000)

EMUL_SENSOR_REG_TABLE_DEFINE(p3t1755_regs, P3T1755_REGS);

/* clang-format off */
static const struct emul_sensor_channel p3t1755_channels[] = {
	/* T11 to T0 in bits 15:4, two's complement, 0.0625 degC per LSB */
	{
		.chan = SENSOR_CHAN_AMBIENT_TEMP, .reg = EMUL_SENSOR_REG_ADDR(TEMP),
		.is_signed = true, .bits = 12, .pos = 4,
		.lsb = 0.0625, .min = -40.0, .max = 125.0,
	},
};
/* clang-format on */

EMUL_SENSOR_REGMAP_DEFINE(p3t1755_regs, p3t1755_channels, .reg_bytes = 2, .big_endian = true);
