/*
 * Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MODELS_H
#define MODELS_H

#include <stddef.h>
#include <stdint.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

#define TENSOR_ARENA_SIZE 16384

#if CONFIG_DCACHE_LINE_SIZE > 16
#define NPU_ALIGN CONFIG_DCACHE_LINE_SIZE
#else
#define NPU_ALIGN 16
#endif

#if (defined(CONFIG_SOC_SERIES_MPS3) || defined(CONFIG_SOC_SERIES_MPS4)) &&                        \
	DT_NODE_HAS_STATUS(DT_NODELABEL(ddr4), okay)
#define TENSOR_ARENA_ATTR __aligned(NPU_ALIGN) __attribute__((section("tflm_arena")))
#define MODEL_ATTR        __aligned(NPU_ALIGN) __attribute__((section("tflm_model")))
#define INPUT_ATTR        __aligned(4) __attribute__((section("tflm_input")))
#define OUTPUT_ATTR       __aligned(4) __attribute__((section("tflm_output")))
#else
#define TENSOR_ARENA_ATTR __aligned(NPU_ALIGN) __attribute__((section(".noinit.tflm_arena")))
#define MODEL_ATTR        __aligned(NPU_ALIGN) __attribute__((section(".rodata.tflm_model")))
#define INPUT_ATTR        __aligned(4) __attribute__((section(".rodata.tflm_input")))
#define OUTPUT_ATTR       __aligned(4) __attribute__((section(".rodata.tflm_output")))
#endif

#include "dft.h"
#include "life.h"

struct model_data {
	const char *name;
	uint8_t *model;
	size_t model_size;
	uint8_t *input;
	size_t input_size;
	uint8_t *expected_output;
	size_t expected_output_size;
};

#define MODEL_DATA(name)                                                                           \
	{#name,                                                                                    \
	 name##_model,                                                                             \
	 sizeof(name##_model),                                                                     \
	 name##_input,                                                                             \
	 sizeof(name##_input),                                                                     \
	 name##_expected_output,                                                                   \
	 sizeof(name##_expected_output)}

static struct model_data models[] = {
	MODEL_DATA(dft),
	MODEL_DATA(life),
};

#endif /* MODELS_H */
