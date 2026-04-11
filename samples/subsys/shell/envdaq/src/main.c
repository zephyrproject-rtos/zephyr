/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Local additions attribution:
 * - Yongci <zhuyongci@hotmail.com>
 * - GitHub Copilot (GPT-5.3-Codex)
 * - Date: 2026-04-09
 */

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <zephyr/device.h>
#include <zephyr/data/json.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/hostname.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/sntp.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/posix/netdb.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/timeutil.h>

LOG_MODULE_REGISTER(envdaq, LOG_LEVEL_INF);

#define LED_NODE DT_ALIAS(led0)
#define RELAY_NODE DT_ALIAS(relay)
#define BUTTON_NODE DT_ALIAS(sw0)

#define EEPROM_NODE DT_NODELABEL(eeprom0)
#define BH1750_NODE DT_NODELABEL(bh1750)

// EEPROM layout in bytes
#define EEPROM_DEVICE_ID_OFFSET 6
#define EEPROM_DEVICE_ID_LEN 6
#define EEPROM_MQTT_USERNAME_OFFSET 12
#define EEPROM_MQTT_USERNAME_LEN 32
#define EEPROM_MQTT_PASSWORD_OFFSET 44
#define EEPROM_MQTT_PASSWORD_LEN 32
#define EEPROM_MQTT_ENDPOINT_OFFSET 76
#define EEPROM_MQTT_ENDPOINT_LEN 128
#define EEPROM_MQTT_ENDPOINT_PORT_OFFSET 204
#define EEPROM_MQTT_ENDPOINT_PORT_LEN 2

#define EEPROM_APP_VERSION_OFFSET 252
#define EEPROM_APP_VERSION_LEN 3
#define EEPROM_BOARD_INITIALIZED_OFFSET 255 /* value of FF is not initialized, value of 00 is initialized */

#define EEPROM_BOARD_INIT_UNINITIALIZED 0xFF
#define EEPROM_BOARD_INIT_INITIALIZED 0x00

#define ENVDAQ_DEFAULT_MQTT_PORT 1883
#define ENVDAQ_APP_VERSION_MAJOR 1
#define ENVDAQ_APP_VERSION_MINOR 0
#define ENVDAQ_APP_VERSION_PATCH 0

#define BUTTON_FACTORY_RESET_HOLD_MS 3000
#define LED_FACTORY_RESET_BLINK_MS 100
#define MQTT_KEEPALIVE_SECONDS 30
#define MQTT_RETRY_INTERVAL_MS (10 * MSEC_PER_SEC)
#define MQTT_CONNACK_TIMEOUT_MS (8 * MSEC_PER_SEC)
#define MQTT_RX_BUFFER_SIZE 1024
#define MQTT_TX_BUFFER_SIZE 1024
#define MQTT_INPUT_TIMEOUT_MS 100
#define MQTT_CONTROL_MAX_SKEW_SEC 60
#define ENVDAQ_VALID_UNIX_TIME_MIN 1700000000LL

#define RELAY_THREAD_STACK_SIZE 1024
#define RELAY_THREAD_PRIORITY 8

#define LED_THREAD_STACK_SIZE 1024
#define LED_THREAD_PRIORITY 11

#define MQTT_THREAD_STACK_SIZE 4096
#define MQTT_THREAD_PRIORITY 6

#define STATUS_THREAD_STACK_SIZE 3072
#define STATUS_THREAD_PRIORITY 7

#define DHT_THREAD_STACK_SIZE 1536
#define DHT_THREAD_PRIORITY 9
#define DHT_SAMPLE_INTERVAL_SEC 2

#define TIME_SYNC_THREAD_STACK_SIZE 1536
#define TIME_SYNC_THREAD_PRIORITY 10
#define TIME_SYNC_PERIOD_HOURS 4
#define TIME_SYNC_CHECK_INTERVAL_SEC 30
#define TIME_SYNC_PERIOD_MS ((int64_t)TIME_SYNC_PERIOD_HOURS * 60 * 60 * MSEC_PER_SEC)

#define CONTROL_APPLY_DEBOUNCE_MS 250

#define BUTTON_DEBOUNCE_MS 40
#define BUTTON_TOGGLE_GUARD_MS 200

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);
static const struct gpio_dt_spec relay = GPIO_DT_SPEC_GET(RELAY_NODE, gpios);
#if DT_NODE_HAS_STATUS(BUTTON_NODE, okay)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);
static struct gpio_callback button_cb;
static struct k_work_delayable button_work;
static int64_t button_last_toggle_ms;
static bool envdaq_button_is_active(int pin_value);
#endif

static const struct device *eeprom_dev = DEVICE_DT_GET(EEPROM_NODE);
static const struct device *light_dev = DEVICE_DT_GET(BH1750_NODE);
#if DT_NODE_HAS_STATUS(DT_ALIAS(dht0), okay)
static const struct device *dht_dev = DEVICE_DT_GET(DT_ALIAS(dht0));
#else
static const struct device *dht_dev;
#endif
#if DT_HAS_CHOSEN(zephyr_rtc)
static const struct device *rtc_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_rtc));
#else
static const struct device *rtc_dev;
#endif

static struct net_if *eth_iface;
static bool app_echo_enabled = true;

static struct mqtt_client mqtt_client_ctx;
static struct sockaddr_storage mqtt_broker;
static uint8_t mqtt_rx_buffer[MQTT_RX_BUFFER_SIZE];
static uint8_t mqtt_tx_buffer[MQTT_TX_BUFFER_SIZE];
static bool mqtt_connected;
static bool mqtt_connecting;
static int64_t mqtt_next_retry_ms;
static int64_t mqtt_connack_deadline_ms;
static struct mqtt_utf8 mqtt_user;
static struct mqtt_utf8 mqtt_pass;
static char mqtt_status_topic[96];
static char mqtt_subscription_topic[96];
static struct mqtt_topic mqtt_sub_topic;
static struct mqtt_subscription_list mqtt_sub_list;
static int64_t mqtt_control_last_timestamp = INT64_MIN;
static uint32_t mqtt_pubcomp_count;
static int64_t mqtt_pubcomp_last_info_ms;
static int64_t mqtt_setup_warn_ms;

static uint16_t mqtt_next_msg_id = 1;

static atomic_t relay_target_state = ATOMIC_INIT(0);
static atomic_t relay_state_dirty = ATOMIC_INIT(0);
K_THREAD_STACK_DEFINE(relay_thread_stack, RELAY_THREAD_STACK_SIZE);
static struct k_thread relay_thread_data;
K_THREAD_STACK_DEFINE(led_thread_stack, LED_THREAD_STACK_SIZE);
static struct k_thread led_thread_data;
K_THREAD_STACK_DEFINE(mqtt_thread_stack, MQTT_THREAD_STACK_SIZE);
static struct k_thread mqtt_thread_data;
K_THREAD_STACK_DEFINE(status_thread_stack, STATUS_THREAD_STACK_SIZE);
static struct k_thread status_thread_data;
K_THREAD_STACK_DEFINE(dht_thread_stack, DHT_THREAD_STACK_SIZE);
static struct k_thread dht_thread_data;
K_THREAD_STACK_DEFINE(time_sync_thread_stack, TIME_SYNC_THREAD_STACK_SIZE);
static struct k_thread time_sync_thread_data;
K_MUTEX_DEFINE(mqtt_lock);

static struct sensor_value dht_temp_cache;
static struct sensor_value dht_humidity_cache;
static bool dht_temp_valid;
static bool dht_humidity_valid;
static int64_t dht_last_sample_ms;
K_MUTEX_DEFINE(dht_cache_lock);

static struct k_work_delayable control_apply_work;
static char control_latest_msg[192];
static bool control_latest_valid;
K_MUTEX_DEFINE(control_msg_lock);
static int64_t control_stale_warn_ms;

struct envdaq_control_sets {
	bool relay;
};

struct envdaq_control_message {
	int64_t timestamp;
	struct envdaq_control_sets sets;
};

struct envdaq_status_data {
	bool relay;
	struct json_obj_token lux;
	char lux_buf[16];
	bool lux_valid;
	float temperature;
	bool temperature_valid;
	float humidity;
	bool humidity_valid;
};

struct envdaq_status_payload {
	int64_t timestamp;
	struct envdaq_status_data status;
};

static const struct json_obj_descr envdaq_control_sets_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct envdaq_control_sets, relay, JSON_TOK_TRUE),
};

static const struct json_obj_descr envdaq_control_message_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct envdaq_control_message, timestamp, JSON_TOK_INT64),
	JSON_OBJ_DESCR_OBJECT(struct envdaq_control_message, sets, envdaq_control_sets_descr),
};

static const struct json_obj_descr envdaq_status_data_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct envdaq_status_data, relay, JSON_TOK_TRUE),
	JSON_OBJ_DESCR_PRIM(struct envdaq_status_data, lux, JSON_TOK_FLOAT),
	JSON_OBJ_DESCR_PRIM(struct envdaq_status_data, lux_valid, JSON_TOK_TRUE),
	JSON_OBJ_DESCR_PRIM(struct envdaq_status_data, temperature, JSON_TOK_FLOAT_FP),
	JSON_OBJ_DESCR_PRIM(struct envdaq_status_data, temperature_valid, JSON_TOK_TRUE),
	JSON_OBJ_DESCR_PRIM(struct envdaq_status_data, humidity, JSON_TOK_FLOAT_FP),
	JSON_OBJ_DESCR_PRIM(struct envdaq_status_data, humidity_valid, JSON_TOK_TRUE),
};

static const struct json_obj_descr envdaq_status_payload_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct envdaq_status_payload, timestamp, JSON_TOK_INT64),
	JSON_OBJ_DESCR_OBJECT(struct envdaq_status_payload, status, envdaq_status_data_descr),
};

struct app_config {
	uint8_t device_id[EEPROM_DEVICE_ID_LEN];
	char mqtt_username[EEPROM_MQTT_USERNAME_LEN + 1];
	char mqtt_password[EEPROM_MQTT_PASSWORD_LEN + 1];
	char mqtt_endpoint[EEPROM_MQTT_ENDPOINT_LEN + 1];
	uint16_t mqtt_port;
	uint8_t app_version[EEPROM_APP_VERSION_LEN];
	uint8_t board_initialized;
};

static struct app_config g_cfg = {
	.device_id = { 0 },
	.mqtt_username = "",
	.mqtt_password = "",
	.mqtt_endpoint = "",
	.mqtt_port = ENVDAQ_DEFAULT_MQTT_PORT,
	.app_version = { 0, 0, 0 },
	.board_initialized = EEPROM_BOARD_INIT_UNINITIALIZED,
};

static void sanitize_ascii(char *buf, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		if (buf[i] == '\0') {
			return;
		}

		if (!isprint((unsigned char)buf[i])) {
			buf[i] = '_';
		}
	}

	buf[len] = '\0';
}

static void format_ipv4(const struct net_in_addr *addr, char *out, size_t out_len)
{
	if (net_addr_ntop(AF_INET, addr, out, out_len) == NULL) {
		snprintk(out, out_len, "n/a");
	}
}

static void format_mac(const uint8_t *mac, size_t len, char *out, size_t out_len)
{
	if ((mac == NULL) || (len < 6U)) {
		snprintk(out, out_len, "n/a");
		return;
	}

	snprintk(out, out_len, "%02x:%02x:%02x:%02x:%02x:%02x",
		 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static bool envdaq_is_erased_or_zero(const uint8_t *buf, size_t len)
{
	bool all_zero = true;
	bool all_ff = true;

	for (size_t i = 0; i < len; i++) {
		if (buf[i] != 0x00) {
			all_zero = false;
		}

		if (buf[i] != 0xFF) {
			all_ff = false;
		}
	}

	return all_zero || all_ff;
}

static void envdaq_format_device_id(char *out, size_t out_len)
{
	if (envdaq_is_erased_or_zero(g_cfg.device_id, sizeof(g_cfg.device_id))) {
		snprintk(out, out_len, "unknown");
		return;
	}

	snprintk(out, out_len, "%02x%02x%02x%02x%02x%02x",
		 g_cfg.device_id[0], g_cfg.device_id[1], g_cfg.device_id[2],
		 g_cfg.device_id[3], g_cfg.device_id[4], g_cfg.device_id[5]);
}

static bool envdaq_mqtt_config_ready(void)
{
	return (g_cfg.board_initialized == EEPROM_BOARD_INIT_INITIALIZED) &&
	       (g_cfg.mqtt_endpoint[0] != '\0') &&
	       (g_cfg.mqtt_username[0] != '\0') &&
	       (g_cfg.mqtt_password[0] != '\0') &&
	       (g_cfg.mqtt_port > 0U);
}

static int envdaq_eeprom_write_padded(off_t offset, size_t len, const char *src)
{
	uint8_t buf[EEPROM_MQTT_ENDPOINT_LEN] = { 0 };
	size_t copy_len;

	if (len > sizeof(buf)) {
		return -EINVAL;
	}

	copy_len = MIN(strlen(src), len);
	memcpy(buf, src, copy_len);

	return eeprom_write(eeprom_dev, offset, buf, len);
}

static void envdaq_refresh_mqtt_auth(void)
{
	mqtt_user.utf8 = (uint8_t *)g_cfg.mqtt_username;
	mqtt_user.size = strlen(g_cfg.mqtt_username);
	mqtt_pass.utf8 = (uint8_t *)g_cfg.mqtt_password;
	mqtt_pass.size = strlen(g_cfg.mqtt_password);
}

static int envdaq_sync_app_version_to_eeprom(void)
{
	const uint8_t expected[EEPROM_APP_VERSION_LEN] = {
		ENVDAQ_APP_VERSION_MAJOR,
		ENVDAQ_APP_VERSION_MINOR,
		ENVDAQ_APP_VERSION_PATCH,
	};

	if (!device_is_ready(eeprom_dev)) {
		return -ENODEV;
	}

	if (memcmp(g_cfg.app_version, expected, sizeof(expected)) != 0) {
		int ret = eeprom_write(eeprom_dev, EEPROM_APP_VERSION_OFFSET,
					       expected, sizeof(expected));

		if (ret < 0) {
			return ret;
		}

		memcpy(g_cfg.app_version, expected, sizeof(expected));
		LOG_INF("APP version synced to EEPROM: %u.%u.%u",
			expected[0], expected[1], expected[2]);
	}

	return 0;
}

static int envdaq_mark_board_initialized(uint8_t marker)
{
	int ret;

	if (!device_is_ready(eeprom_dev)) {
		return -ENODEV;
	}

	ret = eeprom_write(eeprom_dev, EEPROM_BOARD_INITIALIZED_OFFSET, &marker, 1);
	if (ret < 0) {
		return ret;
	}

	g_cfg.board_initialized = marker;

	return 0;
}

static void envdaq_led_blink_fast_three_times(void)
{
	for (int i = 0; i < 3; i++) {
		(void)gpio_pin_set_dt(&led, 1);
		k_msleep(LED_FACTORY_RESET_BLINK_MS);
		(void)gpio_pin_set_dt(&led, 0);
		k_msleep(LED_FACTORY_RESET_BLINK_MS);
	}
}

static int envdaq_factory_reset_mqtt_config(void)
{
	uint16_t port = 0;
	int ret;

	if (!device_is_ready(eeprom_dev)) {
		return -ENODEV;
	}

	ret = envdaq_eeprom_write_padded(EEPROM_MQTT_USERNAME_OFFSET,
					 EEPROM_MQTT_USERNAME_LEN, "");
	if (ret < 0) {
		return ret;
	}

	ret = envdaq_eeprom_write_padded(EEPROM_MQTT_PASSWORD_OFFSET,
					 EEPROM_MQTT_PASSWORD_LEN, "");
	if (ret < 0) {
		return ret;
	}

	ret = envdaq_eeprom_write_padded(EEPROM_MQTT_ENDPOINT_OFFSET,
					 EEPROM_MQTT_ENDPOINT_LEN, "");
	if (ret < 0) {
		return ret;
	}

	ret = eeprom_write(eeprom_dev, EEPROM_MQTT_ENDPOINT_PORT_OFFSET,
			   &port, sizeof(port));
	if (ret < 0) {
		return ret;
	}

	ret = envdaq_mark_board_initialized(EEPROM_BOARD_INIT_UNINITIALIZED);
	if (ret < 0) {
		return ret;
	}

	g_cfg.mqtt_username[0] = '\0';
	g_cfg.mqtt_password[0] = '\0';
	g_cfg.mqtt_endpoint[0] = '\0';
	g_cfg.mqtt_port = ENVDAQ_DEFAULT_MQTT_PORT;
	envdaq_refresh_mqtt_auth();

	k_mutex_lock(&mqtt_lock, K_FOREVER);
	if (mqtt_connected || mqtt_connecting) {
		(void)mqtt_abort(&mqtt_client_ctx);
	}
	mqtt_connected = false;
	mqtt_connecting = false;
	k_mutex_unlock(&mqtt_lock);

	return 0;
}

#if DT_NODE_HAS_STATUS(BUTTON_NODE, okay)
static void envdaq_check_factory_reset_on_boot(void)
{
	int pin_value;
	int64_t deadline;

	if (!gpio_is_ready_dt(&button)) {
		return;
	}

	pin_value = gpio_pin_get_dt(&button);
	if (!envdaq_button_is_active(pin_value)) {
		return;
	}

	LOG_WRN("Factory reset button detected, hold to confirm...");
	deadline = k_uptime_get() + BUTTON_FACTORY_RESET_HOLD_MS;
	while (k_uptime_get() < deadline) {
		pin_value = gpio_pin_get_dt(&button);
		if (!envdaq_button_is_active(pin_value)) {
			LOG_INF("Factory reset canceled");
			return;
		}
		k_msleep(50);
	}

	if (envdaq_factory_reset_mqtt_config() == 0) {
		envdaq_led_blink_fast_three_times();
		LOG_WRN("Factory reset complete: MQTT config cleared");
	} else {
		LOG_ERR("Factory reset failed");
	}
}
#endif

static void envdaq_request_relay_state(int state)
{
	atomic_set(&relay_target_state, state ? 1 : 0);
	atomic_set(&relay_state_dirty, 1);
}

static void envdaq_toggle_relay_state(void)
{
	envdaq_request_relay_state(!atomic_get(&relay_target_state));
}

static void envdaq_relay_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (1) {
		if (atomic_cas(&relay_state_dirty, 1, 0)) {
			int target = atomic_get(&relay_target_state);

			if (gpio_pin_set_dt(&relay, target) < 0) {
				LOG_WRN("relay output update failed");
			} else {
				LOG_INF("relay output -> %s", target ? "on" : "off");
			}
		}

		k_msleep(20);
	}
}

static bool envdaq_network_ready(void)
{
	if (eth_iface == NULL) {
		return false;
	}

	if (!net_if_is_up(eth_iface)) {
		return false;
	}

	return net_if_ipv4_get_global_addr(eth_iface, NET_ADDR_PREFERRED) != NULL;
}

static void envdaq_led_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (1) {
		if (mqtt_connected) {
			/* Two flashes every 0.5 second when cloud MQTT is connected. */
			(void)gpio_pin_set_dt(&led, 1);
			k_msleep(70);
			(void)gpio_pin_set_dt(&led, 0);
			k_msleep(50);
			(void)gpio_pin_set_dt(&led, 1);
			k_msleep(70);
			(void)gpio_pin_set_dt(&led, 0);
			k_msleep(800);
			continue;
		}

		if (envdaq_network_ready()) {
			/* One flash per second after network comes up. */
			(void)gpio_pin_set_dt(&led, 1);
			k_msleep(150);
			(void)gpio_pin_set_dt(&led, 0);
			k_msleep(850);
			continue;
		}

		/* Solid ON when there is no network. */
		(void)gpio_pin_set_dt(&led, 1);
		k_msleep(250);
	}
}

#if DT_NODE_HAS_STATUS(BUTTON_NODE, okay)
static bool envdaq_button_is_active(int pin_value)
{
	if (pin_value < 0) {
		return false;
	}

	/* gpio_pin_get_dt() already returns logical active/inactive value. */
	return pin_value > 0;
}

static void envdaq_button_work_handler(struct k_work *work)
{
	int pin_value;
	int64_t now;

	ARG_UNUSED(work);

	pin_value = gpio_pin_get_dt(&button);
	if (!envdaq_button_is_active(pin_value)) {
		return;
	}

	now = k_uptime_get();
	if ((now - button_last_toggle_ms) < BUTTON_TOGGLE_GUARD_MS) {
		return;
	}

	button_last_toggle_ms = now;
	envdaq_toggle_relay_state();
}

static void envdaq_button_callback(const struct device *dev,
				   struct gpio_callback *cb,
				   uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	k_work_reschedule(&button_work, K_MSEC(BUTTON_DEBOUNCE_MS));
}
#endif

static int cmd_hikit_setup(const struct shell *sh, size_t argc, char **argv)
{
	char endpoint[EEPROM_MQTT_ENDPOINT_LEN + 1];
	char username[EEPROM_MQTT_USERNAME_LEN + 1];
	char password[EEPROM_MQTT_PASSWORD_LEN + 1];
	char *endptr;
	unsigned long port_ul;
	uint16_t port;
	int ret;

	if (!device_is_ready(eeprom_dev)) {
		shell_error(sh, "EEPROM not ready");
		return -ENODEV;
	}

	if ((argc == 1) || (strcmp(argv[1], "status") == 0)) {
		shell_print(sh, "init_flag=0x%02x", g_cfg.board_initialized);
		shell_print(sh, "mqtt endpoint=%s", g_cfg.mqtt_endpoint[0] ? g_cfg.mqtt_endpoint : "<empty>");
		shell_print(sh, "mqtt port=%u", g_cfg.mqtt_port);
		shell_print(sh, "mqtt username=%s", g_cfg.mqtt_username[0] ? g_cfg.mqtt_username : "<empty>");
		shell_print(sh, "mqtt password=%s", g_cfg.mqtt_password[0] ? "<set>" : "<empty>");
		shell_print(sh, "app version=%u.%u.%u",
			    g_cfg.app_version[0], g_cfg.app_version[1], g_cfg.app_version[2]);
		return 0;
	}

	if ((argc != 6) || (strcmp(argv[1], "mqtt") != 0)) {
		shell_error(sh, "usage: hikit setup mqtt <endpoint> <port> <username> <password>");
		return -EINVAL;
	}

	if (strlen(argv[2]) > EEPROM_MQTT_ENDPOINT_LEN ||
	    strlen(argv[4]) > EEPROM_MQTT_USERNAME_LEN ||
	    strlen(argv[5]) > EEPROM_MQTT_PASSWORD_LEN) {
		shell_error(sh, "input too long");
		return -EINVAL;
	}

	port_ul = strtoul(argv[3], &endptr, 10);
	if ((*endptr != '\0') || (port_ul == 0) || (port_ul > UINT16_MAX)) {
		shell_error(sh, "invalid port");
		return -EINVAL;
	}
	port = (uint16_t)port_ul;

	snprintk(endpoint, sizeof(endpoint), "%s", argv[2]);
	snprintk(username, sizeof(username), "%s", argv[4]);
	snprintk(password, sizeof(password), "%s", argv[5]);
	sanitize_ascii(endpoint, EEPROM_MQTT_ENDPOINT_LEN);
	sanitize_ascii(username, EEPROM_MQTT_USERNAME_LEN);
	sanitize_ascii(password, EEPROM_MQTT_PASSWORD_LEN);

	ret = envdaq_eeprom_write_padded(EEPROM_MQTT_ENDPOINT_OFFSET,
					 EEPROM_MQTT_ENDPOINT_LEN, endpoint);
	if (ret < 0) {
		shell_error(sh, "write endpoint failed: %d", ret);
		return ret;
	}

	ret = envdaq_eeprom_write_padded(EEPROM_MQTT_USERNAME_OFFSET,
					 EEPROM_MQTT_USERNAME_LEN, username);
	if (ret < 0) {
		shell_error(sh, "write username failed: %d", ret);
		return ret;
	}

	ret = envdaq_eeprom_write_padded(EEPROM_MQTT_PASSWORD_OFFSET,
					 EEPROM_MQTT_PASSWORD_LEN, password);
	if (ret < 0) {
		shell_error(sh, "write password failed: %d", ret);
		return ret;
	}

	ret = eeprom_write(eeprom_dev, EEPROM_MQTT_ENDPOINT_PORT_OFFSET, &port, sizeof(port));
	if (ret < 0) {
		shell_error(sh, "write port failed: %d", ret);
		return ret;
	}

	ret = envdaq_mark_board_initialized(EEPROM_BOARD_INIT_INITIALIZED);
	if (ret < 0) {
		shell_error(sh, "write init flag failed: %d", ret);
		return ret;
	}

	(void)envdaq_sync_app_version_to_eeprom();

	snprintk(g_cfg.mqtt_endpoint, sizeof(g_cfg.mqtt_endpoint), "%s", endpoint);
	snprintk(g_cfg.mqtt_username, sizeof(g_cfg.mqtt_username), "%s", username);
	snprintk(g_cfg.mqtt_password, sizeof(g_cfg.mqtt_password), "%s", password);
	g_cfg.mqtt_port = port;
	envdaq_refresh_mqtt_auth();

	k_mutex_lock(&mqtt_lock, K_FOREVER);
	if (mqtt_connected || mqtt_connecting) {
		(void)mqtt_abort(&mqtt_client_ctx);
		mqtt_connected = false;
		mqtt_connecting = false;
	}
	k_mutex_unlock(&mqtt_lock);
	mqtt_next_retry_ms = 0;

	shell_print(sh, "setup ok, board initialized");
	return 0;
}

static const struct net_linkaddr *envdaq_get_iface_link_addr(void)
{
	if (eth_iface == NULL) {
		return NULL;
	}

	return net_if_get_link_addr(eth_iface);
}

static int envdaq_mqtt_socket_fd(const struct mqtt_client *client)
{
	if (client->transport.type == MQTT_TRANSPORT_NON_SECURE) {
		return client->transport.tcp.sock;
	}

#if defined(CONFIG_MQTT_LIB_WEBSOCKET)
	if (client->transport.type == MQTT_TRANSPORT_NON_SECURE_WEBSOCKET) {
		return client->transport.websocket.sock;
	}
#endif

	return -1;
}

static bool envdaq_wait_for_ipv4(uint32_t timeout_ms)
{
	int64_t deadline = k_uptime_get() + timeout_ms;

	while (k_uptime_get() < deadline) {
		if ((eth_iface != NULL) &&
		    (net_if_ipv4_get_global_addr(eth_iface, NET_ADDR_PREFERRED) != NULL)) {
			return true;
		}

		k_msleep(200);
	}

	return false;
}

static void envdaq_format_sensor_2dp(const struct sensor_value *value, char *out, size_t out_len)
{
	int32_t frac = value->val2;

	if (frac < 0) {
		frac = -frac;
	}

	frac = (frac + 5000) / 10000;
	snprintk(out, out_len, "%d.%02d", value->val1, frac);
}

static float envdaq_sensor_to_float(const struct sensor_value *value)
{
	return (float)value->val1 + ((float)value->val2 / 1000000.0f);
}

static int32_t envdaq_sensor_to_centi(const struct sensor_value *value)
{
	int64_t centi;
	int32_t frac = value->val2;

	if (frac < 0) {
		frac = -frac;
		frac = (frac + 5000) / 10000;
		centi = ((int64_t)value->val1 * 100) - frac;
	} else {
		frac = (frac + 5000) / 10000;
		centi = ((int64_t)value->val1 * 100) + frac;
	}

	if (centi > INT32_MAX) {
		return INT32_MAX;
	}

	if (centi < INT32_MIN) {
		return INT32_MIN;
	}

	return (int32_t)centi;
}

static void envdaq_format_centi_json(int32_t centi, char *buf, size_t buf_len)
{
	int32_t whole = centi / 100;
	int32_t frac = centi % 100;

	if (frac < 0) {
		frac = -frac;
	}

	snprintk(buf, buf_len, "%d.%02d", whole, frac);
}

static int envdaq_read_relay_state(void)
{
	int relay_state = gpio_pin_get_dt(&relay);

	return (relay_state >= 0) ? relay_state : 0;
}

static bool envdaq_time_is_valid(time_t now)
{
	return ((int64_t)now >= ENVDAQ_VALID_UNIX_TIME_MIN);
}

static bool envdaq_rtc_is_available(void)
{
	return (rtc_dev != NULL) && device_is_ready(rtc_dev);
}

static int envdaq_restore_time_from_rtc(void)
{
	struct rtc_time rtc_tm;
	struct tm tm = { 0 };
	struct timespec tv = { 0 };
	time_t rtc_unix;
	int ret;

	if (!envdaq_rtc_is_available()) {
		return -ENODEV;
	}

	ret = rtc_get_time(rtc_dev, &rtc_tm);
	if (ret < 0) {
		return ret;
	}

	tm.tm_sec = rtc_tm.tm_sec;
	tm.tm_min = rtc_tm.tm_min;
	tm.tm_hour = rtc_tm.tm_hour;
	tm.tm_mday = rtc_tm.tm_mday;
	tm.tm_mon = rtc_tm.tm_mon;
	tm.tm_year = rtc_tm.tm_year;
	tm.tm_wday = rtc_tm.tm_wday;
	tm.tm_yday = rtc_tm.tm_yday;
	tm.tm_isdst = rtc_tm.tm_isdst;

	rtc_unix = timeutil_timegm(&tm);
	if ((rtc_unix == (time_t)-1) || !envdaq_time_is_valid(rtc_unix)) {
		return -ENODATA;
	}

	tv.tv_sec = rtc_unix;
	tv.tv_nsec = 0;

	ret = sys_clock_settime(SYS_CLOCK_REALTIME, &tv);
	if (ret < 0) {
		return -errno;
	}

	LOG_INF("System time restored from RTC: %lld", (long long)rtc_unix);

	return 0;
}

static int envdaq_store_time_to_rtc(time_t unix_time)
{
	struct rtc_time rtc_tm = { 0 };
	struct tm tm = { 0 };

	if (!envdaq_rtc_is_available()) {
		return -ENODEV;
	}

	if (!envdaq_time_is_valid(unix_time)) {
		return -EINVAL;
	}

	if (gmtime_r(&unix_time, &tm) == NULL) {
		return -EINVAL;
	}

	rtc_tm.tm_sec = tm.tm_sec;
	rtc_tm.tm_min = tm.tm_min;
	rtc_tm.tm_hour = tm.tm_hour;
	rtc_tm.tm_mday = tm.tm_mday;
	rtc_tm.tm_mon = tm.tm_mon;
	rtc_tm.tm_year = tm.tm_year;
	rtc_tm.tm_wday = tm.tm_wday;
	rtc_tm.tm_yday = tm.tm_yday;
	rtc_tm.tm_isdst = -1;
	rtc_tm.tm_nsec = 0;

	return rtc_set_time(rtc_dev, &rtc_tm);
}

static int envdaq_force_sntp_sync_once(void)
{
	struct sntp_time stime;
	struct timespec tv = { 0 };
	int ret;

	ret = sntp_simple(CONFIG_NET_CONFIG_SNTP_INIT_SERVER, 4 * MSEC_PER_SEC, &stime);
	if (ret < 0) {
		return ret;
	}

	tv.tv_sec = (time_t)stime.seconds;
	tv.tv_nsec = 0;

	ret = sys_clock_settime(SYS_CLOCK_REALTIME, &tv);
	if (ret < 0) {
		return -errno;
	}

	ret = envdaq_store_time_to_rtc(tv.tv_sec);
	if (ret < 0) {
		LOG_WRN("RTC update after SNTP failed: %d", ret);
	}

	return 0;
}

static void envdaq_require_time_sync(void)
{
	int ret;
	time_t now;
	bool rtc_fallback_logged = false;

	while (1) {
		now = time(NULL);
		if (envdaq_time_is_valid(now)) {
			LOG_INF("System time synchronized: %lld", (long long)now);
			return;
		}

		ret = envdaq_restore_time_from_rtc();
		if (ret == 0) {
			if (!rtc_fallback_logged) {
				LOG_WRN("SNTP unavailable, using RTC backup time");
				rtc_fallback_logged = true;
			}
			return;
		}

		if (!envdaq_wait_for_ipv4(1000)) {
			LOG_WRN("Waiting for IPv4 before SNTP sync");
			k_sleep(K_SECONDS(1));
			continue;
		}

		ret = envdaq_force_sntp_sync_once();
		if (ret < 0) {
			LOG_WRN("SNTP sync failed: %d, retrying", ret);
			ret = envdaq_restore_time_from_rtc();
			if (ret == 0) {
				if (!rtc_fallback_logged) {
					LOG_WRN("SNTP failed, switched to RTC backup time");
					rtc_fallback_logged = true;
				}
				return;
			}
			k_sleep(K_SECONDS(2));
			continue;
		}

		now = time(NULL);
		if (envdaq_time_is_valid(now)) {
			LOG_INF("SNTP sync successful: %lld", (long long)now);
			return;
		}

		LOG_WRN("SNTP returned invalid epoch, retrying");
		k_sleep(K_SECONDS(2));
	}
}

static void envdaq_time_sync_thread(void *p1, void *p2, void *p3)
{
	int64_t next_sync_ms;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	next_sync_ms = k_uptime_get() + TIME_SYNC_PERIOD_MS;

	while (1) {
		time_t now = time(NULL);

		if (!envdaq_time_is_valid(now)) {
			if (envdaq_restore_time_from_rtc() == 0) {
				next_sync_ms = k_uptime_get() + TIME_SYNC_PERIOD_MS;
				k_sleep(K_SECONDS(TIME_SYNC_CHECK_INTERVAL_SEC));
				continue;
			}

			LOG_WRN("System time became invalid, forcing SNTP sync");
			envdaq_require_time_sync();
			next_sync_ms = k_uptime_get() + TIME_SYNC_PERIOD_MS;
			k_sleep(K_SECONDS(TIME_SYNC_CHECK_INTERVAL_SEC));
			continue;
		}

		if (k_uptime_get() >= next_sync_ms) {
			int ret = envdaq_force_sntp_sync_once();

			if (ret < 0) {
				LOG_WRN("Periodic SNTP resync failed: %d", ret);
			} else {
				LOG_INF("Periodic SNTP resync done");
			}

			now = time(NULL);
			if (!envdaq_time_is_valid(now)) {
				LOG_WRN("Time invalid after periodic sync, forcing sync loop");
				envdaq_require_time_sync();
			}

			next_sync_ms = k_uptime_get() + TIME_SYNC_PERIOD_MS;
		}

		k_sleep(K_SECONDS(TIME_SYNC_CHECK_INTERVAL_SEC));
	}
}

static int envdaq_build_payload_json(char *out, size_t out_len)
{
	struct envdaq_status_payload payload = {
		.timestamp = (int64_t)time(NULL),
		.status = {
			.relay = envdaq_read_relay_state() ? true : false,
			.lux = { 0 },
			.lux_buf = "0.00",
			.lux_valid = false,
			.temperature = 0,
			.temperature_valid = false,
			.humidity = 0,
			.humidity_valid = false,
		},
	};
	struct sensor_value lux = { 0 };
	struct sensor_value temp = { 0 };
	struct sensor_value humidity = { 0 };
	int32_t lux_centi;
	int ret;

	payload.status.lux.start = payload.status.lux_buf;
	payload.status.lux.length = strlen(payload.status.lux_buf);

	if (device_is_ready(light_dev) && sensor_sample_fetch(light_dev) == 0 &&
	    sensor_channel_get(light_dev, SENSOR_CHAN_LIGHT, &lux) == 0) {
		lux_centi = envdaq_sensor_to_centi(&lux);
		envdaq_format_centi_json(lux_centi,
					 payload.status.lux_buf,
					 sizeof(payload.status.lux_buf));
		payload.status.lux.start = payload.status.lux_buf;
		payload.status.lux.length = strlen(payload.status.lux_buf);
		payload.status.lux_valid = true;
	}

	k_mutex_lock(&dht_cache_lock, K_FOREVER);
	if (dht_temp_valid) {
		temp = dht_temp_cache;
		payload.status.temperature = envdaq_sensor_to_float(&temp);
		payload.status.temperature_valid = true;
	}

	if (dht_humidity_valid) {
		humidity = dht_humidity_cache;
		payload.status.humidity = envdaq_sensor_to_float(&humidity);
		payload.status.humidity_valid = true;
	}
	k_mutex_unlock(&dht_cache_lock);

	ret = json_obj_encode_buf(envdaq_status_payload_descr,
				  ARRAY_SIZE(envdaq_status_payload_descr),
				  &payload,
				  out,
				  out_len);
	if (ret < 0) {
		LOG_WRN("Status JSON encode failed: %d", ret);
		return ret;
	}

	return 0;
}

static int envdaq_mqtt_publish_status(void)
{
	static char payload[320];
	struct mqtt_publish_param param = { 0 };
	int ret;

	k_mutex_lock(&mqtt_lock, K_FOREVER);

	if (!mqtt_connected) {
		ret = -ENOTCONN;
		goto out;
	}

	ret = envdaq_build_payload_json(payload, sizeof(payload));
	if (ret < 0) {
		goto out;
	}

	param.message.topic.topic.utf8 = (uint8_t *)mqtt_status_topic;
	param.message.topic.topic.size = strlen(mqtt_status_topic);
	param.message.topic.qos = MQTT_QOS_2_EXACTLY_ONCE;
	param.message.payload.data = payload;
	param.message.payload.len = strlen(payload);
	param.message_id = mqtt_next_msg_id++;
	param.dup_flag = 0U;
	param.retain_flag = 1U;

	ret = mqtt_publish(&mqtt_client_ctx, &param);
	if (ret < 0) {
		LOG_WRN("mqtt_publish failed: %d", ret);
		goto out;
	}

	ret = 0;

out:
	k_mutex_unlock(&mqtt_lock);
	return ret;
}

static int envdaq_parse_control_message(char *msg, int64_t *timestamp, int *relay_set)
{
	struct envdaq_control_message decoded = { 0 };
	int64_t parsed_fields;
	const int64_t required_fields = BIT(0) | BIT(1);

	*timestamp = 0;
	*relay_set = 0;

	if ((strstr(msg, "\"timestamp\"") == NULL) || (strstr(msg, "\"relay\"") == NULL)) {
		return -EINVAL;
	}

	parsed_fields = json_obj_parse(msg,
				      strlen(msg),
				      envdaq_control_message_descr,
				      ARRAY_SIZE(envdaq_control_message_descr),
				      &decoded);
	if (parsed_fields < 0) {
		return (int)parsed_fields;
	}

	if ((parsed_fields & required_fields) != required_fields) {
		return -EINVAL;
	}

	*timestamp = decoded.timestamp;
	*relay_set = decoded.sets.relay ? 1 : 0;

	return 0;
}

static void envdaq_apply_relay_from_message(char *msg)
{
	int relay_set = 0;
	int64_t timestamp = 0;
	time_t now;
	int64_t delta;
	int64_t uptime_now;
	int ret;

	ret = envdaq_parse_control_message(msg, &timestamp, &relay_set);
	if (ret < 0) {
		LOG_WRN("Ignore control with invalid JSON payload: %d", ret);
		return;
	}

	now = time(NULL);
	if (!envdaq_time_is_valid(now)) {
		LOG_WRN("Ignore control while system time is invalid");
		return;
	}

	delta = llabs(timestamp - (int64_t)now);
	if (delta > MQTT_CONTROL_MAX_SKEW_SEC) {
		uptime_now = k_uptime_get();
		if ((uptime_now - control_stale_warn_ms) > 2000) {
			LOG_WRN("Ignore stale/future control, delta=%lld sec", (long long)delta);
			control_stale_warn_ms = uptime_now;
		}
		return;
	}

	if (timestamp < mqtt_control_last_timestamp) {
		LOG_WRN("Ignore out-of-order control ts=%lld last=%lld",
			(long long)timestamp,
			(long long)mqtt_control_last_timestamp);
		return;
	}

	mqtt_control_last_timestamp = timestamp;
	envdaq_request_relay_state(relay_set);
}

static void envdaq_control_apply_work_handler(struct k_work *work)
{
	char msg[sizeof(control_latest_msg)];

	ARG_UNUSED(work);

	k_mutex_lock(&control_msg_lock, K_FOREVER);
	if (!control_latest_valid) {
		k_mutex_unlock(&control_msg_lock);
		return;
	}

	memcpy(msg, control_latest_msg, sizeof(msg));
	control_latest_valid = false;
	k_mutex_unlock(&control_msg_lock);

	envdaq_apply_relay_from_message(msg);
}

static int envdaq_mqtt_subscribe_topic(void)
{
	mqtt_sub_topic.topic.utf8 = (uint8_t *)mqtt_subscription_topic;
	mqtt_sub_topic.topic.size = strlen(mqtt_subscription_topic);
	mqtt_sub_topic.qos = MQTT_QOS_2_EXACTLY_ONCE;

	mqtt_sub_list.list = &mqtt_sub_topic;
	mqtt_sub_list.list_count = 1;
	mqtt_sub_list.message_id = mqtt_next_msg_id++;

	return mqtt_subscribe(&mqtt_client_ctx, &mqtt_sub_list);
}

static void envdaq_mqtt_evt_handler(struct mqtt_client *client, const struct mqtt_evt *evt)
{
	switch (evt->type) {
	case MQTT_EVT_CONNACK:
		if (evt->result != 0) {
			LOG_WRN("MQTT CONNACK rejected, reason=%d", evt->result);
			mqtt_connecting = false;
			break;
		}

		mqtt_connected = true;
		mqtt_connecting = false;
		LOG_INF("MQTT connected");
		if (envdaq_mqtt_subscribe_topic() < 0) {
			LOG_WRN("MQTT subscribe failed");
		}
		break;

	case MQTT_EVT_DISCONNECT:
		mqtt_connected = false;
		mqtt_connecting = false;
		LOG_WRN("MQTT disconnected: %d", evt->result);
		break;

	case MQTT_EVT_SUBACK:
		LOG_INF("MQTT subscribed to %s (id=%u)", mqtt_subscription_topic,
			evt->param.suback.message_id);
		break;

	case MQTT_EVT_PUBLISH:
		{
			uint32_t len = evt->param.publish.message.payload.len;
			char msg[192];
			int ret;
			struct mqtt_puback_param ack;
			struct mqtt_pubrec_param rec;

			if (len >= sizeof(msg)) {
				len = sizeof(msg) - 1;
			}

			ret = mqtt_readall_publish_payload(client, (uint8_t *)msg, len);
			if (ret >= 0) {
				msg[len] = '\0';
				k_mutex_lock(&control_msg_lock, K_FOREVER);
				snprintk(control_latest_msg, sizeof(control_latest_msg), "%s", msg);
				control_latest_valid = true;
				k_mutex_unlock(&control_msg_lock);
				k_work_reschedule(&control_apply_work, K_MSEC(CONTROL_APPLY_DEBOUNCE_MS));
			}

			if (evt->param.publish.message.topic.qos == MQTT_QOS_1_AT_LEAST_ONCE) {
				ack.message_id = evt->param.publish.message_id;
				(void)mqtt_publish_qos1_ack(client, &ack);
			} else if (evt->param.publish.message.topic.qos == MQTT_QOS_2_EXACTLY_ONCE) {
				rec.message_id = evt->param.publish.message_id;
				(void)mqtt_publish_qos2_receive(client, &rec);
			}
		}
		break;

	case MQTT_EVT_PUBREC:
		{
			struct mqtt_pubrel_param rel = {
				.message_id = evt->param.pubrec.message_id,
			};

			(void)mqtt_publish_qos2_release(client, &rel);
		}
		break;

	case MQTT_EVT_PUBREL:
		{
			struct mqtt_pubcomp_param comp = {
				.message_id = evt->param.pubrel.message_id,
			};

			(void)mqtt_publish_qos2_complete(client, &comp);
		}
		break;

	case MQTT_EVT_PUBCOMP:
		mqtt_pubcomp_count++;
		LOG_DBG("MQTT publish complete id=%u", evt->param.pubcomp.message_id);
		if ((k_uptime_get() - mqtt_pubcomp_last_info_ms) > (30 * MSEC_PER_SEC)) {
			mqtt_pubcomp_last_info_ms = k_uptime_get();
			LOG_INF("MQTT publish healthy, pubcomp count=%u", mqtt_pubcomp_count);
		}
		break;

	default:
		break;
	}
}

static int envdaq_mqtt_resolve_broker(void)
{
	struct zsock_addrinfo hints = { 0 };
	struct zsock_addrinfo *res;
	char addr_str[NET_IPV4_ADDR_LEN] = "n/a";
	char port_str[8];
	int ret;
	struct sockaddr_in *broker4;

	if (g_cfg.mqtt_endpoint[0] == '\0') {
		return -EINVAL;
	}

	snprintk(port_str, sizeof(port_str), "%u", g_cfg.mqtt_port);

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	ret = zsock_getaddrinfo(g_cfg.mqtt_endpoint, port_str, &hints, &res);
	if (ret != 0) {
		LOG_WRN("MQTT broker resolve failed host=%s port=%s rc=%d",
			g_cfg.mqtt_endpoint, port_str, ret);
		return -EHOSTUNREACH;
	}

	memset(&mqtt_broker, 0, sizeof(mqtt_broker));
	memcpy(&mqtt_broker, res->ai_addr, res->ai_addrlen);
	broker4 = (struct sockaddr_in *)&mqtt_broker;
	(void)net_addr_ntop(AF_INET, &broker4->sin_addr, addr_str, sizeof(addr_str));
	LOG_INF("MQTT broker %s:%s resolved to %s",
		g_cfg.mqtt_endpoint, port_str, addr_str);
	zsock_freeaddrinfo(res);

	return 0;
}

static int envdaq_mqtt_connect(void)
{
	int fd;
	int ret;
	char port_str[8];
	struct zsock_timeval tv = {
		.tv_sec = 0,
		.tv_usec = MQTT_INPUT_TIMEOUT_MS * USEC_PER_MSEC,
	};

	mqtt_client_init(&mqtt_client_ctx);

	ret = envdaq_mqtt_resolve_broker();
	if (ret < 0) {
		return ret;
	}

	mqtt_client_ctx.broker = &mqtt_broker;
	mqtt_client_ctx.evt_cb = envdaq_mqtt_evt_handler;
	mqtt_client_ctx.client_id.utf8 = (uint8_t *)net_hostname_get();
	mqtt_client_ctx.client_id.size = strlen(net_hostname_get());
	envdaq_refresh_mqtt_auth();
	mqtt_client_ctx.user_name = &mqtt_user;
	mqtt_client_ctx.password = &mqtt_pass;
	mqtt_client_ctx.protocol_version = MQTT_VERSION_5_0;
	mqtt_client_ctx.rx_buf = mqtt_rx_buffer;
	mqtt_client_ctx.rx_buf_size = sizeof(mqtt_rx_buffer);
	mqtt_client_ctx.tx_buf = mqtt_tx_buffer;
	mqtt_client_ctx.tx_buf_size = sizeof(mqtt_tx_buffer);
	mqtt_client_ctx.keepalive = MQTT_KEEPALIVE_SECONDS;
	mqtt_client_ctx.transport.type = MQTT_TRANSPORT_NON_SECURE;
	mqtt_client_ctx.clean_session = 1U;

	if (mqtt_client_ctx.client_id.size == 0U) {
		LOG_WRN("MQTT connect blocked: empty client id");
		return -EINVAL;
	}

	if (!envdaq_mqtt_config_ready()) {
		LOG_WRN("MQTT connect blocked: setup incomplete");
		return -EINVAL;
	}

	snprintk(port_str, sizeof(port_str), "%u", g_cfg.mqtt_port);

	LOG_INF("MQTT connecting: host=%s port=%s client_id=%s",
		g_cfg.mqtt_endpoint,
		port_str,
		net_hostname_get());

	ret = mqtt_connect(&mqtt_client_ctx);
	if (ret < 0) {
		LOG_WRN("mqtt_connect failed: %d", ret);
		if (ret == -ETIMEDOUT) {
			LOG_WRN("TCP connect timeout: no SYN-ACK from broker (port blocked, service down, or path filtered)");
		}
		if (ret == -ENOENT) {
			LOG_WRN("ENOENT from connect: increase CONFIG_NET_MAX_CONN/CONFIG_NET_MAX_CONTEXTS");
		}
		mqtt_connecting = false;
		return ret;
	}

	mqtt_connecting = true;
	mqtt_connack_deadline_ms = k_uptime_get() + MQTT_CONNACK_TIMEOUT_MS;
	LOG_INF("MQTT TCP connected, waiting for CONNACK");

	fd = envdaq_mqtt_socket_fd(&mqtt_client_ctx);
	if (fd >= 0) {
		(void)zsock_setsockopt(fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_RCVTIMEO, &tv, sizeof(tv));
	}

	return 0;
}

static void envdaq_mqtt_service(void)
{
	int64_t now = k_uptime_get();
	int ret;

	if (!envdaq_wait_for_ipv4(100)) {
		return;
	}

	if (!envdaq_mqtt_config_ready()) {
		if ((k_uptime_get() - mqtt_setup_warn_ms) > (30 * MSEC_PER_SEC)) {
			mqtt_setup_warn_ms = k_uptime_get();
			LOG_WRN("MQTT setup is incomplete; run hikit setup mqtt <endpoint> <port> <username> <password>");
		}
		return;
	}

	if (!mqtt_connected && !mqtt_connecting) {
		if (now < mqtt_next_retry_ms) {
			return;
		}

		mqtt_next_retry_ms = now + MQTT_RETRY_INTERVAL_MS;
		k_mutex_lock(&mqtt_lock, K_FOREVER);
		ret = envdaq_mqtt_connect();
		k_mutex_unlock(&mqtt_lock);
		if (ret < 0) {
			return;
		}
	}

	if (mqtt_connecting && (now > mqtt_connack_deadline_ms)) {
		LOG_WRN("MQTT CONNACK timeout, reconnecting");
		k_mutex_lock(&mqtt_lock, K_FOREVER);
		mqtt_connecting = false;
		mqtt_connected = false;
		(void)mqtt_abort(&mqtt_client_ctx);
		k_mutex_unlock(&mqtt_lock);
		mqtt_next_retry_ms = now + MQTT_RETRY_INTERVAL_MS;
		return;
	}

	if (!mqtt_connected && !mqtt_connecting) {
		return;
	}

	k_mutex_lock(&mqtt_lock, K_FOREVER);
	ret = mqtt_input(&mqtt_client_ctx);
	if ((ret < 0) && (ret != -EAGAIN) && (ret != -ETIMEDOUT)) {
		LOG_WRN("mqtt_input error: %d", ret);
		mqtt_connected = false;
		mqtt_connecting = false;
		(void)mqtt_abort(&mqtt_client_ctx);
		k_mutex_unlock(&mqtt_lock);
		return;
	}

	ret = mqtt_live(&mqtt_client_ctx);
	if ((ret < 0) && (ret != -EAGAIN)) {
		LOG_WRN("mqtt_live error: %d", ret);
		mqtt_connected = false;
		mqtt_connecting = false;
		(void)mqtt_abort(&mqtt_client_ctx);
		k_mutex_unlock(&mqtt_lock);
		return;
	}
	k_mutex_unlock(&mqtt_lock);
}

static void envdaq_mqtt_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (1) {
		envdaq_mqtt_service();
		k_msleep(100);
	}
}

static void envdaq_status_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (1) {
		(void)envdaq_mqtt_publish_status();
		k_sleep(K_SECONDS(1));
	}
}

static void envdaq_dht_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (1) {
		struct sensor_value temp = { 0 };
		struct sensor_value humidity = { 0 };
		bool temp_ok = false;
		bool humidity_ok = false;

		if ((dht_dev != NULL) && device_is_ready(dht_dev) && sensor_sample_fetch(dht_dev) == 0) {
			if (sensor_channel_get(dht_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp) == 0) {
				temp_ok = true;
			}

			if (sensor_channel_get(dht_dev, SENSOR_CHAN_HUMIDITY, &humidity) == 0) {
				humidity_ok = true;
			}
		}

		k_mutex_lock(&dht_cache_lock, K_FOREVER);
		if (temp_ok) {
			dht_temp_cache = temp;
			dht_temp_valid = true;
			dht_last_sample_ms = k_uptime_get();
		}
		if (humidity_ok) {
			dht_humidity_cache = humidity;
			dht_humidity_valid = true;
			dht_last_sample_ms = k_uptime_get();
		}
		k_mutex_unlock(&dht_cache_lock);

		k_sleep(K_SECONDS(DHT_SAMPLE_INTERVAL_SEC));
	}
}

static int envdaq_gpio_init(void)
{
	int ret;

	if (!gpio_is_ready_dt(&led) || !gpio_is_ready_dt(&relay)) {
		LOG_ERR("GPIO devices not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&relay, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		return ret;
	}

	atomic_set(&relay_target_state, 0);
	atomic_set(&relay_state_dirty, 0);
	k_thread_create(&relay_thread_data, relay_thread_stack,
			RELAY_THREAD_STACK_SIZE,
			envdaq_relay_thread,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(RELAY_THREAD_PRIORITY),
			0, K_NO_WAIT);
	k_thread_name_set(&relay_thread_data, "relay_worker");

#if DT_NODE_HAS_STATUS(BUTTON_NODE, okay)
	if (gpio_is_ready_dt(&button)) {
		ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
		if (ret == 0) {
			envdaq_check_factory_reset_on_boot();
			k_work_init_delayable(&button_work, envdaq_button_work_handler);
			gpio_init_callback(&button_cb, envdaq_button_callback, BIT(button.pin));
			(void)gpio_add_callback(button.port, &button_cb);
			(void)gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
		}
	}
#endif

	k_thread_create(&led_thread_data, led_thread_stack,
			LED_THREAD_STACK_SIZE,
			envdaq_led_thread,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(LED_THREAD_PRIORITY),
			0, K_NO_WAIT);
	k_thread_name_set(&led_thread_data, "led_worker");

	return 0;
}

static int envdaq_load_eeprom_config(void)
{
	int ret;
	uint8_t device_id[EEPROM_DEVICE_ID_LEN];
	uint8_t mqtt_username_raw[EEPROM_MQTT_USERNAME_LEN];
	uint8_t mqtt_password_raw[EEPROM_MQTT_PASSWORD_LEN];
	uint8_t mqtt_endpoint_raw[EEPROM_MQTT_ENDPOINT_LEN];
	uint16_t mqtt_port_raw = 0;
	uint8_t board_initialized = EEPROM_BOARD_INIT_UNINITIALIZED;

	if (!device_is_ready(eeprom_dev)) {
		LOG_WRN("EEPROM device not ready, fallback defaults");
		return -ENODEV;
	}

	ret = eeprom_read(eeprom_dev, EEPROM_DEVICE_ID_OFFSET, device_id, sizeof(device_id));
	if (ret == 0) {
		memcpy(g_cfg.device_id, device_id, sizeof(g_cfg.device_id));
	}

	ret = eeprom_read(eeprom_dev, EEPROM_MQTT_USERNAME_OFFSET,
			  mqtt_username_raw, sizeof(mqtt_username_raw));
	if ((ret == 0) && !envdaq_is_erased_or_zero(mqtt_username_raw, sizeof(mqtt_username_raw))) {
		memcpy(g_cfg.mqtt_username, mqtt_username_raw, EEPROM_MQTT_USERNAME_LEN);
		g_cfg.mqtt_username[EEPROM_MQTT_USERNAME_LEN] = '\0';
		sanitize_ascii(g_cfg.mqtt_username, EEPROM_MQTT_USERNAME_LEN);
	} else {
		g_cfg.mqtt_username[0] = '\0';
	}

	ret = eeprom_read(eeprom_dev, EEPROM_MQTT_PASSWORD_OFFSET,
			  mqtt_password_raw, sizeof(mqtt_password_raw));
	if ((ret == 0) && !envdaq_is_erased_or_zero(mqtt_password_raw, sizeof(mqtt_password_raw))) {
		memcpy(g_cfg.mqtt_password, mqtt_password_raw, EEPROM_MQTT_PASSWORD_LEN);
		g_cfg.mqtt_password[EEPROM_MQTT_PASSWORD_LEN] = '\0';
		sanitize_ascii(g_cfg.mqtt_password, EEPROM_MQTT_PASSWORD_LEN);
	} else {
		g_cfg.mqtt_password[0] = '\0';
	}

	ret = eeprom_read(eeprom_dev, EEPROM_MQTT_ENDPOINT_OFFSET,
			  mqtt_endpoint_raw, sizeof(mqtt_endpoint_raw));
	if ((ret == 0) && !envdaq_is_erased_or_zero(mqtt_endpoint_raw, sizeof(mqtt_endpoint_raw))) {
		memcpy(g_cfg.mqtt_endpoint, mqtt_endpoint_raw, EEPROM_MQTT_ENDPOINT_LEN);
		g_cfg.mqtt_endpoint[EEPROM_MQTT_ENDPOINT_LEN] = '\0';
		sanitize_ascii(g_cfg.mqtt_endpoint, EEPROM_MQTT_ENDPOINT_LEN);
	} else {
		g_cfg.mqtt_endpoint[0] = '\0';
	}

	ret = eeprom_read(eeprom_dev, EEPROM_MQTT_ENDPOINT_PORT_OFFSET,
			  &mqtt_port_raw, sizeof(mqtt_port_raw));
	if ((ret == 0) && (mqtt_port_raw != 0U) && (mqtt_port_raw != 0xFFFFU)) {
		g_cfg.mqtt_port = mqtt_port_raw;
	} else {
		g_cfg.mqtt_port = ENVDAQ_DEFAULT_MQTT_PORT;
	}

	ret = eeprom_read(eeprom_dev, EEPROM_APP_VERSION_OFFSET,
			  g_cfg.app_version, EEPROM_APP_VERSION_LEN);
	if (ret < 0) {
		memset(g_cfg.app_version, 0xFF, EEPROM_APP_VERSION_LEN);
	}

	ret = eeprom_read(eeprom_dev, EEPROM_BOARD_INITIALIZED_OFFSET,
			  &board_initialized, 1);
	if (ret == 0) {
		g_cfg.board_initialized = board_initialized;
	} else {
		g_cfg.board_initialized = EEPROM_BOARD_INIT_UNINITIALIZED;
	}

	envdaq_refresh_mqtt_auth();
	(void)envdaq_sync_app_version_to_eeprom();

	LOG_INF("EEPROM config loaded: init=0x%02x, mqtt endpoint=%s, port=%u",
		g_cfg.board_initialized,
		g_cfg.mqtt_endpoint[0] ? g_cfg.mqtt_endpoint : "<empty>",
		g_cfg.mqtt_port);

	if (g_cfg.board_initialized != EEPROM_BOARD_INIT_INITIALIZED) {
		LOG_WRN("Board not initialized, run: hikit setup mqtt <endpoint> <port> <username> <password>");
	}

	return 0;
}

static void envdaq_set_hostname_from_mac(void)
{
	const struct net_linkaddr *lladdr = envdaq_get_iface_link_addr();
	char hostname[32];
	int ret;

	if ((lladdr == NULL) || (lladdr->len < 6U)) {
		LOG_WRN("No valid iface MAC for hostname, keeping default");
		return;
	}

	snprintk(hostname, sizeof(hostname), "HIKIT-SC1-%02X%02X%02X",
		 lladdr->addr[3], lladdr->addr[4], lladdr->addr[5]);

	ret = net_hostname_set(hostname, strlen(hostname));
	if (ret < 0) {
		LOG_WRN("Failed to set hostname: %d", ret);
		return;
	}

	LOG_INF("Hostname set: %s", net_hostname_get());
}

static void envdaq_init_mqtt_topic(void)
{
	char device_id_str[2 * EEPROM_DEVICE_ID_LEN + 1];

	envdaq_format_device_id(device_id_str, sizeof(device_id_str));
	snprintk(mqtt_status_topic, sizeof(mqtt_status_topic), "/envdaq/%s/status", device_id_str);
	snprintk(mqtt_subscription_topic, sizeof(mqtt_subscription_topic), "/envdaq/%s/subscription",
		 device_id_str);
}

static int envdaq_network_init(void)
{
	const struct device *net_dev;

	eth_iface = net_if_get_default();
	if (eth_iface == NULL) {
		LOG_ERR("No default net iface");
		return -ENODEV;
	}

	net_dev = net_if_get_device(eth_iface);
	if (net_dev == NULL) {
		LOG_ERR("No net device bound to default iface");
		return -ENODEV;
	}

	LOG_INF("net iface device: %s (ready=%d)",
		net_dev->name, device_is_ready(net_dev));
	if (!device_is_ready(net_dev)) {
		LOG_ERR("Net device not ready");
		return -ENODEV;
	}

	envdaq_set_hostname_from_mac();
	envdaq_init_mqtt_topic();
	LOG_INF("Network auto-init is managed by NET_CONFIG");

	return 0;
}

static int cmd_hikit_relay(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	if (argc < 2) {
		shell_help(sh);
		return -EINVAL;
	}

	if (strcmp(argv[1], "on") == 0) {
		envdaq_request_relay_state(1);
		ret = 0;
	} else if (strcmp(argv[1], "off") == 0) {
		envdaq_request_relay_state(0);
		ret = 0;
	} else if (strcmp(argv[1], "toggle") == 0) {
		envdaq_toggle_relay_state();
		ret = 0;
	} else if (strcmp(argv[1], "status") == 0) {
		ret = gpio_pin_get_dt(&relay);
		if (ret >= 0) {
			shell_print(sh, "relay is %s", ret ? "on" : "off");
			return 0;
		}
	} else {
		shell_help(sh);
		return -EINVAL;
	}

	if (ret < 0) {
		shell_error(sh, "relay command failed: %d", ret);
		return ret;
	}

	if (app_echo_enabled) {
		shell_print(sh, "relay command ok");
	}

	return 0;
}

static int cmd_hikit_led(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	if (argc < 2) {
		shell_help(sh);
		return -EINVAL;
	}

	if (strcmp(argv[1], "on") == 0) {
		ret = gpio_pin_set_dt(&led, 1);
	} else if (strcmp(argv[1], "off") == 0) {
		ret = gpio_pin_set_dt(&led, 0);
	} else if (strcmp(argv[1], "toggle") == 0) {
		ret = gpio_pin_toggle_dt(&led);
	} else {
		shell_help(sh);
		return -EINVAL;
	}

	if (ret < 0) {
		shell_error(sh, "led command failed: %d", ret);
		return ret;
	}

	if (app_echo_enabled) {
		shell_print(sh, "led command ok");
	}

	return 0;
}

static int cmd_hikit_light(const struct shell *sh, size_t argc, char **argv)
{
	unsigned long count = 1;

	if (!device_is_ready(light_dev)) {
		shell_error(sh, "BH1750 not ready");
		return -ENODEV;
	}

	if (argc > 1) {
		count = strtoul(argv[1], NULL, 10);
		if (count == 0) {
			count = 1;
		}
	}

	for (unsigned long i = 0; i < count; i++) {
		struct sensor_value lux;
		int ret = sensor_sample_fetch(light_dev);

		if (ret < 0) {
			shell_error(sh, "BH1750 sample fetch failed: %d", ret);
			return ret;
		}

		ret = sensor_channel_get(light_dev, SENSOR_CHAN_LIGHT, &lux);
		if (ret < 0) {
			shell_error(sh, "BH1750 channel get failed: %d", ret);
			return ret;
		}

		shell_print(sh, "lux[%lu] = %d.%06d", i, lux.val1, lux.val2);
		if (i + 1 < count) {
			k_msleep(200);
		}
	}

	return 0;
}

static int cmd_hikit_dht(const struct shell *sh, size_t argc, char **argv)
{
	if ((dht_dev == NULL) || !device_is_ready(dht_dev)) {
		shell_error(sh, "DHT11 not ready");
		return -ENODEV;
	}

	if ((argc == 1) || (strcmp(argv[1], "status") == 0)) {
		char temp_str[24] = "null";
		char humidity_str[24] = "null";
		struct sensor_value temp = { 0 };
		struct sensor_value humidity = { 0 };
		bool temp_valid;
		bool humidity_valid;
		int64_t last_sample_ms;

		k_mutex_lock(&dht_cache_lock, K_FOREVER);
		temp_valid = dht_temp_valid;
		humidity_valid = dht_humidity_valid;
		if (temp_valid) {
			temp = dht_temp_cache;
		}
		if (humidity_valid) {
			humidity = dht_humidity_cache;
		}
		last_sample_ms = dht_last_sample_ms;
		k_mutex_unlock(&dht_cache_lock);

		if (temp_valid) {
			envdaq_format_sensor_2dp(&temp, temp_str, sizeof(temp_str));
		}
		if (humidity_valid) {
			envdaq_format_sensor_2dp(&humidity, humidity_str, sizeof(humidity_str));
		}

		shell_print(sh, "dht cache temp=%s humidity=%s last_sample_ms=%lld",
			    temp_str, humidity_str, last_sample_ms);
		return 0;
	}

	if (strcmp(argv[1], "read") == 0) {
		unsigned long count = 1;

		if (argc > 2) {
			count = strtoul(argv[2], NULL, 10);
			if (count == 0) {
				count = 1;
			}
		}

		for (unsigned long i = 0; i < count; i++) {
			struct sensor_value temp = { 0 };
			struct sensor_value humidity = { 0 };
			char temp_str[24] = "null";
			char humidity_str[24] = "null";
			int ret;

			ret = sensor_sample_fetch(dht_dev);
			if (ret < 0) {
				shell_error(sh, "DHT11 sample fetch failed: %d", ret);
				return ret;
			}

			ret = sensor_channel_get(dht_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
			if (ret == 0) {
				envdaq_format_sensor_2dp(&temp, temp_str, sizeof(temp_str));
			}

			ret = sensor_channel_get(dht_dev, SENSOR_CHAN_HUMIDITY, &humidity);
			if (ret == 0) {
				envdaq_format_sensor_2dp(&humidity, humidity_str, sizeof(humidity_str));
			}

			k_mutex_lock(&dht_cache_lock, K_FOREVER);
			dht_temp_cache = temp;
			dht_humidity_cache = humidity;
			dht_temp_valid = true;
			dht_humidity_valid = true;
			dht_last_sample_ms = k_uptime_get();
			k_mutex_unlock(&dht_cache_lock);

			shell_print(sh, "dht[%lu] temp=%s humidity=%s",
				    i, temp_str, humidity_str);

			if (i + 1 < count) {
				k_sleep(K_SECONDS(1));
			}
		}

		return 0;
	}

	shell_help(sh);
	return -EINVAL;
}

static int cmd_hikit_eeprom(const struct shell *sh, size_t argc, char **argv)
{
	if (!device_is_ready(eeprom_dev)) {
		shell_error(sh, "EEPROM device not ready");
		return -ENODEV;
	}

	if (argc < 2) {
		shell_help(sh);
		return -EINVAL;
	}

	if (strcmp(argv[1], "dump") == 0) {
		off_t offset = 0;
		size_t len = 256;
		uint8_t buf[16];

		if (argc > 2) {
			offset = (off_t)strtol(argv[2], NULL, 0);
		}
		if (argc > 3) {
			len = (size_t)strtoul(argv[3], NULL, 0);
		}

		for (size_t pos = 0; pos < len; pos += sizeof(buf)) {
			size_t chunk = MIN(sizeof(buf), len - pos);
			int ret = eeprom_read(eeprom_dev, offset + (off_t)pos, buf, chunk);
			char line[128];
			size_t w = 0;

			if (ret < 0) {
				shell_error(sh, "eeprom dump failed at 0x%lx: %d",
					    (long)(offset + (off_t)pos), ret);
				return ret;
			}

			w += snprintk(line + w, sizeof(line) - w, "%08lX: ",
				      (long)(offset + (off_t)pos));

			for (size_t i = 0; i < sizeof(buf); i++) {
				if (i < chunk) {
					w += snprintk(line + w, sizeof(line) - w, "%02x ", buf[i]);
				} else {
					w += snprintk(line + w, sizeof(line) - w, "   ");
				}

				if (i == 7) {
					w += snprintk(line + w, sizeof(line) - w, " ");
				}
			}

			w += snprintk(line + w, sizeof(line) - w, "|");
			for (size_t i = 0; i < sizeof(buf); i++) {
				char c = ' ';

				if (i < chunk) {
					c = isprint((unsigned char)buf[i]) ? (char)buf[i] : '.';
				}

				w += snprintk(line + w, sizeof(line) - w, "%c", c);

				if (i == 7) {
					w += snprintk(line + w, sizeof(line) - w, " ");
				}
			}

			w += snprintk(line + w, sizeof(line) - w, "|");
			shell_print(sh, "%s", line);
		}

		return 0;
	}

	if (strcmp(argv[1], "read") == 0) {
		uint8_t value;
		off_t offset;
		int ret;

		if (argc != 3) {
			shell_help(sh);
			return -EINVAL;
		}

		offset = (off_t)strtol(argv[2], NULL, 0);
		ret = eeprom_read(eeprom_dev, offset, &value, 1);
		if (ret < 0) {
			shell_error(sh, "eeprom read failed: %d", ret);
			return ret;
		}

		shell_print(sh, "eeprom[0x%lx] = 0x%02x", (long)offset, value);
		return 0;
	}

	if (strcmp(argv[1], "write") == 0) {
		off_t offset;
		uint8_t value;
		int ret;

		if (argc != 4) {
			shell_help(sh);
			return -EINVAL;
		}

		offset = (off_t)strtol(argv[2], NULL, 0);
		value = (uint8_t)strtoul(argv[3], NULL, 0);

		ret = eeprom_write(eeprom_dev, offset, &value, 1);
		if (ret < 0) {
			shell_error(sh, "eeprom write failed: %d", ret);
			return ret;
		}

		shell_print(sh, "write ok: eeprom[0x%lx] = 0x%02x", (long)offset, value);
		return 0;
	}

	shell_help(sh);
	return -EINVAL;
}

static int cmd_hikit_sysinfo(const struct shell *sh, size_t argc, char **argv)
{
	char ipstr[NET_IPV4_ADDR_LEN] = "n/a";
	char iface_mac_str[24] = "n/a";
	struct net_in_addr *addr;
	const struct net_linkaddr *lladdr;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	addr = (eth_iface != NULL) ? net_if_ipv4_get_global_addr(eth_iface, NET_ADDR_PREFERRED) : NULL;
	if (addr != NULL) {
		format_ipv4(addr, ipstr, sizeof(ipstr));
	}

	lladdr = envdaq_get_iface_link_addr();
	format_mac((lladdr != NULL) ? lladdr->addr : NULL,
		   (lladdr != NULL) ? lladdr->len : 0U,
		   iface_mac_str, sizeof(iface_mac_str));

	shell_print(sh, "board: %s", CONFIG_BOARD);
	shell_print(sh, "uptime: %lld ms", k_uptime_get());
	shell_print(sh, "device_id: %02x:%02x:%02x:%02x:%02x:%02x",
		    g_cfg.device_id[0], g_cfg.device_id[1], g_cfg.device_id[2],
		    g_cfg.device_id[3], g_cfg.device_id[4], g_cfg.device_id[5]);
	shell_print(sh, "ipv4: %s", ipstr);
	shell_print(sh, "iface_mac: %s", iface_mac_str);

	return 0;
}

static int cmd_hikit_nettest(const struct shell *sh, size_t argc, char **argv)
{
	char ipstr[NET_IPV4_ADDR_LEN] = "n/a";
	char port_str[8];
	struct net_in_addr *addr;
	int ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (eth_iface == NULL) {
		shell_error(sh, "network interface unavailable");
		return -ENODEV;
	}

	addr = net_if_ipv4_get_global_addr(eth_iface, NET_ADDR_PREFERRED);
	if (addr != NULL) {
		format_ipv4(addr, ipstr, sizeof(ipstr));
	}

	shell_print(sh, "nettest: connectivity diagnostics (identity details in 'hikit sysinfo')");
	shell_print(sh, "iface up: %s", net_if_is_up(eth_iface) ? "yes" : "no");
	shell_print(sh, "ipv4 assigned: %s", addr != NULL ? "yes" : "no");
	shell_print(sh, "ipv4 addr: %s", ipstr);
	snprintk(port_str, sizeof(port_str), "%u", g_cfg.mqtt_port);
	shell_print(sh, "mqtt broker: %s:%s",
		    g_cfg.mqtt_endpoint[0] ? g_cfg.mqtt_endpoint : "<empty>",
		    port_str);
	shell_print(sh, "mqtt connected: %s", mqtt_connected ? "yes" : "no");
	shell_print(sh, "mqtt connecting: %s", mqtt_connecting ? "yes" : "no");
	shell_print(sh, "board initialized: %s",
		    g_cfg.board_initialized == EEPROM_BOARD_INIT_INITIALIZED ? "yes" : "no");

	ret = envdaq_mqtt_resolve_broker();
	if (ret == 0) {
		shell_print(sh, "mqtt resolve: ok");
	} else {
		shell_error(sh, "mqtt resolve: failed (%d)", ret);
		return ret;
	}

	return 0;
}

#if defined(CONFIG_THREAD_MONITOR)
static void thread_dump_cb(const struct k_thread *thread, void *user_data)
{
	const struct shell *sh = user_data;
	char state[32];
	const char *name;

	name = k_thread_name_get((k_tid_t)thread);
	k_thread_state_str((k_tid_t)thread, state, sizeof(state));

	shell_print(sh, "thread=%p prio=%d state=%s name=%s",
		    thread,
		    k_thread_priority_get((k_tid_t)thread),
		    state,
		    name ? name : "(unnamed)");
}
#endif

static int cmd_hikit_threads(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_THREAD_MONITOR)
	k_thread_foreach(thread_dump_cb, (void *)sh);
	return 0;
#else
	shell_error(sh, "thread monitor is disabled");
	return -ENOTSUP;
#endif
}

static int cmd_hikit_echo(const struct shell *sh, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_help(sh);
		return -EINVAL;
	}

	if (strcmp(argv[1], "on") == 0) {
		app_echo_enabled = true;
		shell_print(sh, "echo help enabled");
		return 0;
	}

	if (strcmp(argv[1], "off") == 0) {
		app_echo_enabled = false;
		shell_print(sh, "echo help disabled");
		return 0;
	}

	shell_help(sh);
	return -EINVAL;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_hikit,
	SHELL_CMD_ARG(relay, NULL, "Control relay (on|off|toggle|status)", cmd_hikit_relay, 2, 0),
	SHELL_CMD_ARG(led, NULL, "Control LED (on|off|toggle, temporary; status thread overrides)", cmd_hikit_led, 2, 0),
	SHELL_CMD_ARG(setup, NULL, "Setup mqtt config: status | mqtt <endpoint> <port> <username> <password>", cmd_hikit_setup, 1, 5),
	SHELL_CMD_ARG(light, NULL, "Read BH1750 light sensor [count]", cmd_hikit_light, 1, 1),
	SHELL_CMD_ARG(dht, NULL, "DHT11 operations (status|read [count])", cmd_hikit_dht, 1, 2),
	SHELL_CMD_ARG(eeprom, NULL, "EEPROM operations (read|write|dump)", cmd_hikit_eeprom, 2, 2),
	SHELL_CMD_ARG(sysinfo, NULL, "Show system identity and status", cmd_hikit_sysinfo, 1, 0),
	SHELL_CMD_ARG(nettest, NULL, "Run network connectivity diagnostics", cmd_hikit_nettest, 1, 0),
	SHELL_CMD_ARG(threads, NULL, "Show thread information", cmd_hikit_threads, 1, 0),
	SHELL_CMD_ARG(echo, NULL, "Echo control help (on|off)", cmd_hikit_echo, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(hikit, &sub_hikit, "Hikit SC1 board commands", NULL);

int main(void)
{
	int ret;

	printk("EnvDAQ booting...\n");

	ret = envdaq_gpio_init();
	if (ret < 0) {
		LOG_ERR("GPIO init failed: %d", ret);
	}

	ret = envdaq_load_eeprom_config();
	if (ret < 0) {
		LOG_WRN("EEPROM init fallback used: %d", ret);
	}

	ret = envdaq_network_init();
	if (ret < 0) {
		LOG_ERR("Network init failed: %d", ret);
	}

	envdaq_require_time_sync();

	k_work_init_delayable(&control_apply_work, envdaq_control_apply_work_handler);

	k_thread_create(&mqtt_thread_data, mqtt_thread_stack,
			MQTT_THREAD_STACK_SIZE,
			envdaq_mqtt_thread,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(MQTT_THREAD_PRIORITY),
			0, K_NO_WAIT);
	k_thread_name_set(&mqtt_thread_data, "mqtt_worker");

	k_thread_create(&status_thread_data, status_thread_stack,
			STATUS_THREAD_STACK_SIZE,
			envdaq_status_thread,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(STATUS_THREAD_PRIORITY),
			0, K_NO_WAIT);
	k_thread_name_set(&status_thread_data, "status_worker");

	k_thread_create(&dht_thread_data, dht_thread_stack,
			DHT_THREAD_STACK_SIZE,
			envdaq_dht_thread,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(DHT_THREAD_PRIORITY),
			0, K_NO_WAIT);
	k_thread_name_set(&dht_thread_data, "dht_worker");

	k_thread_create(&time_sync_thread_data, time_sync_thread_stack,
			TIME_SYNC_THREAD_STACK_SIZE,
			envdaq_time_sync_thread,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(TIME_SYNC_THREAD_PRIORITY),
			0, K_NO_WAIT);
	k_thread_name_set(&time_sync_thread_data, "time_sync_worker");

	LOG_INF("EnvDAQ ready: use 'hikit' command");
	printk("EnvDAQ ready. Type 'hikit'.\n");

	while (1) {
		k_sleep(K_SECONDS(10));
	}
}