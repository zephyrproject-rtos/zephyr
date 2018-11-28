/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <zephyr/types.h>
#include <string.h>
#include <version.h>

#include <soc.h>
#include <toolchain.h>
#include <errno.h>
#include <atomic.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_vs.h>
#include <bluetooth/buf.h>
#include <bluetooth/bluetooth.h>
#include <drivers/bluetooth/hci_driver.h>
#include <misc/byteorder.h>
#include <misc/util.h>

#include "util/util.h"
#include "hal/ecb.h"
#include "ll_sw/pdu.h"
#include "ll_sw/ctrl.h"
#include "ll.h"
#include "hci/hci_internal.h"
#include "hal/hci_vendor_hal.h"

static void vs_read_version_info(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_vs_read_version_info *rp;

	rp = cmd_complete(evt, sizeof(*rp));

	rp->status = 0x00;
	rp->hw_platform = BT_HCI_VS_HW_PLAT;
	rp->hw_variant = BT_HCI_VS_HW_VAR;

	rp->fw_variant = 0;
	rp->fw_version = (KERNEL_VERSION_MAJOR & 0xff);
	rp->fw_revision = KERNEL_VERSION_MINOR;
	rp->fw_build = (KERNEL_PATCHLEVEL & 0xffff);
}

static void vs_read_supported_commands(struct net_buf *buf,
				       struct net_buf **evt)
{
	struct bt_hci_rp_vs_read_supported_commands *rp;

	rp = cmd_complete(evt, sizeof(*rp));

	rp->status = 0x00;
	(void)memset(&rp->commands[0], 0, sizeof(rp->commands));

	/* Set Version Information, Supported Commands, Supported Features. */
	rp->commands[0] |= BIT(0) | BIT(1) | BIT(2);
#if defined(CONFIG_BT_HCI_VS_EXT)
	/* Write BD_ADDR, Read Build Info */
	rp->commands[0] |= BIT(5) | BIT(7);
	/* Read Static Addresses, Read Key Hierarchy Roots */
	rp->commands[1] |= BIT(0) | BIT(1);
#endif /* CONFIG_BT_HCI_VS_EXT */
}

static void vs_read_supported_features(struct net_buf *buf,
				       struct net_buf **evt)
{
	struct bt_hci_rp_vs_read_supported_features *rp;

	rp = cmd_complete(evt, sizeof(*rp));

	rp->status = 0x00;
	(void)memset(&rp->features[0], 0x00, sizeof(rp->features));
}

#if defined(CONFIG_BT_HCI_VS_EXT)
static void vs_write_bd_addr(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_vs_write_bd_addr *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;

	ll_addr_set(0, &cmd->bdaddr.val[0]);

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = 0x00;
}

static void vs_read_build_info(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_vs_read_build_info *rp;

#define HCI_VS_BUILD_INFO "Zephyr OS v" \
	KERNEL_VERSION_STRING CONFIG_BT_CTLR_HCI_VS_BUILD_INFO

	const char build_info[] = HCI_VS_BUILD_INFO;

#define BUILD_INFO_EVT_LEN (sizeof(struct bt_hci_evt_hdr) + \
			    sizeof(struct bt_hci_evt_cmd_complete) + \
			    sizeof(struct bt_hci_rp_vs_read_build_info) + \
			    sizeof(build_info))

	BUILD_ASSERT(CONFIG_BT_RX_BUF_LEN >= BUILD_INFO_EVT_LEN);

	rp = cmd_complete(evt, sizeof(*rp) + sizeof(build_info));
	rp->status = 0x00;
	memcpy(rp->info, build_info, sizeof(build_info));
}

static void vs_read_static_addrs(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_vs_read_static_addrs *rp;

#if defined(CONFIG_SOC_COMPATIBLE_NRF)
	/* Read address from nRF5-specific storage
	 * Non-initialized FICR values default to 0xFF, skip if no address
	 * present. Also if a public address lives in FICR, do not use in this
	 * function.
	 */
	if (((NRF_FICR->DEVICEADDR[0] != UINT32_MAX) ||
	    ((NRF_FICR->DEVICEADDR[1] & UINT16_MAX) != UINT16_MAX)) &&
	      (NRF_FICR->DEVICEADDRTYPE & 0x01)) {
		struct bt_hci_vs_static_addr *addr;

		rp = cmd_complete(evt, sizeof(*rp) + sizeof(*addr));
		rp->status = 0x00;
		rp->num_addrs = 1;

		addr = &rp->a[0];
		sys_put_le32(NRF_FICR->DEVICEADDR[0], &addr->bdaddr.val[0]);
		sys_put_le16(NRF_FICR->DEVICEADDR[1], &addr->bdaddr.val[4]);
		/* The FICR value is a just a random number, with no knowledge
		 * of the Bluetooth Specification requirements for random
		 * static addresses.
		 */
		BT_ADDR_SET_STATIC(&addr->bdaddr);

		/* Mark IR as invalid */
		(void)memset(addr->ir, 0x00, sizeof(addr->ir));

		return;
	}
#endif /* CONFIG_SOC_FAMILY_NRF */

	rp = cmd_complete(evt, sizeof(*rp));
	rp->status = 0x00;
	rp->num_addrs = 0;
}

static void vs_read_key_hierarchy_roots(struct net_buf *buf,
					struct net_buf **evt)
{
	struct bt_hci_rp_vs_read_key_hierarchy_roots *rp;

	rp = cmd_complete(evt, sizeof(*rp));
	rp->status = 0x00;

#if defined(CONFIG_SOC_COMPATIBLE_NRF)
	/* Fill in IR if present */
	if ((NRF_FICR->IR[0] != UINT32_MAX) &&
	    (NRF_FICR->IR[1] != UINT32_MAX) &&
	    (NRF_FICR->IR[2] != UINT32_MAX) &&
	    (NRF_FICR->IR[3] != UINT32_MAX)) {
		sys_put_le32(NRF_FICR->IR[0], &rp->ir[0]);
		sys_put_le32(NRF_FICR->IR[1], &rp->ir[4]);
		sys_put_le32(NRF_FICR->IR[2], &rp->ir[8]);
		sys_put_le32(NRF_FICR->IR[3], &rp->ir[12]);
	} else {
		/* Mark IR as invalid */
		(void)memset(rp->ir, 0x00, sizeof(rp->ir));
	}

	/* Fill in ER if present */
	if ((NRF_FICR->ER[0] != UINT32_MAX) &&
	    (NRF_FICR->ER[1] != UINT32_MAX) &&
	    (NRF_FICR->ER[2] != UINT32_MAX) &&
	    (NRF_FICR->ER[3] != UINT32_MAX)) {
		sys_put_le32(NRF_FICR->ER[0], &rp->er[0]);
		sys_put_le32(NRF_FICR->ER[1], &rp->er[4]);
		sys_put_le32(NRF_FICR->ER[2], &rp->er[8]);
		sys_put_le32(NRF_FICR->ER[3], &rp->er[12]);
	} else {
		/* Mark ER as invalid */
		(void)memset(rp->er, 0x00, sizeof(rp->er));
	}

	return;
#else
	/* Mark IR as invalid */
	(void)memset(rp->ir, 0x00, sizeof(rp->ir));
	/* Mark ER as invalid */
	(void)memset(rp->er, 0x00, sizeof(rp->er));
#endif /* CONFIG_SOC_FAMILY_NRF */
}

#endif /* CONFIG_BT_HCI_VS_EXT */

int vendor_cmd_handle(u16_t ocf, struct net_buf *cmd,
			     struct net_buf **evt)
{
	switch (ocf) {
	case BT_OCF(BT_HCI_OP_VS_READ_VERSION_INFO):
		vs_read_version_info(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_VS_READ_SUPPORTED_COMMANDS):
		vs_read_supported_commands(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_VS_READ_SUPPORTED_FEATURES):
		vs_read_supported_features(cmd, evt);
		break;

#if defined(CONFIG_BT_HCI_VS_EXT)
	case BT_OCF(BT_HCI_OP_VS_READ_BUILD_INFO):
		vs_read_build_info(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_VS_WRITE_BD_ADDR):
		vs_write_bd_addr(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_VS_READ_STATIC_ADDRS):
		vs_read_static_addrs(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_VS_READ_KEY_HIERARCHY_ROOTS):
		vs_read_key_hierarchy_roots(cmd, evt);
		break;
#endif /* CONFIG_BT_HCI_VS_EXT */

	default:
		return -EINVAL;
	}

	return 0;
}
