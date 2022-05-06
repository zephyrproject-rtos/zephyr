/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdbool.h>

#include <zephyr/toolchain.h>

/* FIXME: Implement vendor specific data path configuration */
static bool dummy;

bool ll_data_path_configured(uint8_t data_path_dir, uint8_t data_path_id)
{
	ARG_UNUSED(data_path_dir);
	ARG_UNUSED(data_path_id);

	return dummy;
}

uint8_t ll_configure_data_path(uint8_t data_path_dir, uint8_t data_path_id,
			       uint8_t vs_config_len, uint8_t *vs_config)
{
	ARG_UNUSED(data_path_dir);
	ARG_UNUSED(data_path_id);
	ARG_UNUSED(vs_config_len);
	ARG_UNUSED(vs_config);

	dummy = true;

	return 0;
}
