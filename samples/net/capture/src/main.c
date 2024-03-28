/*
 * Copyright (c) 2021 Intel Corporation.
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_capture_sample, LOG_LEVEL_DBG);

#include <zephyr/posix/net/if_arp.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/net/capture.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/virtual_mgmt.h>

#if defined(CONFIG_NET_CAPTURE_COOKED_MODE)
#define COOKED_MODE_INTERFACE_NAME CONFIG_NET_CAPTURE_COOKED_MODE_INTERFACE_NAME
#else
#define COOKED_MODE_INTERFACE_NAME ""
#endif

static bool started;

uint16_t link_types_to_monitor[] = {
	NET_ETH_PTYPE_CAN,
	NET_ETH_PTYPE_HDLC,
};

struct data_to_send {
	struct net_capture_cooked *ctx;
	size_t len;
	const char *data;
	enum net_capture_packet_type type;
	uint16_t eth_p_type;
};

#define DATA(_ctx, _data, _type, _ptype) {	\
		.ctx = _ctx,			\
		.len = sizeof(_data),		\
		.data = _data,			\
		.type = _type,			\
		.eth_p_type = _ptype		\
	}

static const char can_data[] = {
	0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
};

static struct net_capture_cooked ctx_can = {
	.hatype = ARPHRD_CAN,
	.halen = 0,
	.addr = { 0 }
};

static const char ppp_send_lcp_conf_req_data[] = {
	0xff, 0x7d, 0x23, 0xc0, 0x21, 0x7d, 0x21, 0x7d,
	0x21, 0x7d, 0x20, 0x7d, 0x24, 0xd1, 0xb5,
};

static const char ppp_recv_lcp_conf_req_data[] = {
	0xff, 0x03, 0xc0, 0x21, 0x01, 0x01, 0x00, 0x16,
	0x02, 0x06, 0x00, 0x00, 0x00, 0x00, 0x03, 0x04,
	0xc0, 0x23, 0x05, 0x06, 0xe4, 0xdd, 0x30, 0x57,
	0x07, 0x02, 0x0c, 0x57,
};

static const char ppp_send_lcp_conf_rej_data[] = {
	0xff, 0x7d, 0x23, 0xc0, 0x21, 0x7d, 0x24, 0x7d,
	0x21, 0x7d, 0x20, 0x7d, 0x32, 0x7d, 0x22, 0x7d,
	0x26, 0x7d, 0x20, 0x7d, 0x20, 0x7d, 0x20, 0x7d,
	0x20, 0x7d, 0x25, 0x7d, 0x26, 0x7d, 0x20, 0x38,
	0xea, 0x74, 0x7d, 0x27, 0x7d, 0x22, 0x8c, 0xa3,
};

static struct net_capture_cooked ctx_ppp = {
	.hatype = ARPHRD_PPP,
	.halen = 6,
	.addr = { 0x1, 0x2, 0x3, 0x4, 0x5, 0x6 }
};

/* We just construct some demo packets and send them out */
static const struct data_to_send data[] = {
	DATA(&ctx_can, can_data, NET_CAPTURE_OUTGOING, NET_ETH_PTYPE_CAN),
	DATA(&ctx_ppp, ppp_send_lcp_conf_req_data, NET_CAPTURE_OUTGOING, NET_ETH_PTYPE_HDLC),
	DATA(&ctx_ppp, ppp_recv_lcp_conf_req_data, NET_CAPTURE_HOST, NET_ETH_PTYPE_HDLC),
	DATA(&ctx_ppp, ppp_send_lcp_conf_rej_data, NET_CAPTURE_OUTGOING, NET_ETH_PTYPE_HDLC),
};

static int cmd_sample_send(const struct shell *sh,
			   size_t argc, char *argv[])
{
	if (!IS_ENABLED(CONFIG_NET_CAPTURE_COOKED_MODE)) {
		shell_fprintf(sh, SHELL_NORMAL,
			      "Enable %s to use the sample shell.\n",
			      "CONFIG_NET_CAPTURE_COOKED_MODE");
		return 0;
	}

	if (!started) {
		if (sh != NULL) {
			shell_fprintf(sh, SHELL_WARNING, "%s",
				      "Capturing not enabled, cannot send data.\n");
		}

		return 0;
	}

	ARRAY_FOR_EACH_PTR(data, ptr) {
		net_capture_data(ptr->ctx, ptr->data, ptr->len,
				 ptr->type, ptr->eth_p_type);
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sample_commands,
	SHELL_CMD(send, NULL,
		  "Send example data\n",
		  cmd_sample_send),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(sample, &sample_commands,
		   "Sample application commands", NULL);

static int init_app(void)
{
	/* What this sample does:
	 *  - Create a tunnel that runs on top of the Ethernet interface
	 *  - Start to capture data from cooked mode capture interface
	 *  - Take the cooked mode interface up
	 *
	 * All of the above could be done manually from net-shell with
	 * these commands:
	 *
	 *   net capture setup 192.0.2.2 2001:db8:200::1 2001:db8:200::2
	 *   net capture enable 2
	 *   net iface up 2
	 *
	 * Explanation what those commands do:
	 *
	 * The "net capture setup" creates a tunnel. The tunnel is IPv6
	 * tunnel and our inner end point address is 2001:db8:200::1
	 * and the inner peer end point address is 2001:db8:200::2. In the
	 * tests, the tunnel can be created in Linux host with this command
	 *
	 *   net-setup.sh -c zeth-tunnel.conf
	 *
	 * The net-setup.sh command is found in net-tools Zephyr project.
	 * In host side, the tunnel interface is called zeth-ip6ip where IPv6
	 * packets are run in a IPv4 tunnel.
	 *
	 * The network interfaces in this sample application are:
	 *
	 *   Interface any (0x808ab3c) (Dummy) [1]
	 *   ================================
	 *   Virtual interfaces attached to this : 2
	 *   Device    : NET_ANY (0x80849a4)
	 *
	 *   Interface cooked (0x808ac94) (Virtual) [2]
	 *   ==================================
	 *   Virtual name : Cooked mode capture
	 *   Attached  : 1 (Dummy / 0x808ab3c)
	 *   Device    : NET_COOKED (0x808497c)
	 *
	 *   Interface eth0 (0x808adec) (Ethernet) [3]
	 *   ===================================
	 *   Virtual interfaces attached to this : 4
	 *   Device    : zeth0 (0x80849b8)
	 *   IPv6 unicast addresses (max 4):
	 *        fe80::5eff:fe00:53e6 autoconf preferred infinite
	 *        2001:db8::1 manual preferred infinite
	 *   IPv4 unicast addresses (max 2):
	 *        192.0.2.1/255.255.255.0 overridable preferred infinite
	 *
	 *   Interface net0 (0x808af44) (Virtual) [4]
	 *   ==================================
	 *   Virtual name : Capture tunnel
	 *   Attached  : 3 (Ethernet / 0x808adec)
	 *   Device    : IP_TUNNEL0 (0x8084990)
	 *   IPv6 unicast addresses (max 4):
	 *        2001:db8:200::1 manual preferred infinite
	 *        fe80::efed:6dff:fef2:b1df autoconf preferred infinite
	 *        fe80::56da:1eff:fe5e:bc02 autoconf preferred infinite
	 *
	 * The 192.0.2.2 is the address of the outer end point of the host that
	 * terminates the tunnel. Zephyr uses this address to select the internal
	 * interface to use for the tunnel. In this example it is interface 3.
	 *
	 * The interface 2 is the interface that runs on top of interface 1. The
	 * cooked capture packets are written by the capture API to sink interface 1.
	 * The packets propagate to interface 2 because it is linked to first interface.
	 * The "net capture enable 2" command will cause the packets sent to interface 2
	 * to be written to capture interface 4, which in turn capsulates the packets and
	 * tunnels them to peer via the Ethernet interface 3.
	 *
	 * The above IP addresses might change if you change the addresses in
	 * overlay-tunnel.conf file.
	 *
	 * If the cooked mode capture is enabled (CONFIG_NET_CAPTURE_COOKED_MODE=y),
	 * then we setup the capture automatically to the correct network interface.
	 * User is then able to send sample network packets in cooked mode and monitor
	 * the captured packets in the host zeth-ip6ip network interface. Use "sample send"
	 * command to do that.
	 *
	 * You can use "net-capture.py -i zeth-ip6ip -c" command to capture the cooked
	 * packets in host side. The net-capture.py tool is found in net-tools package.
	 */

	const char remote[] = CONFIG_NET_CONFIG_PEER_IPV4_ADDR;
	const char local[] = CONFIG_NET_SAMPLE_TUNNEL_MY_ADDR;
	const char peer[] = CONFIG_NET_SAMPLE_TUNNEL_PEER_ADDR;
	const struct device *capture_dev;
	int ret;

	ret = net_capture_setup(remote, local, peer, &capture_dev);
	if (ret < 0) {
		LOG_ERR("Capture cannot be setup (%d)", ret);
		return -ENOEXEC;
	}

	if (IS_ENABLED(CONFIG_NET_CAPTURE_COOKED_MODE)) {
		/* If we are running in cooked mode, start to capture the packets
		 * from cooked mode interface.
		 */
		struct virtual_interface_req_params params = { 0 };
		struct virtual_interface_link_types link_types;
		int ifindex;

		ifindex = net_if_get_by_name(COOKED_MODE_INTERFACE_NAME);
		if (ifindex < 0) {
			LOG_ERR("Interface \"%s\" not found.", COOKED_MODE_INTERFACE_NAME);
			return -ENOENT;
		}

		ret = net_capture_enable(capture_dev, net_if_get_by_index(ifindex));
		if (ret < 0) {
			LOG_ERR("Cannot enable capture to interface %d (%d)",
				ifindex, ret);
			return ret;
		}

		/* Setup the cooked interface to capture these types of packets */
		memcpy(&link_types.type, &link_types_to_monitor,
		       MIN(sizeof(link_types.type), sizeof(link_types_to_monitor)));
		link_types.count = MIN(ARRAY_SIZE(link_types.type),
				       ARRAY_SIZE(link_types_to_monitor));

		params.family = AF_UNSPEC;
		memcpy(&params.link_types, &link_types,
		       sizeof(struct virtual_interface_link_types));

		ret = net_mgmt(NET_REQUEST_VIRTUAL_INTERFACE_SET_LINK_TYPE,
			       net_if_get_by_index(ifindex), &params,
			       sizeof(struct virtual_interface_req_params));
		if (ret < 0 && ret != -ENOTSUP) {
			LOG_ERR("Cannot set interface %d link types (%d)",
				ifindex, ret);
			return ret;
		}

		/* Now the capture device and the tunnel interface is setup,
		 * we just need to bring up the actual interface we want to capture
		 * as it is down by default.
		 */
		ret = net_if_up(net_if_get_by_index(ifindex));
		if (ret < 0) {
			LOG_ERR("Cannot take up interface %d (%d)", ifindex, ret);
			return ret;
		}

		LOG_INF("Type \"sample send\" to send dummy capture data to tunnel.");
	} else {
		LOG_INF("Please enable capture manually from net-shell");
		LOG_INF("Use \"net capture enable <ifindex>\" command to start "
			"capturing desired network interface.");
	}

	return 0;
}

#define EVENT_MASK (NET_EVENT_CAPTURE_STARTED | NET_EVENT_CAPTURE_STOPPED)

static void event_handler(struct net_mgmt_event_callback *cb,
			  uint32_t mgmt_event, struct net_if *iface)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(cb);

	if ((mgmt_event & EVENT_MASK) != mgmt_event) {
		return;
	}

	if (mgmt_event == NET_EVENT_CAPTURE_STARTED) {
		started = true;
	}

	if (mgmt_event == NET_EVENT_CAPTURE_STOPPED) {
		started = false;
	}
}

int main(void)
{
	static struct net_mgmt_event_callback mgmt_cb;
	struct net_if *iface;
	uint32_t event;
	int ret;

	LOG_INF("Starting network capture sample");

	net_mgmt_init_event_callback(&mgmt_cb, event_handler, EVENT_MASK);
	net_mgmt_add_event_callback(&mgmt_cb);

	ret = init_app();
	if (ret < 0) {
		LOG_ERR("Cannot start the application.");
		return ret;
	}

	while (1) {
		ret = net_mgmt_event_wait(EVENT_MASK,
					  &event,
					  &iface,
					  NULL,
					  NULL,
					  K_FOREVER);
		if (ret < 0) {
			continue;
		}

		LOG_INF("Capturing %s on interface %d",
			event == NET_EVENT_CAPTURE_STARTED ? "started" : "stopped",
			net_if_get_by_iface(iface));
	}

	return 0;
}
