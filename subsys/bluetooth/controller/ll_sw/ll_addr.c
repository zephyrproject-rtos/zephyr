/*
 * Copyright (c) 2016-2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stddef.h>
#include <string.h>

#include <zephyr/zephyr.h>
#include <soc.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/controller.h>

#include "util/util.h"
#include "util/memq.h"
#include "util/mem.h"

#include "pdu.h"

#include "lll.h"
#include "lll/lll_adv_types.h"
#include "lll_adv.h"
#include "lll/lll_adv_pdu.h"
#include "lll_scan.h"

#include "ull_adv_types.h"
#include "ull_scan_types.h"
#include "ull_adv_internal.h"
#include "ull_scan_internal.h"

#include "ll.h"

static uint8_t pub_addr[BDADDR_SIZE];
static uint8_t rnd_addr[BDADDR_SIZE];

uint8_t ll_addr_set(uint8_t addr_type, uint8_t const *const bdaddr)
{
	if (IS_ENABLED(CONFIG_BT_BROADCASTER)) {
#if defined(CONFIG_BT_CTLR_ADV_EXT)
		if (ull_adv_is_enabled(0) && !ll_adv_cmds_is_ext()) {
#else /* !CONFIG_BT_CTLR_ADV_EXT */
		if (ull_adv_is_enabled(0)) {
#endif /* !CONFIG_BT_CTLR_ADV_EXT */
			return BT_HCI_ERR_CMD_DISALLOWED;
		}
	}

	if (IS_ENABLED(CONFIG_BT_OBSERVER) &&
	    (ull_scan_is_enabled(0) & (BIT(1) | BIT(2)))) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	if (addr_type) {
		if (bdaddr) {
			(void)memcpy(rnd_addr, bdaddr, BDADDR_SIZE);
		} else {
			(void)memset(rnd_addr, 0, BDADDR_SIZE);
		}
	} else {
		if (bdaddr) {
			(void)memcpy(pub_addr, bdaddr, BDADDR_SIZE);
		} else {
			(void)memset(pub_addr, 0, BDADDR_SIZE);
		}
	}

	return 0;
}

uint8_t *ll_addr_get(uint8_t addr_type)
{
	if (addr_type > BT_ADDR_LE_RANDOM) {
		return NULL;
	}

	if (addr_type) {
		return rnd_addr;
	}

	return pub_addr;
}

uint8_t *ll_addr_read(uint8_t addr_type, uint8_t *const bdaddr)
{
	uint8_t *addr;

	addr = ll_addr_get(addr_type);
	if (addr) {
		memcpy(bdaddr, addr, BDADDR_SIZE);
	}

	return addr;
}

void bt_ctlr_set_public_addr(const uint8_t *addr)
{
	(void)memcpy(pub_addr, addr, sizeof(pub_addr));
}
