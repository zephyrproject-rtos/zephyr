/*
 * Copyright 2012,2021-2023,2025-2026 NXP.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "zephyr/uwb/tml.h"
#include "zephyr/uwb/uwb_core.h"
#include "zephyr/uwb/status.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(uwb_tml, CONFIG_UWB_LOG_LEVEL);

#ifndef CONFIG_UCI_MESSAGE_LEN_MAX
#define CONFIG_UCI_MESSAGE_LEN_MAX 4200
#endif /* CONFIG_UCI_MESSAGE_LEN_MAX */

#ifndef CONFIG_UWB_READER_STACK_SIZE
#define CONFIG_UWB_READER_STACK_SIZE 6000
#endif /* CONFIG_UWB_READER_STACK_SIZE */

#ifndef CONFIG_UWB_READER_PRIO
#define CONFIG_UWB_READER_PRIO 0
#endif /* CONFIG_UWB_READER_PRIO */

/**
 * Base Context Structure containing members required for entire session
 */
typedef struct uwb_tml_context {
	k_tid_t reader_thread;
	/** TODO: CA: Don't see a point of having this flag - remove */
	volatile uint8_t b_thread_done; /*Flag to decide whether to run or abort the thread */
	bool is_initialized;
	uint8_t b_enable;
	struct k_mutex read_mutex; /* Mutex for thread-safe access to b_enable */
} uwb_tml_context_t;

/* Initialize Context structure pointer used to access context structure */
static uwb_tml_context_t guwb_tml_context = {0};
static struct k_thread reader_thread;

/* Local Function prototypes */
static uwb_status_code_t uwb_tml_start_thread(void);

static void uwb_tml_reader_thread(void *p_param);

K_THREAD_STACK_DEFINE(TMLREAD_stack, CONFIG_UWB_READER_STACK_SIZE);

uwb_status_code_t uwb_tml_init(void)
{
	uwb_status_code_t status = kUwb_StatusCode_Success;

	/* Check if TML layer is already Initialized */
	if (guwb_tml_context.is_initialized) {
		/* TML initialization is already completed */
		if (guwb_tml_context.b_enable) {
			/* Thread not suspended */
			return status;
		}
		/* Thread initialized but suspended, resume thread */
		if (0 != uwb_transport_open()) {
			return kUwb_StatusCode_TransportFailed;
		}
		uwb_tml_resume_reader();
		uwb_tml_read();
		return status;
	}

	/* Initialise all the internal TML variables */
	memset(&guwb_tml_context, 0, sizeof(uwb_tml_context_t));
	/* Make sure that the thread runs once it is created */
	guwb_tml_context.b_thread_done = 1;

	/* Create mutex for read synchronization */
	if (k_mutex_init(&guwb_tml_context.read_mutex) != kUwb_StatusCode_Success) {
		LOG_ERR("Failed to create read mutex");
		return kUwb_StatusCode_TmlInitFailed;
	}

	/* Open the device file to which data is read/written */
	int ret = uwb_transport_open();

	if (0 != ret) {
		status = kUwb_StatusCode_TransportFailed;
	} else {
		guwb_tml_context.b_enable = 0;
		guwb_tml_context.is_initialized = true;
		/* Start TML thread (to handle write and read operations) */
		status = uwb_tml_start_thread();
	}

	/* Clean up all the TML resources if any error */
	if (kUwb_StatusCode_Success != status) {
		guwb_tml_context.is_initialized = false;
		uwb_transport_close();
		/* Clear memory allocated for storing Context variables */
		memset(&guwb_tml_context, 0, sizeof(uwb_tml_context_t));
	}

	return status;
}

/**
 *
 * Function         uwb_tml_start_thread
 *
 * Description      Initializes comport, reader and writer threads
 *
 * Parameters       None
 *
 * Returns          UWB status:
 *                  kUwb_StatusCode_Success - threads initialized successfully
 *                  kUwb_StatusCode_Failed - initialization failed due to system error
 *
 */
static uwb_status_code_t uwb_tml_start_thread(void)
{
	uwb_status_code_t status = kUwb_StatusCode_Success;

	guwb_tml_context.reader_thread = k_thread_create(
		&reader_thread, (k_thread_stack_t *)&TMLREAD_stack,
		K_THREAD_STACK_SIZEOF(TMLREAD_stack), (k_thread_entry_t)&uwb_tml_reader_thread,
		NULL, NULL, NULL, K_PRIO_COOP(CONFIG_UWB_READER_PRIO), 0, K_NO_WAIT);
	if (NULL == guwb_tml_context.reader_thread) {
		status = kUwb_StatusCode_Failed;
	}
	return status;
}

static void uwb_tml_reader_thread(void *p_param)
{
	(void)(p_param);
	int32_t no_bytes_wr_rd = 0;
	uint8_t temp[CONFIG_UCI_MESSAGE_LEN_MAX];
	uint8_t *pBuffer = &temp[0];
	uint32_t rx_buffer_len = 0;
	uint8_t read_enabled = 0;

	/* Initialize Message structure to post message onto Callback Thread */
	LOG_DBG("Tml Reader Thread Started...");
	/* Writer thread loop shall be running till shutdown is invoked */
	while (guwb_tml_context.b_thread_done) {
		/* Check if Tml read is requested with mutex protection */
		k_mutex_lock(&guwb_tml_context.read_mutex, K_FOREVER);
		read_enabled = guwb_tml_context.b_enable;
		k_mutex_unlock(&guwb_tml_context.read_mutex);

		if (1 == read_enabled) {
			LOG_DBG("Read requested...");

			/* Variable to fetch the actual number of bytes read */
			no_bytes_wr_rd = 0;

			/* Read the data from the file onto the buffer */
			LOG_DBG("Invoking Read...");
			no_bytes_wr_rd =
				uwb_transport_uci_read(pBuffer, CONFIG_UCI_MESSAGE_LEN_MAX);

			if (guwb_tml_context.b_thread_done == 0) {
				break;
			}

			if (no_bytes_wr_rd > CONFIG_UCI_MESSAGE_LEN_MAX) {
				LOG_ERR("Number of bytes read exceeds the limit ...");
			} else if (-1 == no_bytes_wr_rd) {
				LOG_DBG("%s : Read IRQ Timedout, re-read the packet", __FUNCTION__);
			} else if (0 == no_bytes_wr_rd) {
				LOG_DBG("Empty packet Read, Ignore read and try new read...");
			} else {
				rx_buffer_len = no_bytes_wr_rd;
				LOG_DBG("Read successful...");

				no_bytes_wr_rd = 0;

				/* Read operation completed successfully. Post a Message onto
				 * Callback Thread */
				/* Prepare the message to be posted on User thread */
				LOG_DBG("Posting read message...");

				uwb_uci_handle_received_packet(&temp[0], rx_buffer_len);
				pBuffer = &temp[0];
				rx_buffer_len = 0;
			}
		} else {
			LOG_DBG("read request NOT enabled");
			k_msleep(10);
		}
	} /* End of While loop */

	/* Suspend task here so that it does not return in FreeRTOS
	 * Task will be deleted in shutdown sequence
	 */
	k_thread_suspend(guwb_tml_context.reader_thread);
}

void uwb_tml_deinit(void)
{
	if (false == guwb_tml_context.is_initialized) {
		return;
	}

	uwb_tml_read_abort();
	uwb_tml_suspend_reader();
	uwb_transport_close();
}

uwb_status_code_t uwb_tml_write(uint8_t *p_data, uint16_t data_len)
{
	/* Check if TML layer is initialized */
	if (!guwb_tml_context.is_initialized) {
		/* TML initialization is not done */
		return kUwb_StatusCode_NotInitialized;
	}

	uwb_status_code_t wStatus = kUwb_StatusCode_Success;
	int32_t no_bytes_wr_rd = 0;

	if (!guwb_tml_context.is_initialized) {
		return kUwb_StatusCode_NotInitialized;
	}

	if ((NULL == p_data) || (0 == data_len)) {
		return kUwb_StatusCode_InvalidArgument;
	}

	no_bytes_wr_rd = uwb_transport_uci_write(p_data, data_len);
	if (no_bytes_wr_rd == -EBUSY) {
		wStatus = kUwb_StatusCode_TmlBusy;
	} else if (no_bytes_wr_rd < 0) {
		LOG_ERR("Command error in write");
		wStatus = kUwb_StatusCode_Failed;
	} else {
		LOG_HEXDUMP_INF(p_data, data_len, "SEND >:");
	}

	return wStatus;
}

uwb_status_code_t uwb_tml_read()
{
	LOG_WRN("READ");
	/* Check if TML layer is initialized */
	if (!guwb_tml_context.is_initialized) {
		/* TML initialization is not done */
		return kUwb_StatusCode_NotInitialized;
	}

	uwb_status_code_t status = kUwb_StatusCode_Success;

	/* Check if initialized */
	/* Lock mutex */
	k_mutex_lock(&guwb_tml_context.read_mutex, K_FOREVER);

	/* Enable read operation */
	guwb_tml_context.b_enable = 1;

	/* Unlock mutex */
	k_mutex_unlock(&guwb_tml_context.read_mutex);

	return status;
}

void uwb_tml_read_abort(void)
{
	/* Check if TML layer is initialized */
	if (!guwb_tml_context.is_initialized) {
		/* TML initialization is not done */
		return;
	}

	k_mutex_lock(&guwb_tml_context.read_mutex, K_FOREVER);
	guwb_tml_context.b_enable = 0;
	k_mutex_unlock(&guwb_tml_context.read_mutex);
}

void uwb_tml_suspend_reader()
{
	/* Check if TML layer is initialized */
	if (!guwb_tml_context.is_initialized) {
		/* TML initialization is not done */
		return;
	}

	k_thread_suspend(guwb_tml_context.reader_thread);
}

void uwb_tml_resume_reader()
{
	/* Check if TML layer is initialized */
	if (!guwb_tml_context.is_initialized) {
		/* TML initialization is not done */
		return;
	}

	k_thread_resume(guwb_tml_context.reader_thread);
}
