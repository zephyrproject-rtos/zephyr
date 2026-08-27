/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_NORDIC_COMMON_GPPI_EXT_ALLOCATOR_H_
#define ZEPHYR_SOC_NORDIC_COMMON_GPPI_EXT_ALLOCATOR_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief GPPI external allocator IPC message IDs. */
enum gppi_ext_msg_id {
	/** Connection allocation request. */
	GPPI_CONN_ALLOC = 1,
	/** Connection allocation response. */
	GPPI_CONN_ALLOC_RSP,
	/** Connection free request. */
	GPPI_CONN_FREE,
	/** Group allocation request. */
	GPPI_GROUP_ALLOC,
	/** Group allocation response. */
	GPPI_GROUP_ALLOC_RSP,
	/** Group free request. */
	GPPI_GROUP_FREE,
	/** Channel allocation request. */
	GPPI_CHANNEL_ALLOC,
	/** Channel allocation response. */
	GPPI_CHANNEL_ALLOC_RSP,
	/** Channel free request. */
	GPPI_CHANNEL_FREE,
};

/** @brief DPPI resource description used in IPC messages. */
struct gppi_ext_resource {
	/** Domain ID. */
	uint16_t domain_id;
	/** Channel. */
	uint8_t channel;
};

/** @brief Connection allocation request payload. */
struct gppi_ext_msg_conn_alloc {
	/** Producer domain ID. */
	uint32_t producer;
	/** Consumer domain ID. */
	uint32_t consumer;
	/** Non-zero if @ref resource is valid. */
	uint8_t has_resource;
	/** Optional external DPPI resource. */
	struct gppi_ext_resource resource;
};

/** @brief Connection allocation response payload. */
struct gppi_ext_msg_conn_alloc_rsp {
	/** Allocation result. */
	int result;
	/** Allocated connection handle. */
	uint32_t handle;
};

/** @brief Connection free request payload. */
struct gppi_ext_msg_conn_free {
	/** Connection handle to free. */
	uint32_t handle;
};

/** @brief Group allocation request payload. */
struct gppi_ext_msg_group_alloc {
	/** Domain ID for the group. */
	uint32_t domain_id;
};

/** @brief Group allocation response payload. */
struct gppi_ext_msg_group_alloc_rsp {
	/** Allocation result. */
	int result;
	/** Allocated group handle. */
	uint32_t handle;
};

/** @brief Group free request payload. */
struct gppi_ext_msg_group_free {
	/** Group handle to free. */
	uint32_t handle;
};

/** @brief Channel allocation request payload. */
struct gppi_ext_msg_channel_alloc {
	/** Target-specific identifier of the DPPI system node. */
	uint32_t node_id;
};

/** @brief Channel allocation response payload. */
struct gppi_ext_msg_channel_alloc_rsp {
	/** Allocation result (non-negative channel or negative errno). */
	int result;
};

/** @brief Channel free request payload. */
struct gppi_ext_msg_channel_free {
	/** Target-specific identifier of the DPPI system node. */
	uint32_t node_id;
	/** Channel to free. */
	uint8_t channel;
};

/** @brief GPPI external allocator IPC message. */
struct gppi_ext_msg {
	/** Message ID. */
	uint8_t id;
	union {
		/** Connection allocation request. */
		struct gppi_ext_msg_conn_alloc conn_alloc;
		/** Connection allocation response. */
		struct gppi_ext_msg_conn_alloc_rsp conn_alloc_rsp;
		/** Connection free request. */
		struct gppi_ext_msg_conn_free conn_free;
		/** Group allocation request. */
		struct gppi_ext_msg_group_alloc group_alloc;
		/** Group allocation response. */
		struct gppi_ext_msg_group_alloc_rsp group_alloc_rsp;
		/** Group free request. */
		struct gppi_ext_msg_group_free group_free;
		/** Channel allocation request. */
		struct gppi_ext_msg_channel_alloc channel_alloc;
		/** Channel allocation response. */
		struct gppi_ext_msg_channel_alloc_rsp channel_alloc_rsp;
		/** Channel free request. */
		struct gppi_ext_msg_channel_free channel_free;
	};
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_NORDIC_COMMON_GPPI_EXT_ALLOCATOR_H_ */
