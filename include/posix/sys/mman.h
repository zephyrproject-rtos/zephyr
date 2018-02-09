/*
 * Copyright (c) 2018 Juan Manuel Torres Palma
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MMAN_H__
#define __MMAN_H__

#include <sys/types.h>

/* TODO: Move to common header sys/types.h */
#define PATH_MAX       (64)
#define NAME_MAX       (64)
#define O_CREAT        (1 << 0)
#define O_EXCL         (1 << 1)
#define O_TRUNC        (1 << 2)
#define O_RDONLY       (1 << 3)
#define O_RDWR         (1 << 4)

/* TODO: add missing permissions in sys/stat.h */
typedef int mode_t;
#define S_IRWXU 0700
#define S_IWUSR 0200
#define S_IRUSR 0400

#define PROT_EXEC   1
#define PROT_NONE   2
#define PROT_READ   3
#define PROT_WRITE  4

#define MAP_FIXED    (1 << 0)
#define MAP_PRIVATE  (1 << 1)
#define MAP_SHARED   (1 << 2)

#define MS_ASYNC        1
#define MS_INVALIDATE   2
#define MS_SYNC         3

#define MCL_CURRENT 1
#define MCL_FUTURE  2

#define POSIX_MADV_DONTNEED    (1 << 0)
#define POSIX_MADV_NORMAL      (1 << 1)
#define POSIX_MADV_RANDOM      (1 << 2)
#define POSIX_MADV_SEQUENTIAL  (1 << 3)
#define POSIX_MADV_WILLNEED    (1 << 4)

#define POSIX_TYPED_MEM_ALLOCATE         (1 << 0)
#define POSIX_TYPED_MEM_ALLOCATE_CONTIG  (1 << 1)
#define POSIX_TYPED_MEM_MAP_ALLOCATABLE  (1 << 2)

#define MAP_FAILED (NULL)

#ifdef __cplusplus
extern "C" {
#endif

struct posix_typed_mem_info {
	size_t posix_tmi_length;
};

/*
 * TODO: Implement type agnostic version
 */
int close(int fd);

/**
 * @brief POSIX shared memory API
 *
 * See IEEE 1003.1
 */
int mlock(const void *addr, size_t len);

/**
 * @brief POSIX shared memory API
 *
 * See IEEE 1003.1
 */
int mlockall(int flags);

/**
 * @brief POSIX shared memory API
 *
 * See IEEE 1003.1
 */
void *mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off);

/**
 * @brief POSIX shared memory API
 *
 * See IEEE 1003.1
 */
int mprotect(void *addr, size_t len, int prot);

/**
 * @brief POSIX shared memory API
 *
 * See IEEE 1003.1
 */
int msync(void *addr, size_t len, int flags);

/**
 * @brief POSIX shared memory API
 *
 * See IEEE 1003.1
 */
int munlock(const void *addr, size_t len);

/**
 * @brief POSIX shared memory API
 *
 * See IEEE 1003.1
 */
int munlockall(void);

/**
 * @brief POSIX shared memory API
 *
 * See IEEE 1003.1
 */
int munmap(void *addr, size_t len);

/**
 * @brief POSIX shared memory API
 *
 * See IEEE 1003.1
 */
int posix_madvise(void *addr, size_t len, int advice);

/**
 * @brief POSIX shared memory API
 *
 * See IEEE 1003.1
 */
int posix_mem_offset(const void *restrict addr, size_t len,
		     off_t *restrict off, size_t *restrict contig_len,
		     int *restrict fildes);

/**
 * @brief POSIX shared memory API
 *
 * See IEEE 1003.1
 */
int posix_typed_mem_get_info(int fildes, struct posix_typed_mem_info *info);

/**
 * @brief POSIX shared memory API
 *
 * See IEEE 1003.1
 */
int posix_typed_mem_open(const char *name, int oflag, int tflag);

/**
 * @brief POSIX shared memory API
 *
 * See IEEE 1003.1
 */
int shm_open(const char *name, int oflag, mode_t mode);

/**
 * @brief POSIX shared memory API
 *
 * See IEEE 1003.1
 */
int shm_unlink(const char *name);

#ifdef __cplusplus
}
#endif

#endif /* __MMAN_H___ */
