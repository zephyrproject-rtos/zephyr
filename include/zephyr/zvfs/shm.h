/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_ZVFS_SHM_H_
#define ZEPHYR_INCLUDE_ZEPHYR_ZVFS_SHM_H_

/**
 * @file
 * @brief ZVFS shared memory objects
 *
 * Named, system-wide memory objects reached through file descriptors: sized
 * with zvfs_ftruncate(), read and written at offsets, and mapped in place by
 * zvfs_mmap(), so every mapping of an object aliases the same pages.
 */

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Open, and optionally create, a shared memory object.
 *
 * @p flags takes the ZVFS_O_RDONLY or ZVFS_O_RDWR access mode with any of
 * ZVFS_O_CREAT, ZVFS_O_EXCL and ZVFS_O_TRUNC. A new object has size zero until
 * zvfs_ftruncate() sizes it. An object outlives its name: zvfs_shm_unlink()
 * removes the name, and the object is freed once its last descriptor and its
 * last mapping are gone.
 *
 * @param name Object name, at most CONFIG_ZVFS_SHM_NAME_MAX - 1 characters.
 * @param flags Access mode and creation flags.
 * @param mode Permission bits recorded for the object when it is created.
 *
 * @return A file descriptor on success, or -1 on error (errno is set).
 */
__syscall int zvfs_shm_open(const char *name, int flags, int mode);

/**
 * @brief Remove the name of a shared memory object.
 *
 * @param name Object name.
 *
 * @return 0 on success, or -1 on error (errno is set).
 */
__syscall int zvfs_shm_unlink(const char *name);

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/shm.h>

#endif /* ZEPHYR_INCLUDE_ZEPHYR_ZVFS_SHM_H_ */
