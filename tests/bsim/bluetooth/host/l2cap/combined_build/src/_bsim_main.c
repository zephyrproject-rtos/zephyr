/*
 * Copyright (c) 2025 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bstests.h"

extern struct bst_test_list *test_main_l2cap_credits_seg_recv_install(struct bst_test_list *tests);
extern struct bst_test_list *test_main_l2cap_credits_install(struct bst_test_list *tests);
extern struct bst_test_list *test_main_l2cap_send_on_connect_install(struct bst_test_list *tests);
extern struct bst_test_list *test_main_l2cap_einprogress_install(struct bst_test_list *tests);

bst_test_install_t test_installers[] = {
        test_main_l2cap_credits_install,
        test_main_l2cap_credits_seg_recv_install,
        test_main_l2cap_send_on_connect_install,
        test_main_l2cap_einprogress_install,
        NULL
};

int main(void)
{
	bst_main();

	return 0;
}
