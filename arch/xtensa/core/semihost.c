/*
 * Copyright (c) 2022 Intel Corporation.
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/byteorder.h>
#include <zephyr/arch/common/semihost.h>
#include "semihost_types.h"

#ifdef CONFIG_SIMULATOR_XTENSA
#include <xtensa/simcall.h>

#define SEMIHOST_INSTR "simcall"

#define XTENSA_SEMIHOST_OPEN   SYS_open
#define XTENSA_SEMIHOST_CLOSE  SYS_close
#define XTENSA_SEMIHOST_READ   SYS_read
#define XTENSA_SEMIHOST_WRITE  SYS_write
#define XTENSA_SEMIHOST_LSEEK  SYS_lseek
#define XTENSA_SEMIHOST_RENAME SYS_rename
#define XTENSA_SEMIHOST_FSTAT  SYS_fstat
#else
#define SEMIHOST_INSTR "break 1, 14\n\t"

#define XTENSA_SEMIHOST_OPEN   (-2)
#define XTENSA_SEMIHOST_CLOSE  (-3)
#define XTENSA_SEMIHOST_READ   (-4)
#define XTENSA_SEMIHOST_WRITE  (-5)
#define XTENSA_SEMIHOST_LSEEK  (-6)
#define XTENSA_SEMIHOST_RENAME (-7)
#define XTENSA_SEMIHOST_FSTAT  (-10)
#endif /* CONFIG_SIMULATOR_XTENSA */

#define SEMIHOST_SEEK_SET 0
#define SEMIHOST_SEEK_CUR 1
#define SEMIHOST_SEEK_END 2

enum semihost_open_flag {
	SEMIHOST_RDONLY = 0x0,
	SEMIHOST_WRONLY = 0x1,
	SEMIHOST_RDWR = 0x2,
	SEMIHOST_APPEND = 0x8,
#ifdef CONFIG_SIMULATOR_XTENSA
#ifdef CONFIG_QEMU_TARGET
	SEMIHOST_CREAT = 0x40, /* 0100 */
	SEMIHOST_EXCL = 0x80,  /* 0200 */
#else
	SEMIHOST_CREAT = 0x100,
	SEMIHOST_EXCL = 0x400,
#endif /* CONFIG_QEMU_TARGET */
	SEMIHOST_TRUNC = 0x200,
#else
	SEMIHOST_CREAT = 0x200,
	SEMIHOST_TRUNC = 0x400,
	SEMIHOST_EXCL = 0x800,
#endif /* CONFIG_SIMULATOR_XTENSA */
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

/* Not used by any simulator calls at the moment */
#ifndef CONFIG_SIMULATOR_XTENSA
static inline uintptr_t xtensa_semihost_call_4(uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
					       uintptr_t arg4, uintptr_t call_id)
{
	register uintptr_t r2 __asm__("%a2") = call_id;
	register uintptr_t r3 __asm__("%a6") = arg1;
	register uintptr_t r4 __asm__("%a3") = arg2;
	register uintptr_t r5 __asm__("%a4") = arg3;
	register uintptr_t r6 __asm__("%a5") = arg4;

	__asm__ volatile(SEMIHOST_INSTR
			 : "=r"(r2)
			 : "r"(r2), "r"(r3), "r"(r4), "r"(r5), "r"(r6)
			 : "memory");

	return r2;
}
#endif /* CONFIG_SIMULATOR_XTENSA */

static inline uintptr_t xtensa_semihost_call_3(uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
					       uintptr_t call_id)
{
	register uintptr_t r2 __asm__("%a2") = call_id;
#ifdef CONFIG_SIMULATOR_XTENSA
	register uintptr_t r3 __asm__("%a3") = arg1;
	register uintptr_t r4 __asm__("%a4") = arg2;
	register uintptr_t r5 __asm__("%a5") = arg3;
#else
	register uintptr_t r3 __asm__("%a6") = arg1;
	register uintptr_t r4 __asm__("%a3") = arg2;
	register uintptr_t r5 __asm__("%a4") = arg3;
#endif /* CONFIG_SIMULATOR_XTENSA */

	__asm__ volatile(SEMIHOST_INSTR : "=r"(r2) : "r"(r2), "r"(r3), "r"(r4), "r"(r5) : "memory");

	return r2;
}

static inline uintptr_t xtensa_semihost_call_2(uintptr_t arg1, uintptr_t arg2, uintptr_t call_id)
{
	register uintptr_t r2 __asm__("%a2") = call_id;
#ifdef CONFIG_SIMULATOR_XTENSA
	register uintptr_t r3 __asm__("%a3") = arg1;
	register uintptr_t r4 __asm__("%a4") = arg2;
#else
	register uintptr_t r3 __asm__("%a6") = arg1;
	register uintptr_t r4 __asm__("%a3") = arg2;
#endif /* CONFIG_SIMULATOR_XTENSA */

	__asm__ volatile(SEMIHOST_INSTR : "=r"(r2) : "r"(r2), "r"(r3), "r"(r4) : "memory");

	return r2;
}

static inline uintptr_t xtensa_semihost_call_1(uintptr_t arg1, uintptr_t call_id)
{
	register uintptr_t r2 __asm__("%a2") = call_id;
#ifdef CONFIG_SIMULATOR_XTENSA
	register uintptr_t r3 __asm__("%a3") = arg1;
#else
	register uintptr_t r3 __asm__("%a6") = arg1;
#endif /* CONFIG_SIMULATOR_XTENSA */

	__asm__ volatile(SEMIHOST_INSTR : "=r"(r2) : "r"(r2), "r"(r3) : "memory");

	return r2;
}

long xtensa_semihost_open(struct semihost_open_args *args)
{
#ifdef CONFIG_SIMULATOR_XTENSA
	return xtensa_semihost_call_3((uintptr_t)args->path, semihost_flags(args->mode),
				      semihost_mode(args->mode), XTENSA_SEMIHOST_OPEN);
#else
	return xtensa_semihost_call_4((uintptr_t)args->path, semihost_flags(args->mode),
				      semihost_mode(args->mode), args->path_len + 1,
				      XTENSA_SEMIHOST_OPEN);
#endif /* CONFIG_SIMULATOR_XTENSA */
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

long xtensa_semihost_seek(struct semihost_seek_args *args, int whence)
{
	long ret;

	ret = (long)xtensa_semihost_call_3(args->fd, args->offset, whence, XTENSA_SEMIHOST_LSEEK);

	if (ret == args->offset) {
		return 0;
	}

	return ret;
}

#ifndef CONFIG_QEMU_TARGET
long xtensa_semihost_flen(long fd)
{
	uint8_t buf[64] = {0};
	long ret;

	ret = (long)xtensa_semihost_call_2(fd, (uintptr_t)buf, XTENSA_SEMIHOST_FSTAT);
	if (ret < 0) {
		return -1;
	}

#ifdef CONFIG_SIMULATOR_XTENSA
	/* Simulator returns a struct stat with st_size field at offset 16. */
	ret = *((long *)&buf[16]);

	return ret;
#else
	/* Struct stat is 64 bytes, bytes 28-35 correspond to st_size
	 * field. 8-bytes cannot fit into long data type so return
	 * only the lower 4 bytes.
	 */
	ret = *((long *)&buf[32]);

	return sys_be32_to_cpu(ret);
#endif /* CONFIG_SIMULATOR_XTENSA */
}
#else
long xtensa_semihost_flen(long fd)
{
	/* QEMU does not support fstat, use lseek to determine the file length. */
	long ret, cur;
	struct semihost_seek_args args = {
		.fd = fd,
		.offset = 0,
	};

	/* Save the current file position. */
	cur = xtensa_semihost_seek(&args, SEMIHOST_SEEK_CUR);
	if (cur < 0) {
		return -1;
	}

	/* Find the file size by seeking to the end of the file. */
	args.offset = 0;
	ret = xtensa_semihost_seek(&args, SEMIHOST_SEEK_END);
	if (ret < 0) {
		return -1;
	}

	/* Restore the file position to the original location. */
	args.offset = cur;
	cur = xtensa_semihost_seek(&args, SEMIHOST_SEEK_SET);
	if (cur < 0) {
		return -1;
	}

	return ret;
}
#endif /* CONFIG_QEMU_TARGET */

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
		return xtensa_semihost_seek((struct semihost_seek_args *)args, SEMIHOST_SEEK_SET);
	case SEMIHOST_FLEN:
		return xtensa_semihost_flen(((struct semihost_flen_args *)args)->fd);
	default:
		return -1;
	}
}
