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

#define LOG_MODULE_NAME bt_ctlr_ti_ecb
#include "common/log.h"
#include "hal/debug.h"

void ecb_encrypt_be(u8_t const *const key_be, u8_t const *const clear_text_be,
		u8_t *const cipher_text_be)
{
	BT_DBG("key_be: %p clear_text_be: %p cipher_text_be: %p", key_be,
	       clear_text_be, cipher_text_be);
}

void ecb_encrypt(u8_t const *const key_le, u8_t const *const clear_text_le,
		 u8_t *const cipher_text_le, u8_t *const cipher_text_be)
{
	BT_DBG("key_le: %p clear_text: %p cipher_text_le: %p "
			"cipher_text_be: %p", key_le, clear_text_le,
			cipher_text_le,	cipher_text_be);
}

u32_t ecb_encrypt_nonblocking(struct ecb *ecb)
{
	BT_DBG("ecb: %p", ecb);
	return 0;
}

static void ecb_cleanup(void)
{
	BT_DBG("");
}

static void ecb_cb(u32_t status, u8_t *cipher_be, void *context)
{
	BT_DBG("status: %u cipher_be: %p context: %p", status, cipher_be,
			context);
}

void isr_ecb(void *param)
{
	BT_DBG("param: %p", param);
}

u32_t ecb_ut(void)
{
	BT_DBG("");
	return 0;
}
