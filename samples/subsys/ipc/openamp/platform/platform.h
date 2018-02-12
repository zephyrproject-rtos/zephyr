/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PLATFORM_H__
#define PLATFORM_H__

#include <openamp/open_amp.h>

#define SHM_START_ADDRESS       0x04000400
#define SHM_SIZE                0x7c00
#define SHM_DEVICE_NAME         "sramx.shm"

#define VRING_COUNT             2
#define VRING_RX_ADDRESS        0x04007800
#define VRING_TX_ADDRESS        0x04007C00
#define VRING_ALIGNMENT         4
#define VRING_SIZE              16

struct hil_proc *platform_init(int role);

#endif

