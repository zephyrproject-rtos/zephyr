/*
 * Copyright 2022 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

const char *modelName = "keyword_spotting_cnn_small_int8";

#if defined(CONFIG_TAINT_BLOBS_TFLM_ETHOSU)
#define TENSOR_ARENA_SIZE 80000
#else
#define TENSOR_ARENA_SIZE 50000
#endif

#if CONFIG_DCACHE_LINE_SIZE > 16
#define NPU_ALIGN CONFIG_DCACHE_LINE_SIZE
#else
#define NPU_ALIGN 16
#endif

#if (defined(CONFIG_SOC_SERIES_MPS3) || defined(CONFIG_SOC_SERIES_MPS4)) &&                        \
	DT_NODE_HAS_STATUS(DT_NODELABEL(ddr4), okay)
#define TENSOR_ARENA_ATTR __aligned(NPU_ALIGN) __attribute__((section("tflm_arena")))
#define MODEL_ATTR        __aligned(NPU_ALIGN) __attribute__((section("tflm_model")))
#else
#define TENSOR_ARENA_ATTR __aligned(NPU_ALIGN) __attribute__((section(".noinit.tflm_arena")))
#define MODEL_ATTR        __aligned(NPU_ALIGN) __attribute__((section(".rodata.tflm_model")))
#endif

MODEL_ATTR uint8_t networkModelData[] = {
	/* FIXME: Check readme for generating Vela-compiled model blob */
};
