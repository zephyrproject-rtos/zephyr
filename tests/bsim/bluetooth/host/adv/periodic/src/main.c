/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bstests.h"

extern struct bst_test_list *test_per_adv_syncer(struct bst_test_list *tests);
extern struct bst_test_list *test_per_adv_advertiser(struct bst_test_list *tests);

bst_test_install_t test_installers[] = {
	test_per_adv_syncer,
	test_per_adv_advertiser,
	NULL
};

int main(void)
{
	bst_main();
	return 0;
}
