/*
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bstests.h"

extern struct bst_test_list *test_main_bis_broadcaster_install(struct bst_test_list *tests);
extern struct bst_test_list *test_main_bis_receiver_install(struct bst_test_list *tests);

bst_test_install_t test_installers[] = {
	test_main_bis_broadcaster_install,
	test_main_bis_receiver_install,
	NULL,
};

int main(void)
{
	bst_main();
	return 0;
}
