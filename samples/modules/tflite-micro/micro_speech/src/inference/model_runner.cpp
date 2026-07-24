/*
 * Copyright 2025 The TensorFlow Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "model_runner.hpp"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(model_runner);

#include "tensorflow/lite/core/c/common.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"

#include "audio_preprocessor_int8_model.hpp"
#include "micro_speech_quantized_model.hpp"

#include "transport/rpmsg_transport.h"

#include <zephyr/kernel.h>
#include <algorithm>
#include <stdio.h>
#include <string.h>

/* Features type for audio processing */
Features g_features;

namespace {

/* Arena size for model operations - split for two models */
constexpr size_t kAudioPreprocessorArenaSize = 16384;  /* ~16KB for audio preprocessor */
constexpr size_t kMicroSpeechArenaSize = 12200;        /* ~12KB for micro speech */
alignas(16) uint8_t g_audio_preprocessor_arena[kAudioPreprocessorArenaSize];
alignas(16) uint8_t g_micro_speech_arena[kMicroSpeechArenaSize];

/* Audio sample constants */
constexpr int kAudioSampleDurationCount = kFeatureDurationMs * kAudioSampleFrequency / 1000;
constexpr int kAudioSampleStrideCount = kFeatureStrideMs * kAudioSampleFrequency / 1000;

/* Operation resolver types */
using MicroSpeechOpResolver = tflite::MicroMutableOpResolver<4>;
using AudioPreprocessorOpResolver = tflite::MicroMutableOpResolver<18>;

/* Static interpreters and models - initialized once */
static const tflite::Model* g_audio_preprocessor_model = nullptr;
static const tflite::Model* g_micro_speech_model = nullptr;
static AudioPreprocessorOpResolver* g_audio_preprocessor_op_resolver = nullptr;
static MicroSpeechOpResolver* g_micro_speech_op_resolver = nullptr;
static tflite::MicroInterpreter* g_audio_preprocessor_interpreter = nullptr;
static tflite::MicroInterpreter* g_micro_speech_interpreter = nullptr;
static bool g_interpreters_initialized = false;


TfLiteStatus register_micro_speech_ops(MicroSpeechOpResolver& op_resolver) {
    TF_LITE_ENSURE_STATUS(op_resolver.AddReshape());
    TF_LITE_ENSURE_STATUS(op_resolver.AddFullyConnected());
    TF_LITE_ENSURE_STATUS(op_resolver.AddDepthwiseConv2D());
    TF_LITE_ENSURE_STATUS(op_resolver.AddSoftmax());
    return kTfLiteOk;
}

TfLiteStatus register_audio_preprocessor_ops(AudioPreprocessorOpResolver& op_resolver) {
    TF_LITE_ENSURE_STATUS(op_resolver.AddReshape());
    TF_LITE_ENSURE_STATUS(op_resolver.AddCast());
    TF_LITE_ENSURE_STATUS(op_resolver.AddStridedSlice());
    TF_LITE_ENSURE_STATUS(op_resolver.AddConcatenation());
    TF_LITE_ENSURE_STATUS(op_resolver.AddMul());
    TF_LITE_ENSURE_STATUS(op_resolver.AddAdd());
    TF_LITE_ENSURE_STATUS(op_resolver.AddDiv());
    TF_LITE_ENSURE_STATUS(op_resolver.AddMinimum());
    TF_LITE_ENSURE_STATUS(op_resolver.AddMaximum());
    TF_LITE_ENSURE_STATUS(op_resolver.AddWindow());
    TF_LITE_ENSURE_STATUS(op_resolver.AddFftAutoScale());
    TF_LITE_ENSURE_STATUS(op_resolver.AddRfft());
    TF_LITE_ENSURE_STATUS(op_resolver.AddEnergy());
    TF_LITE_ENSURE_STATUS(op_resolver.AddFilterBank());
    TF_LITE_ENSURE_STATUS(op_resolver.AddFilterBankSquareRoot());
    TF_LITE_ENSURE_STATUS(op_resolver.AddFilterBankSpectralSubtraction());
    TF_LITE_ENSURE_STATUS(op_resolver.AddPCAN());
    TF_LITE_ENSURE_STATUS(op_resolver.AddFilterBankLog());
    return kTfLiteOk;
}

TfLiteStatus initialize_interpreters() {
    if (g_interpreters_initialized) {
        return kTfLiteOk;  // Already initialized
    }

    LOG_INF("Initializing static interpreters");

    /* Audio preprocessor: Get model */
    g_audio_preprocessor_model = tflite::GetModel(g_audio_preprocessor_int8_model);
    if (g_audio_preprocessor_model->version() != TFLITE_SCHEMA_VERSION) {
        LOG_ERR("Audio preprocessor model version mismatch: %d vs %d", g_audio_preprocessor_model->version(), TFLITE_SCHEMA_VERSION);
        return kTfLiteError;
    }
    /* Audio preprocessor: OpResolver */
    static AudioPreprocessorOpResolver audio_preprocessor_op_resolver;
    g_audio_preprocessor_op_resolver = &audio_preprocessor_op_resolver;
    if (register_audio_preprocessor_ops(*g_audio_preprocessor_op_resolver) != kTfLiteOk) {
        LOG_ERR("Failed to register audio preprocessor ops");
        return kTfLiteError;
    }
    /* Audio preprocessor: Interpreter */
    static tflite::MicroInterpreter audio_preprocessor_interpreter(g_audio_preprocessor_model, *g_audio_preprocessor_op_resolver, g_audio_preprocessor_arena, kAudioPreprocessorArenaSize);
    g_audio_preprocessor_interpreter = &audio_preprocessor_interpreter;
    if (g_audio_preprocessor_interpreter->AllocateTensors() != kTfLiteOk) {
        LOG_ERR("Failed to allocate tensors for audio preprocessor");
        return kTfLiteError;
    }

    /* Micro speech: Get model */
    g_micro_speech_model = tflite::GetModel(g_micro_speech_quantized_model);
    if (g_micro_speech_model->version() != TFLITE_SCHEMA_VERSION) {
        LOG_ERR("MicroSpeech model version mismatch: %d vs %d",
            g_micro_speech_model->version(), TFLITE_SCHEMA_VERSION);
        return kTfLiteError;
    }

    /* Micro speech: Op Resolver */
    static MicroSpeechOpResolver micro_speech_op_resolver;
    g_micro_speech_op_resolver = &micro_speech_op_resolver;
    if (register_micro_speech_ops(*g_micro_speech_op_resolver) != kTfLiteOk) {
        LOG_ERR("Failed to register MicroSpeech ops");
        return kTfLiteError;
    }

    /* Micro speech: Interpreter */
    static tflite::MicroInterpreter micro_speech_interpreter(g_micro_speech_model, *g_micro_speech_op_resolver, g_micro_speech_arena, kMicroSpeechArenaSize);
    g_micro_speech_interpreter = &micro_speech_interpreter;
    if (g_micro_speech_interpreter->AllocateTensors() != kTfLiteOk) {
        LOG_ERR("Failed to allocate tensors for MicroSpeech");
        return kTfLiteError;
    }

    g_interpreters_initialized = true;
    LOG_INF("Static interpreters initialized successfully");

    return kTfLiteOk;
}

TfLiteStatus generate_single_feature(const int16_t* audio_data,
                                    const int audio_data_size,
                                    int8_t* feature_output) {
    if (!g_interpreters_initialized) {
        LOG_ERR("Interpreters not initialized");
        return kTfLiteError;
    }

    TfLiteTensor* input = g_audio_preprocessor_interpreter->input(0);
    if (!input) {
        LOG_ERR("Failed to get input tensor");
        return kTfLiteError;
    }

    /* Check input shape is compatible with our audio sample size */
    if (audio_data_size != kAudioSampleDurationCount) {
        LOG_ERR("Audio data size mismatch: %d vs %d", audio_data_size, kAudioSampleDurationCount);
        return kTfLiteError;
    }

    TfLiteTensor* output = g_audio_preprocessor_interpreter->output(0);
    if (!output) {
        LOG_ERR("Failed to get output tensor");
        return kTfLiteError;
    }

    /* Copy audio data to input tensor */
    std::copy_n(audio_data, audio_data_size, tflite::GetTensorData<int16_t>(input));

    /* Run inference */
    if (g_audio_preprocessor_interpreter->Invoke() != kTfLiteOk) {
        LOG_ERR("Invoke failed");
        return kTfLiteError;
    }

    /* Copy output to feature buffer */
    std::copy_n(tflite::GetTensorData<int8_t>(output), kFeatureSize, feature_output);

    return kTfLiteOk;
}

TfLiteStatus generate_features(const int16_t* audio_data,
                            const size_t audio_data_size,
                            Features* features_output) {
    if (!g_interpreters_initialized) {
        LOG_ERR("Interpreters not initialized");
        return kTfLiteError;
    }

    /* Clear previous features */
    memset(features_output, 0, sizeof(Features));

    /* Process audio in stride windows */
    size_t remaining_samples = audio_data_size;
    size_t feature_index = 0;
    const int16_t* current_audio = audio_data;

    while (remaining_samples >= kAudioSampleDurationCount && feature_index < kFeatureCount) {
        if (generate_single_feature(current_audio, kAudioSampleDurationCount,
                    (*features_output)[feature_index]) != kTfLiteOk) {
            LOG_ERR("Failed to generate feature %d", feature_index);
            return kTfLiteError;
        }

        feature_index++;
        current_audio += kAudioSampleStrideCount;
        remaining_samples -= kAudioSampleStrideCount;
    }

    return kTfLiteOk;
}

TfLiteStatus run_micro_speech_inference(const Features& features) {
    if (!g_interpreters_initialized) {
        LOG_ERR("Interpreters not initialized");
        return kTfLiteError;
    }

    /* Get input tensor */
    TfLiteTensor* input = g_micro_speech_interpreter->input(0);
    if (!input) {
        LOG_ERR("Failed to get input tensor");
        return kTfLiteError;
    }

    /* Check input shape is compatible with our feature data size */
    if (input->dims->data[input->dims->size - 1] != kFeatureElementCount) {
        LOG_ERR("Input tensor size mismatch");
        return kTfLiteError;
    }

    TfLiteTensor* output = g_micro_speech_interpreter->output(0);
    if (!output) {
        LOG_ERR("Failed to get output tensor");
        return kTfLiteError;
    }

    /* Check output shape is compatible with our number of categories */
    if (output->dims->data[output->dims->size - 1] != kCategoryCount) {
        LOG_ERR("Output tensor size mismatch");
        return kTfLiteError;
    }

    float output_scale = output->params.scale;
    int output_zero_point = output->params.zero_point;

    /* Copy feature data to input tensor */
    std::copy_n(&features[0][0], kFeatureElementCount, tflite::GetTensorData<int8_t>(input));

    /* Run inference */
    if (g_micro_speech_interpreter->Invoke() != kTfLiteOk) {
        LOG_ERR("Invoke failed");
        return kTfLiteError;
    }

    /* Dequantize and print results */
    float category_predictions[kCategoryCount];
    for (int i = 0; i < kCategoryCount; i++) {
        category_predictions[i] = (tflite::GetTensorData<int8_t>(output)[i] - output_zero_point) * output_scale;
    }

    int prediction_index = std::distance(
        std::begin(category_predictions),
        std::max_element(std::begin(category_predictions), std::end(category_predictions))
    );

    LOG_INF("Detected: %s", kCategoryLabels[prediction_index]);

    return kTfLiteOk;
}

} /* namespace */

/* C API implementation */
extern "C" {

void model_runner_init(void) {
    /* Initialize static interpreters */
    if (initialize_interpreters() != kTfLiteOk) {
        LOG_ERR("Failed to initialize interpreters");
        return;
    }
}

int micro_speech_process_audio(const int16_t *audio_data,
                                        size_t audio_data_size) {
    /* Generate features */
    if (generate_features(audio_data, audio_data_size, &g_features) != kTfLiteOk) {
        LOG_ERR("Failed to generate features");
        return -1;
    }
    /* Run inference */
    if (run_micro_speech_inference(g_features) != kTfLiteOk) {
        LOG_ERR("Inference failed");
        return -2;
    }
    return 0;
}

} /* extern "C" */
