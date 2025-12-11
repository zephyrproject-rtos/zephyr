/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stddef.h>

#include "bstests.h"

extern struct bst_test_list *test_broadcast_sink_test_install(struct bst_test_list *tests);

bst_test_install_t test_installers[] = {
	test_broadcast_sink_test_install,
	NULL
};
