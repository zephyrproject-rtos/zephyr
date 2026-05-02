/*
 * Copyright (c) 2020-2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __DRIVERS_IPM_SEDI_H
#define __DRIVERS_IPM_SEDI_H

#ifdef __cplusplus
extern "C" {
#endif
#include "sedi_driver_common.h"
#include "sedi_driver_ipc.h"
#include <zephyr/sys/atomic.h>

/*
 * bit 31 indicates whether message is valid, and could generate interrupt
 * while set/clear
 */
#define IPC_BUSY_BIT            31

#define IPM_WRITE_IN_PROC_BIT   0
#define IPM_WRITE_BUSY_BIT      1
#define IPM_READ_BUSY_BIT       2
#define IPM_PEER_READY_BIT      3

#define IPM_TIMEOUT_MS 1000

struct ipm_sedi_config_t {
	sedi_ipc_t ipc_device;
	int32_t irq_num;
	void (*irq_config)(void);
};

struct ipm_sedi_context {
	ipm_callback_t rx_msg_notify_cb;
	void *rx_msg_notify_cb_data;
	uint8_t incoming_data_buf[IPC_DATA_LEN_MAX];
	struct k_sem device_write_msg_sem;
	struct k_mutex device_write_lock;
	atomic_t status;
	uint32_t power_status;
};

#ifdef __cplusplus
}
#endif

#endif /* __DRIVERS_IPM_SEDI_H */
