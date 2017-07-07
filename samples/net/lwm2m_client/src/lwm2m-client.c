/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_DOMAIN "lwm2m-client"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1

#include <board.h>
#include <zephyr.h>
#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/net_app.h>
#include <net/lwm2m.h>

#if defined(CONFIG_NET_L2_BLUETOOTH)
#include <bluetooth/bluetooth.h>
#include <gatt/ipss.h>
#endif

#define APP_BANNER "Run LWM2M client"

#if !defined(CONFIG_NET_APP_PEER_IPV4_ADDR)
#define CONFIG_NET_APP_PEER_IPV4_ADDR ""
#endif

#if !defined(CONFIG_NET_APP_PEER_IPV6_ADDR)
#define CONFIG_NET_APP_PEER_IPV6_ADDR ""
#endif

#define WAIT_TIME	K_SECONDS(10)
#define CONNECT_TIME	K_SECONDS(10)

#define CLIENT_MANUFACTURER	"Zephyr"
#define CLIENT_MODEL_NUMBER	"OMA-LWM2M Sample Client"
#define CLIENT_SERIAL_NUMBER	"345000123"
#define CLIENT_FIRMWARE_VER	"1.0"
#define CLIENT_DEVICE_TYPE	"OMA-LWM2M Client"
#define CLIENT_HW_VER		"1.0.1"

#define ENDPOINT_LEN		32

static int pwrsrc_bat;
static int pwrsrc_usb;
static int battery_voltage = 3800;
static int battery_current = 125;
static int usb_voltage = 5000;
static int usb_current = 900;

#if defined(CONFIG_NET_IPV6)
static struct net_app_ctx udp6;
#endif
#if defined(CONFIG_NET_IPV4)
static struct net_app_ctx udp4;
#endif
static struct k_sem quit_lock;

#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
NET_PKT_TX_SLAB_DEFINE(lwm2m_tx_udp, 5);
NET_PKT_DATA_POOL_DEFINE(lwm2m_data_udp, 20);

static struct k_mem_slab *tx_udp_slab(void)
{
	return &lwm2m_tx_udp;
}

static struct net_buf_pool *data_udp_pool(void)
{
	return &lwm2m_data_udp;
}
#else
#define tx_udp_slab NULL
#define data_udp_pool NULL
#endif /* CONFIG_NET_CONTEXT_NET_PKT_POOL */

static int device_reboot_cb(u16_t obj_inst_id)
{
	SYS_LOG_INF("DEVICE: REBOOT");
	/* Add an error for testing */
	lwm2m_device_add_err(LWM2M_DEVICE_ERROR_LOW_POWER);
	/* Change the battery voltage for testing */
	lwm2m_device_set_pwrsrc_voltage_mv(pwrsrc_bat, --battery_voltage);

	return 1;
}

static int device_factory_default_cb(u16_t obj_inst_id)
{
	SYS_LOG_INF("DEVICE: FACTORY DEFAULT");
	/* Add an error for testing */
	lwm2m_device_add_err(LWM2M_DEVICE_ERROR_GPS_FAILURE);
	/* Change the USB current for testing */
	lwm2m_device_set_pwrsrc_current_ma(pwrsrc_usb, --usb_current);

	return 1;
}

static int firmware_update_cb(u16_t obj_inst_id)
{
	SYS_LOG_DBG("UPDATE");
	return 1;
}

static int firmware_block_received_cb(u16_t obj_inst_id,
				      u8_t *data, u16_t data_len,
				      bool last_block, size_t total_size)
{
	SYS_LOG_INF("FIRMWARE: BLOCK RECEIVED: len:%u last_block:%d",
		    data_len, last_block);
	return 1;
}

static int set_endpoint_name(char *ep_name, sa_family_t family)
{
	int ret;

	ret = snprintk(ep_name, ENDPOINT_LEN, "%s-%s-%u",
		       CONFIG_BOARD, (family == AF_INET6 ? "ipv6" : "ipv4"),
		       sys_rand32_get());
	if (ret < 0 || ret >= ENDPOINT_LEN) {
		SYS_LOG_ERR("Can't fill name buffer");
		return -EINVAL;
	}

	return 0;
}

static int lwm2m_setup(void)
{
	/* setup SECURITY object */
	/* setup SERVER object */

	/* setup DEVICE object */

	lwm2m_engine_set_string("3/0/0", CLIENT_MANUFACTURER);
	lwm2m_engine_set_string("3/0/1", CLIENT_MODEL_NUMBER);
	lwm2m_engine_set_string("3/0/2", CLIENT_SERIAL_NUMBER);
	lwm2m_engine_set_string("3/0/3", CLIENT_FIRMWARE_VER);
	lwm2m_engine_register_exec_callback("3/0/4", device_reboot_cb);
	lwm2m_engine_register_exec_callback("3/0/5", device_factory_default_cb);
	lwm2m_engine_set_u8("3/0/9", 95); /* battery level */
	lwm2m_engine_set_u32("3/0/10", 15); /* mem free */
	lwm2m_engine_set_string("3/0/17", CLIENT_DEVICE_TYPE);
	lwm2m_engine_set_string("3/0/18", CLIENT_HW_VER);
	lwm2m_engine_set_u8("3/0/20", LWM2M_DEVICE_BATTERY_STATUS_CHARGING);
	lwm2m_engine_set_u32("3/0/21", 25); /* mem total */

	pwrsrc_bat = lwm2m_device_add_pwrsrc(LWM2M_DEVICE_PWR_SRC_TYPE_BAT_INT);
	if (pwrsrc_bat < 0) {
		SYS_LOG_ERR("LWM2M battery power source enable error (err:%d)",
			pwrsrc_bat);
		return pwrsrc_bat;
	}
	lwm2m_device_set_pwrsrc_voltage_mv(pwrsrc_bat, battery_voltage);
	lwm2m_device_set_pwrsrc_current_ma(pwrsrc_bat, battery_current);

	pwrsrc_usb = lwm2m_device_add_pwrsrc(LWM2M_DEVICE_PWR_SRC_TYPE_USB);
	if (pwrsrc_usb < 0) {
		SYS_LOG_ERR("LWM2M usb power source enable error (err:%d)",
			pwrsrc_usb);
		return pwrsrc_usb;
	}
	lwm2m_device_set_pwrsrc_voltage_mv(pwrsrc_usb, usb_voltage);
	lwm2m_device_set_pwrsrc_current_ma(pwrsrc_usb, usb_current);

	/* setup FIRMWARE object */

	lwm2m_engine_register_post_write_callback("5/0/0",
						  firmware_block_received_cb);
	lwm2m_firmware_set_write_cb(firmware_block_received_cb);
	lwm2m_engine_register_exec_callback("5/0/2", firmware_update_cb);

	return 0;
}

int setup_net_app_ctx(struct net_app_ctx *ctx, const char *peer)
{
	int ret;

	ret = net_app_init_udp_client(ctx, NULL, NULL, peer,
				      CONFIG_LWM2M_PEER_PORT, WAIT_TIME, NULL);
	if (ret < 0) {
		SYS_LOG_ERR("Cannot init UDP client (%d)", ret);
		return ret;
	}

#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
	net_app_set_net_pkt_pool(ctx, tx_udp_slab, data_udp_pool);
#endif

	return ret;
}

void main(void)
{
	int ret;
	char ep_name[ENDPOINT_LEN];

	SYS_LOG_INF(APP_BANNER);

	k_sem_init(&quit_lock, 0, UINT_MAX);

#if defined(CONFIG_NET_L2_BLUETOOTH)
	if (bt_enable(NULL)) {
		SYS_LOG_ERR("Bluetooth init failed");
		return;
	}
	ipss_init();
	ipss_advertise();
#endif

	ret = lwm2m_setup();
	if (ret < 0) {
		SYS_LOG_ERR("Cannot setup LWM2M fields (%d)", ret);
		return;
	}

#if defined(CONFIG_NET_IPV6)
	ret = setup_net_app_ctx(&udp6, CONFIG_NET_APP_PEER_IPV6_ADDR);
	if (ret < 0) {
		goto cleanup_ipv6;
	}

	ret = set_endpoint_name(ep_name, udp6.ipv6.local.family);
	if (ret < 0) {
		SYS_LOG_ERR("Cannot set IPv6 endpoint name (%d)", ret);
		goto cleanup_ipv6;
	}


	ret = lwm2m_engine_start(udp6.ipv6.ctx);
	if (ret < 0) {
		SYS_LOG_ERR("Cannot init LWM2M IPv6 engine (%d)", ret);
		goto cleanup_ipv6;
	}

	ret = lwm2m_rd_client_start(udp6.ipv6.ctx, &udp6.ipv6.remote,
				    ep_name);
	if (ret < 0) {
		SYS_LOG_ERR("LWM2M init LWM2M IPv6 RD client error (%d)",
			ret);
		goto cleanup_ipv6;
	}

	SYS_LOG_INF("IPv6 setup complete.");
#endif

#if defined(CONFIG_NET_IPV4)
	ret = setup_net_app_ctx(&udp4, CONFIG_NET_APP_PEER_IPV4_ADDR);
	if (ret < 0) {
		goto cleanup_ipv4;
	}

	ret = set_endpoint_name(ep_name, udp4.ipv4.local.family);
	if (ret < 0) {
		SYS_LOG_ERR("Cannot set IPv4 endpoint name (%d)", ret);
		goto cleanup_ipv4;
	}

	ret = lwm2m_engine_start(udp4.ipv4.ctx);
	if (ret < 0) {
		SYS_LOG_ERR("Cannot init LWM2M IPv4 engine (%d)", ret);
		goto cleanup_ipv4;
	}

	ret = lwm2m_rd_client_start(udp4.ipv4.ctx, &udp4.ipv4.remote,
				    ep_name);
	if (ret < 0) {
		SYS_LOG_ERR("LWM2M init LWM2M IPv4 RD client error (%d)",
			ret);
		goto cleanup_ipv4;
	}

	SYS_LOG_INF("IPv4 setup complete.");
#endif

	k_sem_take(&quit_lock, K_FOREVER);

#if defined(CONFIG_NET_IPV4)
cleanup_ipv4:
	net_app_close(&udp4);
	net_app_release(&udp4);
#endif

#if defined(CONFIG_NET_IPV6)
cleanup_ipv6:
	net_app_close(&udp6);
	net_app_release(&udp6);
#endif
}
