/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fff.h>

/* List of fakes used by this unit tester */
#define HCI_CORE_FFF_FAKES_LIST(FAKE)         \
		FAKE(bt_hci_le_rand)                  \

DECLARE_FAKE_VALUE_FUNC(int, bt_hci_le_rand, void *, size_t);
