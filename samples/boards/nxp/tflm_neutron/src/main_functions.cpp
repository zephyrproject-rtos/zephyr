/*
 * Copyright 2026 NXP
 * All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Image classification sample using TensorFlow Lite Micro
 * Based on MobileNet model with NPU (Neutron) acceleration
 */

#include "main_functions.h"
#include <model.hpp>
/* Include image data and labels */
#include "image_data.h"
#include "labels.h"

/* TensorFlow Lite Micro headers */
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/micro/micro_mutable_op_resolver.h>
#include <tensorflow/lite/micro/kernels/micro_ops.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include <tensorflow/lite/micro/kernels/neutron/neutron.h>

#include <zephyr/kernel.h>
#include "timer.h"
#include <zephyr/sys/printk.h>
#include <zephyr/linker/section_tags.h>
#include <zephyr/autoconf.h>
#include <zephyr/cache.h>

/* Number of top results to display */
#define TOP_K_RESULTS 5
#define LABEL_COUNT 10

#ifdef __arm__
#include <cmsis_compiler.h>
#else
#define __ALIGNED(x) __attribute__((aligned(x)))
#endif


/* Tensor arena size */
constexpr int kTensorArenaSize = 60 * 1024;
#if defined(CONFIG_BOARD_MIMXRT700_EVK) || defined(CONFIG_SOC_MIMXRT798S)
static uint8_t __nocache __ALIGNED(16) tensor_arena[kTensorArenaSize];
#elif defined(CONFIG_SOC_SERIES_IMXRT266X)
/*
 * RT266x memory map for the NPU (see the board overlay). The whole NPU workload
 * runs from on-chip SRAM, which is directly coupled to the M85 (fast, not
 * LLC-cached) and coherent with the NPU without any cache maintenance (the L1
 * D-cache is off; see soc.c):
 *   - the tensor arena the NPU DMAs into lives in the SRAM0+SRAM1 region;
 *   - the model is copied out of flash into SRAM2 so the CPU and NPU read it
 *     from fast on-chip RAM instead of external XSPI flash.
 */
#include <zephyr/devicetree.h>
#include <zephyr/linker/devicetree_regions.h>
#define SRAM01_NPU_SECTION \
	__attribute__((section(LINKER_DT_NODE_REGION_NAME(DT_NODELABEL(sram0)))))
#define SRAM2_SECTION \
	__attribute__((section(LINKER_DT_NODE_REGION_NAME(DT_NODELABEL(sram2)))))

/* Tensor arena the NPU DMAs into: on-chip SRAM0+SRAM1. */
static uint8_t __ALIGNED(16) SRAM01_NPU_SECTION tensor_arena[kTensorArenaSize];
/* Model copied out of flash into fast on-chip SRAM2. */
static uint8_t __ALIGNED(16) SRAM2_SECTION g_model_sram[sizeof(g_model)];
#else
static __attribute__((aligned(16))) uint8_t tensor_arena[kTensorArenaSize];
#endif

/* Globals */
static tflite::MicroInterpreter *interpreter = nullptr;
static TfLiteTensor *input = nullptr;
static TfLiteTensor *output = nullptr;

extern "C" void cleanCache_by_Addr(uint32_t addr, uint32_t size);

void setup(void)
{
    /* Step 1: Load model */
#if defined(CONFIG_SOC_SERIES_IMXRT266X)
    /* Copy the model out of flash into fast on-chip SRAM2. */
    memcpy(g_model_sram, g_model, sizeof(g_model));
    cleanCache_by_Addr((uint32_t)(uintptr_t)g_model_sram, sizeof(g_model_sram));
    const tflite::Model *model = tflite::GetModel(g_model_sram);
#else
    const tflite::Model *model = tflite::GetModel(g_model);
#endif
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        printk("Model provided is schema version %d not equal "
               "to supported version %d!\r\n",
               model->version(), TFLITE_SCHEMA_VERSION);
        return;
    }

    /* Step 2: Create op resolver with NPU support */
    static tflite::MicroMutableOpResolver<7> resolver;
    resolver.AddDequantize();
    resolver.AddReshape();
    resolver.AddSlice();
    resolver.AddPad();
    resolver.AddConv2D();
    resolver.AddSoftmax();
    resolver.AddCustom(tflite::GetString_NEUTRON_GRAPH(),
                       tflite::Register_NEUTRON_GRAPH());

    /* Step 3: Create interpreter */
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize);
    interpreter = &static_interpreter;

    /* Step 4: Allocate tensors */
    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk) {
        interpreter = nullptr;
        return;
    }

    /* Step 5: Get input/output tensors */
    input = interpreter->input(0);
    output = interpreter->output(0);

    if (input == nullptr || output == nullptr) {
        return;
    }
}

void loop(void)
{
    static int inference_count = 0;

    if (interpreter == nullptr || input == nullptr || output == nullptr) {
        printk("Model not initialized. Cannot run inference.\n");
        return;
    }

    printk("Inference #%d\n", ++inference_count);
    printk("----------------------------------------\n");

    /* Copy input data */
    int input_size = 1;
    for (int i = 0; i < input->dims->size; i++) {
        input_size *= input->dims->data[i];
    }
    int output_size = 1;
    for (int i = 0; i < output->dims->size; i++) {
        output_size *= output->dims->data[i];
    }

    /* Copy input data */
    int copy_size = (input_size < (int)sizeof(image_data)) ?
                    input_size : (int)sizeof(image_data);

    if (input->type == kTfLiteInt8) {
        for (int i = 0; i < copy_size; i++) {
            input->data.int8[i] = (int8_t)(image_data[i] - 128);
        }
    } else if (input->type == kTfLiteUInt8) {
        for (int i = 0; i < copy_size; i++) {
            input->data.uint8[i] = image_data[i];
        }
    } else if (input->type == kTfLiteFloat32) {
        for (int i = 0; i < copy_size; i++) {
            input->data.f[i] = (image_data[i] - 127.5f) / 127.5f;
        }
    } else {
        printk("Unknown input tensor data type: %d", input->type);
        return;
    }

    /* Run inference */
    printk("Running inference...\n");
    struct timer timer;
    struct timer_result perf;
    timer_start(&timer);
    TfLiteStatus status = interpreter->Invoke();
    timer_stop(&timer, &perf);
    if (status != kTfLiteOk) {
        printk("ERROR: Invoke failed with status %d\n", status);
        return;
    }
    printk("Inference complete\n");
    timer_report("Invoke time", &perf);

    /* Convert output to float */
    /*
     * The classifier output is small (one score per label). Use fixed buffers
     * sized to the label table instead of heap allocation, and clamp the count
     * so a bad/oversized output tensor can never overrun them or exhaust the
     * heap (operator new aborts on failure since C++ exceptions are disabled).
     */
    if (output_size > LABEL_COUNT) {
        output_size = LABEL_COUNT;
    }
    float scores[LABEL_COUNT];
    int indices[LABEL_COUNT];
    if (output->type == kTfLiteFloat32) {
        for (int i = 0; i < output_size; i++) {
            scores[i] = output->data.f[i];
        }
    } else if (output->type == kTfLiteInt8) {
        float scale = output->params.scale;
        int zp = output->params.zero_point;
        for (int i = 0; i < output_size; i++) {
            scores[i] = (output->data.int8[i] - zp) * scale;
        }
    } else if (output->type == kTfLiteUInt8) {
        float scale = output->params.scale;
        int zp = output->params.zero_point;
        for (int i = 0; i < output_size; i++) {
            scores[i] = (output->data.uint8[i] - zp) * scale;
        }
    }

    /* Find top-k */
    for (int i = 0; i < output_size; i++) {
        indices[i] = i;
    }

    for (int i = 0; i < TOP_K_RESULTS && i < output_size; i++) {
        int max_idx = i;
        for (int j = i + 1; j < output_size; j++) {
            if (scores[indices[j]] > scores[indices[max_idx]]) {
                max_idx = j;
            }
        }
        int tmp = indices[i];
        indices[i] = indices[max_idx];
        indices[max_idx] = tmp;
    }

    /* Print results */
    printk("\nTop %d Results:\n", TOP_K_RESULTS);
    printk("----------------------------------------\n");
    for (int i = 0; i < TOP_K_RESULTS && i < output_size; i++) {
        int idx = indices[i];
        const char *label = (idx >= 0 && idx < LABEL_COUNT) ? labels[idx] : "?";
        printk("%d. %-30s %.2f%%\n", i + 1, label, (double)(scores[idx] * 100.0f));
    }
    printk("----------------------------------------\n\n");
}
