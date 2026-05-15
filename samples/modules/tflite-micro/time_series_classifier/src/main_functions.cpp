/*
 * Copyright 2020 The TensorFlow Authors. All Rights Reserved.
 * Copyright (c) 2025 Aerlync Labs Inc.
 * Copyright (c) 2026 Mahad Faisal
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "main_functions.h"

#include <math.h>

#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/micro/micro_log.h>
#include <tensorflow/lite/micro/micro_mutable_op_resolver.h>
#include <tensorflow/lite/micro/system_setup.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include <zephyr/kernel.h>

#include "features.h"
#include "input_data.h"
#include "model.hpp"

namespace {
const tflite::Model *model = nullptr;
tflite::MicroInterpreter *interpreter = nullptr;
TfLiteTensor *input = nullptr;
TfLiteTensor *output = nullptr;
size_t inference_count = 0U;

constexpr int kTensorArenaSize = 3000;
uint8_t tensor_arena[kTensorArenaSize];

float normalize_feature(size_t index, const struct time_series_features *features)
{
switch (index) {
case 0:
return features->mean / 3200.0f;
case 1:
return features->rms / 1200.0f;
case 2:
return features->peak / 3200.0f;
case 3:
return features->zero_crossing_rate / 0.10f;
default:
return 0.0f;
}
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

enum time_series_label output_to_label(size_t index)
{
switch (index) {
case 0:
return TIME_SERIES_NORMAL;
case 1:
return TIME_SERIES_LOW_FREQUENCY;
case 2:
return TIME_SERIES_TRANSIENT;
default:
return TIME_SERIES_NORMAL;
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
resolver.AddFullyConnected();

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

inference_count = 0U;
}

void loop(void)
{
const struct time_series_window *window =
time_series_get_window(inference_count);
struct time_series_features features;
enum time_series_label predicted;
float best_score;
size_t best_index = 0U;

time_series_extract_features(window->samples, window->len, &features);

for (size_t i = 0; i < 4U; i++) {
input->data.int8[i] = quantize_input(normalize_feature(i, &features));
}

uint32_t start_cycles = k_cycle_get_32();
TfLiteStatus invoke_status = interpreter->Invoke();
uint32_t end_cycles = k_cycle_get_32();
uint32_t invoke_cycles = end_cycles - start_cycles;
uint32_t invoke_us = (uint32_t)(k_cyc_to_ns_floor64(invoke_cycles) / 1000U);

if (invoke_status != kTfLiteOk) {
MicroPrintf("Invoke failed");
return;
}

best_score = dequantize_output(output->data.int8[0]);
for (size_t i = 1U; i < 3U; i++) {
float score = dequantize_output(output->data.int8[i]);

if (score > best_score) {
best_score = score;
best_index = i;
}
}

predicted = output_to_label(best_index);

MicroPrintf("input_window: %s\n", window->name);
MicroPrintf("features: mean=%f, rms=%f, peak=%f, zcr=%f\n",
    (double)features.mean,
    (double)features.rms,
    (double)features.peak,
    (double)features.zero_crossing_rate);
MicroPrintf("invoke_cycles: %u, invoke_time_us: %u\n",
    (unsigned int)invoke_cycles, (unsigned int)invoke_us);
MicroPrintf("prediction: %s, score: %f\n",
    time_series_label_name(predicted),
    (double)best_score);
MicroPrintf("result: %s\n",
    predicted == window->label ? "PASS" : "FAIL");

inference_count++;
if (inference_count >= TIME_SERIES_NUM_WINDOWS) {
inference_count = 0U;
}
}
