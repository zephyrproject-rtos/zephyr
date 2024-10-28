/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdbool.h>
#include <zephyr/toolchain.h>

/**
 * Dummy version which does nothing
 * Boards which do not have a better implementation can use this
 */
__weak bool native_emb_addr_remap(void **addr)
{
	return false;
}
