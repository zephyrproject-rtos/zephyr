/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bstests.h"
#include "mesh_test.h"

#if defined(CONFIG_SETTINGS)
extern struct bst_test_list *test_persistence_install(struct bst_test_list *tests);
#else
extern struct bst_test_list *test_transport_install(struct bst_test_list *tests);
extern struct bst_test_list *test_friendship_install(struct bst_test_list *tests);
extern struct bst_test_list *test_provision_install(struct bst_test_list *tests);
extern struct bst_test_list *test_beacon_install(struct bst_test_list *tests);
#endif


bst_test_install_t test_installers[] = {
#if defined(CONFIG_SETTINGS)
	test_persistence_install,
#else
	test_transport_install,
	test_friendship_install,
	test_provision_install,
	test_beacon_install,
#endif
	NULL
};

void main(void)
{
	bst_main();
}
