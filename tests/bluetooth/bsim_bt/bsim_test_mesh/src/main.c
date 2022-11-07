/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bstests.h"
#include "mesh_test.h"

#if defined(CONFIG_SETTINGS)
extern struct bst_test_list *test_persistence_install(struct bst_test_list *tests);
extern struct bst_test_list *test_rpc_install(struct bst_test_list *tests);
#elif defined(CONFIG_BT_MESH_GATT_PROXY)
extern struct bst_test_list *test_adv_install(struct bst_test_list *test);
#elif defined(CONFIG_BT_CTLR_LOW_LAT)
extern struct bst_test_list *test_transport_install(struct bst_test_list *tests);
extern struct bst_test_list *test_friendship_install(struct bst_test_list *tests);
#else
extern struct bst_test_list *test_transport_install(struct bst_test_list *tests);
extern struct bst_test_list *test_friendship_install(struct bst_test_list *tests);
extern struct bst_test_list *test_provision_install(struct bst_test_list *tests);
extern struct bst_test_list *test_beacon_install(struct bst_test_list *tests);
extern struct bst_test_list *test_scanner_install(struct bst_test_list *test);
extern struct bst_test_list *test_heartbeat_install(struct bst_test_list *test);
extern struct bst_test_list *test_access_install(struct bst_test_list *test);
extern struct bst_test_list *test_ivi_install(struct bst_test_list *test);
extern struct bst_test_list *test_adv_install(struct bst_test_list *test);
#endif

bst_test_install_t test_installers[] = {
#if defined(CONFIG_SETTINGS)
	test_persistence_install,
	test_rpc_install,
#elif defined(CONFIG_BT_MESH_GATT_PROXY)
	test_adv_install,
#elif defined(CONFIG_BT_CTLR_LOW_LAT)
	test_transport_install,
	test_friendship_install,
#else
	test_transport_install,
	test_friendship_install,
	test_provision_install,
	test_beacon_install,
	test_scanner_install,
	test_heartbeat_install,
	test_access_install,
	test_ivi_install,
	test_adv_install,
#endif
	NULL
};

void main(void)
{
	bst_main();
}
