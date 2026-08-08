/*
 * Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>

#include <zephyr/mp/core/mp.h>
#include <zephyr/mp/core/mp_pipeline.h>
#include <zephyr/mp/core/mp_bin.h>
#include <zephyr/mp/core/mp_element.h>
#include <zephyr/mp/core/mp_pad.h>
#include <zephyr/mp/ipc/mp_ipc.h>
#include <zephyr/mp/ipc/mp_ipc_protocol.h>

#include "mp_test_sink.h"

LOG_MODULE_REGISTER(mp_ipc_remote_app, LOG_LEVEL_INF);

#define PIPE_B_ID    0
#define IPC_SRC_ID   1
#define TEST_SINK_ID 2

static struct mp_pipeline pipeline_b;
static struct mp_ipc_src ipc_src;
static struct mp_test_sink test_sink;

/* Semaphore to signal from app_create_remote_pipeline() to main() */
K_SEM_DEFINE(pipeline_created_sem, 0, 1);

/* Application hook called from mp_ipc_src.c when CPU0 sends CREATE_PIPELINE */
int app_create_remote_pipeline(void)
{
	int ret;

	LOG_INF("[CPU1] Creating remote pipeline");

	/* Build the pipeline and elements */
	MP_ELEMENT_INIT(&pipeline_b, mp_pipeline_init, PIPE_B_ID);
	MP_ELEMENT_INIT(&test_sink, mp_test_sink_init, TEST_SINK_ID);

	ret = mp_element_link((struct mp_element *)&ipc_src, (struct mp_element *)&test_sink, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to link ipc_src to test_sink (%d)", ret);
		return ret;
	}
	LOG_INF("[CPU1] Elements linked successfully.");

	ret = mp_bin_add((struct mp_bin *)&pipeline_b,
			 (struct mp_element *)&ipc_src,
			 (struct mp_element *)&test_sink,
			 NULL);
	if (ret < 0) {
		LOG_ERR("Failed to add elements to bin (%d)", ret);
		return ret;
	}
	LOG_INF("[CPU1] Elements added to bin successfully.");

	if (mp_element_set_state((struct mp_element *)&pipeline_b,
				 MP_STATE_PAUSED) != MP_STATE_CHANGE_SUCCESS) {
		LOG_ERR("Failed to pause receiver pipeline");
		return -EIO;
	}
	LOG_INF("[CPU1] Pipeline_b PAUSED state transition complete.");

	/* Notify main() that pipeline_b is now constructed */
	k_sem_give(&pipeline_created_sem);

	return 0;
}

int main(void)
{
	int ret;

	LOG_INF("Topology: ===== (IPC Service) =====> [ipc_src] -> [test_sink]");

	/* Initialize only ipc_src and register endpoint */
	LOG_INF("[CPU1] Initializing ipc_src endpoint");
	MP_ELEMENT_INIT(&ipc_src, mp_ipc_src_init, IPC_SRC_ID);

	if (mp_element_set_state((struct mp_element *)&ipc_src,
				 MP_STATE_PAUSED) != MP_STATE_CHANGE_SUCCESS) {
		LOG_ERR("Failed to pause ipc_src for endpoint registration");
		return -EIO;
	}

	LOG_INF("[CPU1] Waiting for CREATE_PIPELINE command from CPU0...");
	k_sem_take(&pipeline_created_sem, K_FOREVER);
	LOG_INF("[CPU1] Pipeline_b created; waiting for PLAYING and EOS / ERROR messages...");

	struct mp_bus *bus = mp_element_get_bus((struct mp_element *)&pipeline_b);
	struct mp_message msg;

	mp_bus_pop_msg(bus, MP_MESSAGE_ERROR | MP_MESSAGE_EOS, &msg);

	switch (msg.type) {
	case MP_MESSAGE_ERROR:
		LOG_ERR("ERROR message from element %d", msg.origin->object.id);
		break;
	case MP_MESSAGE_EOS:
		LOG_INF("EOS message from element %d (SUCCESS: Frame transfer completed)", msg.origin->object.id);
		break;
	default:
		LOG_ERR("Unexpected message from element %d", msg.origin->object.id);
		break;
	}

	/* Stop/Deinit pipeline */
	LOG_INF("Stopping pipeline...");
	(void)mp_element_set_state((struct mp_element *)&pipeline_b, MP_STATE_READY);

	LOG_INF("--- Remote Core Receiver Loop Finished ---");
	return 0;
}