/*
 * Copyright (c) 2024 sensry.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OPENSBI_H
#define OPENSBI_H

#include "sbi_ecall_interface.h"

typedef struct sbi_ret {
	int32_t error;
	uint32_t value;
} sbi_ret_t;

extern sbi_ret_t user_mode_sbi_ecall(int ext, int fid, unsigned long arg0, unsigned long arg1,
				     unsigned long arg2, unsigned long arg3, unsigned long arg4,
				     unsigned long arg5);

/* --- VERSION --- */
static inline sbi_ret_t sbi_version_get(void)
{
	return user_mode_sbi_ecall(SBI_EXT_BASE, SBI_EXT_BASE_GET_IMP_VERSION, 0, 0, 0, 0, 0, 0);
}

/* --- CONSOLE --- */
static inline sbi_ret_t sbi_console_put_char(char c)
{
	return user_mode_sbi_ecall(SBI_EXT_DBCN, SBI_EXT_DBCN_CONSOLE_WRITE_BYTE, c, 0, 0, 0, 0, 0);
}

static inline sbi_ret_t sbi_console_puts(char *str, uint32_t len)
{
	return user_mode_sbi_ecall(SBI_EXT_DBCN, SBI_EXT_DBCN_CONSOLE_WRITE, len,
				   (unsigned long)str, 0, 0, 0, 0);
}

static inline sbi_ret_t sbi_console_get_string(char *str, int32_t size)
{
	return user_mode_sbi_ecall(SBI_EXT_DBCN, SBI_EXT_DBCN_CONSOLE_READ, size, (uint32_t)str, 0,
				   0, 0, 0);
}

/* --- TIMER --- */
static inline sbi_ret_t sbi_console_set_timer(uint32_t timeL, uint32_t timeH)
{
	return user_mode_sbi_ecall(SBI_EXT_TIME, SBI_EXT_TIME_SET_TIMER, timeL, timeH, 0, 0, 0, 0);
}

#endif /* OPENSBI_H */
