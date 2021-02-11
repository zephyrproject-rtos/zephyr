/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bstests.h"
#include "mesh_test.h"

extern struct bst_test_list *
test_transport_install(struct bst_test_list *tests);

bst_test_install_t test_installers[] = {
	test_transport_install,
	NULL
};

void main(void)
{
	bst_main();
}
