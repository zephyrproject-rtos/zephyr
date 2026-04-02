/*
 * Copyright 2022 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if (defined(CONFIG_SOC_SERIES_MPS3) || defined(CONFIG_SOC_SERIES_MPS4)) &&                        \
	DT_NODE_HAS_STATUS(DT_NODELABEL(ddr4), okay)
#define OUTPUT_ATTR __aligned(4) __attribute__((section("tflm_output")))
#else
#define OUTPUT_ATTR __aligned(4) __attribute__((section(".rodata.tflm_output")))
#endif

OUTPUT_ATTR uint8_t expectedOutputData[] = {
	0x80, 0x23, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x83, 0x80, 0x80, 0x81, 0xd9
};
