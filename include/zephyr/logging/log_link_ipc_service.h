/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for the IPC service log link.
 * @ingroup log_link_ipc_service
 */

#ifndef ZEPHYR_INCLUDE_LOGGING_LOG_LINK_IPC_SERVICE_H_
#define ZEPHYR_INCLUDE_LOGGING_LOG_LINK_IPC_SERVICE_H_

#include <zephyr/ipc/ipc_service.h>
#include <zephyr/logging/log_internal.h>
#include <zephyr/logging/log_link.h>
#include <zephyr/logging/log_msg.h>
#include <zephyr/sys/mpsc_pbuf.h>
#include <zephyr/sys/spsc_lockfree.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

/**
 * @defgroup log_link_ipc_service IPC service log link
 * @ingroup log_link
 * @brief Log link implementation that receives messages over IPC service.
 * @{
 */

/** @cond INTERNAL_HIDDEN */

union log_link_ipc_rsp {
	uint16_t count;

	struct {
		char *rsp;
		size_t len;
	} name;

	struct {
		uint8_t level;
		uint8_t runtime_level;
	} levels;

	struct {
		uint8_t level;
	} set_runtime_level;
};

struct log_link_ipc_msg_buffer {
	void *buffer;
	size_t size;
};

struct log_link_ipc_data {
	struct ipc_ept ept;
	struct k_sem rdy_sem;
	union log_link_ipc_rsp rsp;
	struct k_timer dropped_timer;
	struct log_source_const_data *source_addr;
	const char **log_str_ptr;
	union log_msg_generic *msg;
	size_t current_offset;
	atomic_t dropped_cnt;
	int status;
	bool ready;
};

SPSC_DECLARE(log_link_ipc_msg_buffer, struct log_link_ipc_msg_buffer);

struct log_link_ipc {
	struct ipc_ept_cfg ept_cfg;
	const struct device *ipc_instance;
	struct log_link_ipc_data *data;
	struct mpsc_pbuf_buffer *buffer;
	const struct log_link *link;
	struct spsc_log_link_ipc_msg_buffer *msg_buffer;
	const struct mpsc_pbuf_buffer_config *buffer_config;
};

extern struct log_link_api log_link_ipc_api;

void log_link_ipc_bound_cb(void *priv);
void log_link_ipc_received_cb(const void *data, size_t len, void *priv);

#define LOG_LINK_IPC_MPSC_BUFFER_DEFINE(_name, _buffer_size) \
	static uint32_t __aligned(Z_LOG_MSG_ALIGNMENT) buf32##_name[_buffer_size / sizeof(int)]; \
	static const struct mpsc_pbuf_buffer_config mpsc_config_##_name = { \
		.buf = (uint32_t *)buf32##_name, \
		.size = ARRAY_SIZE(buf32##_name), \
		.notify_drop = z_log_notify_drop, \
		.get_wlen = log_msg_generic_get_wlen, \
		.flags = (IS_ENABLED(CONFIG_LOG_MODE_OVERFLOW) ? \
			  MPSC_PBUF_MODE_OVERWRITE : 0) | \
			 (IS_ENABLED(CONFIG_LOG_MEM_UTILIZATION) ? \
			  MPSC_PBUF_MAX_UTILIZATION : 0) \
	}; \
	static struct mpsc_pbuf_buffer mpsc_pbuf_##_name

/** @endcond */

/**
 * @brief Create an instance of the IPC service log link.
 *
 * @param _name        Instance name. It is used as the domain name.
 * @param _ipc_node    Devicetree node of the IPC instance used by the link.
 * @param _buffer_size Size (in bytes) of the buffer for remote messages. It is not
 *                     used when @kconfig{CONFIG_LOG_LINK_IPC_SERVICE_RX_HOLD} is
 *                     enabled as in that case messages are kept in the IPC RX buffers.
 * @param _mem_access  Set to true if the local domain can access the memory of the
 *                     remote domain. It allows to avoid copying of read-only strings.
 */
#define LOG_LINK_IPC_DEFINE(_name, _ipc_node, _buffer_size, _mem_access) \
	static struct log_link_ipc_data log_link_ipc_data_##_name;\
	static const struct log_link_ipc log_link_ipc_##_name;\
	LOG_LINK_DEFINE(_name, log_link_ipc_api, (void *)&log_link_ipc_##_name); \
	COND_CODE_1(CONFIG_LOG_LINK_IPC_SERVICE_RX_HOLD, \
		(SPSC_DEFINE(msg_buffer_##_name, struct log_link_ipc_msg_buffer, \
			CONFIG_LOG_LINK_IPC_SERVICE_RX_HOLD_QUEUE_SIZE)), \
		(BUILD_ASSERT(_buffer_size > 0, "Buffer size must be greater than 0");\
		 LOG_LINK_IPC_MPSC_BUFFER_DEFINE(_name, _buffer_size))); \
	static const struct log_link_ipc log_link_ipc_##_name = { \
		.ept_cfg = { \
			.name = "logging", \
			.prio = 0, \
			.cb = { \
				.bound = log_link_ipc_bound_cb, \
				.received = log_link_ipc_received_cb, \
			}, \
			.priv = (void *)&log_link_ipc_##_name, \
		}, \
		.ipc_instance = DEVICE_DT_GET(IPC_NODE), \
		.link = &_name, \
		.data = &log_link_ipc_data_##_name, \
		COND_CODE_1(CONFIG_LOG_LINK_IPC_SERVICE_RX_HOLD, \
			(.msg_buffer = \
				(struct spsc_log_link_ipc_msg_buffer *)&msg_buffer_##_name), \
			(.buffer = &mpsc_pbuf_##_name, .buffer_config = &mpsc_config_##_name)), \
	}

/** @} */

#endif /* ZEPHYR_INCLUDE_LOGGING_LOG_LINK_IPC_SERVICE_H_ */
