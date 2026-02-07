/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/process/llext.h>
#include <zephyr/kernel.h>

const uint8_t ext[] = {
#include <ext_ext.inc>
};

static struct llext_buf_loader buf_loader = LLEXT_BUF_LOADER(ext, sizeof(ext));
static struct llext_load_param load_param = LLEXT_LOAD_PARAM_DEFAULT;
static struct k_process_llext llext;

static int register_llext(void)
{
	struct k_process *process = k_process_llext_init(&llext,
							 "ext",
							 &buf_loader.loader,
							 &load_param);
	return k_process_register(process);
}

SYS_INIT(register_llext, POST_KERNEL, 0);
