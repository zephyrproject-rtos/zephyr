/*
 * Copyright (c) 2026 Leica Geosystems AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/cellular.h>
#include <zephyr/drivers/modem/modem_cellular.h>
#include <zephyr/shell/shell.h>

static bool device_is_modem_cellular(const struct device *dev)
{
	return dev->api == &modem_cellular_api;
}

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_filter(idx, device_is_modem_cellular);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

static const struct device *modem_cellular_shell_get_device(const struct shell *sh,
							    const char *name)
{
	const struct device *dev = shell_device_get_binding(name);

	if (dev == NULL) {
		shell_error(sh, "device %s not found", name);
		return NULL;
	}

	if (!device_is_modem_cellular(dev)) {
		shell_error(sh, "device %s is not a modem_cellular driver instance", name);
		return NULL;
	}

	return dev;
}

static int cmd_modem_cellular_pause(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int ret;

	dev = modem_cellular_shell_get_device(sh, argv[1]);
	if (dev == NULL) {
		return -ENODEV;
	}

	ret = cellular_modem_pause_periodic_script(dev);
	if (ret < 0) {
		shell_error(sh, "cellular_modem_pause_periodic_script: %d", ret);
		return ret;
	}

	return 0;
}

static int cmd_modem_cellular_resume(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int ret;

	dev = modem_cellular_shell_get_device(sh, argv[1]);
	if (dev == NULL) {
		return -ENODEV;
	}

	ret = cellular_modem_resume_periodic_script(dev);
	if (ret < 0) {
		shell_error(sh, "cellular_modem_resume_periodic_script: %d", ret);
		return ret;
	}

	return 0;
}

static const char *registration_name(enum cellular_registration_status status)
{
	switch (status) {
	case CELLULAR_REGISTRATION_NOT_REGISTERED:
		return "not registered";
	case CELLULAR_REGISTRATION_REGISTERED_HOME:
		return "registered, home";
	case CELLULAR_REGISTRATION_SEARCHING:
		return "searching";
	case CELLULAR_REGISTRATION_DENIED:
		return "denied";
	case CELLULAR_REGISTRATION_UNKNOWN:
		return "unknown";
	case CELLULAR_REGISTRATION_REGISTERED_ROAMING:
		return "registered, roaming";
	case CELLULAR_REGISTRATION_SMS_ONLY_HOME:
		return "sms only, home";
	case CELLULAR_REGISTRATION_SMS_ONLY_ROAMING:
		return "sms only, roaming";
	case CELLULAR_REGISTRATION_EMERGENCY_ONLY:
		return "emergency bearer services only";
	case CELLULAR_REGISTRATION_CSFB_NOT_PREFERRED_HOME:
		return "csfb not preferred, home";
	case CELLULAR_REGISTRATION_CSFB_NOT_PREFERRED_ROAMING:
		return "csfb not preferred, roaming";
	case CELLULAR_REGISTRATION_RLOS:
		return "restricted local operator services";
	default:
		return "unknown";
	}
}

static const char *access_tech_name(enum cellular_access_technology tech)
{
	switch (tech) {
	case CELLULAR_ACCESS_TECHNOLOGY_GSM:
		return "GSM";
	case CELLULAR_ACCESS_TECHNOLOGY_GSM_EGPRS:
		return "EGPRS";
	case CELLULAR_ACCESS_TECHNOLOGY_UTRAN:
		return "UTRAN";
	case CELLULAR_ACCESS_TECHNOLOGY_E_UTRAN:
		return "E-UTRAN (LTE/LTE-M)";
	case CELLULAR_ACCESS_TECHNOLOGY_E_UTRAN_NB_S1:
		return "E-UTRAN NB-S1 (NB-IoT)";
	case CELLULAR_ACCESS_TECHNOLOGY_NR_5G_CN:
		return "NR";
	default:
		return "unknown";
	}
}

static bool registration_is_registered(enum cellular_registration_status status)
{
	return (status == CELLULAR_REGISTRATION_REGISTERED_HOME) ||
	       (status == CELLULAR_REGISTRATION_REGISTERED_ROAMING);
}

static void print_modem_info(const struct shell *sh, const struct device *dev,
			     enum cellular_modem_info_type type, const char *label)
{
	/* Large enough for the longest string the driver stores (firmware version, model) */
	char info[MODEM_CELLULAR_DATA_FW_VERSION_LEN];

	if ((cellular_get_modem_info(dev, type, info, sizeof(info)) == 0) && (info[0] != '\0')) {
		shell_print(sh, "%-20s%s", label, info);
	}
}

/* Print the serving cell reported by the driver; return false when there is none. */
static bool print_serving_cell(const struct shell *sh, const struct device *dev)
{
	struct cellular_evt_network_status net;

	/* After a deregistration the driver keeps a report without a cell. */
	if ((cellular_get_network_status(dev, &net) != 0) ||
	    !registration_is_registered(net.status)) {
		return false;
	}

	shell_print(sh, "%-20s%s", "access technology:", access_tech_name(net.access_tech));

	if ((net.access_tech != CELLULAR_ACCESS_TECHNOLOGY_E_UTRAN) &&
	    (net.access_tech != CELLULAR_ACCESS_TECHNOLOGY_E_UTRAN_NB_S1)) {
		return false;
	}

	/* The MNC is an integer: its width (2 or 3 digits) is lost, so no MCC-MNC pair */
	shell_print(sh, "%-20s%03u", "mcc:", net.cell.lte.mcc);
	shell_print(sh, "%-20s%u", "mnc:", net.cell.lte.mnc);
	/* Band 0 marks a driver that does not know the band */
	if (net.cell.lte.band != 0U) {
		shell_print(sh, "%-20s%u (earfcn %u)", "band:", net.cell.lte.band,
			    (unsigned int)net.cell.lte.earfcn);
	} else {
		shell_print(sh, "%-20s%u", "earfcn:", (unsigned int)net.cell.lte.earfcn);
	}
	if (net.cell.lte.gci != CELLULAR_EUTRAN_CELL_ID_INVALID) {
		shell_print(sh, "%-20s%07x (tac %04x, pci %u)",
			    "cell:", (unsigned int)net.cell.lte.gci, (unsigned int)net.cell.lte.tac,
			    net.cell.lte.phys_cell_id);
	}
	shell_print(sh, "%-20s%d dBm", "rsrp:", net.cell.lte.rsrp);
	shell_print(sh, "%-20s%d dB", "rsrq:", net.cell.lte.rsrq);

	return true;
}

static void print_signal_value(const struct shell *sh, const struct device *dev,
			       enum cellular_signal_type type, const char *label, const char *unit)
{
	int16_t value;
	int ret;

	ret = cellular_get_signal(dev, type, &value);
	if (ret == 0) {
		shell_print(sh, "%-20s%d %s", label, value, unit);
	} else if (ret == -EBUSY) {
		/* Another script, such as the periodic one, holds the chat */
		shell_print(sh, "%-20s%s", label, "busy, try again");
	} else {
		/* Not available in this state, or not reported by the modem */
	}
}

static void print_signal(const struct shell *sh, const struct device *dev)
{
	print_signal_value(sh, dev, CELLULAR_SIGNAL_RSSI, "rssi:", "dBm");
	print_signal_value(sh, dev, CELLULAR_SIGNAL_RSRP, "rsrp:", "dBm");
	print_signal_value(sh, dev, CELLULAR_SIGNAL_RSRQ, "rsrq:", "dB");
}

/* A technology the driver never reads stays not registered, so print it only when set */
static void print_registration_if_set(const struct shell *sh, const struct device *dev,
				      enum cellular_access_technology tech, const char *label)
{
	enum cellular_registration_status reg;

	if ((cellular_get_registration_status(dev, tech, &reg) == 0) &&
	    (reg != CELLULAR_REGISTRATION_NOT_REGISTERED)) {
		shell_print(sh, "%-20s%s", label, registration_name(reg));
	}
}

static int cmd_modem_cellular_info(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	enum cellular_registration_status reg;
	bool registered = false;

	ARG_UNUSED(argc);

	dev = modem_cellular_shell_get_device(sh, argv[1]);
	if (dev == NULL) {
		return -ENODEV;
	}

	print_modem_info(sh, dev, CELLULAR_MODEM_INFO_MANUFACTURER, "manufacturer:");
	print_modem_info(sh, dev, CELLULAR_MODEM_INFO_MODEL_ID, "model:");
	print_modem_info(sh, dev, CELLULAR_MODEM_INFO_FW_VERSION, "firmware:");
	print_modem_info(sh, dev, CELLULAR_MODEM_INFO_IMEI, "imei:");
	print_modem_info(sh, dev, CELLULAR_MODEM_INFO_SIM_ICCID, "iccid:");
	print_modem_info(sh, dev, CELLULAR_MODEM_INFO_SIM_IMSI, "imsi:");

	if (cellular_get_registration_status(dev, CELLULAR_ACCESS_TECHNOLOGY_E_UTRAN, &reg) == 0) {
		shell_print(sh, "%-20s%s", "lte registration:", registration_name(reg));
		registered = registration_is_registered(reg);
	}
	print_registration_if_set(sh, dev, CELLULAR_ACCESS_TECHNOLOGY_GSM, "gsm registration:");
	print_registration_if_set(sh, dev, CELLULAR_ACCESS_TECHNOLOGY_UTRAN, "gprs registration:");
	print_registration_if_set(sh, dev, CELLULAR_ACCESS_TECHNOLOGY_NR_5G_CN, "5g registration:");

	/*
	 * The serving cell report is LTE only, and the driver keeps the last one while the
	 * modem is suspended, so print one only while registered on LTE. Without it, ask the
	 * modem for the signal.
	 */
	if (!registered || !print_serving_cell(sh, dev)) {
		print_signal(sh, dev);
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(modem_cellular_cmds,
			       SHELL_CMD_ARG(pause, &dsub_device_name,
					     SHELL_HELP("Pause periodic chat script", "<device>"),
					     cmd_modem_cellular_pause, 2, 0),
			       SHELL_CMD_ARG(resume, &dsub_device_name,
					     SHELL_HELP("Resume periodic chat script", "<device>"),
					     cmd_modem_cellular_resume, 2, 0),
			       SHELL_CMD_ARG(info, &dsub_device_name,
					     SHELL_HELP("Print modem, SIM, registration and "
							"serving cell information",
							"<device>"),
					     cmd_modem_cellular_info, 2, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(modem_cellular, &modem_cellular_cmds,
		   "modem_cellular driver shell commands", NULL);
