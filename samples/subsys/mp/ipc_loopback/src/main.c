/*
 * Copyright 2026 NXP
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
#include <zephyr/mp/core/mp_fake_src.h>
#include <zephyr/mp/ipc/mp_ipc.h>
#include <zephyr/mp/ipc/mp_ipc_protocol.h>

#include "mp_test_sink.h"

LOG_MODULE_REGISTER(mp_ipc_loopback_app, LOG_LEVEL_INF);

#define PIPE_A_ID    0
#define FAKE_SRC_ID  1
#define IPC_SINK_ID  2

#define PIPE_B_ID    3
#define IPC_SRC_ID   4
#define TEST_SINK_ID 5

static struct mp_pipeline pipeline_a;
static struct mp_fake_src fake_src;
static struct mp_ipc_sink ipc_sink;

static struct mp_pipeline pipeline_b;
static struct mp_ipc_src ipc_src;
static struct mp_test_sink test_sink;

int main(void)
{
	int ret;

	LOG_INF("--- Starting Approach 1: Single-Binary Loopback Simulation ---");
	LOG_INF("Topology: [fake_src] -> [ipc_sink] ===== (IPC Loopback) =====> [ipc_src] -> [test_sink]");

	/* 1. Build the pipelines and elements using standard MP_ELEMENT_INIT */
	MP_ELEMENT_INIT(&pipeline_a, mp_pipeline_init, PIPE_A_ID);
	MP_ELEMENT_INIT(&fake_src, mp_fake_src_init, FAKE_SRC_ID);
	MP_ELEMENT_INIT(&ipc_sink, mp_ipc_sink_init, IPC_SINK_ID);

	MP_ELEMENT_INIT(&pipeline_b, mp_pipeline_init, PIPE_B_ID);
	MP_ELEMENT_INIT(&ipc_src, mp_ipc_src_init, IPC_SRC_ID);
	MP_ELEMENT_INIT(&test_sink, mp_test_sink_init, TEST_SINK_ID);

	/* 2. Link elements together using upstream-compliant mp_element_link */
	ret = mp_element_link((struct mp_element *)&fake_src, (struct mp_element *)&ipc_sink, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to link elements on Pipeline A (%d)", ret);
		return ret;
	}

	ret = mp_element_link((struct mp_element *)&ipc_src, (struct mp_element *)&test_sink, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to link elements on Pipeline B (%d)", ret);
		return ret;
	}

	/* 3. Add elements to respective bins */
	ret = mp_bin_add((struct mp_bin *)&pipeline_a, (struct mp_element *)&fake_src, (struct mp_element *)&ipc_sink, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to add elements to Pipeline A (%d)", ret);
		return ret;
	}

	ret = mp_bin_add((struct mp_bin *)&pipeline_b, (struct mp_element *)&ipc_src, (struct mp_element *)&test_sink, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to add elements to Pipeline B (%d)", ret);
		return ret;
	}

	/* 4. Start Pipeline B (Slave / Receiver Side) */
	LOG_INF("Starting Pipeline B...");
	if (mp_element_set_state((struct mp_element *)&pipeline_b, MP_STATE_PLAYING) != MP_STATE_CHANGE_SUCCESS) {
		LOG_ERR("Failed to start Pipeline B");
		return -EIO;
	}

	/* 5. Start Pipeline A (Master / Sender Side) */
	LOG_INF("Starting Pipeline A...");
	if (mp_element_set_state((struct mp_element *)&pipeline_a, MP_STATE_PLAYING) != MP_STATE_CHANGE_SUCCESS) {
		LOG_ERR("Failed to start Pipeline A");
		return -EIO;
	}

	/* 6. Wait for EOS message on Pipeline B's Bus (notifying that processing is complete) */
	LOG_INF("Pipeline is playing. Waiting for EOS / complete message on Bus...");
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

	/* Stop/Deinit both pipelines */
	LOG_INF("Stopping pipelines...");
	(void)mp_element_set_state((struct mp_element *)&pipeline_a, MP_STATE_READY);
	(void)mp_element_set_state((struct mp_element *)&pipeline_b, MP_STATE_READY);

	LOG_INF("--- Approach 1 Completed Successfully ---");
	return 0;
}