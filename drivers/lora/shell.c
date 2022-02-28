/*
 * Copyright (c) 2020 Andreas Sandberg
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/lora.h>
#include <inttypes.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include <string.h>

LOG_MODULE_REGISTER(lora_shell, CONFIG_LORA_LOG_LEVEL);

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS(DEFAULT_RADIO_NODE, okay),
	     "No default LoRa radio specified in DT");

static struct lora_modem_config modem_config = {
	.frequency = 0,
	.bandwidth = BW_125_KHZ,
	.datarate = SF_10,
	.coding_rate = CR_4_5,
	.preamble_len = 8,
	.tx_power = 4,
};

static const int bw_table[] = {
	[BW_125_KHZ] = 125,
	[BW_250_KHZ] = 250,
	[BW_500_KHZ] = 500,
};

static int parse_long(long *out, const struct shell *sh, const char *arg)
{
	char *eptr;
	long lval;

	lval = strtol(arg, &eptr, 0);
	if (*eptr != '\0') {
		shell_error(sh, "'%s' is not an integer", arg);
		return -EINVAL;
	}

	*out = lval;
	return 0;
}

static int parse_long_range(long *out, const struct shell *sh,
			    const char *arg, const char *name, long min,
			    long max)
{
	int ret;

	ret = parse_long(out, sh, arg);
	if (ret < 0) {
		return ret;
	}

	if (*out < min || *out > max) {
		shell_error(sh, "Parameter '%s' is out of range. "
			    "Valid range is %li -- %li.",
			    name, min, max);
		return -EINVAL;
	}

	return 0;
}

static int parse_freq(uint32_t *out, const struct shell *sh, const char *arg)
{
	char *eptr;
	unsigned long val;

	val = strtoul(arg, &eptr, 0);
	if (*eptr != '\0') {
		shell_error(sh, "Invalid frequency, '%s' is not an integer",
			    arg);
		return -EINVAL;
	}

	if (val == ULONG_MAX) {
		shell_error(sh, "Frequency %s out of range", arg);
		return -EINVAL;
	}

	*out = (uint32_t)val;
	return 0;
}

static const struct device *get_modem(const struct shell *sh)
{
	const struct device *dev;

	dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE);

	if (!device_is_ready(dev)) {
		shell_error(sh, "LORA Radio device not ready");
		return NULL;
	}

	return dev;
}

static const struct device *get_configured_modem(const struct shell *sh)
{
	int ret;
	const struct device *dev;

	dev = get_modem(sh);
	if (!dev) {
		return NULL;
	}

	if (modem_config.frequency == 0) {
		shell_error(sh, "No frequency specified.");
		return NULL;
	}

	ret = lora_config(dev, &modem_config);
	if (ret < 0) {
		shell_error(sh, "LoRa config failed");
		return NULL;
	}

	return dev;
}

static int lora_conf_dump(const struct shell *sh)
{
	shell_print(sh, "  Frequency: %" PRIu32 " Hz",
		    modem_config.frequency);
	shell_print(sh, "  TX power: %" PRIi8 " dBm",
		    modem_config.tx_power);
	shell_print(sh, "  Bandwidth: %i kHz",
		    bw_table[modem_config.bandwidth]);
	shell_print(sh, "  Spreading factor: SF%i",
		    (int)modem_config.datarate);
	shell_print(sh, "  Coding rate: 4/%i",
		    (int)modem_config.coding_rate + 4);
	shell_print(sh, "  Preamble length: %" PRIu16,
		    modem_config.preamble_len);

	return 0;
}

static int lora_conf_set(const struct shell *sh, const char *param,
			 const char *value)
{
	long lval;

	if (!strcmp("freq", param)) {
		if (parse_freq(&modem_config.frequency, sh, value) < 0) {
			return -EINVAL;
		}
	} else if (!strcmp("tx-power", param)) {
		if (parse_long_range(&lval, sh, value,
				     "tx-power", INT8_MIN, INT8_MAX) < 0) {
			return -EINVAL;
		}
		modem_config.tx_power = lval;
	} else if (!strcmp("bw", param)) {
		if (parse_long_range(&lval, sh, value,
				     "bw", 0, INT16_MAX) < 0) {
			return -EINVAL;
		}
		switch (lval) {
		case 125:
			modem_config.bandwidth = BW_125_KHZ;
			break;
		case 250:
			modem_config.bandwidth = BW_250_KHZ;
			break;
		case 500:
			modem_config.bandwidth = BW_500_KHZ;
			break;
		default:
			shell_error(sh, "Invalid bandwidth: %ld", lval);
			return -EINVAL;
		}
	} else if (!strcmp("sf", param)) {
		if (parse_long_range(&lval, sh, value, "sf", 6, 12) < 0) {
			return -EINVAL;
		}
		modem_config.datarate = SF_6 + (unsigned int)lval - 6;
	} else if (!strcmp("cr", param)) {
		if (parse_long_range(&lval, sh, value, "cr", 5, 8) < 0) {
			return -EINVAL;
		}
		modem_config.coding_rate = CR_4_5 + (unsigned int)lval - 5;
	} else if (!strcmp("pre-len", param)) {
		if (parse_long_range(&lval, sh, value,
				     "pre-len", 0, UINT16_MAX) < 0) {
			return -EINVAL;
		}
		modem_config.preamble_len = lval;
	} else {
		shell_error(sh, "Unknown parameter '%s'", param);
		return -EINVAL;
	}

	return 0;
}

static int cmd_lora_conf(const struct shell *sh, size_t argc, char **argv)
{
	int i;
	int ret;

	if (argc < 2) {
		return lora_conf_dump(sh);
	}

	for (i = 1; i < argc; i += 2) {
		if (i + 1 >= argc) {
			shell_error(sh, "'%s' expects an argument",
				    argv[i]);
			return -EINVAL;
		}

		ret = lora_conf_set(sh, argv[i], argv[i + 1]);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

static int cmd_lora_send(const struct shell *sh,
			size_t argc, char **argv)
{
	int ret;
	const struct device *dev;

	modem_config.tx = true;
	dev = get_configured_modem(sh);
	if (!dev) {
		return -ENODEV;
	}

	ret = lora_send(dev, argv[1], strlen(argv[1]));
	if (ret < 0) {
		shell_error(sh, "LoRa send failed: %i", ret);
		return ret;
	}

	return 0;
}

static int cmd_lora_recv(const struct shell *sh, size_t argc, char **argv)
{
	static char buf[0xff];
	const struct device *dev;
	long timeout = 0;
	int ret;
	int16_t rssi;
	int8_t snr;

	modem_config.tx = false;
	dev = get_configured_modem(sh);
	if (!dev) {
		return -ENODEV;
	}

	if (argc >= 2 && parse_long_range(&timeout, sh, argv[1],
					  "timeout", 0, INT_MAX) < 0) {
		return -EINVAL;
	}

	ret = lora_recv(dev, buf, sizeof(buf),
			timeout ? K_MSEC(timeout) : K_FOREVER, &rssi, &snr);
	if (ret < 0) {
		shell_error(sh, "LoRa recv failed: %i", ret);
		return ret;
	}

	shell_hexdump(sh, buf, ret);
	shell_print(sh, "RSSI: %" PRIi16 " dBm, SNR:%" PRIi8 " dBm",
		    rssi, snr);

	return 0;
}

static int cmd_lora_test_cw(const struct shell *sh,
			    size_t argc, char **argv)
{
	const struct device *dev;
	int ret;
	uint32_t freq;
	long power, duration;

	dev = get_modem(sh);
	if (!dev) {
		return -ENODEV;
	}

	if (parse_freq(&freq, sh, argv[1]) < 0 ||
	    parse_long_range(&power, sh, argv[2],
			     "power", INT8_MIN, INT8_MAX) < 0 ||
	    parse_long_range(&duration, sh, argv[3],
			     "duration", 0, UINT16_MAX) < 0) {
		return -EINVAL;
	}

	ret = lora_test_cw(dev, (uint32_t)freq, (int8_t)power, (uint16_t)duration);
	if (ret < 0) {
		shell_error(sh, "LoRa test CW failed: %i", ret);
		return ret;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_lora,
	SHELL_CMD(config, NULL,
		  "Configure the LoRa radio\n"
		  " Usage: config [freq <Hz>] [tx-power <dBm>] [bw <kHz>] "
		  "[sf <int>] [cr <int>] [pre-len <int>]\n",
		  cmd_lora_conf),
	SHELL_CMD_ARG(send, NULL,
		      "Send LoRa packet\n"
		      " Usage: send <data>",
		      cmd_lora_send, 2, 0),
	SHELL_CMD_ARG(recv, NULL,
		      "Receive LoRa packet\n"
		      " Usage: recv [timeout (ms)]",
		      cmd_lora_recv, 1, 1),
	SHELL_CMD_ARG(test_cw, NULL,
		  "Send a continuous wave\n"
		  " Usage: test_cw <freq (Hz)> <power (dBm)> <duration (s)>",
		  cmd_lora_test_cw, 4, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(lora, &sub_lora, "LoRa commands", NULL);
