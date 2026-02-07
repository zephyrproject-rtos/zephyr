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
#include "transport/rpmsg_transport.h"

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
static int16_t buffer_a[BUFFER_SIZE_SAMPLES];
static int16_t buffer_b[BUFFER_SIZE_SAMPLES];
static int16_t *write_buffer = buffer_a;
static int16_t *processing_buffer = buffer_b;
/* Threading & Synchronization */
K_THREAD_STACK_DEFINE(thread_receive_stack, APP_RECEIVE_TASK_STACK_SIZE);
K_THREAD_STACK_DEFINE(thread_audio_processing_stack, APP_AUDIO_PROCESSING_TASK_STACK_SIZE);
static struct k_thread thread_receive_data;
static struct k_thread thread_audio_processing_data;
static K_SEM_DEFINE(processing_buffer_ready_sem, 0, 1);
static K_SEM_DEFINE(ml_complete_sem, 1, 1); /* Starts at 1 to allow the first file */
static K_MUTEX_DEFINE(buffer_access_mutex);
/* Shared State (protected by buffer_access_mutex) */
static uint16_t g_samples_in_write_buffer = 0;
static uint16_t g_samples_in_processing_buffer = 0;
static bool g_eof_received = false;

extern "C" {

void app_receive_data_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	int ret;

	LOG_INF("Receiving data thread started");

	k_sem_take(&data_tty_ready_sem, K_FOREVER);
	ret = rpmsg_create_ept(&tty_ept, rpdev, "audio_pcm", RPMSG_ADDR_ANY, RPMSG_ADDR_ANY, rpmsg_recv_tty_callback, NULL);
	if (ret != 0) {
		LOG_ERR("Could not create RPMsg endpoint");
		return;
	}

	while (1) {
                /* Wait for previous buffer to be fully processed */
		k_sem_take(&ml_complete_sem, K_FOREVER);

		while(1) {
			struct rpmsg_rcv_msg rx_msg;
                        /* Wait for messages */
			k_msgq_get(&tty_msgq, &rx_msg, K_FOREVER);

			bool should_break = false;

			if (rx_msg.len == 3 && memcmp(rx_msg.data, "EOF", 3) == 0) { /* EOF */
				k_mutex_lock(&buffer_access_mutex, K_FOREVER);
				g_eof_received = true;
                                /* Swap and process any leftover data */
				if (g_samples_in_write_buffer > 0) {
					int16_t *temp = write_buffer;
					write_buffer = processing_buffer;
					processing_buffer = temp;
					g_samples_in_processing_buffer = g_samples_in_write_buffer;
					g_samples_in_write_buffer = 0;
					k_sem_give(&processing_buffer_ready_sem);
				} else { /* Still signal to trigger final processing if needed */
					k_sem_give(&processing_buffer_ready_sem);
				}
				k_mutex_unlock(&buffer_access_mutex);
				should_break = true;
			} else { /* Receive data */
				k_mutex_lock(&buffer_access_mutex, K_FOREVER);
				size_t bytes_to_copy = MIN(rx_msg.len, (BUFFER_SIZE_SAMPLES - g_samples_in_write_buffer) * SAMPLE_SIZE_BYTES);
				memcpy((uint8_t*)write_buffer + g_samples_in_write_buffer * SAMPLE_SIZE_BYTES, rx_msg.data, bytes_to_copy);
				g_samples_in_write_buffer += bytes_to_copy / SAMPLE_SIZE_BYTES;
                                /* Buffer is full. Swap buffer */
				if (g_samples_in_write_buffer >= BUFFER_SIZE_SAMPLES) {
					int16_t *temp = write_buffer;
					write_buffer = processing_buffer;
					processing_buffer = temp;
					g_samples_in_processing_buffer = g_samples_in_write_buffer;
					g_samples_in_write_buffer = 0;
					k_sem_give(&processing_buffer_ready_sem);
				}
				k_mutex_unlock(&buffer_access_mutex);
			}
			rpmsg_release_rx_buffer(&tty_ept, rx_msg.data);
			if (should_break) {
				break; /* Wait for ml_complete_sem */
			}
		}
	}
}

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
		bool is_eof = g_eof_received;
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

		if (is_eof) { /* EOF processing */
			k_mutex_lock(&buffer_access_mutex, K_FOREVER);
			g_eof_received = false;
			k_mutex_unlock(&buffer_access_mutex);

			LOG_DBG("Buffer processing complete. Ready for next buffer.");
			k_sem_give(&ml_complete_sem);
		}
		k_sem_give(&ml_complete_sem);
	}
}

void setup() {

        LOG_INF("Starting Micro Speech OpenAMP application");

        /* Initialize model runner */
        model_runner_init();

        /* Initialize RPMsg transport */
        rpmsg_transport_start();

        /* Create data receiving thread (higher priority) */
        k_thread_create(&thread_receive_data, thread_receive_stack, APP_RECEIVE_TASK_STACK_SIZE,
                        app_receive_data_thread, NULL, NULL, NULL, 5, 0, K_NO_WAIT);

        /* Create audio processing thread (lower priority) */
        k_thread_create(&thread_audio_processing_data, thread_audio_processing_stack, APP_AUDIO_PROCESSING_TASK_STACK_SIZE,
                        app_audio_processing_thread, NULL, NULL, NULL, 6, 0, K_NO_WAIT);
}

void loop() {
        k_sleep(K_FOREVER);
}
} /* extern "C" */
