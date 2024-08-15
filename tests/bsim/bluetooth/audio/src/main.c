/*
 * Copyright (c) 2020-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

#include "bstests.h"

extern struct bst_test_list *test_vcp_install(struct bst_test_list *tests);
extern struct bst_test_list *test_vcp_vol_ctlr_install(struct bst_test_list *tests);
extern struct bst_test_list *test_micp_install(struct bst_test_list *tests);
extern struct bst_test_list *test_micp_mic_ctlr_install(struct bst_test_list *tests);
extern struct bst_test_list *test_csip_set_member_install(struct bst_test_list *tests);
extern struct bst_test_list *test_csip_set_coordinator_install(struct bst_test_list *tests);
extern struct bst_test_list *test_tbs_install(struct bst_test_list *tests);
extern struct bst_test_list *test_tbs_client_install(struct bst_test_list *tests);
extern struct bst_test_list *test_mcs_install(struct bst_test_list *tests);
extern struct bst_test_list *test_mcc_install(struct bst_test_list *tests);
extern struct bst_test_list *test_media_controller_install(struct bst_test_list *tests);
extern struct bst_test_list *test_unicast_client_install(struct bst_test_list *tests);
extern struct bst_test_list *test_unicast_server_install(struct bst_test_list *tests);
extern struct bst_test_list *test_broadcast_source_install(struct bst_test_list *tests);
extern struct bst_test_list *test_broadcast_sink_install(struct bst_test_list *tests);
extern struct bst_test_list *test_scan_delegator_install(struct bst_test_list *tests);
extern struct bst_test_list *test_bap_broadcast_assistant_install(struct bst_test_list *tests);
extern struct bst_test_list *test_bass_broadcaster_install(struct bst_test_list *tests);
extern struct bst_test_list *test_cap_acceptor_install(struct bst_test_list *tests);
extern struct bst_test_list *test_cap_commander_install(struct bst_test_list *tests);
extern struct bst_test_list *test_cap_initiator_broadcast_install(struct bst_test_list *tests);
extern struct bst_test_list *test_cap_initiator_unicast_install(struct bst_test_list *tests);
extern struct bst_test_list *test_has_install(struct bst_test_list *tests);
extern struct bst_test_list *test_has_client_install(struct bst_test_list *tests);
extern struct bst_test_list *test_ias_install(struct bst_test_list *tests);
extern struct bst_test_list *test_ias_client_install(struct bst_test_list *tests);
extern struct bst_test_list *test_tmap_client_install(struct bst_test_list *tests);
extern struct bst_test_list *test_tmap_server_install(struct bst_test_list *tests);
extern struct bst_test_list *test_pacs_notify_client_install(struct bst_test_list *tests);
extern struct bst_test_list *test_pacs_notify_server_install(struct bst_test_list *tests);
extern struct bst_test_list *test_public_broadcast_source_install(struct bst_test_list *tests);
extern struct bst_test_list *test_public_broadcast_sink_install(struct bst_test_list *tests);
extern struct bst_test_list *test_csip_notify_client_install(struct bst_test_list *tests);
extern struct bst_test_list *test_csip_notify_server_install(struct bst_test_list *tests);
extern struct bst_test_list *test_gmap_ugg_install(struct bst_test_list *tests);
extern struct bst_test_list *test_gmap_ugt_install(struct bst_test_list *tests);
extern struct bst_test_list *test_ccp_call_control_server_install(struct bst_test_list *tests);

bst_test_install_t test_installers[] = {
	test_vcp_install,
	test_vcp_vol_ctlr_install,
	test_micp_install,
	test_micp_mic_ctlr_install,
	test_csip_set_member_install,
	test_csip_set_coordinator_install,
	test_tbs_install,
	test_tbs_client_install,
	test_mcs_install,
	test_mcc_install,
	test_media_controller_install,
	test_unicast_client_install,
	test_unicast_server_install,
	test_broadcast_source_install,
	test_broadcast_sink_install,
	test_scan_delegator_install,
	test_bap_broadcast_assistant_install,
	test_bass_broadcaster_install,
	test_cap_commander_install,
	test_cap_acceptor_install,
	test_cap_initiator_broadcast_install,
	test_cap_initiator_unicast_install,
	test_has_install,
	test_has_client_install,
	test_ias_install,
	test_ias_client_install,
	test_tmap_server_install,
	test_tmap_client_install,
	test_pacs_notify_client_install,
	test_pacs_notify_server_install,
	test_public_broadcast_source_install,
	test_public_broadcast_sink_install,
	test_csip_notify_client_install,
	test_csip_notify_server_install,
	test_gmap_ugg_install,
	test_gmap_ugt_install,
	test_ccp_call_control_server_install,
	NULL,
};

int main(void)
{
	bst_main();
	return 0;
}
