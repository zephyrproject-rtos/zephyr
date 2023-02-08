/* btp_core.c - Bluetooth Core service */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/atomic.h>
#include <zephyr/types.h>
#include <string.h>

#include <zephyr/toolchain.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/net/buf.h>

#include <hci_core.h>

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME bttester_core
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include "btp/btp.h"


static void supported_commands(uint8_t *data, uint16_t len)
{
	uint8_t buf[1];
	struct btp_core_read_supported_commands_rp *rp = (void *) buf;

	(void)memset(buf, 0, sizeof(buf));

	tester_set_bit(buf, BTP_CORE_READ_SUPPORTED_COMMANDS);
	tester_set_bit(buf, BTP_CORE_READ_SUPPORTED_SERVICES);
	tester_set_bit(buf, BTP_CORE_REGISTER_SERVICE);
	tester_set_bit(buf, BTP_CORE_UNREGISTER_SERVICE);

	tester_send(BTP_SERVICE_ID_CORE, BTP_CORE_READ_SUPPORTED_COMMANDS,
		    BTP_INDEX_NONE, (uint8_t *) rp, sizeof(buf));
}

static void supported_services(uint8_t *data, uint16_t len)
{
	uint8_t buf[2];
	struct btp_core_read_supported_services_rp *rp = (void *) buf;

	(void)memset(buf, 0, sizeof(buf));

	tester_set_bit(buf, BTP_SERVICE_ID_CORE);
	tester_set_bit(buf, BTP_SERVICE_ID_GAP);
	tester_set_bit(buf, BTP_SERVICE_ID_GATT);
#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
	tester_set_bit(buf, BTP_SERVICE_ID_L2CAP);
#endif /* CONFIG_BT_L2CAP_DYNAMIC_CHANNEL */
#if defined(CONFIG_BT_MESH)
	tester_set_bit(buf, BTP_SERVICE_ID_MESH);
#endif /* CONFIG_BT_MESH */
#if defined(CONFIG_BT_VCP_VOL_REND)
	tester_set_bit(buf, BTP_SERVICE_ID_VCS);
#endif /* CONFIG_BT_VCP_VOL_REND */
#if defined(CONFIG_BT_IAS) || defined(CONFIG_BT_IAS_CLIENT)
	tester_set_bit(buf, BTP_SERVICE_ID_IAS);
#endif /* CONFIG_BT_IAS */
#if defined(CONFIG_BT_AICS) || defined(CONFIG_BT_AICS_CLIENT)
	tester_set_bit(buf, BTP_SERVICE_ID_AICS);
#endif /*CONFIG_BT_AICS */
#if defined(CONFIG_BT_VOCS) || defined(CONFIG_BT_VOCS_CLIENT)
	tester_set_bit(buf, BTP_SERVICE_ID_VOCS);
#endif /* CONFIG_BT_VOCS */

	tester_send(BTP_SERVICE_ID_CORE, BTP_CORE_READ_SUPPORTED_SERVICES,
		    BTP_INDEX_NONE, (uint8_t *) rp, sizeof(buf));
}

static void register_service(uint8_t *data, uint16_t len)
{
	struct btp_core_register_service_cmd *cmd = (void *) data;
	uint8_t status;

	switch (cmd->id) {
	case BTP_SERVICE_ID_GAP:
		status = tester_init_gap();
		/* Rsp with success status will be handled by bt enable cb */
		if (status == BTP_STATUS_FAILED) {
			goto rsp;
		}
		return;
	case BTP_SERVICE_ID_GATT:
		status = tester_init_gatt();
		break;
#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
	case BTP_SERVICE_ID_L2CAP:
		status = tester_init_l2cap();
#endif /* CONFIG_BT_L2CAP_DYNAMIC_CHANNEL */
		break;
#if defined(CONFIG_BT_MESH)
	case BTP_SERVICE_ID_MESH:
		status = tester_init_mesh();
		break;
#endif /* CONFIG_BT_MESH */
#if defined(CONFIG_BT_VCP_VOL_REND)
	case BTP_SERVICE_ID_VCS:
		status = tester_init_vcp();
		break;
#endif /* CONFIG_BT_VCP_VOL_REND */
#if defined(CONFIG_BT_VOCS) || defined(CONFIG_BT_VOCS_CLIENT)
	case BTP_SERVICE_ID_VOCS:
		status = tester_init_vcp();
		break;
#endif /* CONFIG_BT_VOCS */
#if defined(CONFIG_BT_AICS) || defined(CONFIG_BT_AICS_CLIENT)
	case BTP_SERVICE_ID_AICS:
		status = tester_init_vcp();
		break;
#endif /* CONFIG_BT_AICS */
#if defined(CONFIG_BT_IAS) || defined(CONFIG_BT_IAS_CLIENT)
	case BTP_SERVICE_ID_IAS:
		status = BTP_STATUS_SUCCESS;
		break;
#endif /* CONFIG_BT_IAS */
#if defined(CONFIG_BT_PACS)
	case BTP_SERVICE_ID_PACS:
		status = tester_init_bap();
		break;
#endif /* CONFIG_BT_PACS */
	default:
		LOG_WRN("unknown id: 0x%02x", cmd->id);
		status = BTP_STATUS_FAILED;
		break;
	}

rsp:
	tester_rsp(BTP_SERVICE_ID_CORE, BTP_CORE_REGISTER_SERVICE, BTP_INDEX_NONE,
		   status);
}

static void unregister_service(uint8_t *data, uint16_t len)
{
	struct btp_core_unregister_service_cmd *cmd = (void *) data;
	uint8_t status;

	switch (cmd->id) {
	case BTP_SERVICE_ID_GAP:
		status = tester_unregister_gap();
		break;
	case BTP_SERVICE_ID_GATT:
		status = tester_unregister_gatt();
		break;
#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
	case BTP_SERVICE_ID_L2CAP:
		status = tester_unregister_l2cap();
		break;
#endif /* CONFIG_BT_L2CAP_DYNAMIC_CHANNEL */
#if defined(CONFIG_BT_MESH)
	case BTP_SERVICE_ID_MESH:
		status = tester_unregister_mesh();
		break;
#endif /* CONFIG_BT_MESH */
#if defined(CONFIG_BT_VCP_VOL_REND)
	case BTP_SERVICE_ID_VCS:
		status = tester_unregister_vcp();
		break;
#endif /* CONFIG_BT_VCP_VOL_REND */
#if defined(CONFIG_BT_AICS) || defined(CONFIG_BT_AICS_CLIENT)
	case BTP_SERVICE_ID_AICS:
		status = tester_unregister_vcp();
		break;
#endif /* CONFIG_BT_AICS */
#if defined(CONFIG_BT_VOCS) || defined(CONFIG_BT_VOCS_CLIENT)
	case BTP_SERVICE_ID_VOCS:
		status = tester_unregister_vcp();
		break;
#endif /* CONFIG_BT_VOCS */
#if defined(CONFIG_BT_PACS)
	case BTP_SERVICE_ID_PACS:
		status = tester_unregister_bap();
		break;
#endif /* CONFIG_BT_PACS */
	default:
		LOG_WRN("unknown id: 0x%x", cmd->id);
		status = BTP_STATUS_FAILED;
		break;
	}

	tester_rsp(BTP_SERVICE_ID_CORE, BTP_CORE_UNREGISTER_SERVICE, BTP_INDEX_NONE,
		   status);
}

void tester_handle_core(uint8_t opcode, uint8_t index, uint8_t *data,
		        uint16_t len)
{
	if (index != BTP_INDEX_NONE) {
		LOG_WRN("index != BTP_INDEX_NONE: 0x%x", index);
		tester_rsp(BTP_SERVICE_ID_CORE, opcode, index, BTP_STATUS_FAILED);
		return;
	}

	switch (opcode) {
	case BTP_CORE_READ_SUPPORTED_COMMANDS:
		supported_commands(data, len);
		return;
	case BTP_CORE_READ_SUPPORTED_SERVICES:
		supported_services(data, len);
		return;
	case BTP_CORE_REGISTER_SERVICE:
		register_service(data, len);
		return;
	case BTP_CORE_UNREGISTER_SERVICE:
		unregister_service(data, len);
		return;
	default:
		LOG_WRN("unknown opcode: 0x%x", opcode);
		tester_rsp(BTP_SERVICE_ID_CORE, opcode, BTP_INDEX_NONE,
			   BTP_STATUS_UNKNOWN_CMD);
		return;
	}
}
