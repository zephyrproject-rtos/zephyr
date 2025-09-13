/*
 * Copyright 2022 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if (defined(CONFIG_SOC_SERIES_MPS3) || defined(CONFIG_SOC_SERIES_MPS4)) &&                        \
	DT_NODE_HAS_STATUS(DT_NODELABEL(ddr4), okay)
#define INPUT_ATTR __aligned(4) __attribute__((section("tflm_input")))
#else
#define INPUT_ATTR __aligned(4) __attribute__((section(".rodata.tflm_input")))
#endif

INPUT_ATTR uint8_t inputData[] = {
	/* FIXME: Check readme for input blob matching regenerated model */
};
