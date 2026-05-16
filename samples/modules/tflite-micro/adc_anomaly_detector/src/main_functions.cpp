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

#include "features.h"
#include "model.hpp"
#include "sample_source.h"

namespace {
const tflite::Model *model = nullptr;
tflite::MicroInterpreter *interpreter = nullptr;
TfLiteTensor *input = nullptr;
TfLiteTensor *output = nullptr;

int16_t current_window[ADC_ANOMALY_WINDOW_SIZE];
enum adc_anomaly_label prediction_history[3];
size_t sample_window_index = 0U;
size_t history_count = 0U;
size_t history_head = 0U;

constexpr int kTensorArenaSize = 4096;
uint8_t tensor_arena[kTensorArenaSize];

void prediction_history_reset(void)
{
history_count = 0U;
history_head = 0U;
}

void prediction_history_push(enum adc_anomaly_label label)
{
prediction_history[history_head] = label;
history_head = (history_head + 1U) % 3U;

if (history_count < 3U) {
history_count++;
}
}

enum adc_anomaly_label prediction_history_majority(void)
{
uint32_t counts[ADC_ANOMALY_NUM_CLASSES] = {0U};
uint32_t best_count = 0U;
enum adc_anomaly_label best_label = ADC_ANOMALY_NORMAL;

for (size_t i = 0U; i < history_count; i++) {
enum adc_anomaly_label label = prediction_history[i];

counts[(size_t)label]++;
}

for (size_t i = 0U; i < ADC_ANOMALY_NUM_CLASSES; i++) {
if (counts[i] > best_count) {
best_count = counts[i];
best_label = (enum adc_anomaly_label)i;
}
}

return best_label;
}

float normalize_feature(size_t index, const struct adc_anomaly_features *features)
{
switch (index) {
case 0:
return features->mean / 3000.0f;
case 1:
return features->rms / 2000.0f;
case 2:
return features->peak / 4000.0f;
case 3:
return features->zero_crossing_rate / 0.20f;
case 4:
return features->slope / 100.0f;
case 5:
return features->energy / 200000000.0f;
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

enum adc_anomaly_label output_to_label(size_t index)
{
switch (index) {
case 0:
return ADC_ANOMALY_NORMAL;
case 1:
return ADC_ANOMALY_DRIFT;
case 2:
return ADC_ANOMALY_TRANSIENT;
default:
return ADC_ANOMALY_NORMAL;
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

if (sample_source_init() != 0) {
MicroPrintf("sample source init failed\n");
return;
}

prediction_history_reset();
sample_window_index = 0U;
MicroPrintf("sample_source: %s\n", sample_source_name());
}

void loop(void)
{
struct adc_anomaly_features features;
enum adc_anomaly_label expected;
enum adc_anomaly_label raw_prediction;
enum adc_anomaly_label stable_prediction;
size_t best_index = 0U;
float best_score;
uint32_t invoke_start;
uint32_t invoke_cycles;
uint32_t invoke_us;
int ret;

ret = sample_source_load_window(sample_window_index, current_window,
ADC_ANOMALY_WINDOW_SIZE);
if (ret != 0) {
MicroPrintf("sample source failed: %d\n", ret);
k_sleep(K_FOREVER);
return;
}

expected = sample_source_expected_label(sample_window_index);

	if (sample_source_has_expected_labels() &&
	    (sample_window_index % ADC_ANOMALY_WINDOWS_PER_CLASS) == 0U) {
		prediction_history_reset();
	}
adc_anomaly_extract_features(current_window, ADC_ANOMALY_WINDOW_SIZE,
	 &features);

for (size_t i = 0U; i < ADC_ANOMALY_NUM_FEATURES; i++) {
input->data.int8[i] = quantize_input(normalize_feature(i, &features));
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
for (size_t i = 1U; i < ADC_ANOMALY_NUM_CLASSES; i++) {
float score = dequantize_output(output->data.int8[i]);

if (score > best_score) {
best_score = score;
best_index = i;
}
}

raw_prediction = output_to_label(best_index);
prediction_history_push(raw_prediction);
stable_prediction = prediction_history_majority();

MicroPrintf("sample_window: %u\n", (unsigned int)sample_window_index);
MicroPrintf("window_start: %u\n",
	(unsigned int)sample_source_window_start(sample_window_index));
MicroPrintf("features: mean=%f, rms=%f, peak=%f, zcr=%f, slope=%f, energy=%f\n",
	(double)features.mean,
	(double)features.rms,
	(double)features.peak,
	(double)features.zero_crossing_rate,
	(double)features.slope,
	(double)features.energy);
MicroPrintf("invoke_cycles: %u, invoke_time_us: %u\n",
	(unsigned int)invoke_cycles,
	(unsigned int)invoke_us);
MicroPrintf("raw_prediction: %s, score: %f\n",
	sample_source_label_name(raw_prediction),
	(double)best_score);
MicroPrintf("stable_prediction: %s\n",
	sample_source_label_name(stable_prediction));

if (stable_prediction != ADC_ANOMALY_NORMAL) {
MicroPrintf("event: %s detected\n",
	sample_source_label_name(stable_prediction));
}

if (sample_source_has_expected_labels()) {
MicroPrintf("result: %s\n",
	raw_prediction == expected ? "PASS" : "FAIL");
} else {
MicroPrintf("result: LIVE\n");
}

sample_window_index++;
if (sample_window_index >= ADC_ANOMALY_TOTAL_WINDOWS) {
k_sleep(K_FOREVER);
}
}
