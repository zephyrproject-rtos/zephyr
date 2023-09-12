/*
 * Copyright (c) 2017-2021 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (c) 2022 T-Mobile USA, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME rsi_bt_core
#include "rs9116w_ble_core.h"

#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/random/rand32.h>
#include <zephyr/drivers/bluetooth/rs9116w.h>

#if !IS_ENABLED(CONFIG_WIFI_RS9116W)
/* Buffer for WiSeConnect */
uint8_t global_buf[BT_GLOBAL_BUFF_LEN];
#endif

bt_ready_cb_t ready_cb;

ATOMIC_DEFINE(bt_dev_flags, 8);

static void init_work(struct k_work *work);

struct k_work bt_dev_init = Z_WORK_INITIALIZER(init_work);

uint8_t rsi_bt_random_addr[6];

K_THREAD_STACK_DEFINE(rsi_bt_rx_thread_stack, 2048);
static struct k_thread rsi_bt_rx_thread_data;
void rsi_bt_rx_thread(void *a, void *b, void *c);

#define DT_DRV_COMPAT silabs_rs9116w

#define INT_PIN	 DT_INST_GPIO_PIN(0, int_gpios)
#define INT_PORT device_get_binding(DT_INST_GPIO_LABEL(0, int_gpios))

/**
 * @brief Initialize bluetooth stack
 *
 * @return 0 on success, -ERRNO on error
 */
static int bt_init(void)
{
	int err;

	if (IS_ENABLED(CONFIG_BT_PRIVACY)) {
		sys_rand_get(rsi_bt_random_addr, 6);
		rsi_ble_set_random_address_with_value(rsi_bt_random_addr);
	}

	if (IS_ENABLED(CONFIG_BT_CONN)) {
		uint8_t locaddr[6];

		err = rsi_bt_get_local_device_address(locaddr);
		BT_DBG("Device MAC: %02X:%02X:%02X:%02X:%02X:%02X", locaddr[5],
		       locaddr[4], locaddr[3], locaddr[2], locaddr[1],
		       locaddr[0]);
		err = bt_conn_init();
		if (err) {
			return err;
		}
	}

	atomic_set_bit_to(bt_dev_flags, BT_DEV_READY, true);
	return 0;
}

static void init_work(struct k_work *work)
{
	int err;

	err = bt_init();
	if (ready_cb) {
		ready_cb(err);
	}
}

int32_t device_init(void);

int bt_enable(bt_ready_cb_t cb)
{
	int err;

	BT_INFO("Enabling BT");
	if (atomic_test_and_set_bit(bt_dev_flags, BT_DEV_ENABLE)) {
		return -EALREADY;
	}

	device_init();

	char name[17] = {0};

	if (strlen(CONFIG_BT_DEVICE_NAME) > 16) {
		BT_WARN("Configured name is too long, truncating to 16 bytes");
	}

	strncpy(name, CONFIG_BT_DEVICE_NAME, 16);

	err = rsi_bt_set_local_name(name);
	if (err) {
		BT_ERR("Name set fail: 0x%X\n", err);
	} else {
		BT_INFO("Device name set to: %s\n", name);
	}

	/*spawn the thread*/
	k_thread_create(&rsi_bt_rx_thread_data, rsi_bt_rx_thread_stack,
			K_THREAD_STACK_SIZEOF(rsi_bt_rx_thread_stack),
			rsi_bt_rx_thread, NULL, NULL, NULL, K_PRIO_COOP(8), 0,
			K_NO_WAIT);

	ready_cb = cb;
	/* TODO: Use proper kwork event for the callback */
	if (!cb) {
		int status = bt_init();
		return status;
	}

	k_work_submit(&bt_dev_init);
	return 0;
}

struct k_poll_signal int_rx_evt_signal;
#if IS_ENABLED(CONFIG_WISECONNECT_USE_OS_BINDINGS)
struct k_poll_signal osb_rx_evt_signal;
#else
static struct gpio_callback cb_data;
#endif

static struct k_poll_event ble_events[] = {
	K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY,
				 &int_rx_evt_signal),
#if IS_ENABLED(CONFIG_WISECONNECT_USE_OS_BINDINGS)
	K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY,
				 &osb_rx_evt_signal),
#endif
};

/**
 * @brief Simple interrupt handler to signal thread
 */
void int_rx_event_cb(const struct device *port, struct gpio_callback *cb,
		     gpio_port_pins_t pins)
{
	k_poll_signal_raise(&int_rx_evt_signal, 1);
}

void force_rx_evt(void) { k_poll_signal_raise(&int_rx_evt_signal, 1); }

#if IS_ENABLED(CONFIG_WISECONNECT_USE_OS_BINDINGS) &&                          \
	!IS_ENABLED(CONFIG_WIFI_RS9116W)
K_THREAD_STACK_DEFINE(driver_task_stack, 2048);
struct k_thread driver_task;

void driver_task_entry(void *p1, void *p2, void *p3)
{
	rsi_wireless_driver_task();
}
#endif

/**
 * @brief Initializes bluetooth hardware
 *
 * @return 0 on success
 */
int32_t device_init(void)
{
/* Assuming basic init is already completed if wifi is enabled (for now)*/
#if !IS_ENABLED(CONFIG_WIFI_RS9116W)
	int32_t status;

	status = rsi_driver_init(global_buf, BT_GLOBAL_BUFF_LEN);
	if ((status < 0) || (status > BT_GLOBAL_BUFF_LEN)) {
		return status;
	}
	status = rsi_device_init(LOAD_NWP_FW); /* Semaphore 1: Device INIT */
	if (status != RSI_SUCCESS) {
		BT_ERR("\r\nDevice Initialization Failed, Error Code : "
		       "0x%X\r\n",
		       status);
		return status;
	}

#if IS_ENABLED(CONFIG_WISECONNECT_USE_OS_BINDINGS)
	k_thread_create(&driver_task, driver_task_stack,
			K_THREAD_STACK_SIZEOF(driver_task_stack),
			driver_task_entry, NULL, NULL, NULL, K_PRIO_COOP(8),
			K_INHERIT_PERMS, K_NO_WAIT);
#endif
	status = rsi_wireless_init(
		0, RSI_OPERMODE_WLAN_BLE); /* Semaphore 2: Wireless INIT */
	if (status != RSI_SUCCESS) {
		BT_ERR("\r\nWireless Initialization Failed, Error Code : "
		       "0x%X\r\n",
		       status);
		return status;
	}
#endif

	/* Add new callback to raise the event for the RX thread */
	k_poll_signal_init(&int_rx_evt_signal);

#if !IS_ENABLED(CONFIG_WISECONNECT_USE_OS_BINDINGS)
	gpio_init_callback(&cb_data, int_rx_event_cb, BIT(INT_PIN));
	gpio_add_callback(INT_PORT, &cb_data);
#else
	k_poll_signal_init(&osb_rx_evt_signal);
#endif
	return 0;
}

/**
 * @brief Converts Zephyr bt_uuid structure to WiSeConnect uuid_t
 *
 * @param uuid Zephyr bt_uuid structure
 * @param out WiSeConnect uuid_t (output)
 */
void rsi_uuid_convert(const struct bt_uuid *uuid, uuid_t *out)
{
	switch (uuid->type) {
	case BT_UUID_TYPE_16:
		out->size = BT_UUID_SIZE_16;
		out->val.val16 = ((struct bt_uuid_16 *)uuid)->val;
		break;
	case BT_UUID_TYPE_32:
		out->size = BT_UUID_SIZE_32;
		out->val.val32 = ((struct bt_uuid_32 *)uuid)->val;
		break;
	case BT_UUID_TYPE_128:
		out->size = BT_UUID_SIZE_128;
		uint8_t *uuid_val = ((struct bt_uuid_128 *)uuid)->val;

		out->val.val128.data1 = uuid_val[12];
		out->val.val128.data1 += uuid_val[13] << 8;
		out->val.val128.data1 += uuid_val[14] << 16;
		out->val.val128.data1 += uuid_val[15] << 24;
		out->val.val128.data2 = uuid_val[10];
		out->val.val128.data2 += uuid_val[11] << 8;
		out->val.val128.data3 = uuid_val[8];
		out->val.val128.data3 += uuid_val[9] << 8;
		out->val.val128.data4[0] = uuid_val[6];
		out->val.val128.data4[1] = uuid_val[7];
		out->val.val128.data4[2] = uuid_val[4];
		out->val.val128.data4[3] = uuid_val[5];
		out->val.val128.data4[4] = uuid_val[0];
		out->val.val128.data4[5] = uuid_val[1];
		out->val.val128.data4[6] = uuid_val[2];
		out->val.val128.data4[7] = uuid_val[3];
		break;
	default:
		break;
	}
}

extern void bt_le_adv_resume(void);

int bt_set_name(const char *name)
{
#if defined(CONFIG_BT_DEVICE_NAME_DYNAMIC)
	size_t len = strlen(name);
	int ret;

	if (len > CONFIG_BT_DEVICE_NAME_MAX) {
		return -ENOMEM;
	}

	if (!strcmp(bt_get_name(), name)) {
		return 0;
	}

	bt_le_adv_stop();
	ret = rsi_bt_set_local_name((uint8_t *)name);
	bt_le_adv_resume();
	return ret;

#else
	return -ENOMEM;
#endif
}

#if defined(CONFIG_BT_DEVICE_NAME_DYNAMIC)
static char name_buf[50];
#endif

const char *bt_get_name(void)
{
#if defined(CONFIG_BT_DEVICE_NAME_DYNAMIC)
	rsi_bt_resp_get_local_name_t bt_resp_get_local_name;

	if (rsi_bt_get_local_name(&bt_resp_get_local_name)) {
		return NULL;
	}
	memset(name_buf, 0, 50);
	memcpy(name_buf, bt_resp_get_local_name.name,
			bt_resp_get_local_name.name_len);
	return name_buf;
#else
	return CONFIG_BT_DEVICE_NAME;
#endif
}

ATOMIC_DEFINE(rsi_bt_evt_count, 32);

void rsi_bt_raise_evt(void)
{
	atomic_inc(rsi_bt_evt_count);
#if IS_ENABLED(CONFIG_WISECONNECT_USE_OS_BINDINGS)
	k_poll_signal_raise(&osb_rx_evt_signal, 1);
#endif
}

/**
 * @brief RX thread for the RS9116
 */
void rsi_bt_rx_thread(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	while (!atomic_test_bit(bt_dev_flags, BT_DEV_READY)) {
		k_msleep(100);
	}

	uint32_t last_cleanup_time = k_uptime_get_32();

	while (1) {
#if !IS_ENABLED(CONFIG_WISECONNECT_USE_OS_BINDINGS)
		/* The 5 seconds here is for the edge case when an event is not
		 * fired when it should be
		 */
		k_poll(ble_events, 1, K_MSEC(5000));
		ble_events[0].signal->signaled = 0;
		ble_events[0].state = K_POLL_STATE_NOT_READY;
		rsi_wireless_driver_task();
#else
		k_poll(ble_events, 2, K_FOREVER);
		if (ble_events[0].signal->signaled) {
			ble_events[0].signal->signaled = 0;
			ble_events[0].state = K_POLL_STATE_NOT_READY;
			continue;
		} else if (ble_events[1].signal->signaled) {
			ble_events[1].signal->signaled = 0;
			ble_events[1].state = K_POLL_STATE_NOT_READY;
		}
#endif
		while (atomic_get(rsi_bt_evt_count) > 0) {
			atomic_dec(rsi_bt_evt_count);
#if defined(CONFIG_BT_SMP)
			bt_smp_process();
#endif
			bt_gatt_process();
			bt_gap_process();
		}

		if ((k_uptime_get_32() - last_cleanup_time) > 1000) {
			rsi_connection_cleanup_task();
		}

		k_yield();
	}
}

#define UUID_16_BASE_OFFSET 12

static const struct bt_uuid_128 uuid128_base = {
	.uuid = {BT_UUID_TYPE_128},
	.val = {BT_UUID_128_ENCODE(0x00000000, 0x0000, 0x1000, 0x8000,
				   0x00805F9B34FB)}};

/**
 * @brief Converts a UUID to its 128-bit counterpart
 *
 * @param src Input UUID
 * @param dst Output (128-bit) UUID
 */
static void uuid_to_uuid128(const struct bt_uuid *src, struct bt_uuid_128 *dst)
{
	switch (src->type) {
	case BT_UUID_TYPE_16:
		*dst = uuid128_base;
		sys_put_le16(BT_UUID_16(src)->val,
			     &dst->val[UUID_16_BASE_OFFSET]);
		return;
	case BT_UUID_TYPE_32:
		*dst = uuid128_base;
		sys_put_le32(BT_UUID_32(src)->val,
			     &dst->val[UUID_16_BASE_OFFSET]);
		return;
	case BT_UUID_TYPE_128:
		memcpy(dst, src, sizeof(*dst));
		return;
	}
}

/**
 * @brief Compares two 128-bit bt_uuid structs
 *
 * @return Compare result
 */
static int uuid128_cmp(const struct bt_uuid *u1, const struct bt_uuid *u2)
{
	struct bt_uuid_128 uuid1, uuid2;

	uuid_to_uuid128(u1, &uuid1);
	uuid_to_uuid128(u2, &uuid2);

	return memcmp(uuid1.val, uuid2.val, 16);
}

int bt_uuid_cmp(const struct bt_uuid *u1, const struct bt_uuid *u2)
{
	/* Convert to 128 bit if types don't match */
	if (u1->type != u2->type) {
		return uuid128_cmp(u1, u2);
	}

	switch (u1->type) {
	case BT_UUID_TYPE_16:
		return (int)BT_UUID_16(u1)->val - (int)BT_UUID_16(u2)->val;
	case BT_UUID_TYPE_32:
		return (int)BT_UUID_32(u1)->val - (int)BT_UUID_32(u2)->val;
	case BT_UUID_TYPE_128:
		return memcmp(BT_UUID_128(u1)->val, BT_UUID_128(u2)->val, 16);
	}

	return -EINVAL;
}

bool bt_uuid_create(struct bt_uuid *uuid, const uint8_t *data, uint8_t data_len)
{
	/* Copy UUID from packet data/internal variable to internal bt_uuid */
	switch (data_len) {
	case 2:
		uuid->type = BT_UUID_TYPE_16;
		BT_UUID_16(uuid)->val = sys_get_le16(data);
		break;
	case 4:
		uuid->type = BT_UUID_TYPE_32;
		BT_UUID_32(uuid)->val = sys_get_le32(data);
		break;
	case 16:
		uuid->type = BT_UUID_TYPE_128;
		memcpy(&BT_UUID_128(uuid)->val, data, 16);
		break;
	default:
		return false;
	}
	return true;
}

void bt_uuid_to_str(const struct bt_uuid *uuid, char *str, size_t len)
{
	uint32_t tmp1, tmp5;
	uint16_t tmp0, tmp2, tmp3, tmp4;

	switch (uuid->type) {
	case BT_UUID_TYPE_16:
		snprintk(str, len, "%04x", BT_UUID_16(uuid)->val);
		break;
	case BT_UUID_TYPE_32:
		snprintk(str, len, "%08x", BT_UUID_32(uuid)->val);
		break;
	case BT_UUID_TYPE_128:
		memcpy(&tmp0, &BT_UUID_128(uuid)->val[0], sizeof(tmp0));
		memcpy(&tmp1, &BT_UUID_128(uuid)->val[2], sizeof(tmp1));
		memcpy(&tmp2, &BT_UUID_128(uuid)->val[6], sizeof(tmp2));
		memcpy(&tmp3, &BT_UUID_128(uuid)->val[8], sizeof(tmp3));
		memcpy(&tmp4, &BT_UUID_128(uuid)->val[10], sizeof(tmp4));
		memcpy(&tmp5, &BT_UUID_128(uuid)->val[12], sizeof(tmp5));

		snprintk(str, len, "%08x-%04x-%04x-%04x-%08x%04x",
			 sys_le32_to_cpu(tmp5), sys_le16_to_cpu(tmp4),
			 sys_le16_to_cpu(tmp3), sys_le16_to_cpu(tmp2),
			 sys_le32_to_cpu(tmp1), sys_le16_to_cpu(tmp0));
		break;
	default:
		(void)memset(str, 0, len);
		return;
	}
}

#if defined(CONFIG_BT_WHITELIST)
int bt_le_whitelist_add(const bt_addr_le_t *addr)
{
	int err;

	if (!atomic_test_bit(bt_dev_flags, BT_DEV_READY)) {
		return -EAGAIN;
	}

	err = rsi_ble_deletefrom_whitelist(addr->a.val, addr->type);
	if (err) {
		BT_ERR("Failed to add device to whitelist");

		return err;
	}

	return 0;
}

int bt_le_whitelist_rem(const bt_addr_le_t *addr)
{
	int err;

	if (!atomic_test_bit(bt_dev_flags, BT_DEV_READY)) {
		return -EAGAIN;
	}

	err = rsi_ble_addto_whitelist(addr->a.val, addr->type);
	if (err) {
		BT_ERR("Failed to remove device from whitelist");
		return err;
	}

	return 0;
}

int bt_le_whitelist_clear(void)
{
	int err;

	if (!atomic_test_bit(bt_dev_flags, BT_DEV_READY)) {
		return -EAGAIN;
	}

	err = rsi_ble_clear_whitelist();
	if (err) {
		BT_ERR("Failed to clear whitelist");
		return err;
	}

	return 0;
}
#endif /* defined(CONFIG_BT_WHITELIST) */

int bt_le_set_chan_map(uint8_t chan_map[5])
{
	BT_WARN("Set Host Channel Classification command is "
		"not supported");
	return -ENOTSUP;
}

void bt_data_parse(struct net_buf_simple *ad,
		   bool (*func)(struct bt_data *data, void *user_data),
		   void *user_data)
{
	while (ad->len > 1) {
		struct bt_data data;
		uint8_t len;

		len = net_buf_simple_pull_u8(ad);
		if (len == 0U) {
			/* Early termination */
			return;
		}

		if (len > ad->len) {
			BT_WARN("Malformed data");
			return;
		}

		data.type = net_buf_simple_pull_u8(ad);
		data.data_len = len - 1;
		data.data = ad->data;

		if (!func(&data, user_data)) {
			return;
		}

		net_buf_simple_pull(ad, len - 1);
	}
}

int bt_get_mac(char *mac)
{
	if (rsi_bt_get_local_device_address(mac)) {
		return -EIO;
	}

	return 0;
}

#if defined(CONFIG_PM_DEVICE) && !defined(CONFIG_WIFI_RS9116W)
static int rs9116w_pm_action(const struct device *dev,
			     enum pm_device_action action)
{
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		bt_le_adv_resume();
		ret = rsi_bt_power_save_profile(RSI_ACTIVE, RSI_MAX_PSP);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		/* Todo: Add reinitialization logic */
		bt_le_adv_stop();
		ret = rsi_bt_power_save_profile(RSI_SLEEP_MODE_8, RSI_MAX_PSP);
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		bt_le_adv_stop();
		ret = rsi_bt_power_save_profile(RSI_SLEEP_MODE_10, RSI_MAX_PSP);
		pm_device_state_lock(dev);
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}
PM_DEVICE_DT_INST_DEFINE(0, rs9116w_pm_action);
#endif /* defined(CONFIG_PM_DEVICE) && !defined(CONFIG_WIFI_RS9116W) */

#ifndef CONFIG_WIFI_RS9116W
static int rs9116w_dummy_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}
DEVICE_DEFINE(rs9116w_dev, DEVICE_DT_NAME(DT_INST(0, silabs_rs9116w)),
	      rs9116w_dummy_init, PM_DEVICE_DT_INST_GET(0), NULL, NULL,
	      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);
#endif

/* Note: The RS9116 module lacks support for bluetooth ids AFAIK*/

/**
 * @brief UNSUPPORTED
 */
void bt_id_get(bt_addr_le_t *addrs, size_t *count) { *count = 1; }

/**
 * @brief UNSUPPORTED
 */
int bt_id_create(bt_addr_le_t *addr, uint8_t *irk) { return -ENOTSUP; }

/**
 * @brief UNSUPPORTED
 */
int bt_id_reset(uint8_t id, bt_addr_le_t *addr, uint8_t *irk)
{
	return -ENOTSUP;
}

/**
 * @brief UNSUPPORTED
 */
int bt_id_delete(uint8_t id) { return -ENOTSUP; }
