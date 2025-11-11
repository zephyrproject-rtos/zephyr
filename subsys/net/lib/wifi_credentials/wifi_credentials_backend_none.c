/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "wifi_credentials_internal.h"

int wifi_credentials_store_entry(size_t idx, const void *buf, size_t buf_len)
{
	ARG_UNUSED(idx);
	ARG_UNUSED(buf);
	ARG_UNUSED(buf_len);

	return 0;
}

int wifi_credentials_delete_entry(size_t idx)
{
	ARG_UNUSED(idx);

	return 0;
}

int wifi_credentials_load_entry(size_t idx, void *buf, size_t buf_len)
{
	ARG_UNUSED(idx);
	ARG_UNUSED(buf);
	ARG_UNUSED(buf_len);

	return 0;
}

int wifi_credentials_backend_init(void)
{
	return 0;
}
