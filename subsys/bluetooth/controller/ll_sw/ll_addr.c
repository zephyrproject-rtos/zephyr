/*
 * Copyright (c) 2016-2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stddef.h>
#include <string.h>

#include <zephyr/types.h>
#include <bluetooth/hci.h>

/* TODO: remove begin, dependency on ctrl.h when having ll_scan.h interface */
#include <misc/slist.h>

#include "util/util.h"

#include "pdu.h"

#include "ull_adv_internal.h"
#include "ull_scan_internal.h"

static u8_t pub_addr[BDADDR_SIZE];
static u8_t rnd_addr[BDADDR_SIZE];

u8_t *ll_addr_get(u8_t addr_type, u8_t *bdaddr)
{
	if (addr_type > 1) {
		return NULL;
	}

	if (addr_type) {
		if (bdaddr) {
			memcpy(bdaddr, rnd_addr, BDADDR_SIZE);
		}

		return rnd_addr;
	}

	if (bdaddr) {
		memcpy(bdaddr, pub_addr, BDADDR_SIZE);
	}

	return pub_addr;
}

u32_t ll_addr_set(u8_t addr_type, u8_t const *const bdaddr)
{
	if (IS_ENABLED(CONFIG_BT_BROADCASTER) &&
	    ull_adv_is_enabled(0xFFFF)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	if (IS_ENABLED(CONFIG_BT_OBSERVER) &&
	    (ll_scan_is_enabled() & (BIT(1) | BIT(2)))) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	if (addr_type) {
		memcpy(rnd_addr, bdaddr, BDADDR_SIZE);
	} else {
		memcpy(pub_addr, bdaddr, BDADDR_SIZE);
	}

	return 0;
}

/* TODO: Delete below weak functions when porting to ULL/LLL. */
u32_t __weak ull_adv_is_enabled(u16_t handle)
{
	return 0;
}

u32_t __weak ll_scan_is_enabled(void)
{
	return 0;
}
