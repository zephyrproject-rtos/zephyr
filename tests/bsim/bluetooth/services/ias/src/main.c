/*
 * Copyright (c) 2026 Bohdan Vereshchak
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bstests.h"

extern struct bst_test_list *test_ias_install(struct bst_test_list *tests);
extern struct bst_test_list *test_ias_client_install(struct bst_test_list *tests);

bst_test_install_t test_installers[] = {test_ias_install, test_ias_client_install, NULL};

int main(void)
{
	bst_main();

	return 0;
}
