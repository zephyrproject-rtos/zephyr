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

#include "main_functions.hpp"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(micro_speech_openamp);

#include "inference/model_runner.hpp"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/sys/util.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Stack sizes threads */
#define APP_RECEIVE_TASK_STACK_SIZE 			(2048)
#define APP_AUDIO_PROCESSING_TASK_STACK_SIZE 	        (4096)
/* Audio pipeline constants */
#define SAMPLE_SIZE_BYTES 			        (sizeof(int16_t))
#define SAMPLES_PER_SECOND 				(16000)
#define BUFFER_SIZE_SAMPLES 				(SAMPLES_PER_SECOND)
#define BUFFER_SIZE_BYTES 				(BUFFER_SIZE_SAMPLES * SAMPLE_SIZE_BYTES)
/* Double buffering for receiving and processing */
static int16_t buffer_b[BUFFER_SIZE_SAMPLES];
static int16_t *processing_buffer = buffer_b;
/* Threading & Synchronization */
K_THREAD_STACK_DEFINE(thread_receive_stack, APP_RECEIVE_TASK_STACK_SIZE);
K_THREAD_STACK_DEFINE(thread_audio_processing_stack, APP_AUDIO_PROCESSING_TASK_STACK_SIZE);
static struct k_thread thread_audio_processing_data;
static K_SEM_DEFINE(processing_buffer_ready_sem, 0, 1);
static K_SEM_DEFINE(ml_complete_sem, 1, 1); /* Starts at 1 to allow the first file */
static K_MUTEX_DEFINE(buffer_access_mutex);
/* Shared State (protected by buffer_access_mutex) */
static uint16_t g_samples_in_processing_buffer = 0;

extern "C" {

void app_audio_processing_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	LOG_INF("Audio processing thread started");

	while (1) {
                /* Wait for a full buffer or EOF */
		k_sem_take(&processing_buffer_ready_sem, K_FOREVER);

		k_mutex_lock(&buffer_access_mutex, K_FOREVER);
		size_t samples_to_process = g_samples_in_processing_buffer;
		g_samples_in_processing_buffer = 0; /* Mark buffer as claimed */
		k_mutex_unlock(&buffer_access_mutex);

		/* Run inference if data is available */
		if (samples_to_process > 0) {
                        LOG_DBG("Running inferences with %zu samples", samples_to_process);

			if (micro_speech_process_audio(processing_buffer, samples_to_process) != 0) {
				LOG_DBG("Failed to process audio");
			}
			/* Clear the buffer after processing */
			memset(processing_buffer, 0, BUFFER_SIZE_BYTES);
		}
		k_sem_give(&ml_complete_sem);
	}
}

void setup() {

        LOG_INF("Starting Micro Speech OpenAMP application");

        /* Initialize model runner */
        model_runner_init();

        /* Create audio processing thread (lower priority) */
        k_thread_create(&thread_audio_processing_data, thread_audio_processing_stack, APP_AUDIO_PROCESSING_TASK_STACK_SIZE,
                        app_audio_processing_thread, NULL, NULL, NULL, 6, 0, K_NO_WAIT);
}

void loop() {
        k_sleep(K_FOREVER);
}
} /* extern "C" */
