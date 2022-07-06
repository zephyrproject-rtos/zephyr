/*
 * Copyright (c) 2020, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/sys/printk.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_conn_mgr.h>
#include <zephyr/drivers/modem/gsm_ppp.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sample_gsm_ppp, LOG_LEVEL_DBG);

static const struct device *gsm_dev;
static struct net_mgmt_event_callback mgmt_cb;
static bool starting = IS_ENABLED(CONFIG_GSM_PPP_AUTOSTART);

static int cmd_sample_modem_suspend(const struct shell *shell,
				    size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!starting) {
		shell_fprintf(shell, SHELL_NORMAL, "Modem is already stopped.\n");
		return -ENOEXEC;
	}

	gsm_ppp_stop(gsm_dev);
	starting = false;

	return 0;
}

static int cmd_sample_modem_resume(const struct shell *shell,
				   size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (starting) {
		shell_fprintf(shell, SHELL_NORMAL, "Modem is already started.\n");
		return -ENOEXEC;
	}

	gsm_ppp_start(gsm_dev);
	starting = true;

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sample_commands,
	SHELL_CMD(resume, NULL,
		  "Resume the modem\n",
		  cmd_sample_modem_resume),
	SHELL_CMD(suspend, NULL,
		  "Suspend the modem\n",
		  cmd_sample_modem_suspend),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(sample, &sample_commands,
		   "Sample application commands", NULL);


static void event_handler(struct net_mgmt_event_callback *cb,
			  uint32_t mgmt_event, struct net_if *iface)
{
	ARG_UNUSED(cb);
	ARG_UNUSED(iface);

	if ((mgmt_event & (NET_EVENT_L4_CONNECTED
			   | NET_EVENT_L4_DISCONNECTED)) != mgmt_event) {
		return;
	}

	if (mgmt_event == NET_EVENT_L4_CONNECTED) {
		LOG_INF("Network connected");
		return;
	}

	if (mgmt_event == NET_EVENT_L4_DISCONNECTED) {
		LOG_INF("Network disconnected");
		return;
	}
}

static void modem_on_cb(const struct device *dev, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	LOG_INF("GSM modem on callback fired");
}

static void modem_off_cb(const struct device *dev, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	LOG_INF("GSM modem off callback fired");
}

int main(void)
{
	const struct device *uart_dev =
		DEVICE_DT_GET(DT_BUS(DT_INST(0, zephyr_gsm_ppp)));

	gsm_dev = DEVICE_DT_GET(DT_INST(0, zephyr_gsm_ppp));

	/* Optional register modem power callbacks */
	gsm_ppp_register_modem_power_callback(gsm_dev, modem_on_cb, modem_off_cb, NULL);

	LOG_INF("Board '%s' APN '%s' UART '%s' device %p (%s)",
		CONFIG_BOARD, CONFIG_MODEM_GSM_APN,
		uart_dev->name, uart_dev, gsm_dev->name);

	net_mgmt_init_event_callback(&mgmt_cb, event_handler,
				     NET_EVENT_L4_CONNECTED |
				     NET_EVENT_L4_DISCONNECTED);
	net_mgmt_add_event_callback(&mgmt_cb);

	return 0;
}
