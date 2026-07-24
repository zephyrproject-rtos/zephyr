/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief native_sim backend for UWB transport layer (uwb_transport_* APIs)
 *
 * Uses a pipe pair to simulate a UWB device:
 *   host writes  -> pipe_host_to_dev  -> "device" reader
 *   host reads   <- pipe_dev_to_host  <- "device" writer
 *
 * A thread plays the role of the UWB device and echoes back
 * well-formed UCI responses and notifications for every command received.
 */

#include <zephyr/kernel.h>
#include <zephyr/uwb/tml.h>
#include <zephyr/logging/log.h>
#include <zephyr/uwb/uci.h>
#include <zephyr/uwb/types.h>

#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

LOG_MODULE_REGISTER(uwb_transport_native_sim, CONFIG_UWB_LOG_LEVEL);

/* Pipe file descriptors */
static struct k_pipe pipe_dev_to_host;
static struct k_pipe pipe_host_to_dev;
uint8_t dev_to_host_buffer[CONFIG_UCI_FRAME_LEN] = {0};
uint8_t host_to_dev_buffer[CONFIG_UCI_FRAME_LEN] = {0};

static atomic_t sim_running;

static struct {
	bool initialized;
	uint32_t session_id;
	uint32_t session_handle;
} g_session_ctx = {0};

/* -------------------------------------------------------------------------
 * UCI helpers
 * -----------------------------------------------------------------------*/

static void build_rsp(uint8_t *buf, size_t *len, uint8_t gid, uint8_t oid, const uint8_t *payload,
		      uint8_t plen)
{
	buf[0] = (uint8_t)((UCI_MT_RSP << UCI_MT_SHIFT) | gid);
	buf[1] = oid;
	buf[2] = 0x00; /* RFU */
	buf[3] = plen;
	if (plen && payload) {
		memcpy(&buf[4], payload, plen);
	}
	*len = UCI_HEADER_SIZE + plen;
}

static void build_ntf(uint8_t *buf, size_t *len, uint8_t gid, uint8_t oid, const uint8_t *payload,
		      uint8_t plen)
{
	buf[0] = (uint8_t)((UCI_MT_NTF << UCI_MT_SHIFT) | gid);
	buf[1] = oid;
	buf[2] = 0x00; /* RFU */
	buf[3] = plen;
	if (plen && payload) {
		memcpy(&buf[4], payload, plen);
	}
	*len = UCI_HEADER_SIZE + plen;
}

/* -------------------------------------------------------------------------
 * Simulated device thread – processes commands and produces responses
 * -----------------------------------------------------------------------*/

static void handle_core_reset(int write_fd)
{
	uint8_t rsp[UCI_HEADER_SIZE + 1];
	size_t rsp_len;
	uint8_t payload = kUci_Status_Ok;

	build_rsp(rsp, &rsp_len, UCI_GID_CORE, UCI_MSG_CORE_DEVICE_RESET, &payload, 1);
	k_pipe_write(&pipe_dev_to_host, rsp, rsp_len, K_FOREVER);
	payload = 1;
	build_ntf(rsp, &rsp_len, UCI_GID_CORE, UCI_MSG_CORE_DEVICE_STATUS_NTF, &payload, 1);
	k_pipe_write(&pipe_dev_to_host, rsp, rsp_len, K_FOREVER);
	LOG_DBG("sim: responded to CORE_RESET");
}

static void handle_core_get_capabilities(int write_fd)
{
	uint8_t rsp[UCI_MAX_CTRL_PACKET_SIZE];
	size_t rsp_len;
	/* status | uciVerMaj | uciVerMin | macVerMaj | macVerMin |
	 * phyVerMaj | phyVerMin | vendor_spec_len */
	uint8_t payload[] = {
		0x00, 0x25, 0x00, 0x02, 0xF7, 0x07, 0x01, 0x02, 0xF7, 0x07, 0x02, 0x04, 0x01, 0x00,
		0x03, 0x00, 0x03, 0x04, 0x01, 0x00, 0x03, 0x00, 0x04, 0x01, 0x0B, 0x05, 0x02, 0x23,
		0x01, 0x06, 0x02, 0x7E, 0x06, 0x07, 0x01, 0x1F, 0x08, 0x01, 0x07, 0x09, 0x01, 0x02,
		0x0A, 0x01, 0x07, 0x0B, 0x01, 0x01, 0x0C, 0x01, 0x01, 0x0D, 0x01, 0x01, 0x0E, 0x01,
		0x09, 0x0F, 0x01, 0x0B, 0x10, 0x01, 0x03, 0x11, 0x01, 0x3F, 0x12, 0x05, 0xAF, 0xAA,
		0xAA, 0xAA, 0x01, 0x13, 0x01, 0x07, 0x14, 0x01, 0x00, 0x15, 0x01, 0x01, 0x16, 0x01,
		0x03, 0x17, 0x01, 0x00, 0x18, 0x01, 0x14, 0x19, 0x01, 0x01, 0x1A, 0x01, 0x00, 0x1B,
		0x02, 0x0D, 0x55, 0x1C, 0x01, 0x01, 0xA0, 0x01, 0x7F, 0xA1, 0x04, 0x00, 0x0F, 0x00,
		0x00, 0xA2, 0x01, 0x1F, 0xA3, 0x01, 0x03, 0xA4, 0x02, 0x00, 0x01, 0xA5, 0x04, 0x00,
		0x00, 0x01, 0x00, 0xA6, 0x03, 0x00, 0x11, 0x22, 0xA7, 0x01, 0x01,
	};

	build_rsp(rsp, &rsp_len, UCI_GID_CORE, UCI_MSG_CORE_GET_CAPS_INFO, payload,
		  sizeof(payload));
	k_pipe_write(&pipe_dev_to_host, rsp, rsp_len, K_FOREVER);
	LOG_DBG("sim: responded to UCI_MSG_CORE_GET_CAPS_INFO");
}

static void handle_core_get_device_state(int write_fd)
{
	uint8_t rsp[UCI_MAX_CTRL_PACKET_SIZE];
	size_t rsp_len;
	/* status | uciVerMaj | uciVerMin | macVerMaj | macVerMin |
	 * phyVerMaj | phyVerMin | vendor_spec_len */
	uint8_t payload[] = {0x00, 1, kUwb_CoreConfig_DeviceState, 1, kUci_DeviceState_Ready};

	build_rsp(rsp, &rsp_len, UCI_GID_CORE, UCI_MSG_CORE_GET_CONFIG, payload, sizeof(payload));
	k_pipe_write(&pipe_dev_to_host, rsp, rsp_len, K_FOREVER);
	LOG_DBG("sim: responded to UCI_MSG_CORE_GET_CONFIG");
}

static void handle_core_get_device_info(int write_fd)
{
	uint8_t rsp[UCI_HEADER_SIZE + 10];
	size_t rsp_len;
	/* status | uciVerMaj | uciVerMin | macVerMaj | macVerMin |
	 * phyVerMaj | phyVerMin | vendor_spec_len */
	uint8_t payload[] = {0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x94};

	build_rsp(rsp, &rsp_len, UCI_GID_CORE, UCI_MSG_CORE_DEVICE_INFO, payload, sizeof(payload));
	k_pipe_write(&pipe_dev_to_host, rsp, rsp_len, K_FOREVER);
	LOG_DBG("sim: responded to CORE_GET_DEVICE_INFO");
}

static void handle_session_init(uint32_t session_id)
{
	if (g_session_ctx.initialized) {
		/* Session already initialized */
		uint8_t rsp[UCI_HEADER_SIZE + 1];
		size_t rsp_len;
		uint8_t payload = kUci_Status_Rejected;

		build_rsp(rsp, &rsp_len, UCI_GID_SESSION_MANAGE, UCI_MSG_SESSION_INIT, &payload, 1);
		k_pipe_write(&pipe_dev_to_host, rsp, rsp_len, K_FOREVER);
		LOG_DBG("sim: responded to SESSION_INIT");
	} else {
		g_session_ctx.initialized = true;
		g_session_ctx.session_id = session_id;
		g_session_ctx.session_handle = 0x01000000;
		uint8_t rsp[UCI_HEADER_SIZE + sizeof(uint32_t) + 1 + 1];
		size_t rsp_len = sizeof(rsp);
		uint8_t payload[] = {kUci_Status_Ok, 0x00, 0x00, 0x00, 0x01};
		uint8_t ntf_payload[] = {0x00, 0x00, 0x00, 0x01, kUwb_SessionStatus_Initialized, 0};

		build_rsp(rsp, &rsp_len, UCI_GID_SESSION_MANAGE, UCI_MSG_SESSION_INIT, payload,
			  sizeof(payload));
		k_pipe_write(&pipe_dev_to_host, rsp, rsp_len, K_FOREVER);
		build_ntf(rsp, &rsp_len, UCI_GID_SESSION_MANAGE, UCI_MSG_SESSION_STATUS_NTF,
			  ntf_payload, sizeof(ntf_payload));
		k_pipe_write(&pipe_dev_to_host, rsp, rsp_len, K_FOREVER);
		LOG_DBG("sim: responded to SESSION_INIT");
	}
}

static void handle_session_deinit(uint32_t session_handle)
{
	if (!g_session_ctx.initialized || (g_session_ctx.session_handle != session_handle)) {
		/* Session not initialized */
		uint8_t rsp[UCI_HEADER_SIZE + 1];
		size_t rsp_len;
		uint8_t payload = kUci_Status_ErrorSessionNotExist;

		build_rsp(rsp, &rsp_len, UCI_GID_SESSION_MANAGE, UCI_MSG_SESSION_DEINIT, &payload,
			  1);
		k_pipe_write(&pipe_dev_to_host, rsp, rsp_len, K_FOREVER);
		LOG_DBG("sim: responded to SESSION_INIT");
	} else {
		g_session_ctx.initialized = false;
		uint8_t rsp[UCI_HEADER_SIZE + sizeof(uint32_t) + 1 + 1];
		size_t rsp_len = sizeof(rsp);
		uint8_t payload = kUci_Status_Ok;
		uint8_t ntf_payload[] = {0x00, 0x00, 0x00, 0x01, kUwb_SessionStatus_DeInitialized,
					 0};

		build_rsp(rsp, &rsp_len, UCI_GID_SESSION_MANAGE, UCI_MSG_SESSION_DEINIT, &payload,
			  1);
		k_pipe_write(&pipe_dev_to_host, rsp, rsp_len, K_FOREVER);
		build_ntf(rsp, &rsp_len, UCI_GID_SESSION_MANAGE, UCI_MSG_SESSION_STATUS_NTF,
			  ntf_payload, sizeof(ntf_payload));
		k_pipe_write(&pipe_dev_to_host, rsp, rsp_len, K_FOREVER);
		LOG_DBG("sim: responded to SESSION_INIT");
	}
}

static void handle_session_get_state(uint32_t session_handle)
{
	if (!g_session_ctx.initialized || (g_session_ctx.session_handle != session_handle)) {
		/* Session not initialized */
		uint8_t rsp[UCI_HEADER_SIZE + 1];
		size_t rsp_len;
		uint8_t payload = kUci_Status_ErrorSessionNotExist;

		build_rsp(rsp, &rsp_len, UCI_GID_SESSION_MANAGE, UCI_MSG_SESSION_GET_STATE,
			  &payload, 1);
		k_pipe_write(&pipe_dev_to_host, rsp, rsp_len, K_FOREVER);
		LOG_DBG("sim: responded to SESSION_INIT");
	} else {
		uint8_t rsp[UCI_HEADER_SIZE + sizeof(uint32_t) + 1 + 1];
		size_t rsp_len = sizeof(rsp);
		uint8_t payload[] = {kUci_Status_Ok, kUwb_SessionStatus_Initialized};

		build_rsp(rsp, &rsp_len, UCI_GID_SESSION_MANAGE, UCI_MSG_SESSION_GET_STATE, payload,
			  2);
		k_pipe_write(&pipe_dev_to_host, rsp, rsp_len, K_FOREVER);
		LOG_DBG("sim: responded to SESSION_INIT");
	}
}

static void handle_range_start(int write_fd)
{
	uint8_t rsp[UCI_HEADER_SIZE + 1];
	size_t rsp_len;
	uint8_t payload = kUci_Status_Ok;

	build_rsp(rsp, &rsp_len, UCI_GID_RANGE_MANAGE, UCI_MSG_RANGE_START, &payload, 1);
	k_pipe_write(&pipe_dev_to_host, rsp, rsp_len, K_FOREVER);
	LOG_DBG("sim: responded to RANGE_START");
}

static void handle_range_stop(int write_fd)
{
	uint8_t rsp[UCI_HEADER_SIZE + 1];
	size_t rsp_len;
	uint8_t payload = kUci_Status_Ok;

	build_rsp(rsp, &rsp_len, UCI_GID_RANGE_MANAGE, UCI_MSG_RANGE_STOP, &payload, 1);
	k_pipe_write(&pipe_dev_to_host, rsp, rsp_len, K_FOREVER);
	LOG_DBG("sim: responded to RANGE_STOP");
}

static void *sim_device_fn(void *arg)
{
	uint8_t cmd[256];

	LOG_DBG("sim: device thread started");

	while (atomic_get(&sim_running)) {
		/* Block until a full UCI header arrives */
		LOG_INF("Waiting");
		int n = k_pipe_read(&pipe_host_to_dev, cmd, UCI_HEADER_SIZE, K_FOREVER);
		LOG_HEXDUMP_INF(cmd, UCI_HEADER_SIZE, "UCI Header");

		if (n <= 0) {
			if (!atomic_get(&sim_running)) {
				break;
			}
			LOG_ERR("sim: pipe read error %zd (errno %d)", n, errno);
			continue;
		}

		uint8_t payload_len = cmd[3];

		if (payload_len > 0) {
			ssize_t pn = k_pipe_read(&pipe_host_to_dev, &cmd[UCI_HEADER_SIZE],
						 payload_len, K_FOREVER);
			LOG_HEXDUMP_INF(cmd, payload_len + UCI_HEADER_SIZE, "UCI packet");

			if (pn != (ssize_t)payload_len) {
				LOG_ERR("sim: short payload read");
				continue;
			}
		}

		uint8_t mt = (cmd[0] >> UCI_MT_SHIFT) & 0x07;
		uint8_t gid = cmd[0] & 0x0F;
		uint8_t oid = cmd[1] & 0x3F;

		LOG_DBG("sim: rx MT=0x%x GID=0x%x OID=0x%x PL=%u", mt, gid, oid, payload_len);

		if (gid == UCI_GID_CORE && oid == UCI_MSG_CORE_DEVICE_RESET) {
			handle_core_reset(0);
		} else if (gid == UCI_GID_CORE && oid == UCI_MSG_CORE_DEVICE_INFO) {
			handle_core_get_device_info(0);
		} else if (gid == UCI_GID_CORE && oid == UCI_MSG_CORE_GET_CAPS_INFO) {
			handle_core_get_capabilities(0);
		} else if (gid == UCI_GID_CORE && oid == UCI_MSG_CORE_GET_CONFIG) {
			uint8_t param_id = cmd[5];
			if (param_id == kUwb_CoreConfig_DeviceState) {
				handle_core_get_device_state(0);
			} else {
				uint8_t rsp[UCI_HEADER_SIZE + 1];
				size_t rsp_len;
				uint8_t payload[] = {kUci_Status_InvalidParam, 1, param_id, 0};

				build_rsp(rsp, &rsp_len, gid, oid, payload, sizeof(payload));
				k_pipe_write(&pipe_dev_to_host, rsp, rsp_len, K_FOREVER);
				LOG_WRN("sim: unknown cmd GID=0x%x OID=0x%x", gid, oid);
			}
		} else if (gid == UCI_GID_SESSION_MANAGE && oid == UCI_MSG_SESSION_INIT) {
			uint32_t session_id =
				cmd[4] + (cmd[5] << 8) + (cmd[6] << 16) + (cmd[7] << 24);
			handle_session_init(session_id);
		} else if (gid == UCI_GID_SESSION_MANAGE && oid == UCI_MSG_SESSION_DEINIT) {
			uint32_t session_handle =
				cmd[4] + (cmd[5] << 8) + (cmd[6] << 16) + (cmd[7] << 24);
			handle_session_deinit(session_handle);
		} else if (gid == UCI_GID_SESSION_MANAGE && oid == UCI_MSG_SESSION_GET_STATE) {
			uint32_t session_handle =
				cmd[4] + (cmd[5] << 8) + (cmd[6] << 16) + (cmd[7] << 24);
			handle_session_get_state(session_handle);
		} else if (gid == UCI_GID_RANGE_MANAGE && oid == UCI_MSG_RANGE_START) {
			handle_range_start(0);
		} else if (gid == UCI_GID_RANGE_MANAGE && oid == UCI_MSG_RANGE_STOP) {
			handle_range_stop(0);
		} else {
			/* Unknown command – respond with generic failure */
			uint8_t rsp[UCI_HEADER_SIZE + 1];
			size_t rsp_len;
			uint8_t payload = kUci_Status_Failed;

			build_rsp(rsp, &rsp_len, gid, oid, &payload, 1);
			k_pipe_write(&pipe_dev_to_host, rsp, rsp_len, K_FOREVER);
			LOG_WRN("sim: unknown cmd GID=0x%x OID=0x%x", gid, oid);
		}
	}

	LOG_DBG("sim: device thread exiting");
	return NULL;
}

int uwb_vendor_initialize(void)
{
	return 0;
}

void uwb_vendor_deinitialize(void)
{
}

/* -------------------------------------------------------------------------
 * uwb_transport_* API implementation
 * -----------------------------------------------------------------------*/
static k_tid_t uwb_dev;
static struct k_thread uwb_dev_thread;
K_THREAD_STACK_DEFINE(uwb_dev_stack, 2000);

int uwb_transport_open(void)
{
	k_pipe_init(&pipe_host_to_dev, host_to_dev_buffer, sizeof(host_to_dev_buffer));
	k_pipe_init(&pipe_dev_to_host, dev_to_host_buffer, sizeof(dev_to_host_buffer));

	atomic_set(&sim_running, 1);

	uwb_dev = k_thread_create(&uwb_dev_thread, (k_thread_stack_t *)&uwb_dev_stack,
				  K_THREAD_STACK_SIZEOF(uwb_dev_stack),
				  (k_thread_entry_t)&sim_device_fn, NULL, NULL, NULL,
				  K_PRIO_COOP(5), 0, K_NO_WAIT);
	if (NULL == uwb_dev) {
		atomic_set(&sim_running, 0);
		return -1;
	}

	LOG_INF("uwb_transport_open: native_sim backend ready");
	return 0;
}

void uwb_transport_close(void)
{
	atomic_set(&sim_running, 0);

	k_thread_join(&uwb_dev_thread, K_FOREVER);

	LOG_INF("uwb_transport_close: native_sim backend closed");
}

int uwb_transport_uci_read(uint8_t *pBuffer, int bytes_to_read)
{
	LOG_INF("requesting read");
	int rc = k_pipe_read(&pipe_dev_to_host, pBuffer, UCI_HEADER_SIZE, K_FOREVER);
	if (rc < UCI_HEADER_SIZE) {
		return -1;
	}
	uint8_t payload_len = pBuffer[3];
	rc += k_pipe_read(&pipe_dev_to_host, &pBuffer[UCI_HEADER_SIZE], payload_len, K_FOREVER);
	LOG_DBG("rc = %d", rc);
	return rc;
}

int uwb_transport_uci_write(uint8_t *pBuffer, uint16_t bytes_to_write)
{
	int rc = k_pipe_write(&pipe_host_to_dev, pBuffer, bytes_to_write, K_FOREVER);
	return rc;
}
