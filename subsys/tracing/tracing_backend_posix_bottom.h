/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * "Bottom" of the CTF tracing backend for the native/hosted targets.
 * When built with the native_simulator this will be built in the runner context,
 * that is, with the host C library, and with the host include paths.
 *
 * Note: None of these functions are public interfaces. But internal to this CTF backend.
 */

#ifndef DRIVERS_FLASH_FLASH_SIMULATOR_NATIVE_H
#define DRIVERS_FLASH_FLASH_SIMULATOR_NATIVE_H

#ifdef __cplusplus
extern "C" {
#endif

void *tracing_backend_posix_init_bottom(const char *file_name);
void tracing_backend_posix_output_bottom(const void *data, unsigned long length, void *out_stream);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_FLASH_FLASH_SIMULATOR_NATIVE_H */
