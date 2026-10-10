/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "babblekit/testcase.h"

void test_peripheral_main(void);
void test_central_main(void);

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "peripheral",
		.test_descr = "Peripheral deleting and resetting the identity of a connection",
		.test_main_f = test_peripheral_main,
	},
	{
		.test_id = "central",
		.test_descr = "Central expecting to be disconnected by the peripheral",
		.test_main_f = test_central_main,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_id_disconnect_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}

bst_test_install_t test_installers[] = {test_id_disconnect_install, NULL};

int main(void)
{
	bst_main();
	return 0;
}
