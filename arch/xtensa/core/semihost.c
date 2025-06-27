/*
 * Copyright (c) 2022 Intel Corporation.
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/byteorder.h>
#include <zephyr/arch/common/semihost.h>
#include "semihost_types.h"

#define XTENSA_SEMIHOST_OPEN   (-2)
#define XTENSA_SEMIHOST_CLOSE  (-3)
#define XTENSA_SEMIHOST_READ   (-4)
#define XTENSA_SEMIHOST_WRITE  (-5)
#define XTENSA_SEMIHOST_LSEEK  (-6)
#define XTENSA_SEMIHOST_RENAME (-7)
#define XTENSA_SEMIHOST_FSTAT  (-10)

enum semihost_open_flag {
	SEMIHOST_RDONLY = 0x0,
	SEMIHOST_WRONLY = 0x1,
	SEMIHOST_RDWR = 0x2,
	SEMIHOST_APPEND = 0x8,
	SEMIHOST_CREAT = 0x200,
	SEMIHOST_TRUNC = 0x400,
	SEMIHOST_EXCL = 0x800,
};

uint32_t semihost_flags(enum semihost_open_mode mode)
{
	uint32_t flags = 0;

	switch (mode) {
	case SEMIHOST_OPEN_R:
	case SEMIHOST_OPEN_RB:
		flags = SEMIHOST_RDONLY;
		break;
	case SEMIHOST_OPEN_R_PLUS:
	case SEMIHOST_OPEN_RB_PLUS:
		flags = SEMIHOST_RDWR;
		break;
	case SEMIHOST_OPEN_W:
	case SEMIHOST_OPEN_WB:
		flags = SEMIHOST_WRONLY | SEMIHOST_CREAT | SEMIHOST_TRUNC;
		break;
	case SEMIHOST_OPEN_W_PLUS:
	case SEMIHOST_OPEN_WB_PLUS:
		flags = SEMIHOST_RDWR | SEMIHOST_CREAT | SEMIHOST_TRUNC;
		break;
	case SEMIHOST_OPEN_A:
	case SEMIHOST_OPEN_AB:
		flags = SEMIHOST_WRONLY | SEMIHOST_CREAT | SEMIHOST_APPEND;
		break;
	case SEMIHOST_OPEN_A_PLUS:
	case SEMIHOST_OPEN_AB_PLUS:
		flags = SEMIHOST_RDWR | SEMIHOST_CREAT | SEMIHOST_APPEND;
		break;
	default:
		return -1;
	}

	return flags;
}

uint32_t semihost_mode(enum semihost_open_mode mode)
{
	switch (mode) {
	case SEMIHOST_OPEN_W:
	case SEMIHOST_OPEN_WB:
	case SEMIHOST_OPEN_W_PLUS:
	case SEMIHOST_OPEN_WB_PLUS:
	case SEMIHOST_OPEN_A:
	case SEMIHOST_OPEN_AB:
	case SEMIHOST_OPEN_A_PLUS:
	case SEMIHOST_OPEN_AB_PLUS:
		/* Octal 0600, S_IRUSR | S_IWUSR */
		return 0x180;
	default:
		return 0;
	}
}

static inline uintptr_t xtensa_semihost_call_4(uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
					       uintptr_t arg4, uintptr_t call_id)
{
	register uintptr_t a2 __asm__("%a2") = call_id;
	register uintptr_t a6 __asm__("%a6") = arg1;
	register uintptr_t a3 __asm__("%a3") = arg2;
	register uintptr_t a4 __asm__("%a4") = arg3;
	register uintptr_t a5 __asm__("%a5") = arg4;

	__asm__ volatile("break 1, 14\n\t"
			 : "=r"(a2)
			 : "r"(a2), "r"(a6), "r"(a3), "r"(a4), "r"(a5)
			 : "memory");

	return a2;
}

static inline uintptr_t xtensa_semihost_call_3(uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
					       uintptr_t call_id)
{
	register uintptr_t a2 __asm__("%a2") = call_id;
	register uintptr_t a6 __asm__("%a6") = arg1;
	register uintptr_t a3 __asm__("%a3") = arg2;
	register uintptr_t a4 __asm__("%a4") = arg3;

	__asm__ volatile("break 1, 14\n\t"
			 : "=r"(a2)
			 : "r"(a2), "r"(a6), "r"(a3), "r"(a4)
			 : "memory");

	return a2;
}

static inline uintptr_t xtensa_semihost_call_2(uintptr_t arg1, uintptr_t arg2, uintptr_t call_id)
{
	register uintptr_t a2 __asm__("%a2") = call_id;
	register uintptr_t a6 __asm__("%a6") = arg1;
	register uintptr_t a3 __asm__("%a3") = arg2;

	__asm__ volatile("break 1, 14\n\t" : "=r"(a2) : "r"(a2), "r"(a6), "r"(a3) : "memory");

	return a2;
}

static inline uintptr_t xtensa_semihost_call_1(uintptr_t arg1, uintptr_t call_id)
{
	register uintptr_t a2 __asm__("%a2") = call_id;
	register uintptr_t a6 __asm__("%a6") = arg1;

	__asm__ volatile("break 1, 14\n\t" : "=r"(a2) : "r"(a2), "r"(a6) : "memory");

	return a2;
}

long xtensa_semihost_open(struct semihost_open_args *args)
{
	return xtensa_semihost_call_4((uintptr_t)args->path, semihost_flags(args->mode),
				      semihost_mode(args->mode), args->path_len,
				      XTENSA_SEMIHOST_OPEN);
}

long xtensa_semihost_close(long fd)
{
	return xtensa_semihost_call_1(fd, XTENSA_SEMIHOST_CLOSE);
}

long xtensa_semihost_write(long fd, const char *buf, long len)
{
	long ret;

	ret = (long)xtensa_semihost_call_3(fd, (uintptr_t)buf, len, XTENSA_SEMIHOST_WRITE);

	/* semihost_write assumes that data was written successfully if ret == 0. */
	if (ret == len) {
		return 0;
	}

	return -1;
}

long xtensa_semihost_read(long fd, void *buf, long len)
{
	long ret;

	ret = (long)xtensa_semihost_call_3(fd, (uintptr_t)buf, len, XTENSA_SEMIHOST_READ);

	/* semihost_read assumes that all bytes were read if ret == 0.
	 * If ret == len, it means EOF was reached.
	 */
	if (ret == len) {
		return 0;
	} else if (ret <= 0) {
		return len;
	} else {
		return ret;
	}
}

long xtensa_semihost_read_char(long fd)
{
	char c = 0;

	xtensa_semihost_call_3(fd, (uintptr_t)&c, 1, XTENSA_SEMIHOST_READ);

	return (long)c;
}

long xtensa_semihost_seek(struct semihost_seek_args *args)
{
	long ret;

	ret = (long)xtensa_semihost_call_3(args->fd, args->offset, 0, XTENSA_SEMIHOST_LSEEK);

	if (ret == args->offset) {
		return 0;
	}

	return ret;
}

long xtensa_semihost_flen(long fd)
{
	uint8_t buf[64] = {0};
	long ret;

	ret = (long)xtensa_semihost_call_2(fd, (uintptr_t)buf, XTENSA_SEMIHOST_FSTAT);
	if (ret < 0) {
		return -1;
	}

	/* Struct stat is 64 bytes, bytes 28-35 correspond to st_size
	 * field. 8-bytes cannot fit into long data type so return
	 * only the lower 4 bytes.
	 */
	ret = *((long *)&buf[32]);

	return sys_be32_to_cpu(ret);
}

long semihost_exec(enum semihost_instr instr, void *args)
{
	switch (instr) {
	case SEMIHOST_OPEN:
		return xtensa_semihost_open((struct semihost_open_args *)args);
	case SEMIHOST_CLOSE:
		return xtensa_semihost_close(((struct semihost_close_args *)args)->fd);
	case SEMIHOST_WRITEC:
		return xtensa_semihost_write(1, (char *)args, 1);
	case SEMIHOST_WRITE:
		return xtensa_semihost_write(((struct semihost_write_args *)args)->fd,
					     ((struct semihost_write_args *)args)->buf,
					     ((struct semihost_write_args *)args)->len);
	case SEMIHOST_READ:
		return xtensa_semihost_read(((struct semihost_read_args *)args)->fd,
					    ((struct semihost_read_args *)args)->buf,
					    ((struct semihost_read_args *)args)->len);
	case SEMIHOST_READC:
		return xtensa_semihost_read_char(((struct semihost_poll_in_args *)args)->zero);
	case SEMIHOST_SEEK:
		return xtensa_semihost_seek((struct semihost_seek_args *)args);
	case SEMIHOST_FLEN:
		return xtensa_semihost_flen(((struct semihost_flen_args *)args)->fd);
	default:
		return -1;
	}
}
