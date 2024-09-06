/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Here we define the shared memory buffers for the RPMSG IPC back-end in simulation.
 * In real HW, these are booked in RAM thru device tree configuration.
 * In this simulated target, we just define them at build time to have the size defined
 * in device tree.
 *
 * Note that this file is only compiled as part of the application core image, and therefore
 * when the network core is built with the IPC service, we cannot produce an executable
 * with the network core image alone, as we would lack this buffer during linking.
 */

#include "nsi_cpu_if.h"
#include <zephyr/device.h>

#if defined(CONFIG_IPC_SERVICE_STATIC_VRINGS)

#define DT_DRV_COMPAT zephyr_ipc_openamp_static_vrings

#define DEFINE_BACKEND_BUFFER(i) \
	NATIVE_SIMULATOR_IF \
	char IPC##i##_shm_buffer[DT_REG_SIZE(DT_INST_PHANDLE(i, memory_region))];

DT_INST_FOREACH_STATUS_OKAY(DEFINE_BACKEND_BUFFER)

#endif

#if defined(CONFIG_IPC_SERVICE_BACKEND_ICBMSG)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT zephyr_ipc_icbmsg

#define DEFINE_BACKEND_BUFFER_DIR(i, dir)	\
	NATIVE_SIMULATOR_IF			\
	char IPC##i##_##dir##_shm_buffer[DT_REG_SIZE(DT_INST_PHANDLE(i, dir##_region))] = {0};

#define DEFINE_BACKEND_BUFFER(i)		\
	DEFINE_BACKEND_BUFFER_DIR(i, tx)	\
	DEFINE_BACKEND_BUFFER_DIR(i, rx)

DT_INST_FOREACH_STATUS_OKAY(DEFINE_BACKEND_BUFFER)

#endif
