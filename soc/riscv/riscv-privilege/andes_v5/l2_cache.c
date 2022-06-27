/*
 * Copyright (c) 2021 Andes Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT andestech_l2c

/**
 * @brief Andes V5 L2 Cache Controller driver
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <soc.h>

/*
 * L2C Register Base Address
 */

#define AMDES_V5_L2C_BASE	DT_INST_REG_ADDR(0)

/*
 * L2C Register Offset
 */

#define L2C_CTRL		0x08

/*
 * L2C Helper Constant
 */

/* enable L2C */
#define L2C_CTRL_CEN		BIT(0)

/* instruction prefetch depth */
#define IPFDPT_FIELD(x)		(x << 3)
#define L2C_CTRL_IPFDPT_0	IPFDPT_FIELD(0)
#define L2C_CTRL_IPFDPT_1	IPFDPT_FIELD(1)
#define L2C_CTRL_IPFDPT_2	IPFDPT_FIELD(2)
#define L2C_CTRL_IPFDPT_3	IPFDPT_FIELD(3)

/* data prefetch depth */
#define DPFDPT_FIELD(x)		(x << 5)
#define L2C_CTRL_DPFDPT_0	DPFDPT_FIELD(0)
#define L2C_CTRL_DPFDPT_2	DPFDPT_FIELD(1)
#define L2C_CTRL_DPFDPT_4	DPFDPT_FIELD(2)
#define L2C_CTRL_DPFDPT_8	DPFDPT_FIELD(3)

static void andes_v5_l2c_enable(void)
{
	volatile uint64_t *l2c_ctrl =
		INT_TO_POINTER(AMDES_V5_L2C_BASE + L2C_CTRL);

	/* Use largest instr/data prefetch depth by default */
	uint64_t l2c_config = L2C_CTRL_IPFDPT_3 | L2C_CTRL_DPFDPT_8;

	/* Configure L2 cache */
	*l2c_ctrl = l2c_config;

	/* Enable L2 cache */
	*l2c_ctrl = (l2c_config | L2C_CTRL_CEN);
}

static int andes_v5_l2c_init(const struct device *dev)
{
	ARG_UNUSED(dev);

#ifdef SMU_BASE
	volatile uint32_t *system_cfg = INT_TO_POINTER(SMU_BASE + SMU_SYSTEMCFG);

	if (!(*system_cfg & SMU_SYSTEMCFG_L2C)) {
		/* This SoC doesn't have L2 cache */
		return -1;
	}
#endif /* SMU_BASE */

	andes_v5_l2c_enable();
	return 0;
}

SYS_INIT(andes_v5_l2c_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
