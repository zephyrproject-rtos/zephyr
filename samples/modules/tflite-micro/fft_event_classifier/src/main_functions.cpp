/*
 * Copyright 2020 The TensorFlow Authors. All Rights Reserved.
 * Copyright (c) 2026 Mahad Faisal
 *
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

#include "dsp_features.h"
#include "input_data.h"
#include "model.hpp"

namespace {
const tflite::Model *model = nullptr;
tflite::MicroInterpreter *interpreter = nullptr;
TfLiteTensor *input = nullptr;
TfLiteTensor *output = nullptr;
size_t inference_count = 0U;

constexpr int kTensorArenaSize = 4096;
uint8_t tensor_arena[kTensorArenaSize];

float normalize_feature(size_t index, const struct fft_event_features *features)
{
switch (index) {
case 0:
return features->mean / 4000.0f;
case 1:
return features->rms / 2000.0f;
case 2:
return features->peak / 4000.0f;
case 3:
return features->zero_crossing_rate / 0.50f;
case 4:
return features->low_band_energy;
case 5:
return features->mid_band_energy;
case 6:
return features->high_band_energy;
case 7:
return features->spectral_centroid;
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

enum fft_event_label output_to_label(size_t index)
{
switch (index) {
case 0:
return FFT_EVENT_NORMAL;
case 1:
return FFT_EVENT_HIGH_FREQUENCY;
case 2:
return FFT_EVENT_TRANSIENT;
default:
return FFT_EVENT_NORMAL;
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
const struct fft_event_window *window =
fft_event_get_window(inference_count);
struct fft_event_features features;
enum fft_event_label predicted;
uint32_t preprocess_start;
uint32_t preprocess_cycles;
uint32_t preprocess_us;
uint32_t invoke_start;
uint32_t invoke_cycles;
uint32_t invoke_us;
uint32_t total_cycles;
uint32_t total_us;
float best_score;
size_t best_index = 0U;

preprocess_start = k_cycle_get_32();

if (fft_event_extract_features(window->samples, window->len, &features) != 0) {
MicroPrintf("Feature extraction failed\n");
return;
}

preprocess_cycles = k_cycle_get_32() - preprocess_start;
preprocess_us = (uint32_t)(k_cyc_to_ns_floor64(preprocess_cycles) / 1000U);

for (size_t i = 0U; i < FFT_EVENT_NUM_FEATURES; i++) {
input->data.int8[i] = quantize_input(normalize_feature(i, &features));
}

invoke_start = k_cycle_get_32();
TfLiteStatus invoke_status = interpreter->Invoke();
invoke_cycles = k_cycle_get_32() - invoke_start;
invoke_us = (uint32_t)(k_cyc_to_ns_floor64(invoke_cycles) / 1000U);

if (invoke_status != kTfLiteOk) {
MicroPrintf("Invoke failed\n");
return;
}

best_score = dequantize_output(output->data.int8[0]);
for (size_t i = 1U; i < FFT_EVENT_NUM_CLASSES; i++) {
float score = dequantize_output(output->data.int8[i]);

if (score > best_score) {
best_score = score;
best_index = i;
}
}

predicted = output_to_label(best_index);
total_cycles = preprocess_cycles + invoke_cycles;
total_us = preprocess_us + invoke_us;

MicroPrintf("input_window: %s\n", window->name);
MicroPrintf("features: rms=%f, peak=%f, zcr=%f, low=%f, mid=%f, high=%f, centroid=%f\n",
    (double)features.rms,
    (double)features.peak,
    (double)features.zero_crossing_rate,
    (double)features.low_band_energy,
    (double)features.mid_band_energy,
    (double)features.high_band_energy,
    (double)features.spectral_centroid);
MicroPrintf("preprocess_cycles: %u, preprocess_time_us: %u\n",
    (unsigned int)preprocess_cycles,
    (unsigned int)preprocess_us);
MicroPrintf("invoke_cycles: %u, invoke_time_us: %u\n",
    (unsigned int)invoke_cycles,
    (unsigned int)invoke_us);
MicroPrintf("total_cycles: %u, total_time_us: %u\n",
    (unsigned int)total_cycles,
    (unsigned int)total_us);
MicroPrintf("prediction: %s, score: %f\n",
    fft_event_label_name(predicted),
    (double)best_score);
MicroPrintf("result: %s\n",
    predicted == window->label ? "PASS" : "FAIL");

inference_count++;
if (inference_count >= FFT_EVENT_NUM_CLASSES) {
inference_count = 0U;
}
}
