/**
 * Copyright 2024 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_inputmux

#include <errno.h>
#include <stdint.h>

#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <fsl_inputmux.h>

#define NXP_INPUTMUX_DEFATTACH(node)                                                               \
	IF_ENABLED(DT_NODE_HAS_PROP(node, signal),                                                 \
		   (INPUTMUX_EnableSignal((INPUTMUX_Type *)DT_REG_ADDR(DT_PARENT(node)),           \
		    DT_PROP(node, signal), true);))                                                \
	IF_ENABLED(DT_NODE_HAS_PROP(node, selector),                                               \
		   (INPUTMUX_AttachSignal((INPUTMUX_Type *)DT_REG_ADDR(DT_PARENT(node)),           \
		    DT_PROP(node, selector), DT_PROP(node, source));))


#define NXP_INPUTMUX(n)                                                                            \
	static int nxp_inputmux_##n##_init(const struct device *dev)                               \
	{                                                                                          \
		INPUTMUX_Init((INPUTMUX_Type *)DT_INST_REG_ADDR(n));                               \
		DT_INST_FOREACH_CHILD(n, NXP_INPUTMUX_DEFATTACH)                                   \
		INPUTMUX_Deinit((INPUTMUX_Type *)DT_INST_REG_ADDR(n));                             \
		return 0;                                                                          \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(n, nxp_inputmux_##n##_init, NULL, NULL, NULL, PRE_KERNEL_1,          \
			      CONFIG_NXP_INPUTMUX_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(NXP_INPUTMUX)
