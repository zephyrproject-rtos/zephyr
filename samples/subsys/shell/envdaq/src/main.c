/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/drivers/eeprom.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/dhcpv4.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>

LOG_MODULE_REGISTER(envdaq, LOG_LEVEL_INF);

#define LED_NODE DT_ALIAS(led0)
#define RELAY_NODE DT_ALIAS(relay)

#define EEPROM_NODE DT_NODELABEL(eeprom0)
#define BH1750_NODE DT_NODELABEL(bh1750)

#define DHCP_TIMEOUT_SEC 15

#define EEPROM_MAC_OFFSET 0
#define EEPROM_DEVICE_ID_OFFSET 6
#define EEPROM_DEVICE_ID_LEN 16
#define EEPROM_CRED_OFFSET 22
#define EEPROM_CRED_LEN 32

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);
static const struct gpio_dt_spec relay = GPIO_DT_SPEC_GET(RELAY_NODE, gpios);

static const struct device *eeprom_dev = DEVICE_DT_GET(EEPROM_NODE);
static const struct device *light_dev = DEVICE_DT_GET(BH1750_NODE);

static struct net_if *eth_iface;
static bool fallback_static_ipv4;
static bool app_echo_enabled = true;

struct app_config {
	uint8_t mac[6];
	char device_id[EEPROM_DEVICE_ID_LEN + 1];
	char credentials[EEPROM_CRED_LEN + 1];
};

static struct app_config g_cfg = {
	.mac = { 0x00, 0x08, 0xdc, 0x00, 0x00, 0x01 },
	.device_id = "hikit-sc1",
	.credentials = "",
};

static bool is_erased(const uint8_t *buf, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		if (buf[i] != 0xFF) {
			return false;
		}
	}

	return true;
}

static bool is_valid_mac(const uint8_t *mac)
{
	if (is_erased(mac, 6)) {
		return false;
	}

	if ((mac[0] & 0x01U) != 0U) {
		return false;
	}

	return true;
}

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

	return 0;
}

static int envdaq_load_eeprom_config(void)
{
	int ret;
	uint8_t mac[6];

	if (!device_is_ready(eeprom_dev)) {
		LOG_WRN("EEPROM device not ready, fallback defaults");
		return -ENODEV;
	}

	ret = eeprom_read(eeprom_dev, EEPROM_MAC_OFFSET, mac, sizeof(mac));
	if ((ret == 0) && is_valid_mac(mac)) {
		memcpy(g_cfg.mac, mac, sizeof(g_cfg.mac));
	}

	ret = eeprom_read(eeprom_dev, EEPROM_DEVICE_ID_OFFSET,
			  g_cfg.device_id, EEPROM_DEVICE_ID_LEN);
	if (ret == 0) {
		g_cfg.device_id[EEPROM_DEVICE_ID_LEN] = '\0';
		sanitize_ascii(g_cfg.device_id, EEPROM_DEVICE_ID_LEN);
	}

	ret = eeprom_read(eeprom_dev, EEPROM_CRED_OFFSET,
			  g_cfg.credentials, EEPROM_CRED_LEN);
	if (ret == 0) {
		g_cfg.credentials[EEPROM_CRED_LEN] = '\0';
		sanitize_ascii(g_cfg.credentials, EEPROM_CRED_LEN);
	}

	LOG_INF("EEPROM config loaded, MAC %02x:%02x:%02x:%02x:%02x:%02x",
		g_cfg.mac[0], g_cfg.mac[1], g_cfg.mac[2],
		g_cfg.mac[3], g_cfg.mac[4], g_cfg.mac[5]);

	return 0;
}

static int envdaq_set_static_ipv4(struct net_if *iface)
{
	struct in_addr ip;
	struct in_addr netmask;
	struct net_if_addr *ifaddr;

	if (net_addr_pton(AF_INET, "192.168.18.1", &ip) != 0) {
		return -EINVAL;
	}

	if (net_addr_pton(AF_INET, "255.255.255.0", &netmask) != 0) {
		return -EINVAL;
	}

	ifaddr = net_if_ipv4_addr_add(iface, &ip, NET_ADDR_MANUAL, 0);
	if (ifaddr == NULL) {
		return -EFAULT;
	}

	if (!net_if_ipv4_set_netmask_by_addr(iface, &ip, &netmask)) {
		return -EFAULT;
	}

	fallback_static_ipv4 = true;
	LOG_WRN("DHCP timeout, fallback to static 192.168.18.1/24");

	return 0;
}

static int envdaq_network_init(void)
{
	int ret;
	struct net_in_addr *addr;

	eth_iface = net_if_get_default();
	if (eth_iface == NULL) {
		LOG_ERR("No default net iface");
		return -ENODEV;
	}

	ret = net_if_set_link_addr(eth_iface, g_cfg.mac, sizeof(g_cfg.mac), NET_LINK_ETHERNET);
	if (ret < 0) {
		LOG_WRN("Failed to set MAC from EEPROM: %d", ret);
	}

	net_if_up(eth_iface);

	net_dhcpv4_start(eth_iface);
	ret = net_mgmt_event_wait_on_iface(eth_iface, NET_EVENT_IPV4_ADDR_ADD,
					   NULL, NULL, NULL,
					   K_SECONDS(DHCP_TIMEOUT_SEC));
	if (ret < 0) {
		ret = envdaq_set_static_ipv4(eth_iface);
		if (ret < 0) {
			return ret;
		}
	}

	addr = net_if_ipv4_get_global_addr(eth_iface, NET_ADDR_PREFERRED);
	if (addr != NULL) {
		char ipstr[NET_IPV4_ADDR_LEN];

		format_ipv4(addr, ipstr, sizeof(ipstr));
		LOG_INF("IPv4 address: %s", ipstr);
	}

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
		ret = gpio_pin_set_dt(&relay, 1);
	} else if (strcmp(argv[1], "off") == 0) {
		ret = gpio_pin_set_dt(&relay, 0);
	} else if (strcmp(argv[1], "toggle") == 0) {
		ret = gpio_pin_toggle_dt(&relay);
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
	struct net_in_addr *addr;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	addr = (eth_iface != NULL) ? net_if_ipv4_get_global_addr(eth_iface, NET_ADDR_PREFERRED) : NULL;
	if (addr != NULL) {
		format_ipv4(addr, ipstr, sizeof(ipstr));
	}

	shell_print(sh, "board: %s", CONFIG_BOARD);
	shell_print(sh, "uptime: %lld ms", k_uptime_get());
	shell_print(sh, "device_id: %s", g_cfg.device_id);
	shell_print(sh, "ipv4: %s%s", ipstr, fallback_static_ipv4 ? " (static fallback)" : "");
	shell_print(sh, "mac: %02x:%02x:%02x:%02x:%02x:%02x",
		    g_cfg.mac[0], g_cfg.mac[1], g_cfg.mac[2],
		    g_cfg.mac[3], g_cfg.mac[4], g_cfg.mac[5]);

	return 0;
}

static int cmd_hikit_nettest(const struct shell *sh, size_t argc, char **argv)
{
	char ipstr[NET_IPV4_ADDR_LEN] = "n/a";
	struct net_in_addr *addr;

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

	shell_print(sh, "iface up: %s", net_if_is_up(eth_iface) ? "yes" : "no");
	shell_print(sh, "ipv4: %s", ipstr);
	shell_print(sh, "mode: %s", fallback_static_ipv4 ? "static fallback" : "dhcp");
	shell_print(sh, "arp: enabled");

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
	SHELL_CMD_ARG(led, NULL, "Control LED (on|off|toggle)", cmd_hikit_led, 2, 0),
	SHELL_CMD_ARG(light, NULL, "Read BH1750 light sensor [count]", cmd_hikit_light, 1, 1),
	SHELL_CMD_ARG(eeprom, NULL, "EEPROM operations (read|write|dump)", cmd_hikit_eeprom, 2, 2),
	SHELL_CMD_ARG(sysinfo, NULL, "Show system information", cmd_hikit_sysinfo, 1, 0),
	SHELL_CMD_ARG(nettest, NULL, "Test network connectivity", cmd_hikit_nettest, 1, 0),
	SHELL_CMD_ARG(threads, NULL, "Show thread information", cmd_hikit_threads, 1, 0),
	SHELL_CMD_ARG(echo, NULL, "Echo control help (on|off)", cmd_hikit_echo, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(hikit, &sub_hikit, "Hikit SC1 board commands", NULL);

int main(void)
{
	int ret;

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

	LOG_INF("EnvDAQ ready: use 'hikit' command");
	printk("EnvDAQ ready. Type 'hikit'.\n");

	while (1) {
		k_sleep(K_SECONDS(1));
	}
}