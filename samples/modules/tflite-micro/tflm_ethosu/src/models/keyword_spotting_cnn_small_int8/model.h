/*
 * Copyright 2022 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

const char *modelName = "keyword_spotting_cnn_small_int8";

#if defined(CONFIG_TAINT_BLOBS_TFLM_ETHOSU)
#define TENSOR_ARENA_SIZE 80000
#define TENSOR_ARENA_ATTR                                                                          \
	__aligned(CONFIG_DCACHE_LINE_SIZE) __attribute__((section(".noinit.tflm_arena")))
#define MODEL_ATTR __aligned(CONFIG_DCACHE_LINE_SIZE) __attribute__((section(".rodata.tflm_model")))
#else
#define TENSOR_ARENA_SIZE 50000
#define TENSOR_ARENA_ATTR __aligned(16) __attribute__((section(".noinit.tflm_arena")))
#define MODEL_ATTR        __aligned(16) __attribute__((section(".rodata.tflm_model")))
#endif

MODEL_ATTR uint8_t networkModelData[] = {
	/* FIXME: Check readme for generating Vela-compiled model blob */
};
