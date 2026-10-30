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

#ifndef MICRO_SPEECH_OPENAMP_MODEL_RUNNER_H_
#define MICRO_SPEECH_OPENAMP_MODEL_RUNNER_H_

#include <cstddef>
#include <cstdint>
#include "micro_model_settings.h"

using Features = int8_t[kFeatureCount][kFeatureSize];

extern Features g_features;

extern "C" {
void model_runner_init(void);
int micro_speech_process_audio(const int16_t *audio_data, size_t audio_data_size);
}

#endif /* MICRO_SPEECH_OPENAMP_MODEL_RUNNER_H_ */
