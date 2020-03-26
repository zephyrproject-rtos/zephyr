/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_IPM_IPM_CAVS_IDC_PRIV_H_
#define ZEPHYR_DRIVERS_IPM_IPM_CAVS_IDC_PRIV_H_

#define IDC_REG_SIZE		DT_INST_REG_SIZE(0)
#define IDC_REG_BASE(x)		\
	(DT_INST_REG_ADDR(0) + x * IDC_REG_SIZE)

#define IDC_CPU_OFFSET		0x10

#define REG_IDCTFC(x)		(0x0 + x * IDC_CPU_OFFSET)
#define REG_IDCTEFC(x)		(0x4 + x * IDC_CPU_OFFSET)
#define REG_IDCITC(x)		(0x8 + x * IDC_CPU_OFFSET)
#define REG_IDCIETC(x)		(0xc + x * IDC_CPU_OFFSET)
#define REG_IDCCTL		0x50

#define REG_IDCTFC_BUSY		(1 << 31)
#define REG_IDCTFC_MSG_MASK	0x7FFFFFFF

#define REG_IDCTEFC_MSG_MASK	0x3FFFFFFF

#define REG_IDCITC_BUSY		(1 << 31)
#define REG_IDCITC_MSG_MASK	0x7FFFFFFF

#define REG_IDCIETC_DONE	(1 << 30)
#define REG_IDCIETC_MSG_MASK	0x3FFFFFFF

#define REG_IDCCTL_IDCIDIE(x)	(0x100 << (x))
#define REG_IDCCTL_IDCTBIE(x)	(0x1 << (x))

static inline u32_t idc_read(u32_t reg, u32_t core_id)
{
	return sys_read32(IDC_REG_BASE(core_id) + reg);
}

static inline void idc_write(u32_t reg, u32_t core_id, u32_t val)
{
	sys_write32(val, IDC_REG_BASE(core_id) + reg);
}

#endif /* ZEPHYR_DRIVERS_IPM_IPM_CAVS_IDC_PRIV_H_ */
