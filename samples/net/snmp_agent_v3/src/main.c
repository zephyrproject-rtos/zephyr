/*
 * Copyright (c) 2026 Zephyr SNMP Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_ip.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/net/snmp/snmp.h>
#include <zephyr/net/snmp/snmp_mib.h>
#include <zephyr/net/snmp/snmp_trap.h>
#include <zephyr/net/snmp/snmp_usm.h>
#include <zephyr/net/snmp/snmp_vacm.h>
#include "app_led.h"
#include "app_sensor.h"
#include "app_button.h"
#include "app_mib.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#ifndef CONFIG_SNMP_SAMPLE_ENTERPRISE_ID
#define CONFIG_SNMP_SAMPLE_ENTERPRISE_ID 54321
#endif

#ifndef CONFIG_SNMP_SAMPLE_TRAP_DEST
#define CONFIG_SNMP_SAMPLE_TRAP_DEST "192.168.16.100"
#endif

#ifndef CONFIG_SNMP_SAMPLE_V3_MONITOR_USER
#define CONFIG_SNMP_SAMPLE_V3_MONITOR_USER "v3monitor"
#endif

#ifndef CONFIG_SNMP_SAMPLE_V3_MONITOR_AUTH_PASS
#define CONFIG_SNMP_SAMPLE_V3_MONITOR_AUTH_PASS "v3-monitor-auth-passphrase-test-only"
#endif

#ifndef CONFIG_SNMP_AGENT_PORT
#define CONFIG_SNMP_AGENT_PORT 161
#endif

#ifndef CONFIG_SNMP_COMMUNITY_READ
#define CONFIG_SNMP_COMMUNITY_READ "public"
#endif

#ifndef CONFIG_SNMP_COMMUNITY_WRITE
#define CONFIG_SNMP_COMMUNITY_WRITE "private"
#endif

static struct net_mgmt_event_callback s_mgmt_cb;
static K_SEM_DEFINE(s_ip_ready_sem, 0, 1);
static struct net_if *s_snmp_iface;

/*
 * Caller-owned agent storage (decision #1 of the design contract): one instance is
 * always present; the optional second one demonstrates true multi-instance operation
 * on another UDP port with its own identity and its own MIB tree.
 */
static struct snmp_agent g_agent_lan;
#if defined(CONFIG_SNMP_SAMPLE_SECOND_INSTANCE)
static struct snmp_agent g_agent_wan;
#endif

static void print_banner(const char *ip_str)
{
	LOG_INF("================================================================");
	LOG_INF("       SNMPv3 VALIDATION SAMPLE ON ZEPHYR RTOS (NUCLEO-H743ZI)   ");
	LOG_INF("================================================================");
	LOG_INF("Protocol Support:");
	LOG_INF("  - SNMPv1  : %s", IS_ENABLED(CONFIG_SNMP_VERSION_1) ? "ENABLED" : "DISABLED");
	LOG_INF("  - SNMPv2c : %s", IS_ENABLED(CONFIG_SNMP_VERSION_2C) ? "ENABLED" : "DISABLED");
	LOG_INF("  - SNMPv3  : %s", IS_ENABLED(CONFIG_SNMP_VERSION_3) ? "ENABLED" : "DISABLED");
	LOG_INF("Security & Network:");
	LOG_INF("  - Agent IP / Port : %s : %d", ip_str, CONFIG_SNMP_AGENT_PORT);
	LOG_INF("  - Read Community  : \"%s\" (v1/v2c)", CONFIG_SNMP_COMMUNITY_READ);
	LOG_INF("  - Write Community : \"%s\" (v1/v2c)", CONFIG_SNMP_COMMUNITY_WRITE);
#if defined(CONFIG_SNMP_VERSION_3)
	LOG_INF("  - v3 authPriv user: \"%s\" (SHA-256 / AES-128-CFB)", CONFIG_SNMP_V3_USER_NAME);
	LOG_INF("  - v3 authNoPriv   : \"%s\" (SHA-1, read-only VACM group)",
		CONFIG_SNMP_SAMPLE_V3_MONITOR_USER);
	LOG_INF("  - engineBoots     : %u (persisted when CONFIG_SETTINGS is on)",
		(unsigned int)CONFIG_SNMP_V3_ENGINE_BOOTS);
	LOG_INF("  -- All credentials above are public test values. Never ship them. --");
#endif
#if defined(CONFIG_SNMP_SAMPLE_SECOND_INSTANCE)
	LOG_INF("  - Second instance: %s : %d (sysName \"%s\")", ip_str,
		CONFIG_SNMP_SAMPLE_WAN_PORT, "NUCLEO-H743ZI-WAN");
#endif
#if defined(CONFIG_SNMP_TRAP_ENABLED)
	LOG_INF("  - Trap Destination: %s : %d", CONFIG_SNMP_SAMPLE_TRAP_DEST,
		CONFIG_SNMP_TRAP_PORT);
#endif
	LOG_INF("Private MIB Structure (1.3.6.1.4.1.%d.1):", CONFIG_SNMP_SAMPLE_ENTERPRISE_ID);
	LOG_INF("  .1.1.1.0 sensorValue       (INTEGER, temp in C)   [R/W]");
	LOG_INF("  .1.1.2.0 ledState          (INTEGER, 0=OFF, 1=ON) [R/W]");
	LOG_INF("  .1.1.3.0 buttonCounter     (Counter32, press cnt) [RO]");
	LOG_INF("  .1.1.4.0 sensorHumidity    (Gauge32, %% relative)  [RO]");
	LOG_INF("  .1.1.5.0 sensorDescription (OCTET STRING)          [RO]");
	LOG_INF("  .1.1.6.0 sensorStatusOid   (OBJECT IDENTIFIER)    [RO]");
	LOG_INF("  .1.3.1 sensorTable       (Columns: idx, name, val)");
	LOG_INF("  .1.2.1 buttonPressed     (Notification/Trap)");
	LOG_INF("v3 validation commands (net-snmp):");
	LOG_INF("  snmpget -v3 -u %s -l authPriv -a SHA-256 -A '%s' -x AES -X '%s' %s \\",
		CONFIG_SNMP_V3_USER_NAME, CONFIG_SNMP_V3_AUTH_PASS, CONFIG_SNMP_V3_PRIV_PASS,
		ip_str);
	LOG_INF("          1.3.6.1.4.1.%d.1.1.1.0", CONFIG_SNMP_SAMPLE_ENTERPRISE_ID);
	LOG_INF("  snmpwalk -v3 -u %s -l authNoPriv -a SHA -A '%s' %s 1.3.6.1.2.1.1",
		CONFIG_SNMP_SAMPLE_V3_MONITOR_USER, CONFIG_SNMP_SAMPLE_V3_MONITOR_AUTH_PASS,
		ip_str);
	LOG_INF("  snmpset -v3 -u %s -l authPriv -a SHA-256 -A '%s' -x AES -X '%s' %s \\",
		CONFIG_SNMP_V3_USER_NAME, CONFIG_SNMP_V3_AUTH_PASS, CONFIG_SNMP_V3_PRIV_PASS,
		ip_str);
	LOG_INF("          1.3.6.1.4.1.%d.1.1.2.0 i 1", CONFIG_SNMP_SAMPLE_ENTERPRISE_ID);
	LOG_INF("  snmptrap -v3 -u %s -l authPriv -a SHA-256 -A '%s' -x AES -X '%s' %s '' \\",
		CONFIG_SNMP_V3_USER_NAME, CONFIG_SNMP_V3_AUTH_PASS, CONFIG_SNMP_V3_PRIV_PASS,
		ip_str);
	LOG_INF("          1.3.6.1.6.3.1.1.5.1 1.3.6.1.2.1.1.3.0 t 0");
	LOG_INF("Expected v3 behaviour to verify:");
	LOG_INF("  - unknown engineID / user / wrong digest answers with a Report PDU");
	LOG_INF("  - 'v3monitor' can read 1.3.6.1.2.1.1 but not the private MIB subtree");
	LOG_INF("  - plain v2c still works on the same instance with the communities above");
	LOG_INF("================================================================");
}

#if defined(CONFIG_SNMP_VERSION_3)
/*
 * Register the second demo user at run time. It is authNoPriv on purpose: the same
 * agent then serves an authPriv account (the compile-time default user) and an
 * authNoPriv one, which is what a validation sample has to exercise. snmp_vacm_init()
 * already mapped this name to group "readonly", so the account can read the system
 * group and nothing else.
 *
 * RFC 3414 section 11.2 requires >= 8-octet pass phrases; snmp_usm_add_user() rejects
 * anything shorter, so a Kconfig typo shows up here instead of as a silent auth failure.
 */
static int register_monitor_user(struct snmp_agent *agent)
{
	struct snmp_usm_user monitor = {
		.auth_protocol = SNMP_USM_AUTH_SHA1,
		.priv_protocol = SNMP_USM_PRIV_NONE,
	};

	if (strlen(CONFIG_SNMP_SAMPLE_V3_MONITOR_USER) >= sizeof(monitor.user_name) ||
	    strlen(CONFIG_SNMP_SAMPLE_V3_MONITOR_AUTH_PASS) >= sizeof(monitor.auth_pass)) {
		return -EINVAL;
	}

	strcpy(monitor.user_name, CONFIG_SNMP_SAMPLE_V3_MONITOR_USER);
	strcpy(monitor.auth_pass, CONFIG_SNMP_SAMPLE_V3_MONITOR_AUTH_PASS);

	int ret = snmp_usm_add_user(agent, &monitor);
	if (ret < 0) {

		return ret;
	}

	return snmp_vacm_add_group_mapping(agent, SNMP_SEC_MODEL_USM,
					   CONFIG_SNMP_SAMPLE_V3_MONITOR_USER, "readonly");
}
#endif /* CONFIG_SNMP_VERSION_3 */

static void net_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
			      struct net_if *iface)
{
	ARG_UNUSED(cb);
	if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD && iface == s_snmp_iface) {
		k_sem_give(&s_ip_ready_sem);
	}
}

#if defined(CONFIG_SHELL)
#include <zephyr/shell/shell.h>
#include <stdlib.h>

static int cmd_app_button(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	shell_print(sh, "Simulating User Button press...");
	app_button_simulate_press();
	shell_print(sh, "Button count now: %u", app_button_get_counter());
	return 0;
}

static int cmd_app_led(const struct shell *sh, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_print(sh, "User LED state: %s (%d)", app_led_get_state() ? "ON" : "OFF",
			    app_led_get_state());
		return 0;
	}
	int val = atoi(argv[1]);
	if (val != 0 && val != 1) {

		shell_error(sh, "Invalid value %d. Must be 0 (OFF) or 1 (ON).", val);
		return -EINVAL;
	}
	app_led_set_state(val);
	shell_print(sh, "User LED set to: %s (%d)", val ? "ON" : "OFF", val);
	return 0;
}

static int cmd_app_sensor(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	shell_print(sh, "Sensor Readings:");
	shell_print(sh, "  Temperature : %d C", app_sensor_get_temp());
	shell_print(sh, "  Humidity    : %u %%", app_sensor_get_humidity());
	shell_print(sh, "  Description : %s", app_sensor_get_description());
	shell_print(sh, "  Button Count: %u", app_button_get_counter());
	shell_print(sh, "  LED State   : %d", app_led_get_state());
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_app,
	SHELL_CMD(button, NULL, "Simulate User Button press and send SNMP Trap", cmd_app_button),
	SHELL_CMD_ARG(led, NULL, "Get or set User LED [0|1]", cmd_app_led, 1, 1),
	SHELL_CMD(sensor, NULL, "Display current sensor and device values", cmd_app_sensor),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(app, &sub_app, "SNMP validation sample commands", NULL);
#endif

int main(void)
{
	LOG_INF("Booting SNMP Validation Sample...");

	s_snmp_iface = net_if_get_first_ethernet();
	if (!s_snmp_iface) {
		LOG_ERR("No Ethernet network interface found");
		return -ENODEV;
	}

	/* 1. Initialize hardware drivers that do not need the agent */
	app_led_init();
	app_sensor_init();

	/* 2. Register network management event listener */
	net_mgmt_init_event_callback(&s_mgmt_cb, net_event_handler,
				     NET_EVENT_IPV4_ADDR_ADD | NET_EVENT_IPV4_ADDR_DEL);
	net_mgmt_add_event_callback(&s_mgmt_cb);

	/* 3. Create the LAN agent instance on caller-owned static storage.
	 *    snmp_agent_init() only builds the domain state (MIB-II groups, identity,
	 *    communities); the UDP listener is opened later by snmp_agent_start().
	 */
	const struct snmp_agent_cfg cfg_lan = {
		.port = CONFIG_SNMP_AGENT_PORT,
		.iface = s_snmp_iface,
		.read_community = CONFIG_SNMP_COMMUNITY_READ,
		.write_community = CONFIG_SNMP_COMMUNITY_WRITE,
		.sys_descr = "Zephyr RTOS SNMP Validation Node (STM32H743ZI)",
		.sys_contact = "admin@example.com",
		.sys_name = "NUCLEO-H743ZI-SNMP",
		.sys_location = "Lab Bench 1",
		.settings_prefix = "snmp/lan",
	};
	int ret = snmp_agent_init(&g_agent_lan, &cfg_lan);
	if (ret < 0) {

		LOG_ERR("Failed to initialize LAN SNMP agent: %d", ret);
		return ret;
	}

#if defined(CONFIG_SNMP_VERSION_3)
	/* 3b. Add the authNoPriv demo account next to the compiled-in authPriv one. */
	ret = register_monitor_user(&g_agent_lan);
	if (ret < 0) {
		LOG_ERR("Failed to register USM monitor user \"%s\": %d",
			CONFIG_SNMP_SAMPLE_V3_MONITOR_USER, ret);
		return ret;
	}
	LOG_INF("Registered USM users: \"%s\" (authPriv), \"%s\" (authNoPriv)",
		CONFIG_SNMP_V3_USER_NAME, CONFIG_SNMP_SAMPLE_V3_MONITOR_USER);

	/*
	 * The collector's createUser line needs this engine ID (RFC 3411 §5). Printing
	 * it once at boot removes the guessing that used to require trace output.
	 */
	{
		uint8_t engine_id[32];
		uint8_t engine_id_len = sizeof(engine_id);
		char engine_hex[2 * 32 + 3] = "?";
		int hex_pos = 0;

		if (snmp_usm_get_engine_id(&g_agent_lan, engine_id, &engine_id_len) == 0) {
			hex_pos = snprintf(engine_hex, sizeof(engine_hex), "0x");
			for (uint8_t i = 0;
			     i < engine_id_len && hex_pos < (int)sizeof(engine_hex) - 2; i++) {
				hex_pos +=
					snprintf(&engine_hex[hex_pos], sizeof(engine_hex) - hex_pos,
						 "%02X", engine_id[i]);
			}
		}
		LOG_INF("snmpEngineID: %s (use as -e in snmptrapd createUser)", engine_hex);
	}
#endif

	/* Button traps fire from a workqueue: hand it the agent to notify through. */
	app_button_init(&g_agent_lan);

	/* 4. Register Private MIB on the LAN agent */
	ret = app_mib_init(&g_agent_lan);
	if (ret < 0) {
		LOG_ERR("Failed to initialize Private MIB: %d", ret);
		return ret;
	}

#if defined(CONFIG_SNMP_TRAP_ENABLED)
	/* 5. Configure trap destination */
	snmp_trap_add_destination(&g_agent_lan, CONFIG_SNMP_SAMPLE_TRAP_DEST);
	LOG_INF("Configured SNMP Trap destination: %s", CONFIG_SNMP_SAMPLE_TRAP_DEST);
#endif

#if defined(CONFIG_SNMP_SAMPLE_SECOND_INSTANCE)
	/*
	 * 6. Second instance: same interface, different UDP port (two RUNNING agents
	 * cannot share :161 without an interface binding — plain socket semantics)
	 * and a distinct sysName. Its MIB tree, counters and community set are fully
	 * independent of g_agent_lan.
	 */
	const struct snmp_agent_cfg cfg_wan = {
		.port = CONFIG_SNMP_SAMPLE_WAN_PORT,
		.iface = s_snmp_iface,
		.read_community = CONFIG_SNMP_COMMUNITY_READ,
		.write_community = CONFIG_SNMP_COMMUNITY_WRITE,
		.sys_descr = "Zephyr RTOS SNMP Validation Node (WAN instance)",
		.sys_contact = "admin@example.com",
		.sys_name = "NUCLEO-H743ZI-WAN",
		.sys_location = "Lab Bench 1",
		.settings_prefix = "snmp/wan",
	};
	ret = snmp_agent_init(&g_agent_wan, &cfg_wan);
	if (ret < 0) {
		LOG_ERR("Failed to initialize WAN SNMP agent: %d", ret);
		return ret;
	}
	ret = app_mib_init(&g_agent_wan);
	if (ret < 0) {
		LOG_ERR("Failed to register Private MIB on WAN agent: %d", ret);
		return ret;
	}
#endif

	/* Check if interface already has IP assigned; if not, wait for DHCP */
	struct net_if *iface = s_snmp_iface;
	bool has_ip = false;
	char ip_buf[NET_IPV4_ADDR_LEN] = "192.0.2.1";

	if (iface && iface->config.ip.ipv4) {
		for (int i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
			if (iface->config.ip.ipv4->unicast[i].ipv4.is_used &&
			    iface->config.ip.ipv4->unicast[i].ipv4.address.in_addr.s_addr != 0) {
				net_addr_ntop(
					AF_INET,
					&iface->config.ip.ipv4->unicast[i].ipv4.address.in_addr,
					ip_buf, sizeof(ip_buf));
				has_ip = true;
				break;
			}
		}
	}

	if (!has_ip) {
		LOG_INF("Waiting for DHCP lease...");
		k_sem_take(&s_ip_ready_sem, K_FOREVER);
		if (iface && iface->config.ip.ipv4) {
			for (int i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
				if (iface->config.ip.ipv4->unicast[i].ipv4.is_used) {
					net_addr_ntop(AF_INET,
						      &iface->config.ip.ipv4->unicast[i]
							       .ipv4.address.in_addr,
						      ip_buf, sizeof(ip_buf));
					break;
				}
			}
		}
	}

	print_banner(ip_buf);

	/* 7. Open the UDP listeners only now that the interface has an address.
	 *    The coldStart trap stays ordered exactly like the old transport facade
	 *    documented it (former snmp_service.c): first an assigned IPv4 address,
	 *    then the listener, then the notification.
	 */
	ret = snmp_agent_start(&g_agent_lan);
	if (ret < 0) {
		LOG_ERR("Failed to start LAN SNMP agent: %d", ret);
		return ret;
	}
#if defined(CONFIG_SNMP_SAMPLE_SECOND_INSTANCE)
	ret = snmp_agent_start(&g_agent_wan);
	if (ret < 0) {
		LOG_ERR("Failed to start WAN SNMP agent: %d", ret);
		return ret;
	}
#endif

#if defined(CONFIG_SNMP_TRAP_ENABLED)
	snmp_trap_send_coldstart(&g_agent_lan);
	LOG_INF("ColdStart Trap dispatched to %s", CONFIG_SNMP_SAMPLE_TRAP_DEST);
#endif

	/* 8. Main loop: periodically update sensor values */
	while (1) {
		k_sleep(K_SECONDS(2));
		app_sensor_update_periodic();
	}

	return 0;
}
