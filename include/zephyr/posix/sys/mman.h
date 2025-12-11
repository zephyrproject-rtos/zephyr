/*
 * Copyright (c) 2024, Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_POSIX_SYS_MMAN_H_
#define ZEPHYR_INCLUDE_ZEPHYR_POSIX_SYS_MMAN_H_

#include <stddef.h>
#include <sys/types.h>

#define PROT_NONE  0x0
#define PROT_READ  0x1
#define PROT_WRITE 0x2
#define PROT_EXEC  0x4

#define MAP_SHARED  0x1
#define MAP_PRIVATE 0x2
#define MAP_FIXED   0x4

/* for Linux compatibility */
#define MAP_ANONYMOUS 0x20

#define MS_SYNC       0x0
#define MS_ASYNC      0x1
#define MS_INVALIDATE 0x2

#define MAP_FAILED ((void *)-1)

#define MCL_CURRENT 0
#define MCL_FUTURE  1

#ifdef __cplusplus
extern "C" {
#endif

int mlock(const void *addr, size_t len);
int mlockall(int flags);
void *mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off);
int msync(void *addr, size_t length, int flags);
int munlock(const void *addr, size_t len);
int munlockall(void);
int munmap(void *addr, size_t len);
int shm_open(const char *name, int oflag, mode_t mode);
int shm_unlink(const char *name);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_POSIX_SYS_MMAN_H_ */
