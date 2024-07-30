/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdbool.h>

#include <zephyr/bluetooth/hci_types.h>
#include "isoal.h"
#include "ull_iso_types.h"

/* FIXME: Implement vendor specific data path configuration */
static bool dummy;

bool ll_data_path_configured(uint8_t data_path_dir, uint8_t data_path_id)
{
	ARG_UNUSED(data_path_dir);
	ARG_UNUSED(data_path_id);

	return dummy;
}

bool ll_data_path_source_create(uint16_t handle,
				struct ll_iso_datapath *datapath,
				isoal_source_pdu_alloc_cb *pdu_alloc,
				isoal_source_pdu_write_cb *pdu_write,
				isoal_source_pdu_emit_cb *pdu_emit,
				isoal_source_pdu_release_cb *pdu_release)
{
	ARG_UNUSED(handle);
	ARG_UNUSED(datapath);
	ARG_UNUSED(pdu_alloc);
	ARG_UNUSED(pdu_write);
	ARG_UNUSED(pdu_emit);
	ARG_UNUSED(pdu_release);

	return false;
}

uint8_t ll_configure_data_path(uint8_t data_path_dir, uint8_t data_path_id,
			       uint8_t vs_config_len, uint8_t *vs_config)
{
	ARG_UNUSED(data_path_dir);
	ARG_UNUSED(data_path_id);
	ARG_UNUSED(vs_config_len);
	ARG_UNUSED(vs_config);

	if (!IS_ENABLED(CONFIG_BT_CTLR_SYNC_ISO) ||
	    (data_path_dir == BT_HCI_DATAPATH_DIR_CTLR_TO_HOST)) {
		dummy = true;
	}

	return 0;
}
