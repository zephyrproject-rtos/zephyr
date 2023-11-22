/**
 * Copyright 2023 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_inputmux

#include <errno.h>
#include <stdint.h>

#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <fsl_inputmux.h>

#define NXP_INPUTMUX_DEFATTACH(node, block, ctrlr)                                                 \
	*((uint32_t *)(DT_REG_ADDR(ctrlr) + DT_REG_ADDR(block)) + DT_REG_ADDR(node)) =             \
		DT_PROP(node, source);

#define NXP_INPUTMUX_DEFATTACH_BLOCK(node)                                                         \
	DT_FOREACH_CHILD_VARGS(node, NXP_INPUTMUX_DEFATTACH, node, DT_PARENT(node))

#define NXP_INPUTMUX(n)                                                                            \
	static int nxp_inputmux_##n##_init(const struct device *dev)                               \
	{                                                                                          \
		INPUTMUX_Init((INPUTMUX_Type *)DT_INST_REG_ADDR(n));                               \
		DT_INST_FOREACH_CHILD(n, NXP_INPUTMUX_DEFATTACH_BLOCK)                             \
		INPUTMUX_Deinit((INPUTMUX_Type *)DT_INST_REG_ADDR(n));                             \
		return 0;                                                                          \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(n, nxp_inputmux_##n##_init, NULL, NULL, NULL, PRE_KERNEL_1,          \
			      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

DT_INST_FOREACH_STATUS_OKAY(NXP_INPUTMUX)
