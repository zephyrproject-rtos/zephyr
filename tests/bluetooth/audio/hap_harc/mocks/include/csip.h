/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCKS_CSIP_SET_COORDINATOR_H_
#define MOCKS_CSIP_SET_COORDINATOR_H_

#include <zephyr/fff.h>
#include <zephyr/bluetooth/audio/csip.h>

/* List of fakes used by this unit tester */
#define CSIP_FFF_FAKES_LIST(FAKE)                                                                  \
	FAKE(bt_csip_set_coordinator_discover)                                                     \
	FAKE(bt_csip_set_coordinator_register_cb)                                                  \
	FAKE(bt_csip_set_coordinator_ordered_access)                                               \
	FAKE(bt_csip_set_coordinator_lock)                                                         \
	FAKE(bt_csip_set_coordinator_release)                                                      \
	FAKE(bt_csip_set_coordinator_set_members_sort_by_rank)                                     \

DECLARE_FAKE_VALUE_FUNC(int, bt_csip_set_coordinator_discover, struct bt_conn *);
DECLARE_FAKE_VALUE_FUNC(int, bt_csip_set_coordinator_register_cb,
			struct bt_csip_set_coordinator_cb *);
DECLARE_FAKE_VALUE_FUNC(int, bt_csip_set_coordinator_ordered_access,
			const struct bt_csip_set_coordinator_set_member **, uint8_t,
			const struct bt_csip_set_coordinator_set_info *,
			bt_csip_set_coordinator_ordered_access_t);
DECLARE_FAKE_VALUE_FUNC(int, bt_csip_set_coordinator_lock,
			const struct bt_csip_set_coordinator_set_member **, uint8_t,
			const struct bt_csip_set_coordinator_set_info *);
DECLARE_FAKE_VALUE_FUNC(int, bt_csip_set_coordinator_release,
			const struct bt_csip_set_coordinator_set_member **, uint8_t,
			const struct bt_csip_set_coordinator_set_info *);
DECLARE_FAKE_VOID_FUNC(bt_csip_set_coordinator_set_members_sort_by_rank,
		       const struct bt_csip_set_coordinator_set_member **, size_t,
		       const struct bt_csip_set_coordinator_set_info *, bool);

#endif /* MOCKS_CSIP_SET_COORDINATOR_H_ */
