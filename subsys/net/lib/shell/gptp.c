/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include <stdlib.h>

#if defined(CONFIG_NET_GPTP)
#include <zephyr/net/gptp.h>
#include "ethernet/gptp/gptp_messages.h"
#include "ethernet/gptp/gptp_md.h"
#include "ethernet/gptp/gptp_state.h"
#include "ethernet/gptp/gptp_data_set.h"
#include "ethernet/gptp/gptp_private.h"
#endif

#include "net_shell_private.h"

#if defined(CONFIG_NET_GPTP)
static const char *selected_role_str(int port);

static void gptp_port_cb(int port, struct net_if *iface, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;

	if (*count == 0) {
		PR("Port Interface  \tRole\n");
	}

	(*count)++;

	PR("%2d   %p [%d]  \t%s\n", port, iface, net_if_get_by_iface(iface),
	   selected_role_str(port));
}

static const char *pdelay_req2str(enum gptp_pdelay_req_states state)
{
	switch (state) {
	case GPTP_PDELAY_REQ_NOT_ENABLED:
		return "REQ_NOT_ENABLED";
	case GPTP_PDELAY_REQ_INITIAL_SEND_REQ:
		return "INITIAL_SEND_REQ";
	case GPTP_PDELAY_REQ_RESET:
		return "REQ_RESET";
	case GPTP_PDELAY_REQ_SEND_REQ:
		return "SEND_REQ";
	case GPTP_PDELAY_REQ_WAIT_RESP:
		return "WAIT_RESP";
	case GPTP_PDELAY_REQ_WAIT_FOLLOW_UP:
		return "WAIT_FOLLOW_UP";
	case GPTP_PDELAY_REQ_WAIT_ITV_TIMER:
		return "WAIT_ITV_TIMER";
	}

	return "<unknown>";
};

static const char *pdelay_resp2str(enum gptp_pdelay_resp_states state)
{
	switch (state) {
	case GPTP_PDELAY_RESP_NOT_ENABLED:
		return "RESP_NOT_ENABLED";
	case GPTP_PDELAY_RESP_INITIAL_WAIT_REQ:
		return "INITIAL_WAIT_REQ";
	case GPTP_PDELAY_RESP_WAIT_REQ:
		return "WAIT_REQ";
	case GPTP_PDELAY_RESP_WAIT_TSTAMP:
		return "WAIT_TSTAMP";
	}

	return "<unknown>";
}

static const char *sync_rcv2str(enum gptp_sync_rcv_states state)
{
	switch (state) {
	case GPTP_SYNC_RCV_DISCARD:
		return "DISCARD";
	case GPTP_SYNC_RCV_WAIT_SYNC:
		return "WAIT_SYNC";
	case GPTP_SYNC_RCV_WAIT_FOLLOW_UP:
		return "WAIT_FOLLOW_UP";
	}

	return "<unknown>";
}

static const char *sync_send2str(enum gptp_sync_send_states state)
{
	switch (state) {
	case GPTP_SYNC_SEND_INITIALIZING:
		return "INITIALIZING";
	case GPTP_SYNC_SEND_SEND_SYNC:
		return "SEND_SYNC";
	case GPTP_SYNC_SEND_SEND_FUP:
		return "SEND_FUP";
	}

	return "<unknown>";
}

static const char *pss_rcv2str(enum gptp_pss_rcv_states state)
{
	switch (state) {
	case GPTP_PSS_RCV_DISCARD:
		return "DISCARD";
	case GPTP_PSS_RCV_RECEIVED_SYNC:
		return "RECEIVED_SYNC";
	}

	return "<unknown>";
}

static const char *pss_send2str(enum gptp_pss_send_states state)
{
	switch (state) {
	case GPTP_PSS_SEND_TRANSMIT_INIT:
		return "TRANSMIT_INIT";
	case GPTP_PSS_SEND_SYNC_RECEIPT_TIMEOUT:
		return "SYNC_RECEIPT_TIMEOUT";
	case GPTP_PSS_SEND_SEND_MD_SYNC:
		return "SEND_MD_SYNC";
	case GPTP_PSS_SEND_SET_SYNC_RECEIPT_TIMEOUT:
		return "SET_SYNC_RECEIPT_TIMEOUT";
	}

	return "<unknown>";
}

static const char *pa_rcv2str(enum gptp_pa_rcv_states state)
{
	switch (state) {
	case GPTP_PA_RCV_DISCARD:
		return "DISCARD";
	case GPTP_PA_RCV_RECEIVE:
		return "RECEIVE";
	}

	return "<unknown>";
};

static const char *pa_info2str(enum gptp_pa_info_states state)
{
	switch (state) {
	case GPTP_PA_INFO_DISABLED:
		return "DISABLED";
	case GPTP_PA_INFO_POST_DISABLED:
		return "POST_DISABLED";
	case GPTP_PA_INFO_AGED:
		return "AGED";
	case GPTP_PA_INFO_UPDATE:
		return "UPDATE";
	case GPTP_PA_INFO_CURRENT:
		return "CURRENT";
	case GPTP_PA_INFO_RECEIVE:
		return "RECEIVE";
	case GPTP_PA_INFO_SUPERIOR_MASTER_PORT:
		return "SUPERIOR_MASTER_PORT";
	case GPTP_PA_INFO_REPEATED_MASTER_PORT:
		return "REPEATED_MASTER_PORT";
	case GPTP_PA_INFO_INFERIOR_MASTER_OR_OTHER_PORT:
		return "INFERIOR_MASTER_OR_OTHER_PORT";
	}

	return "<unknown>";
};

static const char *pa_transmit2str(enum gptp_pa_transmit_states state)
{
	switch (state) {
	case GPTP_PA_TRANSMIT_INIT:
		return "INIT";
	case GPTP_PA_TRANSMIT_PERIODIC:
		return "PERIODIC";
	case GPTP_PA_TRANSMIT_IDLE:
		return "IDLE";
	case GPTP_PA_TRANSMIT_POST_IDLE:
		return "POST_IDLE";
	}

	return "<unknown>";
};

static const char *site_sync2str(enum gptp_site_sync_sync_states state)
{
	switch (state) {
	case GPTP_SSS_INITIALIZING:
		return "INITIALIZING";
	case GPTP_SSS_RECEIVING_SYNC:
		return "RECEIVING_SYNC";
	}

	return "<unknown>";
}

static const char *clk_slave2str(enum gptp_clk_slave_sync_states state)
{
	switch (state) {
	case GPTP_CLK_SLAVE_SYNC_INITIALIZING:
		return "INITIALIZING";
	case GPTP_CLK_SLAVE_SYNC_SEND_SYNC_IND:
		return "SEND_SYNC_IND";
	}

	return "<unknown>";
};

static const char *pr_selection2str(enum gptp_pr_selection_states state)
{
	switch (state) {
	case GPTP_PR_SELECTION_INIT_BRIDGE:
		return "INIT_BRIDGE";
	case GPTP_PR_SELECTION_ROLE_SELECTION:
		return "ROLE_SELECTION";
	}

	return "<unknown>";
};

static const char *cms_rcv2str(enum gptp_cms_rcv_states state)
{
	switch (state) {
	case GPTP_CMS_RCV_INITIALIZING:
		return "INITIALIZING";
	case GPTP_CMS_RCV_WAITING:
		return "WAITING";
	case GPTP_CMS_RCV_SOURCE_TIME:
		return "SOURCE_TIME";
	}

	return "<unknown>";
};

#if !defined(USCALED_NS_TO_NS)
#define USCALED_NS_TO_NS(val) (val >> 16)
#endif

static const char *selected_role_str(int port)
{
	switch (GPTP_GLOBAL_DS()->selected_role[port]) {
	case GPTP_PORT_INITIALIZING:
		return "INITIALIZING";
	case GPTP_PORT_FAULTY:
		return "FAULTY";
	case GPTP_PORT_DISABLED:
		return "DISABLED";
	case GPTP_PORT_LISTENING:
		return "LISTENING";
	case GPTP_PORT_PRE_MASTER:
		return "PRE-MASTER";
	case GPTP_PORT_MASTER:
		return "MASTER";
	case GPTP_PORT_PASSIVE:
		return "PASSIVE";
	case GPTP_PORT_UNCALIBRATED:
		return "UNCALIBRATED";
	case GPTP_PORT_SLAVE:
		return "SLAVE";
	}

	return "<unknown>";
}

static void gptp_print_port_info(const struct shell *sh, int port)
{
	struct gptp_port_bmca_data *port_bmca_data;
	struct gptp_port_param_ds *port_param_ds;
	struct gptp_port_states *port_state;
	struct gptp_domain *domain;
	struct gptp_port_ds *port_ds;
	struct net_if *iface;
	int ret, i;

	domain = gptp_get_domain();

	ret = gptp_get_port_data(domain,
				 port,
				 &port_ds,
				 &port_param_ds,
				 &port_state,
				 &port_bmca_data,
				 &iface);
	if (ret < 0) {
		PR_WARNING("Cannot get gPTP information for port %d (%d)\n",
			   port, ret);
		return;
	}

	NET_ASSERT(port == port_ds->port_id.port_number,
		   "Port number mismatch! (%d vs %d)", port,
		   port_ds->port_id.port_number);

	PR("Port id    : %d (%s)\n", port_ds->port_id.port_number,
	   selected_role_str(port_ds->port_id.port_number));
	PR("Interface  : %p [%d]\n", iface, net_if_get_by_iface(iface));
	PR("Clock id   : ");
	for (i = 0; i < sizeof(port_ds->port_id.clk_id); i++) {
		PR("%02x", port_ds->port_id.clk_id[i]);

		if (i != (sizeof(port_ds->port_id.clk_id) - 1)) {
			PR(":");
		}
	}
	PR("\n");

	PR("Version    : %d\n", port_ds->version);
	PR("AS capable : %s\n", port_ds->as_capable ? "yes" : "no");

	PR("\nConfiguration:\n");
	PR("Time synchronization and Best Master Selection enabled        "
	   ": %s\n", port_ds->ptt_port_enabled ? "yes" : "no");
	PR("The port is measuring the path delay                          "
	   ": %s\n", port_ds->is_measuring_delay ? "yes" : "no");
	PR("One way propagation time on %s    : %u ns\n",
	   "the link attached to this port",
	   (uint32_t)port_ds->neighbor_prop_delay);
	PR("Propagation time threshold for %s : %u ns\n",
	   "the link attached to this port",
	   (uint32_t)port_ds->neighbor_prop_delay_thresh);
	PR("Estimate of the ratio of the frequency with the peer          "
	   ": %u\n", (uint32_t)port_ds->neighbor_rate_ratio);
	PR("Asymmetry on the link relative to the grand master time base  "
	   ": %" PRId64 "\n", port_ds->delay_asymmetry);
	PR("Maximum interval between sync %s                        "
	   ": %" PRIu64 "\n", "messages",
	   port_ds->sync_receipt_timeout_time_itv);
	PR("Maximum number of Path Delay Requests without a response      "
	   ": %d\n", port_ds->allowed_lost_responses);
	PR("Current Sync %s                        : %d\n",
	   "sequence id for this port", port_ds->sync_seq_id);
	PR("Current Path Delay Request %s          : %d\n",
	   "sequence id for this port", port_ds->pdelay_req_seq_id);
	PR("Current Announce %s                    : %d\n",
	   "sequence id for this port", port_ds->announce_seq_id);
	PR("Current Signaling %s                   : %d\n",
	   "sequence id for this port", port_ds->signaling_seq_id);
	PR("Whether neighborRateRatio %s  : %s\n",
	   "needs to be computed for this port",
	   port_ds->compute_neighbor_rate_ratio ? "yes" : "no");
	PR("Whether neighborPropDelay %s  : %s\n",
	   "needs to be computed for this port",
	   port_ds->compute_neighbor_prop_delay ? "yes" : "no");
	PR("Initial Announce Interval %s            : %d\n",
	   "as a Logarithm to base 2", port_ds->ini_log_announce_itv);
	PR("Current Announce Interval %s            : %d\n",
	   "as a Logarithm to base 2", port_ds->cur_log_announce_itv);
	PR("Initial Sync Interval %s                : %d\n",
	   "as a Logarithm to base 2", port_ds->ini_log_half_sync_itv);
	PR("Current Sync Interval %s                : %d\n",
	   "as a Logarithm to base 2", port_ds->cur_log_half_sync_itv);
	PR("Initial Path Delay Request Interval %s  : %d\n",
	   "as a Logarithm to base 2", port_ds->ini_log_pdelay_req_itv);
	PR("Current Path Delay Request Interval %s  : %d\n",
	   "as a Logarithm to base 2", port_ds->cur_log_pdelay_req_itv);
	PR("Time without receiving announce %s %s  : %d ms (%d)\n",
	   "messages", "before running BMCA",
	   gptp_uscaled_ns_to_timer_ms(
		   &port_bmca_data->ann_rcpt_timeout_time_interval),
	   port_ds->announce_receipt_timeout);
	PR("Time without receiving sync %s %s      : %" PRIu64 " ms (%d)\n",
	   "messages", "before running BMCA",
	   (port_ds->sync_receipt_timeout_time_itv >> 16) /
					(NSEC_PER_SEC / MSEC_PER_SEC),
	   port_ds->sync_receipt_timeout);
	PR("Sync event %s                 : %" PRIu64 " ms\n",
	   "transmission interval for the port",
	   USCALED_NS_TO_NS(port_ds->half_sync_itv.low) /
					(NSEC_PER_USEC * USEC_PER_MSEC));
	PR("Path Delay Request %s         : %" PRIu64 " ms\n",
	   "transmission interval for the port",
	   USCALED_NS_TO_NS(port_ds->pdelay_req_itv.low) /
					(NSEC_PER_USEC * USEC_PER_MSEC));
	PR("BMCA %s %s%d%s: %d\n", "default", "priority", 1,
	   "                                        ",
	   domain->default_ds.priority1);
	PR("BMCA %s %s%d%s: %d\n", "default", "priority", 2,
	   "                                        ",
	   domain->default_ds.priority2);

	PR("\nRuntime status:\n");
	PR("Current global port state                          "
	   "      : %s\n", selected_role_str(port));
	PR("Path Delay Request state machine variables:\n");
	PR("\tCurrent state                                    "
	   ": %s\n", pdelay_req2str(port_state->pdelay_req.state));
	PR("\tInitial Path Delay Response Peer Timestamp       "
	   ": %" PRIu64 "\n", port_state->pdelay_req.ini_resp_evt_tstamp);
	PR("\tInitial Path Delay Response Ingress Timestamp    "
	   ": %" PRIu64 "\n", port_state->pdelay_req.ini_resp_ingress_tstamp);
	PR("\tPath Delay Response %s %s            : %u\n",
	   "messages", "received",
	   port_state->pdelay_req.rcvd_pdelay_resp);
	PR("\tPath Delay Follow Up %s %s           : %u\n",
	   "messages", "received",
	   port_state->pdelay_req.rcvd_pdelay_follow_up);
	PR("\tNumber of lost Path Delay Responses              "
	   ": %u\n", port_state->pdelay_req.lost_responses);
	PR("\tTimer expired send a new Path Delay Request      "
	   ": %u\n", port_state->pdelay_req.pdelay_timer_expired);
	PR("\tNeighborRateRatio has been computed successfully "
	   ": %u\n", port_state->pdelay_req.neighbor_rate_ratio_valid);
	PR("\tPath Delay has already been computed after init  "
	   ": %u\n", port_state->pdelay_req.init_pdelay_compute);
	PR("\tCount consecutive reqs with multiple responses   "
	   ": %u\n", port_state->pdelay_req.multiple_resp_count);

	PR("Path Delay Response state machine variables:\n");
	PR("\tCurrent state                                    "
	   ": %s\n", pdelay_resp2str(port_state->pdelay_resp.state));

	PR("SyncReceive state machine variables:\n");
	PR("\tCurrent state                                    "
	   ": %s\n", sync_rcv2str(port_state->sync_rcv.state));
	PR("\tA Sync %s %s                 : %s\n",
	   "Message", "has been received",
	   port_state->sync_rcv.rcvd_sync ? "yes" : "no");
	PR("\tA Follow Up %s %s            : %s\n",
	   "Message", "has been received",
	   port_state->sync_rcv.rcvd_follow_up ? "yes" : "no");
	PR("\tA Follow Up %s %s                      : %s\n",
	   "Message", "timeout",
	   port_state->sync_rcv.follow_up_timeout_expired ? "yes" : "no");
	PR("\tTime at which a Sync %s without Follow Up\n"
	   "\t                             will be discarded   "
	   ": %" PRIu64 "\n", "Message",
	   port_state->sync_rcv.follow_up_receipt_timeout);

	PR("SyncSend state machine variables:\n");
	PR("\tCurrent state                                    "
	   ": %s\n", sync_send2str(port_state->sync_send.state));
	PR("\tA MDSyncSend structure %s         : %s\n",
	   "has been received",
	   port_state->sync_send.rcvd_md_sync ? "yes" : "no");
	PR("\tThe timestamp for the sync msg %s : %s\n",
	   "has been received",
	   port_state->sync_send.md_sync_timestamp_avail ? "yes" : "no");

	PR("PortSyncSyncReceive state machine variables:\n");
	PR("\tCurrent state                                    "
	   ": %s\n", pss_rcv2str(port_state->pss_rcv.state));
	PR("\tGrand Master / Local Clock frequency ratio       "
	   ": %f\n", port_state->pss_rcv.rate_ratio);
	PR("\tA MDSyncReceive struct is ready to be processed  "
	   ": %s\n", port_state->pss_rcv.rcvd_md_sync ? "yes" : "no");
	PR("\tExpiry of SyncReceiptTimeoutTimer                : %s\n",
	   port_state->pss_rcv.rcv_sync_receipt_timeout_timer_expired ?
	   "yes" : "no");

	PR("PortSyncSyncSend state machine variables:\n");
	PR("\tCurrent state                                    "
	   ": %s\n", pss_send2str(port_state->pss_send.state));
	PR("\tFollow Up Correction Field of last recv PSS      "
	   ": %" PRId64 "\n",
	   port_state->pss_send.last_follow_up_correction_field);
	PR("\tUpstream Tx Time of the last recv PortSyncSync   "
	   ": %" PRIu64 "\n", port_state->pss_send.last_upstream_tx_time);
	PR("\tRate Ratio of the last received PortSyncSync     "
	   ": %f\n",
	   port_state->pss_send.last_rate_ratio);
	PR("\tGM Freq Change of the last received PortSyncSync "
	   ": %f\n", port_state->pss_send.last_gm_freq_change);
	PR("\tGM Time Base Indicator of last recv PortSyncSync "
	   ": %d\n", port_state->pss_send.last_gm_time_base_indicator);
	PR("\tReceived Port Number of last recv PortSyncSync   "
	   ": %d\n",
	   port_state->pss_send.last_rcvd_port_num);
	PR("\tPortSyncSync structure is ready to be processed  "
	   ": %s\n", port_state->pss_send.rcvd_pss_sync ? "yes" : "no");
	PR("\tFlag when the %s has expired    : %s\n",
	   "half_sync_itv_timer",
	   port_state->pss_send.half_sync_itv_timer_expired ? "yes" : "no");
	PR("\tHas %s expired twice            : %s\n",
	   "half_sync_itv_timer",
	   port_state->pss_send.sync_itv_timer_expired ? "yes" : "no");
	PR("\tHas syncReceiptTimeoutTime expired               "
	   ": %s\n",
	   port_state->pss_send.send_sync_receipt_timeout_timer_expired ?
	   "yes" : "no");

	PR("PortAnnounceReceive state machine variables:\n");
	PR("\tCurrent state                                    "
	   ": %s\n", pa_rcv2str(port_state->pa_rcv.state));
	PR("\tAn announce message is ready to be processed     "
	   ": %s\n",
	   port_state->pa_rcv.rcvd_announce ? "yes" : "no");

	PR("PortAnnounceInformation state machine variables:\n");
	PR("\tCurrent state                                    "
	   ": %s\n", pa_info2str(port_state->pa_info.state));
	PR("\tExpired announce information                     "
	   ": %s\n", port_state->pa_info.ann_expired ? "yes" : "no");

	PR("PortAnnounceTransmit state machine variables:\n");
	PR("\tCurrent state                                    "
	   ": %s\n", pa_transmit2str(port_state->pa_transmit.state));
	PR("\tTrigger announce information                     "
	   ": %s\n", port_state->pa_transmit.ann_trigger ? "yes" : "no");

#if defined(CONFIG_NET_GPTP_STATISTICS)
	PR("\nStatistics:\n");
	PR("Sync %s %s                 : %u\n",
	   "messages", "received", port_param_ds->rx_sync_count);
	PR("Follow Up %s %s            : %u\n",
	   "messages", "received", port_param_ds->rx_fup_count);
	PR("Path Delay Request %s %s   : %u\n",
	   "messages", "received", port_param_ds->rx_pdelay_req_count);
	PR("Path Delay Response %s %s  : %u\n",
	   "messages", "received", port_param_ds->rx_pdelay_resp_count);
	PR("Path Delay %s threshold %s : %u\n",
	   "messages", "exceeded",
	   port_param_ds->neighbor_prop_delay_exceeded);
	PR("Path Delay Follow Up %s %s : %u\n",
	   "messages", "received", port_param_ds->rx_pdelay_resp_fup_count);
	PR("Announce %s %s             : %u\n",
	   "messages", "received", port_param_ds->rx_announce_count);
	PR("ptp %s discarded                 : %u\n",
	   "messages", port_param_ds->rx_ptp_packet_discard_count);
	PR("Sync %s %s                 : %u\n",
	   "reception", "timeout",
	   port_param_ds->sync_receipt_timeout_count);
	PR("Announce %s %s             : %u\n",
	   "reception", "timeout",
	   port_param_ds->announce_receipt_timeout_count);
	PR("Path Delay Requests without a response "
	   ": %u\n",
	   port_param_ds->pdelay_allowed_lost_resp_exceed_count);
	PR("Sync %s %s                     : %u\n",
	   "messages", "sent", port_param_ds->tx_sync_count);
	PR("Follow Up %s %s                : %u\n",
	   "messages", "sent", port_param_ds->tx_fup_count);
	PR("Path Delay Request %s %s       : %u\n",
	   "messages", "sent", port_param_ds->tx_pdelay_req_count);
	PR("Path Delay Response %s %s      : %u\n",
	   "messages", "sent", port_param_ds->tx_pdelay_resp_count);
	PR("Path Delay Response FUP %s %s  : %u\n",
	   "messages", "sent", port_param_ds->tx_pdelay_resp_fup_count);
	PR("Announce %s %s                 : %u\n",
	   "messages", "sent", port_param_ds->tx_announce_count);
#endif /* CONFIG_NET_GPTP_STATISTICS */
}
#endif /* CONFIG_NET_GPTP */

static int cmd_net_gptp_port(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_GPTP)
	int arg = 1;
	char *endptr;
	int port;
#endif

#if defined(CONFIG_NET_GPTP)
	if (!argv[arg]) {
		PR_WARNING("Port number must be given.\n");
		return -ENOEXEC;
	}

	port = strtol(argv[arg], &endptr, 10);

	if (*endptr == '\0') {
		gptp_print_port_info(sh, port);
	} else {
		PR_WARNING("Not a valid gPTP port number: %s\n", argv[arg]);
	}
#else
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	PR_INFO("Set %s to enable %s support.\n", "CONFIG_NET_GPTP", "gPTP");
#endif

	return 0;
}

static int cmd_net_gptp(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_GPTP)
	/* gPTP status */
	struct gptp_domain *domain = gptp_get_domain();
	int count = 0;
	int arg = 1;
#endif

#if defined(CONFIG_NET_GPTP)
	if (argv[arg]) {
		cmd_net_gptp_port(sh, argc, argv);
	} else {
		struct net_shell_user_data user_data;

		user_data.sh = sh;
		user_data.user_data = &count;

		gptp_foreach_port(gptp_port_cb, &user_data);

		PR("\n");

		PR("SiteSyncSync state machine variables:\n");
		PR("\tCurrent state                  : %s\n",
		   site_sync2str(domain->state.site_ss.state));
		PR("\tA PortSyncSync struct is ready : %s\n",
		   domain->state.site_ss.rcvd_pss ? "yes" : "no");

		PR("ClockSlaveSync state machine variables:\n");
		PR("\tCurrent state                  : %s\n",
		   clk_slave2str(domain->state.clk_slave_sync.state));
		PR("\tA PortSyncSync struct is ready : %s\n",
		   domain->state.clk_slave_sync.rcvd_pss ? "yes" : "no");
		PR("\tThe local clock has expired    : %s\n",
		   domain->state.clk_slave_sync.rcvd_local_clk_tick ?
							   "yes" : "no");

		PR("PortRoleSelection state machine variables:\n");
		PR("\tCurrent state                  : %s\n",
		   pr_selection2str(domain->state.pr_sel.state));

		PR("ClockMasterSyncReceive state machine variables:\n");
		PR("\tCurrent state                  : %s\n",
		   cms_rcv2str(domain->state.clk_master_sync_receive.state));
		PR("\tA ClockSourceTime              : %s\n",
		   domain->state.clk_master_sync_receive.rcvd_clock_source_req
							       ? "yes" : "no");
		PR("\tThe local clock has expired    : %s\n",
		   domain->state.clk_master_sync_receive.rcvd_local_clock_tick
							       ? "yes" : "no");
	}
#else
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	PR_INFO("Set %s to enable %s support.\n", "CONFIG_NET_GPTP", "gPTP");
#endif

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_gptp,
	SHELL_CMD(port, NULL,
		  "'net gptp [<port>]' prints detailed information about "
		  "gPTP port.",
		  cmd_net_gptp_port),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), gptp, &net_cmd_gptp,
		 "Print information about gPTP support.",
		 cmd_net_gptp, 1, 1);
