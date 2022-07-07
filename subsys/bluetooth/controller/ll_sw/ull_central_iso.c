/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_central_iso
#include "common/log.h"
#include "hal/debug.h"

uint8_t ll_cig_parameters_open(uint8_t cig_id,
			       uint32_t c_interval, uint32_t p_interval,
			       uint8_t sca, uint8_t packing, uint8_t framing,
			       uint16_t c_latency, uint16_t p_latency,
			       uint8_t num_cis)
{
	ARG_UNUSED(cig_id);
	ARG_UNUSED(c_interval);
	ARG_UNUSED(p_interval);
	ARG_UNUSED(sca);
	ARG_UNUSED(packing);
	ARG_UNUSED(framing);
	ARG_UNUSED(c_latency);
	ARG_UNUSED(p_latency);
	ARG_UNUSED(num_cis);

	return BT_HCI_ERR_CMD_DISALLOWED;
}

uint8_t ll_cis_parameters_set(uint8_t cis_id,
			      uint16_t c_sdu, uint16_t p_sdu,
			      uint8_t c_phy, uint8_t p_phy,
			      uint8_t c_rtn, uint8_t p_rtn,
			      uint16_t *handle)
{

	ARG_UNUSED(cis_id);
	ARG_UNUSED(c_sdu);
	ARG_UNUSED(p_sdu);
	ARG_UNUSED(c_phy);
	ARG_UNUSED(p_phy);
	ARG_UNUSED(c_rtn);
	ARG_UNUSED(p_rtn);
	ARG_UNUSED(handle);

	return BT_HCI_ERR_CMD_DISALLOWED;
}

uint8_t ll_cig_parameters_test_open(uint8_t cig_id,
				    uint32_t c_interval,
				    uint32_t p_interval,
				    uint8_t  c_ft,
				    uint8_t  p_ft,
				    uint16_t iso_interval,
				    uint8_t  sca,
				    uint8_t  packing,
				    uint8_t  framing,
				    uint8_t  num_cis)
{
	ARG_UNUSED(cig_id);
	ARG_UNUSED(c_interval);
	ARG_UNUSED(p_interval);
	ARG_UNUSED(c_ft);
	ARG_UNUSED(p_ft);
	ARG_UNUSED(iso_interval);
	ARG_UNUSED(sca);
	ARG_UNUSED(packing);
	ARG_UNUSED(framing);
	ARG_UNUSED(num_cis);

	return BT_HCI_ERR_CMD_DISALLOWED;
}

uint8_t ll_cis_parameters_test_set(uint8_t  cis_id,
				   uint16_t c_sdu, uint16_t p_sdu,
				   uint16_t c_pdu, uint16_t p_pdu,
				   uint8_t c_phy, uint8_t p_phy,
				   uint8_t c_bn, uint8_t p_bn,
				   uint16_t *handle)
{
	ARG_UNUSED(cis_id);
	ARG_UNUSED(c_sdu);
	ARG_UNUSED(p_sdu);
	ARG_UNUSED(c_pdu);
	ARG_UNUSED(p_pdu);
	ARG_UNUSED(c_phy);
	ARG_UNUSED(p_phy);
	ARG_UNUSED(c_bn);
	ARG_UNUSED(p_bn);
	ARG_UNUSED(handle);

	return BT_HCI_ERR_CMD_DISALLOWED;
}

uint8_t ll_cig_parameters_commit(uint8_t cig_id)
{
	ARG_UNUSED(cig_id);

	return BT_HCI_ERR_CMD_DISALLOWED;
}

uint8_t ll_cig_remove(uint8_t cig_id)
{
	ARG_UNUSED(cig_id);

	return BT_HCI_ERR_CMD_DISALLOWED;
}

uint8_t ll_cis_create_check(uint16_t cis_handle, uint16_t acl_handle)
{
	ARG_UNUSED(cis_handle);
	ARG_UNUSED(acl_handle);

	return BT_HCI_ERR_CMD_DISALLOWED;
}

void ll_cis_create(uint16_t cis_handle, uint16_t acl_handle)
{
	ARG_UNUSED(cis_handle);
	ARG_UNUSED(acl_handle);
}

int ull_central_iso_init(void)
{
	return 0;
}

int ull_central_iso_reset(void)
{
	return 0;
}
