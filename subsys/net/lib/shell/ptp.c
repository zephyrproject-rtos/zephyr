/*
 * Copyright (c) 2026 Philipp Steiner <philipp.steiner1987@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include <inttypes.h>
#include <stdlib.h>

#if defined(CONFIG_PTP)
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/drivers/ptp_clock.h>
#include "ptp/clock.h"
#include "ptp/port.h"
#endif

#include "net_shell_private.h"

#if defined(CONFIG_PTP)
static int64_t ptp_timeinterval_to_ns(ptp_timeinterval value)
{
	return value >> 16;
}

static const char *ptp_port_state2str(enum ptp_port_state state)
{
	switch (state) {
	case PTP_PS_INITIALIZING:
		return "INITIALIZING";
	case PTP_PS_FAULTY:
		return "FAULTY";
	case PTP_PS_DISABLED:
		return "DISABLED";
	case PTP_PS_LISTENING:
		return "LISTENING";
	case PTP_PS_PRE_TIME_TRANSMITTER:
		return "PRE_TIME_TRANSMITTER";
	case PTP_PS_TIME_TRANSMITTER:
		return "TIME_TRANSMITTER";
	case PTP_PS_GRAND_MASTER:
		return "GRAND_MASTER";
	case PTP_PS_PASSIVE:
		return "PASSIVE";
	case PTP_PS_UNCALIBRATED:
		return "UNCALIBRATED";
	case PTP_PS_TIME_RECEIVER:
		return "TIME_RECEIVER";
	default:
		return "<unknown>";
	}
}

static const char *ptp_clock_type2str(enum ptp_clock_type type)
{
	switch (type) {
	case PTP_CLOCK_TYPE_ORDINARY:
		return "ORDINARY";
	case PTP_CLOCK_TYPE_BOUNDARY:
		return "BOUNDARY";
	case PTP_CLOCK_TYPE_P2P:
		return "TRANSPARENT_P2P";
	case PTP_CLOCK_TYPE_E2E:
		return "TRANSPARENT_E2E";
	case PTP_CLOCK_TYPE_MANAGEMENT:
		return "MANAGEMENT";
	default:
		return "<unknown>";
	}
}

static const char *ptp_delay_mechanism2str(enum ptp_delay_mechanism mechanism)
{
	switch (mechanism) {
	case PTP_DM_E2E:
		return "E2E";
	case PTP_DM_P2P:
		return "P2P";
	case PTP_DM_COMMON_P2P:
		return "COMMON_P2P";
	case PTP_DM_SPECIAL:
		return "SPECIAL";
	case PTP_DM_NO_MECHANISM:
		return "NO_MECHANISM";
	default:
		return "<unknown>";
	}
}

static const char *ptp_time_src2str(uint8_t time_src)
{
	switch (time_src) {
	case PTP_TIME_SRC_ATOMIC_CLK:
		return "ATOMIC_CLOCK";
	case PTP_TIME_SRC_GNSS:
		return "GNSS";
	case PTP_TIME_SRC_TERRESTRIAL_RADIO:
		return "TERRESTRIAL_RADIO";
	case PTP_TIME_SRC_SERIAL_TIME_CODE:
		return "SERIAL_TIME_CODE";
	case PTP_TIME_SRC_PTP:
		return "PTP";
	case PTP_TIME_SRC_NTP:
		return "NTP";
	case PTP_TIME_SRC_HAND_SET:
		return "HAND_SET";
	case PTP_TIME_SRC_OTHER:
		return "OTHER";
	case PTP_TIME_SRC_INTERNAL_OSC:
		return "INTERNAL_OSCILLATOR";
	default:
		return "<unknown>";
	}
}

static const char *ptp_sync_status2str(const struct ptp_current_ds *cds)
{
	return cds->sync_uncertain ? "uncertain" : "stable";
}

static void ptp_sprint_clock_id(const ptp_clk_id *clock_id, char *buf, size_t len)
{
	snprintk(buf, len, "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X", clock_id->id[0],
		 clock_id->id[1], clock_id->id[2], clock_id->id[3], clock_id->id[4],
		 clock_id->id[5], clock_id->id[6], clock_id->id[7]);
}

static struct ptp_port *ptp_find_port(uint16_t port_id)
{
	struct ptp_port *port;

	SYS_SLIST_FOR_EACH_CONTAINER(ptp_clock_ports_list(), port, node) {
		if (port->port_ds.id.port_number == port_id) {
			return port;
		}
	}

	return NULL;
}

static bool ptp_read_hw_timestamp(struct net_if *iface, struct net_ptp_time *ts)
{
	const struct device *phc = net_eth_get_ptp_clock(iface);

	if (!phc) {
		return false;
	}

	return ptp_clock_get(phc, ts) == 0;
}

static void ptp_print_hw_timestamp(const struct shell *sh, struct net_if *iface, const char *name)
{
	struct net_ptp_time ts;

	if (ptp_read_hw_timestamp(iface, &ts)) {
		PR("%s: %" PRIu64 ".%09u\n", name, ts.second, ts.nanosecond);
	} else {
		PR("%s: unavailable\n", name);
	}
}

static void ptp_print_best_foreign(const struct shell *sh)
{
	const struct ptp_foreign_tt_clock *best = ptp_clock_best_time_transmitter();
	char sender_clock_id[sizeof("FF:FF:FF:FF:FF:FF:FF:FF")];
	char receiver_clock_id[sizeof("FF:FF:FF:FF:FF:FF:FF:FF")];

	if (!best) {
		PR("Best foreign transmitter : none\n");
		return;
	}

	ptp_sprint_clock_id(&best->dataset.sender.clk_id, sender_clock_id, sizeof(sender_clock_id));
	ptp_sprint_clock_id(&best->dataset.receiver.clk_id, receiver_clock_id,
			    sizeof(receiver_clock_id));

	PR("Best foreign transmitter : %s-%u (records=%u)\n", sender_clock_id,
	   best->dataset.sender.port_number, best->messages_count);
	PR("  Dataset                : p1=%u p2=%u steps_rm=%u receiver=%s-%u\n",
	   best->dataset.priority1, best->dataset.priority2, best->dataset.steps_rm,
	   receiver_clock_id, best->dataset.receiver.port_number);
}

static void ptp_print_port_overview(const struct shell *sh)
{
	struct ptp_port *port;

	PR("Port  Interface  State\n");
	SYS_SLIST_FOR_EACH_CONTAINER(ptp_clock_ports_list(), port, node) {
		PR("%4u  %9d  %s\n", port->port_ds.id.port_number, net_if_get_by_iface(port->iface),
		   ptp_port_state2str(ptp_port_state(port)));
	}
}

static void ptp_print_instance_info(const struct shell *sh)
{
	const struct ptp_default_ds *dds = ptp_clock_default_ds();
	const struct ptp_current_ds *cds = ptp_clock_current_ds();
	const struct ptp_parent_ds *pds = ptp_clock_parent_ds();
	const struct ptp_time_prop_ds *tpds = ptp_clock_time_prop_ds();
	char clock_id[sizeof("FF:FF:FF:FF:FF:FF:FF:FF")];
	char gm_id[sizeof("FF:FF:FF:FF:FF:FF:FF:FF")];
	char parent_port_clock_id[sizeof("FF:FF:FF:FF:FF:FF:FF:FF")];
	struct ptp_port *first_port = NULL;

	ptp_sprint_clock_id(&dds->clk_id, clock_id, sizeof(clock_id));
	ptp_sprint_clock_id(&pds->gm_id, gm_id, sizeof(gm_id));
	ptp_sprint_clock_id(&pds->port_id.clk_id, parent_port_clock_id,
			    sizeof(parent_port_clock_id));

	PR("PTP Instance:\n");
	PR("Clock ID      : %s\n", clock_id);
	PR("Clock Type    : %s\n", ptp_clock_type2str(ptp_clock_type()));
	PR("Domain        : %u\n", dds->domain);
	PR("Priorities    : %u / %u\n", dds->priority1, dds->priority2);
	PR("Time source   : %s (0x%02x)\n", ptp_time_src2str(tpds->time_src), tpds->time_src);
	PR("Ports         : %u\n", dds->n_ports);
	PR("TR only       : %s\n", dds->time_receiver_only ? "yes" : "no");

	SYS_SLIST_FOR_EACH_CONTAINER(ptp_clock_ports_list(), first_port, node) {
		break;
	}

	if (first_port) {
		ptp_print_hw_timestamp(sh, first_port->iface, "PHC now       ");
	} else {
		PR("PHC now       : unavailable\n");
	}

	PR("\nCurrent dataset:\n");
	PR("steps_rm      : %u\n", cds->steps_rm);
	PR("offset_from_tt: %" PRId64 " ns\n", ptp_timeinterval_to_ns(cds->offset_from_tt));
	PR("mean_delay    : %" PRId64 " ns\n", ptp_timeinterval_to_ns(cds->mean_delay));
	PR("sync status   : %s\n", ptp_sync_status2str(cds));

	PR("\nParent/GM dataset:\n");
	PR("grandmaster   : %s (p1=%u p2=%u class=%u acc=0x%02x)\n", gm_id, pds->gm_priority1,
	   pds->gm_priority2, pds->gm_clk_quality.cls, pds->gm_clk_quality.accuracy);
	PR("parent port   : %s-%u\n", parent_port_clock_id, pds->port_id.port_number);
	PR("UTC offset    : %d\n", tpds->current_utc_offset);

	PR("\n");
	ptp_print_best_foreign(sh);
	PR("\n");
	ptp_print_port_overview(sh);
}

static void ptp_print_port_info(const struct shell *sh, uint16_t port_id)
{
	struct ptp_port *port = ptp_find_port(port_id);
	struct ptp_dataset *best = NULL;
	char port_clock_id[sizeof("FF:FF:FF:FF:FF:FF:FF:FF")];
	char best_sender_id[sizeof("FF:FF:FF:FF:FF:FF:FF:FF")];

	if (!port) {
		PR_WARNING("PTP port %u was not found.\n", port_id);
		return;
	}

	ptp_sprint_clock_id(&port->port_ds.id.clk_id, port_clock_id, sizeof(port_clock_id));

	PR("PTP Port %u:\n", port->port_ds.id.port_number);
	PR("interface            : %d\n", net_if_get_by_iface(port->iface));
	PR("port identity        : %s-%u\n", port_clock_id, port->port_ds.id.port_number);

	PR("\nConfiguration:\n");
	PR("state                : %s\n", ptp_port_state2str(ptp_port_state(port)));
	PR("enabled              : %s\n", port->port_ds.enable ? "yes" : "no");
	PR("time transmitter only: %s\n", port->port_ds.time_transmitter_only ? "yes" : "no");
	PR("announce log itv     : %d\n", port->port_ds.log_announce_interval);
	PR("announce timeout     : %u\n", port->port_ds.announce_receipt_timeout);
	PR("sync log itv         : %d\n", port->port_ds.log_sync_interval);
	PR("min delay_req log itv: %d\n", port->port_ds.log_min_delay_req_interval);
	PR("delay mechanism      : %s\n", ptp_delay_mechanism2str(port->port_ds.delay_mechanism));
	PR("delay asymmetry      : %" PRId64 " ns\n",
	   ptp_timeinterval_to_ns(port->port_ds.delay_asymmetry));
	PR("mean link delay      : %" PRId64 " ns\n",
	   ptp_timeinterval_to_ns(port->port_ds.mean_link_delay));

	PR("\nRuntime:\n");
	PR("seq announce/delay/signaling/sync: %u / %u / %u / %u\n", port->seq_id.announce,
	   port->seq_id.delay, port->seq_id.signaling, port->seq_id.sync);
	PR("timeouts bits      : announce=%u delay=%u sync=%u qualification=%u\n",
	   atomic_test_bit(&port->timeouts, PTP_PORT_TIMER_ANNOUNCE_TO),
	   atomic_test_bit(&port->timeouts, PTP_PORT_TIMER_DELAY_TO),
	   atomic_test_bit(&port->timeouts, PTP_PORT_TIMER_SYNC_TO),
	   atomic_test_bit(&port->timeouts, PTP_PORT_TIMER_QUALIFICATION_TO));
	PR("timer remaining ms : announce=%d delay=%d sync=%d qualification=%d\n",
	   k_timer_remaining_get(&port->timers.announce),
	   k_timer_remaining_get(&port->timers.delay), k_timer_remaining_get(&port->timers.sync),
	   k_timer_remaining_get(&port->timers.qualification));
	ptp_print_hw_timestamp(sh, port->iface, "PHC now            ");

	best = ptp_port_best_foreign_ds(port);
	if (!best) {
		PR("best foreign       : none\n");
		return;
	}

	ptp_sprint_clock_id(&best->sender.clk_id, best_sender_id, sizeof(best_sender_id));
	PR("best foreign       : %s-%u (p1=%u p2=%u steps_rm=%u)\n", best_sender_id,
	   best->sender.port_number, best->priority1, best->priority2, best->steps_rm);
}
#endif /* CONFIG_PTP */

static int cmd_net_ptp_port(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_PTP)
	char *endptr = NULL;
	long port_id;

	ARG_UNUSED(argc);

	if (argv[1] == NULL) {
		PR_WARNING("Port number must be given.\n");
		return -ENOEXEC;
	}

	port_id = strtol(argv[1], &endptr, 10);
	if (*endptr != '\0' || port_id <= 0 || port_id > UINT16_MAX) {
		PR_WARNING("Invalid PTP port number: %s\n", argv[1]);
		return -EINVAL;
	}

	ptp_print_port_info(sh, (uint16_t)port_id);
#else
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	PR_INFO("Set %s to enable %s support.\n", "CONFIG_PTP", "PTP");
#endif

	return 0;
}

static int cmd_net_ptp(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_PTP)
	ARG_UNUSED(argc);

	if (argv[1] != NULL) {
		return cmd_net_ptp_port(sh, argc, argv);
	}

	ptp_print_instance_info(sh);
#else
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	PR_INFO("Set %s to enable %s support.\n", "CONFIG_PTP", "PTP");
#endif

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	net_cmd_ptp,
	SHELL_CMD(port, NULL, SHELL_HELP("Print detailed information about PTP port", "<port>"),
		  cmd_net_ptp_port),
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((net), ptp, &net_cmd_ptp, "Print information about PTP support.", cmd_net_ptp, 1,
		 1);
