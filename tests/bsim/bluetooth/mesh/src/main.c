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
extern struct bst_test_list *test_dfu_install(struct bst_test_list *test);
extern struct bst_test_list *test_blob_pst_install(struct bst_test_list *test);
extern struct bst_test_list *test_lcd_install(struct bst_test_list *test);
extern struct bst_test_list *test_sar_pst_install(struct bst_test_list *test);
extern struct bst_test_list *test_brg_install(struct bst_test_list *test);
#if (CONFIG_BT_MESH_GATT_PROXY && CONFIG_BT_MESH_PROXY_SOLICITATION)
extern struct bst_test_list *test_proxy_sol_install(struct bst_test_list *test);
#endif
#elif defined(CONFIG_BT_MESH_GATT_PROXY)
extern struct bst_test_list *test_adv_install(struct bst_test_list *test);
extern struct bst_test_list *test_suspend_install(struct bst_test_list *test);
extern struct bst_test_list *test_beacon_install(struct bst_test_list *tests);
#elif defined(CONFIG_BT_CTLR_LOW_LAT)
extern struct bst_test_list *test_transport_install(struct bst_test_list *tests);
extern struct bst_test_list *test_friendship_install(struct bst_test_list *tests);
extern struct bst_test_list *test_suspend_install(struct bst_test_list *test);
extern struct bst_test_list *test_adv_install(struct bst_test_list *test);
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
extern struct bst_test_list *test_suspend_install(struct bst_test_list *test);
extern struct bst_test_list *test_blob_install(struct bst_test_list *test);
extern struct bst_test_list *test_op_agg_install(struct bst_test_list *test);
extern struct bst_test_list *test_sar_install(struct bst_test_list *test);
extern struct bst_test_list *test_cdp1_install(struct bst_test_list *test);
extern struct bst_test_list *test_brg_install(struct bst_test_list *test);
#endif

bst_test_install_t test_installers[] = {
#if defined(CONFIG_SETTINGS)
	test_persistence_install,
	test_rpc_install,
	test_provision_pst_install,
	test_dfu_install,
	test_blob_pst_install,
	test_lcd_install,
	test_sar_pst_install,
	test_brg_install,
#if (CONFIG_BT_MESH_GATT_PROXY && CONFIG_BT_MESH_PROXY_SOLICITATION)
	test_proxy_sol_install,
#endif
#elif defined(CONFIG_BT_MESH_GATT_PROXY)
	test_adv_install, test_suspend_install, test_beacon_install,
#elif defined(CONFIG_BT_CTLR_LOW_LAT)
	test_transport_install, test_friendship_install, test_suspend_install, test_adv_install,
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
	test_suspend_install,
	test_blob_install,
	test_op_agg_install,
	test_sar_install,
	test_cdp1_install,
	test_brg_install,
#endif
	NULL};

static struct k_thread bsim_mesh_thread;
static K_KERNEL_STACK_DEFINE(bsim_mesh_thread_stack, 4096);

static void bsim_mesh_entry_point(void *unused1, void *unused2, void *unused3)
{
	bst_main();
}

int main(void)
{
	k_thread_create(&bsim_mesh_thread, bsim_mesh_thread_stack,
			K_KERNEL_STACK_SIZEOF(bsim_mesh_thread_stack), bsim_mesh_entry_point, NULL,
			NULL, NULL, K_PRIO_COOP(1), 0, K_NO_WAIT);
	k_thread_name_set(&bsim_mesh_thread, "BabbleSim BLE Mesh tests");

	return 0;
}
