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
#include <zephyr/drivers/syscon.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(andes_v5_l2_cache, CONFIG_SOC_LOG_LEVEL);

#if DT_NODE_EXISTS(DT_INST(0, andestech_l2c))

/* SMU Configuration Register offset */
#define SMU_SYSTEMCFG		0x08

/* Register bitmask */
#define SMU_SYSTEMCFG_L2C	BIT(8)

/*
 * L2C Register Base Address
 */

#define ANDES_V5_L2C_BASE	DT_INST_REG_ADDR(0)

/*
 * L2C Register Offset
 */

#define L2C_CONFIG		0x00
#define L2C_CTRL		0x08

/*
 * L2C Helper Constant
 */

/* L2 cache version */
#define L2C_CONFIG_VERSION_SHIFT        24

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
	unsigned long mcache_ctl;
	volatile uint64_t *l2c_ctrl =
		INT_TO_POINTER(ANDES_V5_L2C_BASE + L2C_CTRL);

	__asm__ volatile ("csrr %0, %1"
		: "=r" (mcache_ctl) : "i" (NDS_MCACHE_CTL));

	/* Enable L2 cache if L1 I/D cache enabled */
	if (mcache_ctl & (BIT(1) | BIT(0))) {
		volatile uint64_t *l2c_config =
			INT_TO_POINTER(ANDES_V5_L2C_BASE + L2C_CONFIG);

		*l2c_ctrl |= (L2C_CTRL_IPFDPT_3 | L2C_CTRL_DPFDPT_8);

		/* Enable L2 cache manually if device version less than 0xF0 */
		if (!((*l2c_config >> L2C_CONFIG_VERSION_SHIFT) & 0xF0)) {
			*l2c_ctrl |= L2C_CTRL_CEN;
		}
	} else {
		/* Disable L2 cache */
		*l2c_ctrl &= ~L2C_CTRL_CEN;
	}
}

static int andes_v5_l2c_init(const struct device *dev)
{
#if DT_NODE_HAS_STATUS(DT_INST(0, syscon), okay)
	uint32_t system_cfg;
	const struct device *const syscon_dev = DEVICE_DT_GET(DT_NODELABEL(syscon));

	ARG_UNUSED(dev);

	if (device_is_ready(syscon_dev)) {
		syscon_read_reg(syscon_dev, SMU_SYSTEMCFG, &system_cfg);

		if (!(system_cfg & SMU_SYSTEMCFG_L2C)) {
			/* This SoC doesn't have L2 cache */
			return -ENODEV;
		}
	} else {
		LOG_DBG("Init might fail for some hardware combinations "
					"if syscon driver isn't ready\n");
	}
#endif

	andes_v5_l2c_enable();
	return 0;
}

SYS_INIT(andes_v5_l2c_init, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /* DT_NODE_EXISTS(DT_INST(0, andestech_l2c)) */
