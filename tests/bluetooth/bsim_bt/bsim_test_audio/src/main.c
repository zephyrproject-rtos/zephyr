/*
 * Copyright (c) 2020-2021 Nordic Semiconductor ASA
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bstests.h"

extern struct bst_test_list *test_vcs_install(struct bst_test_list *tests);
extern struct bst_test_list *test_vcs_client_install(struct bst_test_list *tests);
extern struct bst_test_list *test_mics_install(struct bst_test_list *tests);
extern struct bst_test_list *test_mics_client_install(struct bst_test_list *tests);
extern struct bst_test_list *test_tbs_install(struct bst_test_list *tests);
extern struct bst_test_list *test_ccp_install(struct bst_test_list *tests);
extern struct bst_test_list *test_csis_install(struct bst_test_list *tests);
extern struct bst_test_list *test_csip_install(struct bst_test_list *tests);
extern struct bst_test_list *test_mcs_install(struct bst_test_list *tests);
extern struct bst_test_list *test_mcc_install(struct bst_test_list *tests);
extern struct bst_test_list *test_bass_install(struct bst_test_list *tests);
extern struct bst_test_list *test_bass_client_install(struct bst_test_list *tests);
extern struct bst_test_list *test_bass_broadcaster_install(struct bst_test_list *tests);


bst_test_install_t test_installers[] = {
	test_vcs_install,
	test_vcs_client_install,
	test_mics_install,
	test_mics_client_install,
	test_tbs_install,
	test_ccp_install,
	test_csis_install,
	test_csip_install,
	test_mcs_install,
	test_mcc_install,
	test_bass_install,
	test_bass_client_install,
	test_bass_broadcaster_install,
	NULL
};

void main(void)
{
	bst_main();
}
