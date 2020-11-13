/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_iso
#include "common/log.h"
#include "hal/debug.h"

/* Contains vendor specific argument, function to be implemented by vendors */
__weak uint8_t ll_configure_data_path(uint8_t data_path_dir,
			       uint8_t data_path_id,
			       uint8_t vs_config_len,
			       uint8_t *vs_config)
{
	ARG_UNUSED(data_path_dir);
	ARG_UNUSED(data_path_id);
	ARG_UNUSED(vs_config_len);
	ARG_UNUSED(vs_config);

	return BT_HCI_ERR_CMD_DISALLOWED;
}

uint8_t ll_setup_iso_path(uint16_t handle, uint8_t path_dir, uint8_t path_id,
			  uint8_t coding_format, uint16_t company_id,
			  uint16_t vendor_id, uint32_t controller_delay,
			  uint8_t codec_config_len, uint8_t *codec_config)
{
	ARG_UNUSED(handle);
	ARG_UNUSED(path_dir);
	ARG_UNUSED(path_id);
	ARG_UNUSED(coding_format);
	ARG_UNUSED(company_id);
	ARG_UNUSED(vendor_id);
	ARG_UNUSED(controller_delay);
	ARG_UNUSED(codec_config_len);
	ARG_UNUSED(codec_config);

	return BT_HCI_ERR_CMD_DISALLOWED;
}

uint8_t ll_remove_iso_path(uint16_t handle, uint8_t path_dir)
{
	ARG_UNUSED(handle);
	ARG_UNUSED(path_dir);

	return BT_HCI_ERR_CMD_DISALLOWED;
}

uint8_t ll_iso_receive_test(uint16_t handle, uint8_t payload_type)
{
	ARG_UNUSED(handle);
	ARG_UNUSED(payload_type);

	return BT_HCI_ERR_CMD_DISALLOWED;
}

uint8_t ll_iso_transmit_test(uint16_t handle, uint8_t payload_type)
{
	ARG_UNUSED(handle);
	ARG_UNUSED(payload_type);

	return BT_HCI_ERR_CMD_DISALLOWED;
}

uint8_t ll_iso_test_end(uint16_t handle, uint32_t *received_cnt,
			uint32_t *missed_cnt, uint32_t *failed_cnt)
{
	ARG_UNUSED(handle);
	ARG_UNUSED(received_cnt);
	ARG_UNUSED(missed_cnt);
	ARG_UNUSED(failed_cnt);

	return BT_HCI_ERR_CMD_DISALLOWED;
}

uint8_t ll_iso_read_test_counters(uint16_t handle, uint32_t *received_cnt,
				  uint32_t *missed_cnt,
				  uint32_t *failed_cnt)
{
	ARG_UNUSED(handle);
	ARG_UNUSED(received_cnt);
	ARG_UNUSED(missed_cnt);
	ARG_UNUSED(failed_cnt);

	return BT_HCI_ERR_CMD_DISALLOWED;
}
