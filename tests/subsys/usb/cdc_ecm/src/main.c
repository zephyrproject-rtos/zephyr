/*
 * SPDX-FileCopyrightText: Copyright 2025 - 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sample_usbd.h>
#include <stdlib.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/icmp.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/random/random.h>
#include <zephyr/usb/class/usb_cdc.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/ztest.h>

#include "icmpv4.h"
#include "usbh_ch9.h"
#include "usbh_device.h"

#define USBD_CDC_ECM_DT_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_cdc_ecm_ethernet)

#define NET_IFACE_CARRIER_WAIT_INTERVAL K_MSEC(1)

#define ICMP_PING_DATA_PREFIX        "USB CDC-ECM ZTEST "
#define ICMP_PING_DATA_PREFIX_LEN    (sizeof(ICMP_PING_DATA_PREFIX) - 1)
#define ICMP_PING_DATA_MIN_LEN       (ICMP_PING_DATA_PREFIX_LEN + 8)
#define ICMP_PING_DATA_MAX_LEN       128
#define ICMP_PING_DATA_DELTA_MIN_LEN 60
#define ICMP_PING_REPLY_TIMEOUT      K_MSEC(500)

LOG_MODULE_REGISTER(cdc_ecm_test, LOG_LEVEL_INF);

USBH_CONTROLLER_DEFINE(uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));

static struct usbd_context *test_usbd;
static struct usb_device *test_udev;

static struct net_if *host_iface;
static struct net_if *device_iface;

static struct net_in_addr iface_netmask = {.s4_addr = {255, 255, 255, 252}};
static struct net_in_addr host_addr = {.s4_addr = {192, 168, 255, 253}};
static struct net_in_addr device_addr = {.s4_addr = {192, 168, 255, 254}};

static char host_tx_icmp_data[ICMP_PING_DATA_MAX_LEN + 1];
static char device_tx_icmp_data[ICMP_PING_DATA_MAX_LEN + 1];

static struct test_icmp_context {
	struct net_if *iface;
	struct net_in_addr src_addr;
	uint16_t id;
	uint16_t seq;
	int data_len;
	char data[ICMP_PING_DATA_MAX_LEN + 1];
	struct k_sem sem;
} host_icmp_ctx, device_icmp_ctx;

static void cdc_ecm_get_eth_iface(void)
{
	const struct device *usbh_cdc_ecm_dev =
		device_get_binding(CONFIG_USBH_CDC_ECM_ETH_DRV_NAME "0");
	const struct device *usbd_cdc_ecm_dev = DEVICE_DT_GET(USBD_CDC_ECM_DT_NODE);

	zassert_not_null(usbh_cdc_ecm_dev, "Failed to find usbh_cdc_ecm device");
	zassert_true(device_is_ready(usbd_cdc_ecm_dev), "CDC-ECM device not ready");

	device_iface = net_if_lookup_by_dev(usbd_cdc_ecm_dev);
	zassert_not_null(device_iface, "Failed to get device-side netif");

	host_iface = net_if_lookup_by_dev(usbh_cdc_ecm_dev);
	zassert_not_null(host_iface, "Failed to get host-side netif");
}

static bool cdc_ecm_wait_netif_carrier(struct net_if *iface, bool expected_carrier,
				       k_timeout_t timeout)
{
	k_timepoint_t end = sys_timepoint_calc(timeout);

	do {
		if (net_if_is_carrier_ok(iface) == expected_carrier) {
			return true;
		}

		if (sys_timepoint_expired(end)) {
			break;
		}

		k_sleep(NET_IFACE_CARRIER_WAIT_INTERVAL);
	} while (true);

	return false;
}

static void *cdc_ecm_test_setup(void)
{
	int ret;

	cdc_ecm_get_eth_iface();

	zassert_ok(usbh_init(&uhs_ctx), "Failed to initialize USB host");
	zassert_ok(usbh_enable(&uhs_ctx), "Failed to enable USB host");
	zassert_ok(uhc_bus_reset(uhs_ctx.dev), "Failed to signal bus reset");
	zassert_ok(uhc_bus_resume(uhs_ctx.dev), "Failed to signal bus resume");
	zassert_ok(uhc_sof_enable(uhs_ctx.dev), "Failed to enable SoF generator");

	zassert_not_null(host_iface, "Invalid host-side netif");
	ret = net_if_up(host_iface);
	zassert_true(ret == 0 || ret == -EALREADY, "Failed to bring up host-side netif");

	LOG_INF("Host controller enabled");

	test_usbd = sample_usbd_init_device(NULL);
	zassert_not_null(test_usbd, "Failed to setup USB device");
	zassert_ok(usbd_enable(test_usbd), "Failed to enable device support");

	zassert_not_null(device_iface, "Invalid device-side netif");
	ret = net_if_up(device_iface);
	zassert_true(ret == 0 || ret == -EALREADY, "Failed to bring up device-side netif");

	LOG_INF("Device support enabled");

	/* Allow the host time to reset the device. */
	k_msleep(200);

	zassert_true(cdc_ecm_wait_netif_carrier(device_iface, true, K_MSEC(200)),
		     "Timeout waiting for device-side carrier ok (expected after net_if_up)");
	zassert_true(cdc_ecm_wait_netif_carrier(host_iface, true, K_MSEC(500)),
		     "Timeout waiting for host-side carrier ok (expected after USB enumeration)");

	test_udev = usbh_device_get_any(&uhs_ctx);
	zassert_not_null(test_udev, "Failed to get USB device after enumeration");

	zassert_not_null(net_if_ipv4_addr_add(host_iface, &host_addr, NET_ADDR_MANUAL, 0),
			 "Failed to add IPv4 address to host-side netif");
	zassert_true(net_if_ipv4_set_netmask_by_addr(host_iface, &host_addr, &iface_netmask),
		     "Failed to set netmask of host-side netif");
	zassert_not_null(net_if_ipv4_addr_add(device_iface, &device_addr, NET_ADDR_MANUAL, 0),
			 "Failed to add IPv4 address to device-side netif");
	zassert_true(net_if_ipv4_set_netmask_by_addr(device_iface, &device_addr, &iface_netmask),
		     "Failed to set netmask of device-side netif");

	return NULL;
}

static void cdc_ecm_test_before(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_not_null(test_udev, "Invalid USB device");

	zassert_not_null(host_iface, "Invalid host-side netif");
	zassert_not_null(device_iface, "Invalid device-side netif");

	zassert_true(net_if_is_up(device_iface), "Device-side netif should be up initially");
	zassert_true(net_if_is_up(host_iface), "Host-side netif should be up initially");

	zassert_true(net_if_is_carrier_ok(device_iface),
		     "Device-side netif carrier should be ok initially");
	zassert_true(net_if_is_carrier_ok(host_iface),
		     "Host-side netif carrier should be ok initially");
}

static void cdc_ecm_test_teardown(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_not_null(host_iface, "Invalid host-side netif");
	zassert_not_null(device_iface, "Invalid device-side netif");

	zassert_ok(net_if_down(host_iface), "Failed to bring down host-side netif");
	zassert_ok(net_if_down(device_iface), "Failed to bring down device-side netif");

	zassert_ok(usbd_disable(test_usbd), "Failed to disable device support");
	zassert_ok(usbd_shutdown(test_usbd), "Failed to shutdown device support");

	LOG_INF("Device support disabled");

	zassert_ok(usbh_disable(&uhs_ctx), "Failed to disable host controller");
	zassert_ok(usbh_shutdown(&uhs_ctx), "Failed to shutdown host controller");

	LOG_INF("Host controller disabled");
}

static void generate_rand_icmp_ping_data(char *data, size_t *len)
{
	static const char charset[] =
		"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	static size_t last_len;
	uint32_t rand_val;
	size_t range = ICMP_PING_DATA_MAX_LEN - ICMP_PING_DATA_MIN_LEN + 1;
	size_t rand_part_len;
	char *rand_part;

	do {
		sys_rand_get(&rand_val, sizeof(rand_val));
		*len = ICMP_PING_DATA_MIN_LEN + (rand_val % range);
	} while (abs((int)*len - (int)last_len) < ICMP_PING_DATA_DELTA_MIN_LEN);

	last_len = *len;

	memcpy(data, ICMP_PING_DATA_PREFIX, ICMP_PING_DATA_PREFIX_LEN);

	rand_part = data + ICMP_PING_DATA_PREFIX_LEN;
	rand_part_len = *len - ICMP_PING_DATA_PREFIX_LEN;

	sys_rand_get(rand_part, rand_part_len);
	for (size_t i = 0; i < rand_part_len; i++) {
		rand_part[i] = charset[(uint8_t)rand_part[i] % (sizeof(charset) - 1)];
	}

	data[*len] = '\0';
}

static enum net_verdict icmp_handler(struct net_icmp_ctx *ctx, struct net_pkt *pkt,
				     struct net_icmp_ip_hdr *ip_hdr, struct net_icmp_hdr *icmp_hdr,
				     void *user_data)
{
	size_t skip_len =
		net_pkt_ip_hdr_len(pkt) + net_pkt_ip_opts_len(pkt) + sizeof(struct net_icmp_hdr);
	struct net_icmpv4_echo_req echo_req;
	struct test_icmp_context *icmp_ctx = user_data;
	enum net_verdict verdict = NET_OK;

	ARG_UNUSED(ctx);
	ARG_UNUSED(icmp_hdr);

	net_pkt_set_overwrite(pkt, true);
	net_pkt_cursor_init(pkt);

	if (net_pkt_skip(pkt, skip_len) != 0) {
		verdict = NET_DROP;
		goto done;
	}

	if (net_pkt_read(pkt, &echo_req, sizeof(echo_req)) != 0) {
		verdict = NET_DROP;
		goto done;
	}

	icmp_ctx->iface = net_pkt_iface(pkt);
	memcpy(icmp_ctx->src_addr.s4_addr, ip_hdr->ipv4->src, sizeof(ip_hdr->ipv4->src));
	icmp_ctx->id = net_ntohs(echo_req.identifier);
	icmp_ctx->seq = net_ntohs(echo_req.sequence);
	icmp_ctx->data_len = net_pkt_remaining_data(pkt);

	if (icmp_ctx->data_len > sizeof(icmp_ctx->data)) {
		verdict = NET_DROP;
		goto done;
	}

	memset(icmp_ctx->data, 0, sizeof(icmp_ctx->data));
	if (net_pkt_read(pkt, icmp_ctx->data, icmp_ctx->data_len) != 0) {
		verdict = NET_DROP;
		goto done;
	}

done:
	k_sem_give(&icmp_ctx->sem);
	return verdict;
}

ZTEST(cdc_ecm, test_get_mac_address)
{
	struct net_linkaddr *ll_addr;
	uint8_t desc_mac_addr[NET_ETH_ADDR_LEN];
	const char *desc_mac_str;

	ll_addr = net_if_get_link_addr(host_iface);
	zassert_not_null(ll_addr, "Failed to get link addr from host-side netif");

	desc_mac_str = DT_PROP(USBD_CDC_ECM_DT_NODE, remote_mac_address);
	zassert_equal(
		hex2bin(desc_mac_str, strlen(desc_mac_str), desc_mac_addr, sizeof(desc_mac_addr)),
		NET_ETH_ADDR_LEN, "Failed to convert MAC address from device");

	zassert_mem_equal(ll_addr->addr, desc_mac_addr, NET_ETH_ADDR_LEN,
			  "MAC address mismatch!\n"
			  "\thost actual:       %02X%02X%02X%02X%02X%02X\n"
			  "\texpected (remote): %s",
			  ll_addr->addr[0], ll_addr->addr[1], ll_addr->addr[2], ll_addr->addr[3],
			  ll_addr->addr[4], ll_addr->addr[5], desc_mac_str);
}

ZTEST(cdc_ecm, test_cdc_notification)
{
	/* Test NetworkConnection notification: Disconnect */
	zassert_ok(net_if_down(device_iface), "Failed to bring down device-side netif");
	zassert_true(cdc_ecm_wait_netif_carrier(host_iface, false, K_MSEC(200)),
		     "Host-side netif should detect carrier down after device sends 'Disconnect'");

	/* Test NetworkConnection notification: Connected */
	zassert_ok(net_if_up(device_iface), "Failed to bring up device-side netif");
	zassert_true(cdc_ecm_wait_netif_carrier(host_iface, true, K_MSEC(200)),
		     "Host-side netif should detect carrier up after device sends 'Connected'");
}

ZTEST(cdc_ecm, test_packet_filter)
{
	const uint8_t bmRequestType = (USB_REQTYPE_DIR_TO_DEVICE << 7) |
				      (USB_REQTYPE_TYPE_CLASS << 5) |
				      USB_REQTYPE_RECIPIENT_INTERFACE;
	const uint16_t wValue = PACKET_TYPE_MULTICAST | PACKET_TYPE_BROADCAST |
				PACKET_TYPE_DIRECTED | PACKET_TYPE_ALL_MULTICAST |
				PACKET_TYPE_PROMISCUOUS;
	struct net_in_addr maddr4 = {{{224, 0, 0, 1}}};
	struct net_if *iface = NULL;

	/* Test SetEthernetPacketFilter request directly */
	zassert_ok(k_mutex_lock(&test_udev->mutex, K_MSEC(200)), "Failed to lock USB device");
	zassert_ok(usbh_req_setup(test_udev, bmRequestType, SET_ETHERNET_PACKET_FILTER, 0x0000, 0,
				  0, NULL),
		   "Failed to clear packet filter");
	zassert_ok(usbh_req_setup(test_udev, bmRequestType, SET_ETHERNET_PACKET_FILTER, wValue, 0,
				  0, NULL),
		   "Failed to set packet filter");
	k_mutex_unlock(&test_udev->mutex);

	/* Test SetEthernetPacketFilter request via network interface API */
	zassert_true(net_ipv4_is_addr_mcast(&maddr4), "Invalid IPv4 multicast address");
	zassert_not_null(net_if_ipv4_maddr_add(host_iface, &maddr4),
			 "Failed to add IPv4 multicast address to host-side netif");
	zassert_not_null(net_if_ipv4_maddr_lookup(&maddr4, &iface),
			 "IPv4 multicast address not found after being added");
	zassert_equal(iface, host_iface,
		      "Multicast address found on wrong interface (expected host_iface)");
	zassert_true(net_if_ipv4_maddr_rm(host_iface, &maddr4),
		     "Failed to remove IPv4 multicast address from host-side netif");
	zassert_is_null(net_if_ipv4_maddr_lookup(&maddr4, &iface),
			"IPv4 multicast address should not be found after removal");
	zassert_ok(net_if_set_promisc(host_iface), "Failed to set promiscuous mode");
	net_if_unset_promisc(host_iface);
	zassert_false(net_if_flag_is_set(host_iface, NET_IF_PROMISC),
		      "Failed to unset promiscuous mode");
}

ZTEST(cdc_ecm, test_data_transfer)
{
	struct net_icmp_ctx ctx;
	struct net_sockaddr_in dst4 = {
		.sin_family = NET_AF_INET,
	};
	struct net_icmp_ping_params params = {
		.tc_tos = 0,
		.priority = -1,
	};

	zassert_ok(k_sem_init(&host_icmp_ctx.sem, 0, 1),
		   "Failed to initialize host TX ICMP semaphore");
	zassert_ok(k_sem_init(&device_icmp_ctx.sem, 0, 1),
		   "Failed to initialize device TX ICMP semaphore");

	zassert_ok(net_icmp_init_ctx(&ctx, NET_AF_INET, NET_ICMPV4_ECHO_REPLY, 0, icmp_handler),
		   "Failed to initialize ICMP context");

	dst4.sin_addr = device_addr;
	params.identifier = 1234;
	params.sequence = 1;
	params.data = host_tx_icmp_data;
	generate_rand_icmp_ping_data(host_tx_icmp_data, &params.data_size);

	zassert_ok(net_icmp_send_echo_request(&ctx, host_iface, (struct net_sockaddr *)&dst4,
					      &params, &host_icmp_ctx),
		   "Failed to send ICMP echo request from host-side netif to device-side's one");

	zassert_ok(k_sem_take(&host_icmp_ctx.sem, ICMP_PING_REPLY_TIMEOUT),
		   "ICMP echo reply (host -> device) timeout");

	zassert_equal(host_icmp_ctx.iface, host_iface,
		      "ICMP echo reply (host -> device) netif mismatch");
	zassert_mem_equal(host_icmp_ctx.src_addr.s4_addr, device_addr.s4_addr,
			  sizeof(device_addr.s4_addr),
			  "ICMP echo reply (host -> device) source address mismatch");
	zassert_equal(host_icmp_ctx.id, params.identifier,
		      "ICMP echo reply (host -> device) identifier mismatch");
	zassert_equal(host_icmp_ctx.seq, params.sequence,
		      "ICMP echo reply (host -> device) sequence mismatch");
	zassert_equal(host_icmp_ctx.data_len, params.data_size,
		      "ICMP echo reply (host -> device) data length mismatch");
	zassert_mem_equal(host_icmp_ctx.data, params.data, params.data_size,
			  "ICMP echo reply (host -> device) data mismatch");

	dst4.sin_addr = host_addr;
	params.identifier = 5678;
	params.sequence = 2;
	params.data = device_tx_icmp_data;
	generate_rand_icmp_ping_data(device_tx_icmp_data, &params.data_size);

	zassert_ok(net_icmp_send_echo_request(&ctx, device_iface, (struct net_sockaddr *)&dst4,
					      &params, &device_icmp_ctx),
		   "Failed to send ICMP echo request from device-side netif to host-side's one");

	zassert_ok(k_sem_take(&device_icmp_ctx.sem, ICMP_PING_REPLY_TIMEOUT),
		   "ICMP echo reply (device -> host) timeout");

	zassert_equal(device_icmp_ctx.iface, device_iface,
		      "ICMP echo reply (device -> host) netif mismatch");
	zassert_mem_equal(device_icmp_ctx.src_addr.s4_addr, host_addr.s4_addr,
			  sizeof(host_addr.s4_addr),
			  "ICMP echo reply (device -> host) source address mismatch");
	zassert_equal(device_icmp_ctx.id, params.identifier,
		      "ICMP echo reply (device -> host) identifier mismatch");
	zassert_equal(device_icmp_ctx.seq, params.sequence,
		      "ICMP echo reply (device -> host) sequence mismatch");
	zassert_equal(device_icmp_ctx.data_len, params.data_size,
		      "ICMP echo reply (device -> host) data length mismatch");
	zassert_mem_equal(device_icmp_ctx.data, params.data, params.data_size,
			  "ICMP echo reply (device -> host) data mismatch");

	zassert_ok(net_icmp_cleanup_ctx(&ctx), "Failed to cleanup ICMP context");
}

ZTEST_SUITE(cdc_ecm, NULL, cdc_ecm_test_setup, cdc_ecm_test_before, NULL, cdc_ecm_test_teardown);
