/*
 * SPDX-FileCopyrightText: © 2025-2026 Tenstorrent USA, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bmc_config, LOG_LEVEL_INF);

#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include <ctype.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>

#include <zephyr/net/hostname.h>
#include <zephyr/posix/arpa/inet.h> /* inet_ntop */

#include "config.h"
#include "bmc_init.h"
#include "net.h"
#include "ntp.h"
#include "fs.h"

#define MAX_HOSTNAME_LEN 20

#define MAX_PW_LEN 20

#define MAX_NTP_SERVER_LEN 40

/* Incrementing this allows a flag-day upgrade, but avoid doing it. */
#define CFG_CURRENT_VERSION 3

enum config_id {
	CFG_VERSION = 1,			/* uint8_t */
	CFG_BMC_ADMIN_PASSWORD = 2,		/* C string */
	CFG_BMC_HOSTNAME = 3,		/* C string */
	CFG_BMC_DEFAULT_IP4 = 4,		/* uint32_t */
	CFG_BMC_DEFAULT_IP4_NM = 5,		/* uint32_t */
	CFG_BMC_DEFAULT_IP4_GW = 6,		/* uint32_t */
	CFG_BMC_USE_DHCP4 = 7,		/* uint8_t */
	CFG_BMC_USE_NTP = 8,			/* uint8_t */
	CFG_BMC_NTP_SERVER = 9,		/* C string */
	CFG_HOST_AUTO_POWERON = 10,		/* uint8_t */
	/*
	 * Add field IDs in increasing order and do not reuse deprecated
	 * ones. Do not change meaning but add new and deprecate old.
	 * Old fields should be deleted after they are upgraded to new --
	 * don't bother handling downgrades.
	 */
};

struct config_data {
	uint8_t version;
	char bmc_hostname[MAX_HOSTNAME_LEN + 1]; /* NULL terminated */
	uint32_t bmc_default_ip4;
	uint8_t bmc_use_dhcp4;
	uint8_t host_auto_poweron;
	char bmc_admin_password[MAX_PW_LEN + 1]; /* NULL terminated */
	uint8_t bmc_use_ntp;
	char bmc_ntp_server[MAX_NTP_SERVER_LEN + 1]; /* NULL terminated */
	uint32_t bmc_default_ip4_nm;
	uint32_t bmc_default_ip4_gw;
};

BUILD_ASSERT((sizeof(CONFIG_DEFAULT_ADMIN_PASSWORD) - 1) <= MAX_PW_LEN);

static struct config_data config_data;

/*
 * strlcpy() is not available in Zephyr, define it ourselves.
 *
 * > strlcpy() copies up to dstsize - 1 characters from the string src to dst,
 * > NUL-terminating the result if dstsize is not 0.
 * > strlcpy() returns the total length of the string it tried to create,
 * > ie. the length of src.
 */
static size_t strlcpy(char *ZRESTRICT dst, const char *ZRESTRICT src, size_t dstsize)
{
	size_t src_len = strlen(src);

	if (dstsize != 0) {
		size_t copy_len = min(src_len, dstsize - 1);
		memcpy(dst, src, copy_len);
		dst[copy_len] = '\0';
	}

	return src_len;
}

static bool config_use_fs = false;

static ssize_t __config_read(uint16_t id, void *buf, size_t size)
{
	if (config_use_fs)
		return fs_key_read(id, buf, size);
	return -ENODEV;
}

static ssize_t __config_write(uint16_t id, const void *buf, size_t size)
{
	if (config_use_fs)
		return fs_key_write(id, buf, size);
	return -ENODEV;
}

#define config_read(id, var)				\
({							\
	__config_read(id, &var, sizeof(var));		\
})

#define config_read_str(id, var)			\
({							\
	ssize_t __rc;					\
	__rc = __config_read(id, var, sizeof(var) - 1);	\
	if (__rc >= 0)					\
		var[__rc] = '\0';			\
	__rc;						\
})

#define config_write(id, var)				\
({							\
	__config_write(id, &var, sizeof(var));		\
})

#define config_write_str(id, var)			\
({							\
	__config_write(id, var, strlen(var));		\
})


const char *config_bmc_hostname(void)
{
	return config_data.bmc_hostname;
}

uint32_t config_bmc_default_ip4(void)
{
	return config_data.bmc_default_ip4;
}

uint32_t config_bmc_default_ip4_nm(void)
{
	return config_data.bmc_default_ip4_nm;
}

uint32_t config_bmc_default_ip4_gw(void)
{
	return config_data.bmc_default_ip4_gw;
}

bool config_bmc_use_dhcp4(void)
{
	return config_data.bmc_use_dhcp4;
}

bool config_host_auto_poweron(void)
{
	return config_data.host_auto_poweron;
}

const char *config_bmc_admin_password(void)
{
	return config_data.bmc_admin_password;
}

bool config_bmc_use_ntp(void)
{
	return config_data.bmc_use_ntp;
}

const char *config_bmc_ntp_server(void)
{
	return config_data.bmc_ntp_server;
}

#if defined(CONFIG_BMC_APP_HTTPS_PSK)
/*
 * This is a placeholder to test PSK. If we want to support it
 * properly it would have to be configurable.
 */
bool config_bmc_https_psk(const char **psk, int *psk_len)
{
	static const char p[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, };

	*psk = p;
	*psk_len = sizeof(p);
	return true;
}
#endif

int config_bmc_hostname_set(const char *hostname)
{
	int rc;

	rc = net_do_set_hostname(hostname);
	if (rc)
		return rc;

	strlcpy(config_data.bmc_hostname, hostname, sizeof(config_data.bmc_hostname));
	rc = config_write_str(CFG_BMC_HOSTNAME, config_data.bmc_hostname);
	if (rc < 0) {
		LOG_ERR("Configuration could not be saved (err=%d)", rc);
		return rc;
	}

	return 0;
}

#define CMD_HELP_BMC_HOSTNAME			\
	"Configure BMC hostname\n"		\
	"Usage: bmc hostname <hostname>"

static int cmd_config_bmc_hostname(const struct shell *sh, size_t argc, char **argv)
{
	int rc;

	ARG_UNUSED(argc);

	if (!is_boot_finished()) {
		shell_error(sh, "must wait for boot to finish");
		return -EAGAIN;
	}

	rc = config_bmc_hostname_set(argv[1]);
	if (rc) {
		shell_error(sh, "Could not set BMC hostname (err=%d)", rc);
		return rc;
	}

	shell_info(sh, "BMC hostname set to %s", config_data.bmc_hostname);

	return 0;
}

/* Not reentrant, don't use outside config.c */
static const char *ip4_atos(uint32_t a)
{
	static char ip4_str[INET_ADDRSTRLEN];
	static struct in_addr addr;

	addr.s_addr = a;

	if (inet_ntop(AF_INET, &addr, ip4_str, sizeof(ip4_str)) == NULL) {
		LOG_ERR("Could not convert IPv4 address 0x%08x to str", a);
		return NULL;
	}

	return ip4_str;
}

/* Not reentrant, don't use outside config.c */
static int ip4_stoa(const char *str, uint32_t *out)
{
	static struct in_addr addr;
	int rc;

	rc = inet_pton(AF_INET, str, &addr);
	if (rc != 1) {
		LOG_ERR("Could not convert IPv4 address %s to in_addr", str);
		return -EINVAL;
	}

	*out = addr.s_addr;
	return 0;
}

static uint32_t default_ip4_from_kconfig(const char *str, const char *field)
{
	uint32_t addr = 0;
	int rc;

	rc = ip4_stoa(str, &addr);
	if (rc < 0) {
		LOG_ERR("Invalid default %s IPv4 setting %s", field, str);
		return 0;
	}

	return addr;
}

int config_bmc_default_ip4_set(const char *str)
{
	int rc;

	if (str) {
		rc = ip4_stoa(str, &config_data.bmc_default_ip4);
		if (rc) {
			LOG_ERR("Invalid IPv4 address string: %s", str);
			return rc;
		}
	} else {
		config_data.bmc_default_ip4 = 0; /* Remove default addr */
	}

	rc = net_do_set_default_ip4_from_config();
	if (rc) {
		LOG_ERR("Could not apply BMC default IPv4 address (err=%d)", rc);
		return rc;
	}

	rc = config_write(CFG_BMC_DEFAULT_IP4, config_data.bmc_default_ip4);
	if (rc < 0) {
		LOG_ERR("Configuration could not be saved (err=%d)", rc);
		return rc;
	}

	return 0;
}

int config_bmc_default_ip4_nm_set(const char *str)
{
	int rc;

	if (str) {
		rc = ip4_stoa(str, &config_data.bmc_default_ip4_nm);
		if (rc)
			return rc;
	} else {
		config_data.bmc_default_ip4_nm = 0;
	}

	rc = net_do_set_default_ip4_from_config();
	if (rc) {
		LOG_ERR("Could not apply BMC default IPv4 subnet mask address (err=%d)", rc);
		return rc;
	}

	rc = config_write(CFG_BMC_DEFAULT_IP4_NM, config_data.bmc_default_ip4_nm);
	if (rc < 0) {
		LOG_ERR("Configuration could not be saved (err=%d)", rc);
		return rc;
	}

	return 0;
}

int config_bmc_default_ip4_gw_set(const char *str)
{
	int rc;

	if (str) {
		rc = ip4_stoa(str, &config_data.bmc_default_ip4_gw);
		if (rc)
			return rc;
	} else {
		config_data.bmc_default_ip4_gw = 0;
	}

	rc = net_do_set_default_ip4_from_config();
	if (rc) {
		LOG_ERR("Could not apply BMC default IPv4 gateway address (err=%d)", rc);
		return rc;
	}

	rc = config_write(CFG_BMC_DEFAULT_IP4_GW, config_data.bmc_default_ip4_gw);
	if (rc < 0) {
		LOG_ERR("Configuration could not be saved (err=%d)", rc);
		return rc;
	}

	return 0;
}

#define CMD_HELP_BMC_DEFAULT_IP4		\
	"Configure BMC default IPv4 address\n"	\
	"Usage: bmc ipv4_address <IPv4 address>"

static int cmd_config_bmc_default_ip4(const struct shell *sh, size_t argc, char **argv)
{
	int rc;

	ARG_UNUSED(argc);

	if (!is_boot_finished()) {
		shell_error(sh, "must wait for boot to finish");
		return -EAGAIN;
	}

	rc = config_bmc_default_ip4_set(argv[1]);
	if (rc) {
		shell_error(sh, "Could not set BMC default IPv4 address (err=%d)", rc);
		return rc;
	}

	shell_info(sh, "BMC default IPv4 address set to %s", argv[1]);

	return 0;
}

#define CMD_HELP_BMC_DEFAULT_IP4_NM			\
	"Configure BMC default IPv4 subnet mask\n"	\
	"Usage: bmc ipv4_subnet_mask <IPv4 address>"

static int cmd_config_bmc_default_ip4_nm(const struct shell *sh, size_t argc, char **argv)
{
	int rc;

	ARG_UNUSED(argc);

	if (!is_boot_finished()) {
		shell_error(sh, "must wait for boot to finish");
		return -EAGAIN;
	}

	rc = config_bmc_default_ip4_nm_set(argv[1]);
	if (rc) {
		shell_error(sh, "Could not set BMC default IPv4 subnet mask (err=%d)", rc);
		return rc;
	}

	shell_info(sh, "BMC default IPv4 subnet mask set to %s", argv[1]);

	return 0;
}

#define CMD_HELP_BMC_DEFAULT_IP4_GW			\
	"Configure BMC default IPv4 gateway\n"	\
	"Usage: bmc ipv4_gateway <IPv4 address>"

static int cmd_config_bmc_default_ip4_gw(const struct shell *sh, size_t argc, char **argv)
{
	int rc;

	ARG_UNUSED(argc);

	if (!is_boot_finished()) {
		shell_error(sh, "must wait for boot to finish");
		return -EAGAIN;
	}

	rc = config_bmc_default_ip4_gw_set(argv[1]);
	if (rc) {
		shell_error(sh, "Could not set BMC default IPv4 gateway (err=%d)", rc);
		return rc;
	}

	shell_info(sh, "BMC default IPv4 gateway set to %s", argv[1]);

	return 0;
}

int config_bmc_use_dhcp4_set(bool use)
{
	int rc;

	if (use) {
		if (config_data.bmc_use_dhcp4 == 1)
			return 0;
		config_data.bmc_use_dhcp4 = 1;
		net_start_dhcp4();
	} else {
		if (config_data.bmc_use_dhcp4 == 0)
			return 0;
		config_data.bmc_use_dhcp4 = 0;
		net_stop_dhcp4();
	}

	rc = config_write(CFG_BMC_USE_DHCP4, config_data.bmc_use_dhcp4);
	if (rc < 0) {
		LOG_ERR("Configuration could not be saved (err=%d)", rc);
		return rc;
	}

	return 0;
}

#define CMD_HELP_BMC_DHCP4			\
	"BMC DHCP4 enabled\n"			\
	"Usage: bmc dhcpv4 <enable|disable>"

static int cmd_config_bmc_dhcp4(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	if (!is_boot_finished()) {
		shell_error(sh, "must wait for boot to finish");
		return -EAGAIN;
	}

	if (!strcmp(argv[1], "enable")) {
		config_bmc_use_dhcp4_set(true);
		shell_info(sh, "BMC DHCPv4 enabled");
	} else if (!strcmp(argv[1], "disable")) {
		config_bmc_use_dhcp4_set(false);
		shell_info(sh, "BMC DHCPv4 disabled");
	} else {
		shell_error(sh, "bmc dhcpv4: unknown argument %s", argv[1]);
		return -EINVAL;
	}

	return 0;
}

int config_bmc_use_ntp_set(bool use)
{
	int rc;

	if (use) {
		if (config_data.bmc_use_ntp == 1)
			return 0;
		config_data.bmc_use_ntp = 1;
		start_ntp();
	} else {
		if (config_data.bmc_use_ntp == 0)
			return 0;
		config_data.bmc_use_ntp = 0;
		stop_ntp();
	}

	rc = config_write(CFG_BMC_USE_NTP, config_data.bmc_use_ntp);
	if (rc < 0) {
		LOG_ERR("Configuration could not be saved (err=%d)", rc);
		return rc;
	}

	return 0;
}

#define CMD_HELP_BMC_NTP				\
	"BMC NTP time sync enabled\n"			\
	"Usage: bmc ntp <enable|disable>"

static int cmd_config_bmc_ntp(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	if (!is_boot_finished()) {
		shell_error(sh, "must wait for boot to finish");
		return -EAGAIN;
	}

	if (!strcmp(argv[1], "enable")) {
		config_bmc_use_ntp_set(true);
		shell_info(sh, "BMC NTP enabled");
	} else if (!strcmp(argv[1], "disable")) {
		config_bmc_use_ntp_set(false);
		shell_info(sh, "BMC NTP disabled");
	} else {
		shell_error(sh, "bmc ntp: unknown argument %s", argv[1]);
		return -EINVAL;
	}

	return 0;
}

int config_bmc_ntp_server_set(const char *ntp_server)
{
	int rc;

	strlcpy(config_data.bmc_ntp_server, ntp_server, sizeof(config_data.bmc_ntp_server));
	rc = config_write_str(CFG_BMC_NTP_SERVER, config_data.bmc_ntp_server);
	if (rc < 0) {
		LOG_ERR("Configuration could not be saved (err=%d)", rc);
		return rc;
	}

	if (config_bmc_use_ntp()) {
		rc = stop_ntp();
		if (rc < 0)
			LOG_ERR("Error stopping NTP (err=%d)", rc);
		rc = start_ntp();
		if (rc < 0) {
			LOG_ERR("Error restarting NTP (err=%d)", rc);
			return rc;
		}
	}

	return 0;
}

#define CMD_HELP_BMC_NTP_SERVER				\
	"BMC NTP server address\n"			\
	"Usage: bmc ntp_server <server address>"

static int cmd_config_bmc_ntp_server(const struct shell *sh, size_t argc, char **argv)
{
	int rc;

	ARG_UNUSED(argc);

	if (!is_boot_finished()) {
		shell_error(sh, "must wait for boot to finish");
		return -EAGAIN;
	}

	rc = config_bmc_ntp_server_set(argv[1]);
	if (rc) {
		shell_error(sh, "Could not set BMC NTP server (err=%d)", rc);
		return rc;
	}

	shell_info(sh, "BMC NTP server updated");

	return 0;
}


int config_bmc_password_set(const char *password)
{
	int rc;

	if (strlen(password) > MAX_PW_LEN) {
		LOG_ERR("Password too long, max length is %d characters", MAX_PW_LEN);
		return -EINVAL;
	}

	strlcpy(config_data.bmc_admin_password, password, sizeof(config_data.bmc_admin_password));
	rc = config_write_str(CFG_BMC_ADMIN_PASSWORD, config_data.bmc_admin_password);
	if (rc < 0) {
		LOG_ERR("Configuration could not be saved (err=%d)", rc);
		return rc;
	}

	return 0;
}

#define CMD_HELP_BMC_ADMIN_PASSWORD		\
	"BMC admin password\n"			\
	"Usage: bmc password <pw>"

static int cmd_config_bmc_password(const struct shell *sh, size_t argc, char **argv)
{
	int rc;

	ARG_UNUSED(argc);

	if (!is_boot_finished()) {
		shell_error(sh, "must wait for boot to finish");
		return -EAGAIN;
	}

	rc = config_bmc_password_set(argv[1]);
	if (rc) {
		shell_error(sh, "Could not set BMC admin password (err=%d)", rc);
		return rc;
	}

	shell_info(sh, "BMC admin password updated");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_config_bmc_cmds,
	SHELL_CMD_ARG(password,		NULL, CMD_HELP_BMC_ADMIN_PASSWORD, cmd_config_bmc_password, 2, 0),
	SHELL_CMD_ARG(hostname,		NULL, CMD_HELP_BMC_HOSTNAME, cmd_config_bmc_hostname, 2, 0),
	SHELL_CMD_ARG(ipv4_address,	NULL, CMD_HELP_BMC_DEFAULT_IP4, cmd_config_bmc_default_ip4, 2, 0),
	SHELL_CMD_ARG(ipv4_subnet_mask,	NULL, CMD_HELP_BMC_DEFAULT_IP4_NM, cmd_config_bmc_default_ip4_nm, 2, 0),
	SHELL_CMD_ARG(ipv4_gateway,	NULL, CMD_HELP_BMC_DEFAULT_IP4_GW, cmd_config_bmc_default_ip4_gw, 2, 0),
	SHELL_CMD_ARG(dhcpv4,		NULL, CMD_HELP_BMC_DHCP4, cmd_config_bmc_dhcp4, 2, 0),
	SHELL_CMD_ARG(ntp,		NULL, CMD_HELP_BMC_NTP, cmd_config_bmc_ntp, 2, 0),
	SHELL_CMD_ARG(ntp_server,	NULL, CMD_HELP_BMC_NTP_SERVER, cmd_config_bmc_ntp_server, 2, 0),
	SHELL_SUBCMD_SET_END
);

int config_host_auto_poweron_set(bool on)
{
	int rc;

	if (on) {
		if (config_data.host_auto_poweron == 1)
			return 0;
		config_data.host_auto_poweron = 1;
	} else {
		if (config_data.host_auto_poweron == 0)
			return 0;
		config_data.host_auto_poweron = 0;
	}

	rc = config_write(CFG_HOST_AUTO_POWERON, config_data.host_auto_poweron);
	if (rc < 0) {
		LOG_ERR("Configuration could not be saved (err=%d)", rc);
		return rc;
	}

	return 0;
}

#define CMD_HELP_HOST_AUTO_POWERON		\
	"Host auto poweron enabled\n"		\
	"Usage: host auto_poweron <enable|disable>"

static int cmd_config_host_auto_poweron(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	if (!is_boot_finished()) {
		shell_error(sh, "must wait for boot to finish");
		return -EAGAIN;
	}

	if (!strcmp(argv[1], "enable")) {
		config_host_auto_poweron_set(true);
		shell_info(sh, "Host auto poweron enabled");
	} else if (!strcmp(argv[1], "disable")) {
		config_host_auto_poweron_set(false);
		shell_info(sh, "Host auto poweron disabled");
	} else {
		shell_error(sh, "host auto_poweron: unknown argument %s", argv[1]);
		return -EINVAL;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_config_host_cmds,
	SHELL_CMD_ARG(auto_poweron,	NULL, CMD_HELP_HOST_AUTO_POWERON, cmd_config_host_auto_poweron, 2, 0),
	SHELL_SUBCMD_SET_END
);

static int cmd_config_show(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "--- Configuration ---");
	shell_print(sh, "BMC hostname: %s",	config_data.bmc_hostname);
	shell_print(sh, "BMC default IPv4 address: %s", ip4_atos(config_data.bmc_default_ip4));
	shell_print(sh, "BMC default IPv4 subnet mask: %s", ip4_atos(config_data.bmc_default_ip4_nm));
	shell_print(sh, "BMC default IPv4 gateway: %s", ip4_atos(config_data.bmc_default_ip4_gw));
	shell_print(sh, "BMC use DHCPv4: %d",	config_data.bmc_use_dhcp4);
	shell_print(sh, "BMC use NTP: %d",	config_data.bmc_use_ntp);
	shell_print(sh, "BMC NTP server: %s",	config_data.bmc_ntp_server);
	shell_print(sh, "Host auto poweron: %d", config_data.host_auto_poweron);
	shell_print(sh, "---------------------");

	return 0;
}

static int cmd_config_clear(const struct shell *sh, size_t argc, char **argv)
{
	int rc;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!is_boot_finished()) {
		shell_error(sh, "must wait for boot to finish");
		return -EAGAIN;
	}

	rc = config_clear();
	if (rc) {
		shell_error(sh, "could not clear config (err=%d)", rc);
		return rc;
	}

	bmc_reboot();

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_config_cmds,
	SHELL_CMD(show,	NULL, "Show configuration.", &cmd_config_show),
	SHELL_CMD(bmc,	&sub_config_bmc_cmds, "BMC configuration commands.", NULL),
	SHELL_CMD(host,	&sub_config_host_cmds, "Host configuration commands.", NULL),
	SHELL_CMD(clear_and_reboot, NULL, "Clear configuration and reboot system.", &cmd_config_clear),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(config, &sub_config_cmds, "Configuration commands", NULL);

int config_clear(void)
{
	return fs_clear();
}

int config_init(void)
{
	ssize_t rc;

	memset(&config_data, 0, sizeof(config_data));

	if (fs_enabled()) {
		config_use_fs = true;

		rc = config_read(CFG_VERSION, config_data.version);
		if (rc < 0) {
			if (rc == -ENOENT)
				LOG_INF("Config creating new");
			else
				LOG_INF("Config could not be read, creating new");
			config_data.version = CFG_CURRENT_VERSION;
			rc = config_write(CFG_VERSION, config_data.version);
		} else if (config_data.version != CFG_CURRENT_VERSION) {
			LOG_WRN("Config version unknown (version=%d), creating new config", config_data.version);
			rc = config_clear();
			if (rc == 0) {
				config_data.version = CFG_CURRENT_VERSION;
				rc = config_write(CFG_VERSION, config_data.version);
			}
		} else {
			LOG_INF("Config loading from flash");
			rc = 0;
		}
		if (rc < 0) {
			LOG_ERR("Config persistent storage failed, continuing with defaults");
			config_use_fs = false;
		}

	} else {
		LOG_INF("Config persistent storage unavailable, using defaults");
	}

	/*
	 * Fields that can not be read from disk should be given defaults and
	 * written back. Could avoid writing in that case, but that would make
	 * it harder to change defaults in future.
	 */
	rc = config_read_str(CFG_BMC_ADMIN_PASSWORD, config_data.bmc_admin_password);
	if (rc < 0) {
		strlcpy(config_data.bmc_admin_password, CONFIG_DEFAULT_ADMIN_PASSWORD, sizeof(config_data.bmc_admin_password));
		config_write_str(CFG_BMC_ADMIN_PASSWORD, config_data.bmc_admin_password);
	}

	rc = config_read_str(CFG_BMC_HOSTNAME, config_data.bmc_hostname);
	if (rc < 0) {
		/* Default to CONFIG_NET_HOSTNAME, which net_hostname_get() will return */
		strlcpy(config_data.bmc_hostname, net_hostname_get(), sizeof(config_data.bmc_hostname));
		config_write_str(CFG_BMC_HOSTNAME, config_data.bmc_hostname);
	}

	rc = config_read(CFG_BMC_DEFAULT_IP4, config_data.bmc_default_ip4);
	if (rc < 0) {
		config_data.bmc_default_ip4 = default_ip4_from_kconfig(
			CONFIG_BMC_DEFAULT_IPV4_ADDRESS, "address");
		config_write(CFG_BMC_DEFAULT_IP4, config_data.bmc_default_ip4);
	}

	rc = config_read(CFG_BMC_DEFAULT_IP4_NM, config_data.bmc_default_ip4_nm);
	if (rc < 0) {
		config_data.bmc_default_ip4_nm = default_ip4_from_kconfig(
			CONFIG_BMC_DEFAULT_IPV4_SUBNET_MASK, "subnet mask");
		config_write(CFG_BMC_DEFAULT_IP4_NM, config_data.bmc_default_ip4_nm);
	}

	rc = config_read(CFG_BMC_DEFAULT_IP4_GW, config_data.bmc_default_ip4_gw);
	if (rc < 0) {
		config_data.bmc_default_ip4_gw = default_ip4_from_kconfig(
			CONFIG_BMC_DEFAULT_IPV4_GATEWAY, "gateway");
		config_write(CFG_BMC_DEFAULT_IP4_GW, config_data.bmc_default_ip4_gw);
	}

	rc = config_read(CFG_BMC_USE_DHCP4, config_data.bmc_use_dhcp4);
	if (rc < 0) {
		config_data.bmc_use_dhcp4 = IS_ENABLED(CONFIG_BMC_DEFAULT_USE_DHCP4);
		config_write(CFG_BMC_USE_DHCP4, config_data.bmc_use_dhcp4);
	}

	rc = config_read(CFG_BMC_USE_NTP, config_data.bmc_use_ntp);
	if (rc < 0) {
		config_data.bmc_use_ntp = IS_ENABLED(CONFIG_BMC_DEFAULT_USE_NTP);
		config_write(CFG_BMC_USE_NTP, config_data.bmc_use_ntp);
	}

	rc = config_read_str(CFG_BMC_NTP_SERVER, config_data.bmc_ntp_server);
	if (rc < 0) {
		strlcpy(config_data.bmc_ntp_server, CONFIG_BMC_DEFAULT_NTP_SERVER,
			sizeof(config_data.bmc_ntp_server));
		config_write_str(CFG_BMC_NTP_SERVER, config_data.bmc_ntp_server);
	}

	rc = config_read(CFG_HOST_AUTO_POWERON, config_data.host_auto_poweron);
	if (rc < 0) {
		config_data.host_auto_poweron =
			IS_ENABLED(CONFIG_BMC_DEFAULT_HOST_AUTO_POWERON);
		config_write(CFG_HOST_AUTO_POWERON, config_data.host_auto_poweron);
	}

	return 0;
}
