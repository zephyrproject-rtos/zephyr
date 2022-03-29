/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <device.h>
#include <kernel.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include <zephyr/shell/shell.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/capabilities.h>
#include <zephyr/bluetooth/audio/mics.h>
#include <zephyr/bluetooth/audio/has.h>
#include <zephyr/bluetooth/audio/vcs.h>
#include <drivers/gpio.h>
#include <sys/byteorder.h>
#include <sys/printk.h>

#include "hearing_aid.h"

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ha_main);

static struct bt_conn *default_conn;
static struct k_work_delayable adv_work;

static uint8_t unicast_server_addata[] = {
	BT_UUID_16_ENCODE(BT_UUID_ASCS_VAL), /* ASCS UUID */
	BT_AUDIO_UNICAST_ANNOUNCEMENT_TARGETED, /* Target Announcement */
	(((AVAILABLE_SINK_CONTEXT) >>  0) & 0xFF),
	(((AVAILABLE_SINK_CONTEXT) >>  8) & 0xFF),
	(((AVAILABLE_SOURCE_CONTEXT) >>  0) & 0xFF),
	(((AVAILABLE_SOURCE_CONTEXT) >>  8) & 0xFF),
	0x00, /* Metadata length */
};

#if defined(CONFIG_BT_PRIVACY)
/* HAP_d1.0r00; 3.3 Service UUIDs AD Type
 * The HA shall not include the Hearing Access Service UUID in the Service UUID AD type field of
 * the advertising data or scan response data if in one of the GAP connectable modes and if the HA
 * is using a resolvable private address.
 */
#define BT_DATA_UUID16_ALL_VAL BT_UUID_16_ENCODE(BT_UUID_ASCS_VAL)
#else
/* HAP_d1.0r00; 3.3 Service UUIDs AD Type
 * The HA shall include the Hearing Access Service Universally Unique Identifier (UUID) defined in
 * [2] in the Service UUID Advertising Data (AD) Type field of the advertising data or scan
 * response data, if in one of the Generic Access Profile (GAP) discoverable modes.
 */
#define BT_DATA_UUID16_ALL_VAL BT_UUID_16_ENCODE(BT_UUID_ASCS_VAL), \
			       BT_UUID_16_ENCODE(BT_UUID_HAS_VAL)
#endif /* CONFIG_BT_PRIVACY */

/* TODO: Expand with BAP data */
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_DATA_UUID16_ALL_VAL),
	BT_DATA(BT_DATA_SVC_DATA16, unicast_server_addata, ARRAY_SIZE(unicast_server_addata)),
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		LOG_ERR("Failed to connect to %s (%u)", addr, err);

		default_conn = NULL;
		return;
	}

	LOG_DBG("Connected: %s", addr);
	default_conn = bt_conn_ref(conn);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn != default_conn) {
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_DBG("Disconnected: %s (reason 0x%02x)", addr, reason);

	bt_conn_unref(default_conn);
	default_conn = NULL;

	/* Restart advertising after disconnection */
	k_work_schedule(&adv_work, K_SECONDS(1));
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static int le_ext_adv_create(struct bt_le_ext_adv **adv)
{
	int err;

	/* Create a non-connectable non-scannable advertising set */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_CONN_NAME, NULL, adv);
	if (err) {
		LOG_ERR("Failed to create advertising set (err %d)", err);
		return err;
	}

	err = bt_le_ext_adv_set_data(*adv, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		LOG_ERR("Failed to set advertising data (err %d)", err);
		return err;
	}

	return err;
}

static void adv_work_process(struct k_work *work)
{
	static struct bt_le_ext_adv *adv = NULL;
	int err;

	if (!adv) {
		err = le_ext_adv_create(&adv);
		__ASSERT_NO_MSG(adv);
	}

	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		LOG_ERR("Failed to start advertising set (err %d)", err);
		return;
	}

	LOG_DBG("Advertising successfully started");
}

static int cmd_init(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return err;
	}

	LOG_DBG("Bluetooth initialized");

	err = hearing_aid_sink_init();
	if (err) {
		LOG_ERR("Sink init failed (err %d)", err);
		return err;
	}

	LOG_DBG("Stream Sink initialized");

	err = hearing_aid_source_init();
	if (err) {
		LOG_ERR("Source init failed (err %d)", err);
		return err;
	}

	LOG_DBG("Stream Source initialized");

	err = hearing_aid_volume_init();
	if (err) {
		LOG_ERR("Volume init failed (err %d)", err);
		return err;
	}

	LOG_DBG("Volume initialized");

	k_work_init_delayable(&adv_work, adv_work_process);
	k_work_schedule(&adv_work, K_NO_WAIT);

	return 0;
}

static int cmd_ha(const struct shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);
	} else {
		shell_error(sh, "%s missing subcomand", argv[0]);
	}

	return -ENOEXEC;
}

#define HELP_NONE "[none]"

SHELL_STATIC_SUBCMD_SET_CREATE(ha_cmds,
	SHELL_CMD_ARG(init, NULL, HELP_NONE, cmd_init, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(ha, &ha_cmds, "Bluetooth HAS shell commands", cmd_ha, 1, 1);

#if defined(CONFIG_GPIO)
#define BTN4_NODE        DT_ALIAS(sw3)
#if DT_NODE_HAS_STATUS(BTN4_NODE, okay)
#define BTN4_LABEL       DT_PROP(BTN4_NODE, label)
static struct gpio_dt_spec btn4_spec = GPIO_DT_SPEC_GET(BTN4_NODE, gpios);
#endif

#if DT_NODE_HAS_STATUS(BTN4_NODE, okay)
static void init_work_handler(struct k_work *work)
{
	cmd_init(NULL, 0, NULL);
}

K_WORK_DEFINE(init_work, init_work_handler);

static struct gpio_callback callback_common;

static void btn4_handler(const struct device *dev, struct gpio_callback *cb,
			uint32_t pins)
{
	k_work_submit(&init_work);
}
#endif

static int buttons_init(const struct device *d)
{
	int err;

#if DT_NODE_HAS_STATUS(BTN4_NODE, okay)
	if (!device_is_ready(btn4_spec.port)) {
		LOG_ERR("%s is not ready", btn4_spec.port->name);
		return false;LOG_DBG("pressed");
	}

	err = gpio_pin_configure_dt(&btn4_spec, GPIO_INPUT);
	if (err < 0) {
		LOG_ERR("Failed to configure %s pin %d: %d",
		       btn4_spec.port->name, btn4_spec.pin, err);
		return false;
	}

	err = gpio_pin_interrupt_configure_dt(&btn4_spec,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (err < 0) {
		LOG_ERR("Failed to configure interrupt on %s pin %d: %d",
		       btn4_spec.port->name, btn4_spec.pin, err);
		return false;
	}

	gpio_init_callback(&callback_common, btn4_handler, BIT(btn4_spec.pin));
	gpio_add_callback(btn4_spec.port, &callback_common);
#endif

	(void)err;
	return true;
}

SYS_INIT(buttons_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
#endif /* CONFIG_GPIO */