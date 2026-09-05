/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Dev It Wise
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * libFuzzer harness for the DHCPv4 client's receive path.
 *
 * The harness registers a dummy interface the way
 * tests/net/dhcpv4/client/src/main.c does, starts the real client with
 * net_dhcpv4_start(), builds a full IPv4 + UDP + DHCP packet around the
 * fuzz case and hands it to the stack with net_recv_data() - the same
 * entry point a real driver uses for an inbound frame.
 *
 * The fuzz case is the DHCP message body (fixed header, SNAME, FILE,
 * cookie, options). xid and chaddr are visible to any on-LAN attacker in
 * the client's DISCOVER but are runtime values (entropy, MAC), so the
 * harness patches them into the case before injection - otherwise
 * dhcpv4.c's xid/chaddr check would drop nearly every case before the
 * option parser runs.
 *
 * The option decoder's plain and vendor-specific (option 43) callbacks
 * are also registered, so that decode surface is fuzzed too.
 *
 * The reply is decoded on a Zephyr thread, not in the caller's context
 * (net_dhcpv4_start() arms a delayable work item under a lock), so each
 * case is handed off via interrupt the way
 * samples/subsys/debug/fuzz/src/main.c does: stash the case, raise a
 * "hardware" interrupt, let the simulator run for a fixed tick count.
 *
 * See README.rst for how to build and run it.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/dhcpv4.h>
#include <zephyr/net/dummy.h>
#include <zephyr/sys/printk.h>
#include <zephyr/toolchain.h>

#include "ipv4.h"
#include "udp_internal.h"
#include "dhcpv4/dhcpv4_internal.h"

#if defined(CONFIG_BOARD_NATIVE_SIM)
#include <irq_ctrl.h>
#include <nsi_cpu_if.h>
#include <nsi_main_semipublic.h>
#else
#error "Platform not supported"
#endif

/* The fuzz case: DHCP body (struct dhcp_msg, SNAME, FILE, cookie,
 * options) appended after the IPv4/UDP headers. Anything shorter than
 * the fixed header is skipped, not padded - padding would report a
 * state no server can produce.
 */
#define FUZZ_MAX_INPUT 512

/* IRQ used to hand a case to main(). Native sim has 32 lines and none
 * are claimed here, so any is free; matches the fuzz sample's default.
 */
#define FUZZ_IRQ 31

/* Ticks the simulator runs per case. A case completes in one shot on
 * the woken thread, so this only covers the ISR hand-off; every seed
 * already reaches the parser at the debug-fuzz sample's default of 2.
 * The larger value is headroom for a mutated case, at ~25% throughput cost.
 */
#define FUZZ_TICKS 100

static const struct net_in_addr server_addr = { { { 192, 0, 2, 1 } } };
static const struct net_in_addr client_addr = { { { 255, 255, 255, 255 } } };

#define SERVER_PORT 67
#define CLIENT_PORT 68

static int dhcpv4_fuzz_dev_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static uint8_t dhcpv4_fuzz_mac[6] = { 0x00, 0x00, 0x5e, 0x00, 0x53, 0x01 };

static void dhcpv4_fuzz_iface_init(struct net_if *iface)
{
	net_if_set_link_addr(iface, dhcpv4_fuzz_mac, sizeof(dhcpv4_fuzz_mac), NET_LINK_ETHERNET);
}

static int dhcpv4_fuzz_send(const struct device *dev, struct net_pkt *pkt)
{
	/* Not part of the fuzz surface: the harness controls what is sent.
	 * Nothing answers it; the harness supplies the reply itself.
	 */
	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);

	return 0;
}

static struct dummy_api dhcpv4_fuzz_if_api = {
	.iface_api.init = dhcpv4_fuzz_iface_init,
	.send = dhcpv4_fuzz_send,
};

NET_DEVICE_INIT(dhcpv4_fuzz, "dhcpv4_fuzz", dhcpv4_fuzz_dev_init, NULL, NULL, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &dhcpv4_fuzz_if_api, DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2), 127);

/* Deliberately smaller than several seed option values, so a handler
 * must rely on the clamp below rather than a buffer that happens to fit.
 */
#define FUZZ_CB_BUF_SIZE 4

static uint8_t fuzz_plain_cb_buf[FUZZ_CB_BUF_SIZE];
static uint8_t fuzz_vendor_cb_buf[FUZZ_CB_BUF_SIZE];
static struct net_dhcpv4_option_callback fuzz_plain_cb;
static struct net_dhcpv4_option_callback fuzz_vendor_cb;

/* Routed by dhcpv4_parse_options() whenever option 15 (domain name) is
 * seen, ahead of - and independent from - the switch that decodes it
 * a second time.
 */
#define FUZZ_PLAIN_CB_OPTION DHCPV4_OPTIONS_DOMAIN_NAME

/* No seed's malformed vendor option (vendor_bad_inner_length in
 * seeds.txt) claims this inner code, so reaching this handler means the
 * well-formed path is being exercised.
 */
#define FUZZ_VENDOR_CB_OPTION 0x09

static void fuzz_option_cb(struct net_dhcpv4_option_callback *cb, size_t length,
			   enum net_dhcpv4_msg_type msg_type, struct net_if *iface)
{
	size_t n = MIN(cb->max_length, length);
	volatile uint8_t sink = 0;

	ARG_UNUSED(msg_type);
	ARG_UNUSED(iface);

	/* Handlers get the unclamped `length`, but only MIN(cb->max_length,
	 * length) bytes were written into cb->data - re-derive the clamp
	 * instead of trusting `length`.
	 */
	for (size_t i = 0; i < n; i++) {
		sink ^= ((uint8_t *)cb->data)[i];
	}
	(void)sink;
}

static void fuzz_register_option_callbacks(void)
{
	net_dhcpv4_init_option_callback(&fuzz_plain_cb, fuzz_option_cb, FUZZ_PLAIN_CB_OPTION,
					fuzz_plain_cb_buf, sizeof(fuzz_plain_cb_buf));
	net_dhcpv4_add_option_callback(&fuzz_plain_cb);

	net_dhcpv4_init_option_vendor_callback(&fuzz_vendor_cb, fuzz_option_cb,
					       FUZZ_VENDOR_CB_OPTION, fuzz_vendor_cb_buf,
					       sizeof(fuzz_vendor_cb_buf));
	net_dhcpv4_add_option_vendor_callback(&fuzz_vendor_cb);
}

/* Fuzz input, handed to main()'s thread via "interrupt" rather than
 * called directly from LLVMFuzzerTestOneInput().
 */
static const uint8_t *fuzz_buf;
static size_t fuzz_sz;

K_SEM_DEFINE(fuzz_sem, 0, K_SEM_MAX_LIMIT);

static void fuzz_isr(const void *arg)
{
	ARG_UNUSED(arg);

	k_sem_give(&fuzz_sem);
}

/* Patch xid and chaddr - the only fields dhcpv4.c checks before handing
 * off to the option parser. Both are visible to any on-LAN attacker in
 * the client's DISCOVER, so this costs the fuzzer nothing realistic.
 */
static void patch_header(uint8_t *body, size_t len, uint32_t xid, const uint8_t *chaddr,
			  uint8_t chaddr_len)
{
	struct dhcp_msg msg;

	__ASSERT_NO_MSG(len >= sizeof(msg));

	memcpy(&msg, body, sizeof(msg));
	msg.xid = net_htonl(xid);
	memcpy(msg.chaddr, chaddr, chaddr_len);
	memcpy(body, &msg, sizeof(msg));
}

static void run_one(struct net_if *iface, const uint8_t *data, size_t size)
{
	uint8_t body[FUZZ_MAX_INPUT];
	struct net_pkt *pkt;
	uint32_t xid;

	net_dhcpv4_start(iface);

	xid = iface->config.dhcpv4.xid;

	memcpy(body, data, size);
	patch_header(body, size, xid, net_if_get_link_addr(iface)->addr,
		     net_if_get_link_addr(iface)->len);

	/* Both checks below are unreachable by fuzz input (bounded by
	 * FUZZ_MAX_INPUT, pool sized for it); reaching one means the
	 * harness itself is broken. Panic so the failure is loud rather
	 * than silently under-reporting coverage.
	 */
	pkt = net_pkt_alloc_with_buffer(iface, size, NET_AF_INET, NET_IPPROTO_UDP, K_FOREVER);
	if (pkt == NULL) {
		printk("dhcpv4 fuzz harness is broken: net_pkt_alloc_with_buffer() failed "
		       "(not a DHCPv4 defect)\n");
		k_panic();
	}

	net_pkt_set_ipv4_ttl(pkt, 0xFF);

	if (net_ipv4_create(pkt, &server_addr, &client_addr) ||
	    net_udp_create(pkt, net_htons(SERVER_PORT), net_htons(CLIENT_PORT)) ||
	    net_pkt_write(pkt, body, size)) {
		net_pkt_unref(pkt);
		printk("dhcpv4 fuzz harness is broken: packet build failed "
		       "(not a DHCPv4 defect)\n");
		k_panic();
	}

	net_pkt_cursor_init(pkt);
	net_ipv4_finalize(pkt, NET_IPPROTO_UDP);

	/* Unlike the panics above, net_recv_data() failing is the stack
	 * rejecting the packet - exactly what this harness is fuzzing for.
	 */
	if (net_recv_data(iface, pkt)) {
		net_pkt_unref(pkt);
	}

	/* Reset before the next case: leftover lease/requested-IP state
	 * would steer the next net_dhcpv4_start() into a different state.
	 */
	net_dhcpv4_stop(iface);
	iface->config.dhcpv4.requested_ip.s_addr = 0;
}

int main(void)
{
	struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));

	__ASSERT_NO_MSG(iface != NULL);

	fuzz_register_option_callbacks();

	IRQ_CONNECT(FUZZ_IRQ, 0, fuzz_isr, NULL, 0);
	irq_enable(FUZZ_IRQ);

	while (true) {
		k_sem_take(&fuzz_sem, K_FOREVER);

		/* Run the case handed off via interrupt. */
		run_one(iface, fuzz_buf, fuzz_sz);
	}

	return 0;
}

/**
 * Entry point for fuzzing: place data into known symbols, trigger an
 * interrupt, then run the simulator long enough to reach quiescence.
 */
#if defined(CONFIG_BOARD_NATIVE_SIM)
NATIVE_SIMULATOR_IF /* We expose this function to the final runner link stage */
#endif
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	static bool runner_initialized;

	if (!runner_initialized) {
		nsi_init(0, NULL);
		runner_initialized = true;
	}

	if (size < sizeof(struct dhcp_msg) || size > FUZZ_MAX_INPUT) {
		/* Skip rather than truncate/pad: keeps a kept case
		 * identical to what ran, and the patched header intact.
		 */
		return 0;
	}

	/* Hand the fuzz data to the embedded OS as an interrupt. */
	fuzz_buf = data;
	fuzz_sz = size;

	hw_irq_ctrl_set_irq(FUZZ_IRQ);

	/* Let the OS process the interrupt and reach idle. */
	nsi_exec_for(k_ticks_to_us_ceil64(FUZZ_TICKS));

	return 0;
}
