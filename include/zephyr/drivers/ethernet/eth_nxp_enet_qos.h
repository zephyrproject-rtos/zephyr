/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ETH_NXP_ENET_QOS_H__
#define ZEPHYR_INCLUDE_DRIVERS_ETH_NXP_ENET_QOS_H__

#include <fsl_device_registers.h>
#include <zephyr/drivers/clock_control.h>

/* Different platforms named the peripheral different in the register definitions */
#ifdef CONFIG_SOC_FAMILY_NXP_MCX
#undef ENET
#define ENET_QOS_NAME ENET
#define ENET_QOS_ALIGNMENT 4
typedef ENET_Type enet_qos_t;
#else
#error "ENET_QOS not enabled on this SOC series"
#endif

#define _PREFIX_UNDERLINE(x) _##x
#define _ENET_QOS_REG_FIELD(reg, field) MACRO_MAP_CAT(_PREFIX_UNDERLINE, reg, field, MASK)
#define _ENET_QOS_REG_MASK(reg, field) CONCAT(ENET_QOS_NAME, _ENET_QOS_REG_FIELD(reg, field))

/* Deciphers value of a field from a read value of an enet qos register
 *
 * reg: name of the register
 * field: name of the bit field within the register
 * val: value that had been read from the register
 */
#define ENET_QOS_REG_GET(reg, field, val) FIELD_GET(_ENET_QOS_REG_MASK(reg, field), val)

/* Prepares value of a field for a write to an enet qos register
 *
 * reg: name of the register
 * field: name of the bit field within the register
 * val: value to put into the field
 */
#define ENET_QOS_REG_PREP(reg, field, val) FIELD_PREP(_ENET_QOS_REG_MASK(reg, field), val)


#define ENET_QOS_ALIGN_ADDR_SHIFT(x) (x >> (ENET_QOS_ALIGNMENT >> 1))

struct nxp_enet_qos_config {
	const struct pinctrl_dev_config *pincfg;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	enet_qos_t *base;
};
#define ENET_QOS_MODULE_CFG(module_dev) ((struct nxp_enet_qos_config *) module_dev->config)

#endif /* ZEPHYR_INCLUDE_DRIVERS_ETH_NXP_ENET_H__ */
