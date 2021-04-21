/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_central_iso
#include "common/log.h"
#include "hal/debug.h"

uint8_t ll_cig_parameters_open(uint8_t cig_id,
			       uint32_t m_interval, uint32_t s_interval,
			       uint8_t sca, uint8_t packing, uint8_t framing,
			       uint16_t m_latency, uint16_t s_latency,
			       uint8_t num_cis)
{
	ARG_UNUSED(cig_id);
	ARG_UNUSED(m_interval);
	ARG_UNUSED(s_interval);
	ARG_UNUSED(sca);
	ARG_UNUSED(packing);
	ARG_UNUSED(framing);
	ARG_UNUSED(m_latency);
	ARG_UNUSED(s_latency);
	ARG_UNUSED(num_cis);

	return BT_HCI_ERR_CMD_DISALLOWED;
}

uint8_t ll_cis_parameters_set(uint8_t cis_id,
			      uint16_t m_sdu, uint16_t s_sdu,
			      uint8_t m_phy, uint8_t s_phy,
			      uint8_t m_rtn, uint8_t s_rtn,
			      uint16_t *handle)
{

	ARG_UNUSED(cis_id);
	ARG_UNUSED(m_sdu);
	ARG_UNUSED(s_sdu);
	ARG_UNUSED(m_phy);
	ARG_UNUSED(s_phy);
	ARG_UNUSED(m_rtn);
	ARG_UNUSED(s_rtn);
	ARG_UNUSED(handle);

	return BT_HCI_ERR_CMD_DISALLOWED;
}

uint8_t ll_cig_parameters_test_open(uint8_t cig_id,
				    uint32_t m_interval,
				    uint32_t s_interval,
				    uint8_t  m_ft,
				    uint8_t  s_ft,
				    uint16_t iso_interval,
				    uint8_t  sca,
				    uint8_t  packing,
				    uint8_t  framing,
				    uint8_t  num_cis)
{
	ARG_UNUSED(cig_id);
	ARG_UNUSED(m_interval);
	ARG_UNUSED(s_interval);
	ARG_UNUSED(m_ft);
	ARG_UNUSED(s_ft);
	ARG_UNUSED(iso_interval);
	ARG_UNUSED(sca);
	ARG_UNUSED(packing);
	ARG_UNUSED(framing);
	ARG_UNUSED(num_cis);

	return BT_HCI_ERR_CMD_DISALLOWED;
}

uint8_t ll_cis_parameters_test_set(uint8_t  cis_id,
				   uint16_t m_sdu, uint16_t s_sdu,
				   uint16_t m_pdu, uint16_t s_pdu,
				   uint8_t m_phy, uint8_t s_phy,
				   uint8_t m_bn, uint8_t s_bn,
				   uint16_t *handle)
{
	ARG_UNUSED(cis_id);
	ARG_UNUSED(m_sdu);
	ARG_UNUSED(s_sdu);
	ARG_UNUSED(m_pdu);
	ARG_UNUSED(s_pdu);
	ARG_UNUSED(m_phy);
	ARG_UNUSED(s_phy);
	ARG_UNUSED(m_bn);
	ARG_UNUSED(s_bn);
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
