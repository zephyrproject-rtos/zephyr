/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This buffer is the shared memory buffer for the RPMSG IPC backed
 * in simulation.
 * It must match the size defined in device tree, but as it will be compiled in
 * the native simulator runner context we cannot refer to DT itself to size it.
 */
char IPC0_shm_buffer[65536];
