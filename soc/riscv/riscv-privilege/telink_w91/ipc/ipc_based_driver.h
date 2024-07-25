/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IPC_BASED_DRIVER_H
#define IPC_BASED_DRIVER_H

#include "ipc_dispatcher.h"
#include <string.h>
#include <zephyr/kernel.h>
#include <assert.h>

enum ipc_dispatcher_id {
	IPC_DISPATCHER_SYS                      = 0x0,
	IPC_DISPATCHER_UART                     = 0x100,
	IPC_DISPATCHER_GPIO                     = 0x200,
	IPC_DISPATCHER_PWM                      = 0x300,
	IPC_DISPATCHER_ENTROPY_TRNG             = 0x400,
	IPC_DISPATCHER_PINCTRL                  = 0x500,
	IPC_DISPATCHER_I2C                      = 0x600,
	IPC_DISPATCHER_FLASH                    = 0x700,
	IPC_DISPATCHER_BLE                      = 0x800,
	IPC_DISPATCHER_WIFI                     = 0x900,
	IPC_DISPATCHER_BLOCKING                 = 0xa00,
	IPC_DISPATCHER_HEARTBEAT                = 0xb00,
	IPC_DISPATCHER_CRYPTO_ECC               = 0xc00,
} __attribute__((__packed__));

typedef void (*ipc_based_driver_unpack_t)(void *result, const uint8_t *data, size_t len);

struct ipc_based_driver_ctx {
	ipc_based_driver_unpack_t unpack;
	void *result;
};

struct ipc_based_driver {
	struct k_mutex mutex;
	struct k_sem resp_sem;
};

void ipc_based_driver_init(struct ipc_based_driver *drv);
int ipc_based_driver_send(struct ipc_based_driver *drv,
	const void *tx_data, size_t tx_len,
	struct ipc_based_driver_ctx *ctx, uint32_t timeout_ms);
uint32_t ipc_based_driver_get_ipc_events(void);

/* Macros to pack/unpack the different types */
#define IPC_DISPATCHER_PACK_FIELD(buff, field)                                 \
do {                                                                           \
	memcpy(buff, &field, sizeof(field));                                       \
	buff += sizeof(field);                                                     \
} while (0)

#define IPC_DISPATCHER_PACK_ARRAY(buff, array, len)                            \
do {                                                                           \
	memcpy(buff, array, len);                                                  \
	buff += len;                                                               \
} while (0)

#define IPC_DISPATCHER_UNPACK_FIELD(buff, field)                               \
do {                                                                           \
	memcpy(&field, buff, sizeof(field));                                       \
	buff += sizeof(field);                                                     \
} while (0)

#define IPC_DISPATCHER_UNPACK_ARRAY(buff, array, len)                          \
do {                                                                           \
	memcpy(array, buff, len);                                                  \
	buff += len;                                                               \
} while (0)

/* Macros for making ipc dispatcher id */

#define IPC_DISPATCHER_MK_ID(ipc, inst)                                        \
	(((ipc & 0xffffff) << 8) | (inst & 0xff))

/* Macros for packing data without param (only id) */

#define IPC_DISPATCHER_PACK_FUNC_WITHOUT_PARAM(name, ipc_id)                   \
static size_t pack_##name(uint8_t inst, void *unpack_data, uint8_t *pack_data) \
{                                                                              \
	if (pack_data != NULL) {                                                   \
		uint32_t id = IPC_DISPATCHER_MK_ID(ipc_id, inst);                      \
                                                                               \
		IPC_DISPATCHER_PACK_FIELD(pack_data, id);                              \
	}                                                                          \
                                                                               \
	return sizeof(uint32_t);                                                   \
}

/* Macros for unpacking data only with error param */

#define IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(name)                 \
static void unpack_##name(void *unpack_data,                                   \
		const uint8_t *pack_data, size_t pack_data_len)                        \
{                                                                              \
	int *p_err = unpack_data;                                                  \
	size_t expect_len = sizeof(uint32_t) + sizeof(*p_err);                     \
                                                                               \
	if (expect_len != pack_data_len) {                                         \
		*p_err = -EINVAL;                                                      \
		return;                                                                \
	}                                                                          \
                                                                               \
	pack_data += sizeof(uint32_t);                                             \
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, *p_err);                            \
}

/* Macros for sending data from host device to remote */

#define IPC_DISPATCHER_HOST_SEND_DATA(ipc, inst, name,                         \
	tx_buff, rx_buff, timeout_ms)                                              \
do {                                                                           \
	uint8_t packed_data[pack_##name(inst, tx_buff, NULL)];                     \
	size_t packed_len = pack_##name(inst, tx_buff, packed_data);               \
	struct ipc_based_driver_ctx ctx = {                                        \
		.unpack = unpack_##name,                                               \
		.result = rx_buff,                                                     \
	};                                                                         \
                                                                               \
	int ipc_err = ipc_based_driver_send(ipc, packed_data, packed_len,          \
		&ctx, timeout_ms);                                                     \
                                                                               \
	if (ipc_err) {                                                             \
		/* TODO: Add assert after IPv6 FreeRTOS implemented: assert(0); */     \
	}                                                                          \
} while (0)

#endif /* IPC_BASED_DRIVER_H */
