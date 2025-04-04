/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stddef.h>

#include "bstests.h"

extern struct bst_test_list *test_ccp_central_install(struct bst_test_list *tests);
extern struct bst_test_list *test_ccp_peripheral_install(struct bst_test_list *tests);
extern struct bst_test_list *test_csip_central_install(struct bst_test_list *tests);
extern struct bst_test_list *test_csip_peripheral_install(struct bst_test_list *tests);
extern struct bst_test_list *test_gap_central_install(struct bst_test_list *tests);
extern struct bst_test_list *test_gap_peripheral_install(struct bst_test_list *tests);
extern struct bst_test_list *test_hap_central_install(struct bst_test_list *tests);
extern struct bst_test_list *test_hap_peripheral_install(struct bst_test_list *tests);
extern struct bst_test_list *test_mcp_central_install(struct bst_test_list *tests);
extern struct bst_test_list *test_mcp_peripheral_install(struct bst_test_list *tests);
extern struct bst_test_list *test_micp_central_install(struct bst_test_list *tests);
extern struct bst_test_list *test_micp_peripheral_install(struct bst_test_list *tests);
extern struct bst_test_list *test_tmap_central_install(struct bst_test_list *tests);
extern struct bst_test_list *test_tmap_peripheral_install(struct bst_test_list *tests);
extern struct bst_test_list *test_vcp_central_install(struct bst_test_list *tests);
extern struct bst_test_list *test_vcp_peripheral_install(struct bst_test_list *tests);
extern struct bst_test_list *test_iso_broadcaster_install(struct bst_test_list *tests);
extern struct bst_test_list *test_iso_sync_receiver_install(struct bst_test_list *tests);

bst_test_install_t test_installers[] = {
	test_ccp_central_install,
	test_ccp_peripheral_install,
	test_csip_central_install,
	test_csip_peripheral_install,
	test_gap_central_install,
	test_gap_peripheral_install,
	test_hap_central_install,
	test_hap_peripheral_install,
	test_mcp_central_install,
	test_mcp_peripheral_install,
	test_micp_central_install,
	test_micp_peripheral_install,
	test_tmap_central_install,
	test_tmap_peripheral_install,
	test_vcp_central_install,
	test_vcp_peripheral_install,
	test_iso_broadcaster_install,
	test_iso_sync_receiver_install,
	NULL,
};

int main(void)
{
	bst_main();
	return 0;
}
