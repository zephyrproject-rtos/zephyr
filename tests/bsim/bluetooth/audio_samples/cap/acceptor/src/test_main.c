/*
 * Copyright (c) 2023-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bstests.h"

extern struct bst_test_list *test_cap_acceptor_sample_install(struct bst_test_list *tests);

bst_test_install_t test_installers[] = {
	test_cap_acceptor_sample_install,
	NULL
};
