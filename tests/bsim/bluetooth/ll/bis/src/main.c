/*
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bstests.h"

extern struct bst_test_list *test_iso_install(struct bst_test_list *tests);

#ifdef CONFIG_BT_PER_ADV_SYNC_TRANSFER_SENDER
extern struct bst_test_list *test_past_install(struct bst_test_list *tests);
#endif /* CONFIG_BT_PER_ADV_SYNC_TRANSFER_SENDER */

bst_test_install_t test_installers[] = {
	test_iso_install,
#ifdef CONFIG_BT_PER_ADV_SYNC_TRANSFER_SENDER
	test_past_install,
#endif /* CONFIG_BT_PER_ADV_SYNC_TRANSFER_SENDER */
	NULL
};

int main(void)
{
	bst_main();
	return 0;
}
