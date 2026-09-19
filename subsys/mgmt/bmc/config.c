/*
 * SPDX-FileCopyrightText: Copyright (c) 2025-2026 Tenstorrent USA, Inc.
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/bmc.h>
#include <zephyr/mgmt/bmc/config.h>
#include <zephyr/net/hostname.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/util.h>

#include "bmc_internal.h"

LOG_MODULE_DECLARE(bmc, CONFIG_BMC_LOG_LEVEL);

#define BMC_SETTINGS_TREE "bmc"

struct bmc_config_data {
	char hostname[BMC_CONFIG_HOSTNAME_MAX_LEN + 1];
	char admin_password[BMC_CONFIG_PASSWORD_MAX_LEN + 1];
	char ntp_server[BMC_CONFIG_NTP_SERVER_MAX_LEN + 1];
	uint32_t static_ip4;
	uint32_t static_ip4_netmask;
	uint32_t static_ip4_gateway;
	uint8_t use_dhcp4;
	uint8_t use_ntp;
	uint8_t host_auto_poweron;
};

BUILD_ASSERT(sizeof(CONFIG_BMC_DEFAULT_ADMIN_PASSWORD) - 1 <= BMC_CONFIG_PASSWORD_MAX_LEN,
	     "CONFIG_BMC_DEFAULT_ADMIN_PASSWORD is too long");
BUILD_ASSERT(sizeof(CONFIG_BMC_ADMIN_USER) - 1 <= BMC_CONFIG_USER_MAX_LEN,
	     "CONFIG_BMC_ADMIN_USER is too long");
BUILD_ASSERT(sizeof(CONFIG_BMC_DEFAULT_NTP_SERVER) - 1 <= BMC_CONFIG_NTP_SERVER_MAX_LEN,
	     "CONFIG_BMC_DEFAULT_NTP_SERVER is too long");

static struct bmc_config_data config;

/*
 * Settings keys, relative to the BMC_SETTINGS_TREE subtree. Do not change the
 * spelling of an existing key, it would silently orphan stored values.
 */
#define KEY_HOSTNAME   "hostname"
#define KEY_PASSWORD   "password"
#define KEY_IP4        "ip4"
#define KEY_IP4_NM     "ip4_nm"
#define KEY_IP4_GW     "ip4_gw"
#define KEY_DHCP4      "dhcp4"
#define KEY_NTP        "ntp"
#define KEY_NTP_SERVER "ntp_server"
#define KEY_AUTO_PWR   "auto_poweron"

static const char *const config_keys[] = {
	KEY_HOSTNAME, KEY_PASSWORD, KEY_IP4,        KEY_IP4_NM,  KEY_IP4_GW,
	KEY_DHCP4,    KEY_NTP,      KEY_NTP_SERVER, KEY_AUTO_PWR,
};

static size_t copy_string(char *dst, const char *src, size_t dst_size)
{
	size_t src_len = strlen(src);
	size_t copy_len = MIN(src_len, dst_size - 1);

	memcpy(dst, src, copy_len);
	dst[copy_len] = '\0';

	return copy_len;
}

static int ip4_from_string(const char *str, uint32_t *out)
{
	struct in_addr addr;

	if (str == NULL) {
		*out = 0;
		return 0;
	}

	if (net_addr_pton(AF_INET, str, &addr) < 0) {
		LOG_ERR("Could not parse IPv4 address %s", str);
		return -EINVAL;
	}

	*out = addr.s_addr;

	return 0;
}

/* Not reentrant, only used by the shell. */
static __maybe_unused const char *ip4_to_string(uint32_t addr)
{
	static char buf[NET_IPV4_ADDR_LEN];
	struct in_addr in_addr = {.s_addr = addr};

	return net_addr_ntop(AF_INET, &in_addr, buf, sizeof(buf));
}

#if defined(CONFIG_BMC_SETTINGS)
static int config_save(const char *key, const void *value, size_t len)
{
	char path[sizeof(BMC_SETTINGS_TREE "/") + 16];
	int ret;

	ret = snprintk(path, sizeof(path), BMC_SETTINGS_TREE "/%s", key);
	if (ret < 0 || ret >= sizeof(path)) {
		return -ENAMETOOLONG;
	}

	ret = settings_save_one(path, value, len);
	if (ret < 0) {
		LOG_ERR("Could not save %s (err=%d)", path, ret);
	}

	return ret;
}

static int config_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	const char *next;
	ssize_t ret = -ENOENT;

	if (settings_name_steq(name, KEY_HOSTNAME, &next) && !next) {
		ret = read_cb(cb_arg, config.hostname, sizeof(config.hostname) - 1);
	} else if (settings_name_steq(name, KEY_PASSWORD, &next) && !next) {
		ret = read_cb(cb_arg, config.admin_password, sizeof(config.admin_password) - 1);
	} else if (settings_name_steq(name, KEY_NTP_SERVER, &next) && !next) {
		ret = read_cb(cb_arg, config.ntp_server, sizeof(config.ntp_server) - 1);
	} else if (settings_name_steq(name, KEY_IP4, &next) && !next) {
		ret = read_cb(cb_arg, &config.static_ip4, sizeof(config.static_ip4));
	} else if (settings_name_steq(name, KEY_IP4_NM, &next) && !next) {
		ret = read_cb(cb_arg, &config.static_ip4_netmask,
			      sizeof(config.static_ip4_netmask));
	} else if (settings_name_steq(name, KEY_IP4_GW, &next) && !next) {
		ret = read_cb(cb_arg, &config.static_ip4_gateway,
			      sizeof(config.static_ip4_gateway));
	} else if (settings_name_steq(name, KEY_DHCP4, &next) && !next) {
		ret = read_cb(cb_arg, &config.use_dhcp4, sizeof(config.use_dhcp4));
	} else if (settings_name_steq(name, KEY_NTP, &next) && !next) {
		ret = read_cb(cb_arg, &config.use_ntp, sizeof(config.use_ntp));
	} else if (settings_name_steq(name, KEY_AUTO_PWR, &next) && !next) {
		ret = read_cb(cb_arg, &config.host_auto_poweron,
			      sizeof(config.host_auto_poweron));
	}

	if (ret < 0) {
		return (int)ret;
	}

	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(bmc, BMC_SETTINGS_TREE, NULL, config_set, NULL, NULL);
#else /* CONFIG_BMC_SETTINGS */
static int config_save(const char *key, const void *value, size_t len)
{
	ARG_UNUSED(key);
	ARG_UNUSED(value);
	ARG_UNUSED(len);

	return 0;
}
#endif /* CONFIG_BMC_SETTINGS */

static int config_save_string(const char *key, const char *value)
{
	return config_save(key, value, strlen(value));
}

static void config_changed(const char *key)
{
	bmc_event_notify(BMC_EVENT_CONFIG_CHANGED, (void *)key, strlen(key) + 1);
}

const char *bmc_config_hostname(void)
{
	return config.hostname;
}

const char *bmc_config_admin_user(void)
{
	return CONFIG_BMC_ADMIN_USER;
}

const char *bmc_config_admin_password(void)
{
	return config.admin_password;
}

uint32_t bmc_config_static_ip4(void)
{
	return config.static_ip4;
}

uint32_t bmc_config_static_ip4_netmask(void)
{
	return config.static_ip4_netmask;
}

uint32_t bmc_config_static_ip4_gateway(void)
{
	return config.static_ip4_gateway;
}

bool bmc_config_use_dhcp4(void)
{
	return config.use_dhcp4 != 0;
}

bool bmc_config_use_ntp(void)
{
	return config.use_ntp != 0;
}

const char *bmc_config_ntp_server(void)
{
	return config.ntp_server;
}

bool bmc_config_host_auto_poweron(void)
{
	return config.host_auto_poweron != 0;
}

int bmc_config_hostname_set(const char *hostname)
{
	int ret;

	if (strlen(hostname) > BMC_CONFIG_HOSTNAME_MAX_LEN) {
		return -ENAMETOOLONG;
	}

	ret = bmc_net_set_hostname(hostname);
	if (ret < 0) {
		return ret;
	}

	copy_string(config.hostname, hostname, sizeof(config.hostname));

	ret = config_save_string(KEY_HOSTNAME, config.hostname);
	if (ret < 0) {
		return ret;
	}

	config_changed(KEY_HOSTNAME);

	return 0;
}

int bmc_config_admin_password_set(const char *password)
{
	int ret;

	if (strlen(password) > BMC_CONFIG_PASSWORD_MAX_LEN) {
		LOG_ERR("Password too long, maximum is %d characters",
			BMC_CONFIG_PASSWORD_MAX_LEN);
		return -ENAMETOOLONG;
	}

	copy_string(config.admin_password, password, sizeof(config.admin_password));

	ret = config_save_string(KEY_PASSWORD, config.admin_password);
	if (ret < 0) {
		return ret;
	}

	config_changed(KEY_PASSWORD);

	return 0;
}

static int static_ip4_set(const char *str, uint32_t *field, const char *key)
{
	int ret;

	ret = ip4_from_string(str, field);
	if (ret < 0) {
		return ret;
	}

	ret = bmc_net_apply_static_ip4();
	if (ret < 0) {
		LOG_ERR("Could not apply static IPv4 configuration (err=%d)", ret);
		return ret;
	}

	ret = config_save(key, field, sizeof(*field));
	if (ret < 0) {
		return ret;
	}

	config_changed(key);

	return 0;
}

int bmc_config_static_ip4_set(const char *str)
{
	return static_ip4_set(str, &config.static_ip4, KEY_IP4);
}

int bmc_config_static_ip4_netmask_set(const char *str)
{
	return static_ip4_set(str, &config.static_ip4_netmask, KEY_IP4_NM);
}

int bmc_config_static_ip4_gateway_set(const char *str)
{
	return static_ip4_set(str, &config.static_ip4_gateway, KEY_IP4_GW);
}

int bmc_config_use_dhcp4_set(bool use)
{
	int ret;

	if (bmc_config_use_dhcp4() == use) {
		return 0;
	}

	config.use_dhcp4 = use ? 1 : 0;

	if (use) {
		ret = bmc_net_start_dhcp4();
	} else {
		ret = bmc_net_stop_dhcp4();
	}

	if (ret < 0) {
		LOG_ERR("Could not %s DHCPv4 (err=%d)", use ? "start" : "stop", ret);
		return ret;
	}

	ret = config_save(KEY_DHCP4, &config.use_dhcp4, sizeof(config.use_dhcp4));
	if (ret < 0) {
		return ret;
	}

	config_changed(KEY_DHCP4);

	return 0;
}

int bmc_config_use_ntp_set(bool use)
{
	int ret;

	if (bmc_config_use_ntp() == use) {
		return 0;
	}

	config.use_ntp = use ? 1 : 0;

	if (use) {
		ret = bmc_ntp_start();
	} else {
		ret = bmc_ntp_stop();
	}

	if (ret < 0 && ret != -ENOTSUP) {
		LOG_ERR("Could not %s NTP (err=%d)", use ? "start" : "stop", ret);
		return ret;
	}

	ret = config_save(KEY_NTP, &config.use_ntp, sizeof(config.use_ntp));
	if (ret < 0) {
		return ret;
	}

	config_changed(KEY_NTP);

	return 0;
}

int bmc_config_ntp_server_set(const char *server)
{
	int ret;

	if (strlen(server) > BMC_CONFIG_NTP_SERVER_MAX_LEN) {
		return -ENAMETOOLONG;
	}

	copy_string(config.ntp_server, server, sizeof(config.ntp_server));

	ret = config_save_string(KEY_NTP_SERVER, config.ntp_server);
	if (ret < 0) {
		return ret;
	}

	if (bmc_config_use_ntp()) {
		(void)bmc_ntp_stop();

		ret = bmc_ntp_start();
		if (ret < 0 && ret != -ENOTSUP) {
			LOG_ERR("Could not restart NTP (err=%d)", ret);
			return ret;
		}
	}

	config_changed(KEY_NTP_SERVER);

	return 0;
}

int bmc_config_host_auto_poweron_set(bool on)
{
	int ret;

	if (bmc_config_host_auto_poweron() == on) {
		return 0;
	}

	config.host_auto_poweron = on ? 1 : 0;

	ret = config_save(KEY_AUTO_PWR, &config.host_auto_poweron,
			  sizeof(config.host_auto_poweron));
	if (ret < 0) {
		return ret;
	}

	config_changed(KEY_AUTO_PWR);

	return 0;
}

int bmc_config_clear(void)
{
#if defined(CONFIG_BMC_SETTINGS)
	int last_err = 0;

	for (size_t i = 0; i < ARRAY_SIZE(config_keys); i++) {
		char path[sizeof(BMC_SETTINGS_TREE "/") + 16];
		int ret;

		ret = snprintk(path, sizeof(path), BMC_SETTINGS_TREE "/%s", config_keys[i]);
		if (ret < 0 || ret >= sizeof(path)) {
			return -ENAMETOOLONG;
		}

		ret = settings_delete(path);
		if (ret < 0 && ret != -ENOENT) {
			LOG_ERR("Could not delete %s (err=%d)", path, ret);
			last_err = ret;
		}
	}

	return last_err;
#else
	ARG_UNUSED(config_keys);

	return -ENOTSUP;
#endif
}

static void config_apply_defaults(void)
{
	memset(&config, 0, sizeof(config));

	copy_string(config.hostname, CONFIG_BMC_DEFAULT_HOSTNAME, sizeof(config.hostname));
	copy_string(config.admin_password, CONFIG_BMC_DEFAULT_ADMIN_PASSWORD,
		    sizeof(config.admin_password));
	copy_string(config.ntp_server, CONFIG_BMC_DEFAULT_NTP_SERVER, sizeof(config.ntp_server));

	(void)ip4_from_string(CONFIG_BMC_DEFAULT_IPV4_ADDRESS, &config.static_ip4);
	(void)ip4_from_string(CONFIG_BMC_DEFAULT_IPV4_SUBNET_MASK, &config.static_ip4_netmask);
	(void)ip4_from_string(CONFIG_BMC_DEFAULT_IPV4_GATEWAY, &config.static_ip4_gateway);

	config.use_dhcp4 = IS_ENABLED(CONFIG_BMC_DEFAULT_USE_DHCP4);
	config.use_ntp = IS_ENABLED(CONFIG_BMC_DEFAULT_USE_NTP);
	config.host_auto_poweron = IS_ENABLED(CONFIG_BMC_DEFAULT_HOST_AUTO_POWERON);
}

int bmc_config_load(void)
{
	config_apply_defaults();

	if (config.hostname[0] == '\0') {
		copy_string(config.hostname, net_hostname_get(), sizeof(config.hostname));
	}

#if defined(CONFIG_BMC_SETTINGS)
	int ret;

	ret = settings_subsys_init();
	if (ret < 0) {
		LOG_WRN("Settings init failed (err=%d), using configuration defaults", ret);
		return 0;
	}

	ret = settings_load_subtree(BMC_SETTINGS_TREE);
	if (ret < 0) {
		LOG_WRN("Settings load failed (err=%d), using configuration defaults", ret);
		return 0;
	}

	LOG_INF("Configuration loaded from persistent storage");
#else
	LOG_INF("Persistent storage disabled, using configuration defaults");
#endif

	return 0;
}

BMC_COMPONENT_DEFINE(bmc_config, BMC_INIT_PHASE_STORAGE, bmc_config_load, false);

#if defined(CONFIG_SHELL)
#include <zephyr/shell/shell.h>

static int parse_enable(const struct shell *sh, const char *arg, bool *out)
{
	if (strcmp(arg, "enable") == 0) {
		*out = true;
	} else if (strcmp(arg, "disable") == 0) {
		*out = false;
	} else {
		shell_error(sh, "Expected \"enable\" or \"disable\", got \"%s\"", arg);
		return -EINVAL;
	}

	return 0;
}

static int cmd_config_hostname(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	ARG_UNUSED(argc);

	ret = bmc_config_hostname_set(argv[1]);
	if (ret < 0) {
		shell_error(sh, "Could not set hostname (err=%d)", ret);
		return ret;
	}

	shell_print(sh, "Hostname set to %s", bmc_config_hostname());

	return 0;
}

static int cmd_config_password(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	ARG_UNUSED(argc);

	ret = bmc_config_admin_password_set(argv[1]);
	if (ret < 0) {
		shell_error(sh, "Could not set password (err=%d)", ret);
		return ret;
	}

	shell_print(sh, "Administrator password updated");

	return 0;
}

static int cmd_config_ipv4_address(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	ARG_UNUSED(argc);

	ret = bmc_config_static_ip4_set(argv[1]);
	if (ret < 0) {
		shell_error(sh, "Could not set static IPv4 address (err=%d)", ret);
		return ret;
	}

	shell_print(sh, "Static IPv4 address set to %s", argv[1]);

	return 0;
}

static int cmd_config_ipv4_netmask(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	ARG_UNUSED(argc);

	ret = bmc_config_static_ip4_netmask_set(argv[1]);
	if (ret < 0) {
		shell_error(sh, "Could not set static IPv4 netmask (err=%d)", ret);
		return ret;
	}

	shell_print(sh, "Static IPv4 netmask set to %s", argv[1]);

	return 0;
}

static int cmd_config_ipv4_gateway(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	ARG_UNUSED(argc);

	ret = bmc_config_static_ip4_gateway_set(argv[1]);
	if (ret < 0) {
		shell_error(sh, "Could not set static IPv4 gateway (err=%d)", ret);
		return ret;
	}

	shell_print(sh, "Static IPv4 gateway set to %s", argv[1]);

	return 0;
}

static int cmd_config_dhcpv4(const struct shell *sh, size_t argc, char **argv)
{
	bool enable;
	int ret;

	ARG_UNUSED(argc);

	ret = parse_enable(sh, argv[1], &enable);
	if (ret < 0) {
		return ret;
	}

	ret = bmc_config_use_dhcp4_set(enable);
	if (ret < 0) {
		shell_error(sh, "Could not change DHCPv4 setting (err=%d)", ret);
		return ret;
	}

	shell_print(sh, "DHCPv4 %s", enable ? "enabled" : "disabled");

	return 0;
}

static int cmd_config_ntp(const struct shell *sh, size_t argc, char **argv)
{
	bool enable;
	int ret;

	ARG_UNUSED(argc);

	ret = parse_enable(sh, argv[1], &enable);
	if (ret < 0) {
		return ret;
	}

	ret = bmc_config_use_ntp_set(enable);
	if (ret < 0) {
		shell_error(sh, "Could not change NTP setting (err=%d)", ret);
		return ret;
	}

	shell_print(sh, "NTP %s", enable ? "enabled" : "disabled");

	return 0;
}

static int cmd_config_ntp_server(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	ARG_UNUSED(argc);

	ret = bmc_config_ntp_server_set(argv[1]);
	if (ret < 0) {
		shell_error(sh, "Could not set NTP server (err=%d)", ret);
		return ret;
	}

	shell_print(sh, "NTP server set to %s", bmc_config_ntp_server());

	return 0;
}

static int cmd_config_auto_poweron(const struct shell *sh, size_t argc, char **argv)
{
	bool enable;
	int ret;

	ARG_UNUSED(argc);

	ret = parse_enable(sh, argv[1], &enable);
	if (ret < 0) {
		return ret;
	}

	ret = bmc_config_host_auto_poweron_set(enable);
	if (ret < 0) {
		shell_error(sh, "Could not change host auto power-on (err=%d)", ret);
		return ret;
	}

	shell_print(sh, "Host auto power-on %s", enable ? "enabled" : "disabled");

	return 0;
}

static int cmd_config_show(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "hostname:          %s", config.hostname);
	shell_print(sh, "ipv4 address:      %s", ip4_to_string(config.static_ip4));
	shell_print(sh, "ipv4 netmask:      %s", ip4_to_string(config.static_ip4_netmask));
	shell_print(sh, "ipv4 gateway:      %s", ip4_to_string(config.static_ip4_gateway));
	shell_print(sh, "dhcpv4:            %s", config.use_dhcp4 ? "enabled" : "disabled");
	shell_print(sh, "ntp:               %s", config.use_ntp ? "enabled" : "disabled");
	shell_print(sh, "ntp server:        %s", config.ntp_server);
	shell_print(sh, "host auto poweron: %s",
		    config.host_auto_poweron ? "enabled" : "disabled");

	return 0;
}

static int cmd_config_clear(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	ret = bmc_config_clear();
	if (ret < 0) {
		shell_error(sh, "Could not clear configuration (err=%d)", ret);
		return ret;
	}

	bmc_reboot();
}

SHELL_SUBCMD_SET_CREATE(bmc_config_subcmds, (bmc, config));
SHELL_SUBCMD_ADD((bmc, config), show, NULL, "Show the BMC configuration.", cmd_config_show, 1, 0);
SHELL_SUBCMD_ADD((bmc, config), hostname, NULL, "Set the BMC hostname.\nUsage: hostname <name>",
		 cmd_config_hostname, 2, 0);
SHELL_SUBCMD_ADD((bmc, config), password, NULL,
		 "Set the administrator password.\nUsage: password <password>",
		 cmd_config_password, 2, 0);
SHELL_SUBCMD_ADD((bmc, config), ipv4_address, NULL,
		 "Set the static IPv4 address.\nUsage: ipv4_address <address>",
		 cmd_config_ipv4_address, 2, 0);
SHELL_SUBCMD_ADD((bmc, config), ipv4_netmask, NULL,
		 "Set the static IPv4 netmask.\nUsage: ipv4_netmask <netmask>",
		 cmd_config_ipv4_netmask, 2, 0);
SHELL_SUBCMD_ADD((bmc, config), ipv4_gateway, NULL,
		 "Set the static IPv4 gateway.\nUsage: ipv4_gateway <gateway>",
		 cmd_config_ipv4_gateway, 2, 0);
SHELL_SUBCMD_ADD((bmc, config), dhcpv4, NULL, "Control DHCPv4.\nUsage: dhcpv4 <enable|disable>",
		 cmd_config_dhcpv4, 2, 0);
SHELL_SUBCMD_ADD((bmc, config), ntp, NULL, "Control NTP.\nUsage: ntp <enable|disable>",
		 cmd_config_ntp, 2, 0);
SHELL_SUBCMD_ADD((bmc, config), ntp_server, NULL,
		 "Set the NTP server.\nUsage: ntp_server <server>", cmd_config_ntp_server, 2, 0);
SHELL_SUBCMD_ADD((bmc, config), auto_poweron, NULL,
		 "Control host auto power-on.\nUsage: auto_poweron <enable|disable>",
		 cmd_config_auto_poweron, 2, 0);
SHELL_SUBCMD_ADD((bmc, config), clear_and_reboot, NULL,
		 "Erase the stored configuration and reboot.", cmd_config_clear, 1, 0);
SHELL_SUBCMD_ADD((bmc), config, &bmc_config_subcmds, "Configuration commands.", NULL, 1, 0);
#endif /* CONFIG_SHELL */
