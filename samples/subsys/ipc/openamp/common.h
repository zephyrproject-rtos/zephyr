/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2018-2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef COMMON_H__
#define COMMON_H__

#define VDEV_START_ADDR		DT_REG_ADDR(DT_CHOSEN(zephyr_ipc_shm))
#define VDEV_SIZE		DT_REG_SIZE(DT_CHOSEN(zephyr_ipc_shm))

#define VDEV_STATUS_ADDR	VDEV_START_ADDR
#define VDEV_STATUS_SIZE	0x400

#define SHM_START_ADDR		(VDEV_START_ADDR + VDEV_STATUS_SIZE)
#define SHM_SIZE		(VDEV_SIZE - VDEV_STATUS_SIZE)
#define SHM_DEVICE_NAME		"sramx.shm"

#define VRING_COUNT		2
#define VRING_RX_ADDRESS	(VDEV_START_ADDR + SHM_SIZE - VDEV_STATUS_SIZE)
#define VRING_TX_ADDRESS	(VDEV_START_ADDR + SHM_SIZE)
#define VRING_ALIGNMENT		4
#define VRING_SIZE		16


#endif
