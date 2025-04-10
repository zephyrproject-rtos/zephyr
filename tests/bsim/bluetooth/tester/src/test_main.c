/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stddef.h>

#include "bstests.h"

extern struct bst_test_list *test_bap_broadcast_sink_install(struct bst_test_list *tests);
extern struct bst_test_list *test_bap_broadcast_source_install(struct bst_test_list *tests);
extern struct bst_test_list *test_bap_central_install(struct bst_test_list *tests);
extern struct bst_test_list *test_bap_peripheral_install(struct bst_test_list *tests);
extern struct bst_test_list *test_cap_central_install(struct bst_test_list *tests);
extern struct bst_test_list *test_cap_peripheral_install(struct bst_test_list *tests);
extern struct bst_test_list *test_gap_central_install(struct bst_test_list *tests);
extern struct bst_test_list *test_gap_peripheral_install(struct bst_test_list *tests);
extern struct bst_test_list *test_vcp_central_install(struct bst_test_list *tests);
extern struct bst_test_list *test_vcp_peripheral_install(struct bst_test_list *tests);

bst_test_install_t test_installers[] = {
	test_bap_broadcast_sink_install,
	test_bap_broadcast_source_install,
	test_bap_central_install,
	test_bap_peripheral_install,
	test_cap_central_install,
	test_cap_peripheral_install,
	test_gap_central_install,
	test_gap_peripheral_install,
	test_vcp_central_install,
	test_vcp_peripheral_install,
	NULL,
};

int main(void)
{
	bst_main();
	return 0;
}
