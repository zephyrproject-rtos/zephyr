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
extern struct bst_test_list *test_provision_pst_install(struct bst_test_list *tests);
#if defined(CONFIG_BT_MESH_V1d1)
extern struct bst_test_list *test_dfu_install(struct bst_test_list *test);
extern struct bst_test_list *test_blob_pst_install(struct bst_test_list *test);
extern struct bst_test_list *test_sar_pst_install(struct bst_test_list *test);
#endif /* defined(CONFIG_BT_MESH_V1d1) */
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
#if defined(CONFIG_BT_MESH_V1d1)
extern struct bst_test_list *test_blob_install(struct bst_test_list *test);
extern struct bst_test_list *test_op_agg_install(struct bst_test_list *test);
extern struct bst_test_list *test_sar_install(struct bst_test_list *test);
extern struct bst_test_list *test_lcd_install(struct bst_test_list *test);
#endif /* defined(CONFIG_BT_MESH_V1d1) */
#endif

bst_test_install_t test_installers[] = {
#if defined(CONFIG_SETTINGS)
	test_persistence_install,
	test_rpc_install,
#if defined(CONFIG_BT_MESH_V1d1)
	test_provision_pst_install,
	test_dfu_install,
	test_blob_pst_install,
	test_sar_pst_install,
#endif /* defined(CONFIG_BT_MESH_V1d1) */
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
#if defined(CONFIG_BT_MESH_V1d1)
	test_blob_install,
	test_op_agg_install,
	test_sar_install,
	test_lcd_install,
#endif /* defined(CONFIG_BT_MESH_V1d1) */
#endif
	NULL
};

int main(void)
{
	bst_main();
	return 0;
}
