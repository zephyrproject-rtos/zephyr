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
#include <zephyr/mp/core/mp_fake_src.h>
#include <zephyr/mp/ipc/mp_ipc.h>
#include <zephyr/mp/ipc/mp_ipc_protocol.h>

LOG_MODULE_REGISTER(mp_ipc_primary_app, LOG_LEVEL_INF);

#define PIPE_A_ID    0
#define FAKE_SRC_ID  1
#define IPC_SINK_ID  2

static struct mp_pipeline pipeline_a;
static struct mp_fake_src fake_src;
static struct mp_ipc_sink ipc_sink;
extern struct k_sem ipc_bound_sem;

#include <zephyr/init.h>

static int reset_sync_flag(void)
{
	*(volatile uint32_t *)0x281FFFC0 = 0;
	return 0;
}
SYS_INIT(reset_sync_flag, PRE_KERNEL_1, 0);

int main(void)
{
	int ret;

	LOG_INF("Topology: [fake_src] -> [ipc_sink] ===== (IPC Service) =====>");

	/* 1. Build the pipelines and elements using standard MP_ELEMENT_INIT */
	LOG_INF("[CPU0] Initializing pipeline elements...");
	MP_ELEMENT_INIT(&pipeline_a, mp_pipeline_init, PIPE_A_ID);
	MP_ELEMENT_INIT(&fake_src, mp_fake_src_init, FAKE_SRC_ID);
	MP_ELEMENT_INIT(&ipc_sink, mp_ipc_sink_init, IPC_SINK_ID);

	/* 2. Link elements together using upstream-compliant mp_element_link */
	ret = mp_element_link((struct mp_element *)&fake_src, (struct mp_element *)&ipc_sink, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to link elements (%d)", ret);
		return ret;
	}
	LOG_INF("[CPU0] Elements linked successfully.");

	/* 3. Add to pipeline bin */
	ret = mp_bin_add((struct mp_bin *)&pipeline_a, (struct mp_element *)&fake_src, (struct mp_element *)&ipc_sink, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to add elements to bin (%d)", ret);
		return ret;
	}
	LOG_INF("[CPU0] Elements added to bin successfully.");

	/* 4. Start connection */
	LOG_INF("[CPU0] Setting pipeline state to PAUSED to initialize backend...");
	if (mp_element_set_state((struct mp_element *)&pipeline_a, MP_STATE_PAUSED) != MP_STATE_CHANGE_SUCCESS) {
		LOG_ERR("Failed to pause pipeline A");
		return -EIO;
	}
	LOG_INF("[CPU0] Pipeline PAUSED state transition complete.");

	/* Signal to CPU1 that we are fully ready and listening! */
	LOG_INF("[CPU0] Signaling CPU1 and waking up remote core...");
	*(volatile uint32_t *)0x281FFFC0 = 0x12345678;

#if defined(CONFIG_SOC_AN521) || defined(CONFIG_SOC_MUSCA_B1)
	extern void wakeup_cpu1(void);
	wakeup_cpu1();
#endif

    k_sem_take(&ipc_bound_sem, K_FOREVER);
	LOG_INF("[CPU0] Remote core connected and bound successfully!");

	/* Ask CPU1 to create its pipeline dynamically now */
	struct mp_ipc_msg msg = {
		.type = IPC_MSG_CMD_CREATE_PIPELINE,
	};
	LOG_INF("[CPU0] Sending CREATE_PIPELINE command to CPU1...");
	ret = ipc_service_send(&ipc_sink.ept, &msg, sizeof(msg));
	if (ret < 0) {
		LOG_ERR("Failed to send CREATE_PIPELINE command: %d", ret);
		return ret;
	}

	/* 5. Start streaming frames across processor boundary */
	LOG_INF("Activating pipeline to PLAYING state...");
	if (mp_element_set_state((struct mp_element *)&pipeline_a, MP_STATE_PLAYING) != MP_STATE_CHANGE_SUCCESS) {
		LOG_ERR("Failed to play pipeline A");
		return -EIO;
	}

	/* Stream for some time */
	LOG_INF("[CPU0] Streaming frames (busy-waiting delay)...");
    k_sleep(K_SECONDS(5));

	/* Stop pipeline stream */
	LOG_INF("Stopping pipeline...");
	(void)mp_element_set_state((struct mp_element *)&pipeline_a, MP_STATE_READY);

	LOG_INF("--- Primary Core Master Loop Finished ---");
	return 0;
}