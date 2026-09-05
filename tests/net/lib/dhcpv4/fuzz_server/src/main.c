/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Dev It Wise
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * libFuzzer harness for the DHCPv4 server's receive path.
 *
 * The server accepts datagrams from any client on the LAN with no
 * handshake, so every byte reaching dhcpv4_process_data() is
 * attacker-controlled. The harness registers a dummy iface, starts
 * the real server, and injects a full IPv4 + UDP + DHCP packet via
 * net_recv_data() - the same entry point a real driver uses - rather
 * than calling the decoder directly.
 *
 * The server parses messages on a dedicated socket-service thread
 * (subsys/net/lib/sockets/sockets_service.c) woken by zsock_poll().
 * Calling net_dhcpv4_server_start()/_stop() and net_recv_data()
 * straight from LLVMFuzzerTestOneInput() runs outside any Zephyr
 * thread context and hangs instead of progressing. So, as in
 * samples/subsys/debug/fuzz/src/main.c, LLVMFuzzerTestOneInput()
 * copies the case into fuzz_case/fuzz_sz, raises a "hardware"
 * interrupt (hw_irq_ctrl_set_irq()), and lets the native simulator
 * run for a fixed tick count (nsi_exec_for()). The ISR only signals a
 * semaphore; main() wakes up on its own thread, restarts the server
 * and runs the case.
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
#include <zephyr/net/dhcpv4_server.h>
#include <zephyr/net/dummy.h>
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

/* Fuzz case is the DHCP message body (struct dhcp_msg + SNAME/FILE +
 * cookie + options). Shorter than sizeof(struct dhcp_msg) is skipped,
 * not padded, since dhcpv4_process_data() requires at least that much
 * and a padded case would report a state no client can produce.
 */
#define FUZZ_MAX_INPUT 512

/* IRQ line LLVMFuzzerTestOneInput() raises to hand off a case. Native
 * sim exposes 32 lines (N_IRQS) and nothing in this config claims one,
 * so any line works; picked to match the in-tree fuzz sample.
 */
#define FUZZ_IRQ 31

/* Ticks the simulator runs per case: must cover the ISR hand-off,
 * server restart and net_recv_data() reaching the socket-service
 * thread via zsock_poll(). Every seed reaches dhcpv4_process_data()
 * within the debug-fuzz sample's default of 2 ticks; the larger value
 * gives a mutated case headroom, at a cost - roughly a fifth of the
 * throughput of 2 ticks, about half at 500.
 */
#define FUZZ_TICKS 100

static const struct net_in_addr server_addr = { { { 192, 0, 2, 1 } } };
static const struct net_in_addr netmask = { { { 255, 255, 255, 0 } } };
static const struct net_in_addr pool_base_addr = { { { 192, 0, 2, 10 } } };
static const struct net_in_addr client_addr = { { { 192, 0, 2, 99 } } };

#define SERVER_PORT 67
#define CLIENT_PORT 68

static uint8_t dhcpv4_fuzz_mac[6] = { 0x00, 0x00, 0x5e, 0x00, 0x53, 0x01 };

static int dhcpv4_fuzz_dev_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static void dhcpv4_fuzz_iface_init(struct net_if *iface)
{
	net_if_set_link_addr(iface, dhcpv4_fuzz_mac, sizeof(dhcpv4_fuzz_mac),
			     NET_LINK_ETHERNET);

	(void)net_if_ipv4_addr_add(iface, &server_addr, NET_ADDR_MANUAL, 0);
	(void)net_if_ipv4_set_netmask_by_addr(iface, &server_addr, &netmask);
}

static int dhcpv4_fuzz_send(const struct device *dev, struct net_pkt *pkt)
{
	/* The server's reply is not fuzz surface - it comes from server
	 * state, not attacker input - so nothing reads it.
	 */
	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);

	return 0;
}

static struct dummy_api dhcpv4_fuzz_if_api = {
	.iface_api.init = dhcpv4_fuzz_iface_init,
	.send = dhcpv4_fuzz_send,
};

NET_DEVICE_INIT(dhcpv4_server_fuzz, "dhcpv4_server_fuzz", dhcpv4_fuzz_dev_init, NULL, NULL, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &dhcpv4_fuzz_if_api, DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2), NET_IPV4_MTU);

/* Case handed off via "interrupt", run on main()'s thread. Copied
 * rather than pointed at: libFuzzer may reuse its buffer once
 * LLVMFuzzerTestOneInput() returns, which would attribute a crash to
 * the wrong input.
 */
static uint8_t fuzz_case[FUZZ_MAX_INPUT];
static size_t fuzz_sz;

K_SEM_DEFINE(fuzz_sem, 0, K_SEM_MAX_LIMIT);

static void fuzz_isr(const void *arg)
{
	ARG_UNUSED(arg);

	k_sem_give(&fuzz_sem);
}

static int run_one(struct net_if *iface, const uint8_t *data, size_t size)
{
	struct net_pkt *pkt;
	struct net_in_addr base_addr = pool_base_addr;
	int ret;

	/* Restart the server for every case so a lease reserved,
	 * allocated or declined by one case cannot steer how the next
	 * case's message is handled.
	 */
	(void)net_dhcpv4_server_stop(iface);

	ret = net_dhcpv4_server_start(iface, &base_addr);
	if (ret != 0) {
		return ret;
	}

	pkt = net_pkt_alloc_with_buffer(iface, size, NET_AF_INET, NET_IPPROTO_UDP, K_FOREVER);
	if (pkt == NULL) {
		return -ENOMEM;
	}

	net_pkt_set_ipv4_ttl(pkt, 0xFF);

	ret = net_ipv4_create(pkt, &client_addr, &server_addr);
	if (ret == 0) {
		ret = net_udp_create(pkt, net_htons(CLIENT_PORT), net_htons(SERVER_PORT));
	}

	if (ret == 0) {
		ret = net_pkt_write(pkt, data, size);
	}

	if (ret != 0) {
		net_pkt_unref(pkt);
		return ret;
	}

	net_pkt_cursor_init(pkt);
	net_ipv4_finalize(pkt, NET_IPPROTO_UDP);

	ret = net_recv_data(iface, pkt);
	if (ret != 0) {
		net_pkt_unref(pkt);
		return ret;
	}

	return 0;
}

int main(void)
{
	struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	int ret;

	__ASSERT_NO_MSG(iface != NULL);

	IRQ_CONNECT(FUZZ_IRQ, 0, fuzz_isr, NULL, 0);
	irq_enable(FUZZ_IRQ);

	while (true) {
		k_sem_take(&fuzz_sem, K_FOREVER);

		ret = run_one(iface, fuzz_case, fuzz_sz);
		if (ret != 0) {
			/* Case never reached the server - say so rather
			 * than report a clean run that didn't happen.
			 */
			printk("dhcpv4 fuzz case not delivered: %d\n", ret);
		}
	}

	return 0;
}

/**
 * Entry point for fuzzing: copies the case into known symbols,
 * triggers an interrupt, and lets the simulator run long enough to
 * reach a quiescent state.
 */
#if defined(CONFIG_BOARD_NATIVE_SIM)
NATIVE_SIMULATOR_IF /* We expose this function to the final runner link stage*/
#endif
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	static bool runner_initialized;

	if (!runner_initialized) {
		nsi_init(0, NULL);
		runner_initialized = true;
	}

	if (size < sizeof(struct dhcp_msg) || size > FUZZ_MAX_INPUT) {
		/* Skip rather than truncate/pad: the corpus must keep
		 * the case that was actually executed.
		 */
		return 0;
	}

	/* DMA-like copy into the symbols the ISR/thread hand-off uses. */
	memcpy(fuzz_case, data, size);
	fuzz_sz = size;

	hw_irq_ctrl_set_irq(FUZZ_IRQ);

	/* Let the sim process the interrupt and reach idle. */
	nsi_exec_for(k_ticks_to_us_ceil64(FUZZ_TICKS));

	return 0;
}
