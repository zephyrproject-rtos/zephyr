/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bstests.h"

extern struct bst_test_list *test_unicast_client_sample_install(struct bst_test_list *tests);

bst_test_install_t test_installers[] = {
	test_unicast_client_sample_install,
	NULL
};
