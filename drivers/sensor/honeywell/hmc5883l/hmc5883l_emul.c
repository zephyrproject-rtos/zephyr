/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT honeywell_hmc5883l

#include <zephyr/drivers/sensor/emul_sensor_regmap.h>

/* HMC5883L 3-Axis Digital Compass IC datasheet, Form 900405 Rev E, Registers, tables 2 to 20 */
#define HMC5883L_REGS(R) \
	R(CRA, 0x00, .reset = 0x10) \
	R(CRB, 0x01, .reset = 0x20) \
	R(MR, 0x02, .reset = 0x01) \
	R(DXRA, 0x03, .flags = EMUL_SENSOR_REG_RO) \
	R(DXRB, 0x04, .flags = EMUL_SENSOR_REG_RO) \
	R(DZRA, 0x05, .flags = EMUL_SENSOR_REG_RO) \
	R(DZRB, 0x06, .flags = EMUL_SENSOR_REG_RO) \
	R(DYRA, 0x07, .flags = EMUL_SENSOR_REG_RO) \
	R(DYRB, 0x08, .flags = EMUL_SENSOR_REG_RO) \
	R(SR, 0x09, .flags = EMUL_SENSOR_REG_RO) \
	/* ASCII "H43" */ \
	R(IRA, 0x0A, .flags = EMUL_SENSOR_REG_RO, .reset = 0x48) \
	R(IRB, 0x0B, .flags = EMUL_SENSOR_REG_RO, .reset = 0x34) \
	R(IRC, 0x0C, .flags = EMUL_SENSOR_REG_RO, .reset = 0x33)

EMUL_SENSOR_REG_TABLE_DEFINE(hmc5883l_regs, HMC5883L_REGS);

/*
 * 16-bit two's complement output from a 12-bit ADC (0xF800 to 0x07FF). CRB.GN selects the gain:
 * 1370, 1090, 820, 660, 440, 390, 330, 230 LSb/gauss. +-1.3 gauss fits the ADC at every gain.
 * Sets SR.RDY.
 */
/* clang-format off */
#define MAGN(_chan, _reg)                                                                          \
	{                                                                                          \
		_chan, .reg = _reg,                                                                \
		.is_signed = true, .bits = 16,                                                     \
		.min = -1.3, .max = 1.3,                                                           \
		.select = {EMUL_SENSOR_REG_ADDR(CRB), GENMASK(7, 5)},                              \
		.variants = {                                                                      \
			{.lsb = 1.0 / 1370},                                                       \
			{.lsb = 1.0 / 1090},                                                       \
			{.lsb = 1.0 / 820},                                                        \
			{.lsb = 1.0 / 660},                                                        \
			{.lsb = 1.0 / 440},                                                        \
			{.lsb = 1.0 / 390},                                                        \
			{.lsb = 1.0 / 330},                                                        \
			{.lsb = 1.0 / 230},                                                        \
		},                                                                                 \
		.ready = {EMUL_SENSOR_REG_ADDR(SR), BIT(0)},                                       \
	}
/* clang-format on */

static const struct emul_sensor_channel hmc5883l_channels[] = {
	MAGN(SENSOR_CHAN_MAGN_X, EMUL_SENSOR_REG_ADDR(DXRA)),
	MAGN(SENSOR_CHAN_MAGN_Z, EMUL_SENSOR_REG_ADDR(DZRA)),
	MAGN(SENSOR_CHAN_MAGN_Y, EMUL_SENSOR_REG_ADDR(DYRA)),
};

EMUL_SENSOR_REGMAP_DEFINE(hmc5883l_regs, hmc5883l_channels, .big_endian = true);
