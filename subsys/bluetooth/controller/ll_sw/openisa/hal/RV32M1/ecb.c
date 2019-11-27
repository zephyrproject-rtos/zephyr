/*
 * Copyright (c) 2016-2017 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <sys/dlist.h>
#include <sys/mempool_base.h>

#include "util/mem.h"
#include "hal/ecb.h"

#define LOG_MODULE_NAME bt_ctlr_rv32m1_ecb
#include "common/log.h"
#include "hal/debug.h"

void ecb_encrypt_be(u8_t const *const key_be, u8_t const *const clear_text_be,
		    u8_t * const cipher_text_be)
{
}

void ecb_encrypt(u8_t const *const key_le, u8_t const *const clear_text_le,
		 u8_t * const cipher_text_le, u8_t * const cipher_text_be)
{
}

u32_t ecb_encrypt_nonblocking(struct ecb *ecb)
{
	return 0;
}

#ifdef RV32M1_ECB_IMPL
static void ecb_cleanup(void)
{
}

static void ecb_cb(u32_t status, u8_t *cipher_be, void *context)
{
}

#endif /* RV32M1_ECB_IMPL */

void isr_ecb(void *param)
{
}


u32_t ecb_ut(void)
{
	return 0;
}
