/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bs_bt_utils.h"
#include "bstests.h"

extern struct bst_test_list *test_main_rpa_rotation_install(struct bst_test_list *tests);
extern struct bst_test_list *test_main_rpa_expired_install(struct bst_test_list *tests);

bst_test_install_t test_installers[] = {
	test_main_rpa_rotation_install,
	test_main_rpa_expired_install,
	NULL,
};

int main(void)
{
	bst_main();
	return 0;
}
