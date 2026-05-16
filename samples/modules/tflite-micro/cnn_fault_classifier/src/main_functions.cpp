/*
 * SPDX-FileCopyrightText: Copyright 2020 The TensorFlow Authors. All Rights Reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Mahad Faisal
 * SPDX-License-Identifier: Apache-2.0
 */

#include "main_functions.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>

#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/micro/micro_log.h>
#include <tensorflow/lite/micro/micro_mutable_op_resolver.h>
#include <tensorflow/lite/micro/system_setup.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include <zephyr/kernel.h>

#include "input_data.h"
#include "model.hpp"

namespace {
const tflite::Model *model = nullptr;
tflite::MicroInterpreter *interpreter = nullptr;
TfLiteTensor *input = nullptr;
TfLiteTensor *output = nullptr;

size_t window_index = 0U;

constexpr int kTensorArenaSize = 8192;
uint8_t tensor_arena[kTensorArenaSize];

float normalize_sample(int16_t sample)
{
return (float)sample / 3500.0f;
}

int8_t quantize_input(float value)
{
int32_t quantized = (int32_t)roundf(value / input->params.scale) +
    input->params.zero_point;

if (quantized > 127) {
return 127;
}

if (quantized < -128) {
return -128;
}

return (int8_t)quantized;
}

float dequantize_output(int8_t value)
{
return ((float)value - (float)output->params.zero_point) *
       output->params.scale;
}

enum cnn_fault_label output_to_label(size_t index)
{
switch (index) {
case 0:
return CNN_FAULT_NORMAL;
case 1:
return CNN_FAULT_HIGH_FREQUENCY;
case 2:
return CNN_FAULT_IMPULSE;
default:
return CNN_FAULT_NORMAL;
}
}
}  /* namespace */

void setup(void)
{
model = tflite::GetModel(g_model);
if (model->version() != TFLITE_SCHEMA_VERSION) {
MicroPrintf("Model schema version %d not equal to supported version %d.",
    model->version(), TFLITE_SCHEMA_VERSION);
return;
}

static tflite::MicroMutableOpResolver<1> resolver;
	resolver.AddConv2D();

static tflite::MicroInterpreter static_interpreter(
model, resolver, tensor_arena, kTensorArenaSize);
interpreter = &static_interpreter;

TfLiteStatus allocate_status = interpreter->AllocateTensors();
if (allocate_status != kTfLiteOk) {
MicroPrintf("AllocateTensors() failed");
return;
}

input = interpreter->input(0);
output = interpreter->output(0);
window_index = 0U;
}

void loop(void)
{
const struct cnn_fault_window *window = cnn_fault_get_window(window_index);
enum cnn_fault_label prediction;
size_t best_index = 0U;
float best_score;
uint32_t invoke_start;
uint32_t invoke_cycles;
uint32_t invoke_us;

for (size_t i = 0U; i < window->len; i++) {
input->data.int8[i] = quantize_input(normalize_sample(window->samples[i]));
}

invoke_start = k_cycle_get_32();
TfLiteStatus invoke_status = interpreter->Invoke();
invoke_cycles = k_cycle_get_32() - invoke_start;
invoke_us = (uint32_t)(k_cyc_to_ns_floor64(invoke_cycles) / 1000U);

if (invoke_status != kTfLiteOk) {
MicroPrintf("Invoke failed\n");
k_sleep(K_FOREVER);
return;
}

best_score = dequantize_output(output->data.int8[0]);
for (size_t i = 1U; i < CNN_FAULT_NUM_CLASSES; i++) {
float score = dequantize_output(output->data.int8[i]);

if (score > best_score) {
best_score = score;
best_index = i;
}
}

prediction = output_to_label(best_index);

MicroPrintf("input_window: %s\n", window->name);
MicroPrintf("invoke_cycles: %u, invoke_time_us: %u\n",
    (unsigned int)invoke_cycles,
    (unsigned int)invoke_us);
MicroPrintf("prediction: %s, score: %f\n",
    cnn_fault_label_name(prediction),
    (double)best_score);
MicroPrintf("result: %s\n",
    prediction == window->label ? "PASS" : "FAIL");

window_index++;
	if (window_index >= CNN_FAULT_NUM_CLASSES) {
		while (true) {
			k_sleep(K_MSEC(1000));
		}
	}
}
