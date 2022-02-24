/*
 * Copyright (c) 2022 ITE Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_I2C_IT8XXX2_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_I2C_IT8XXX2_H_


#define IT8XXX2_ECPM_CGCTRL4R_OFF	0x09
/*
 * The clock gate offsets combine the register offset from ECPM_BASE and the
 * mask within that register into one value. These are used for
 * clock_enable_peripheral() and clock_disable_peripheral()
 */
#define CGC_OFFSET_SMBF		((IT8XXX2_ECPM_CGCTRL4R_OFF << 8) | 0x80)
#define CGC_OFFSET_SMBE		((IT8XXX2_ECPM_CGCTRL4R_OFF << 8) | 0x40)
#define CGC_OFFSET_SMBD		((IT8XXX2_ECPM_CGCTRL4R_OFF << 8) | 0x20)
#define CGC_OFFSET_SMBC		((IT8XXX2_ECPM_CGCTRL4R_OFF << 8) | 0x10)
#define CGC_OFFSET_SMBB		((IT8XXX2_ECPM_CGCTRL4R_OFF << 8) | 0x08)
#define CGC_OFFSET_SMBA		((IT8XXX2_ECPM_CGCTRL4R_OFF << 8) | 0x04)

#endif	/* ZEPHYR_INCLUDE_DT_BINDINGS_I2C_IT8XXX2_H_ */
