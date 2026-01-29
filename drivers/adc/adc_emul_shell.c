/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc/adc_emul.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>

#define ADC_EMUL_HDL_LIST_ENTRY(node_id) { .dev = DEVICE_DT_GET(node_id) },

static const struct {
	const struct device *dev;
} adc_emul_list[] = {
	DT_FOREACH_STATUS_OKAY(zephyr_adc_emul, ADC_EMUL_HDL_LIST_ENTRY)
};

static const struct device *get_adc_emul(const char *device_name)
{
	for (size_t i = 0; i < ARRAY_SIZE(adc_emul_list); i++) {
		if (!strcmp(device_name, adc_emul_list[i].dev->name)) {
			return adc_emul_list[i].dev;
		}
	}

	/* The device name is provided by shell dynamic command completion. */
	__ASSERT_NO_MSG(false);
	return NULL;
}

static int cmd_adc_emul_raw(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	const struct device *dev = get_adc_emul(argv[-1]);
	char *endptr;
	unsigned long chan_ul;
	unsigned long raw_ul;
	int ret;

	if (!device_is_ready(dev)) {
		shell_error(sh, "ADC emulator device not ready");
		return -ENODEV;
	}

	endptr = NULL;
	chan_ul = strtoul(argv[1], &endptr, 0);
	if ((endptr == argv[1]) || (*endptr != '\0')) {
		shell_error(sh, "<channel> must be a number");
		return -EINVAL;
	}

	endptr = NULL;
	raw_ul = strtoul(argv[2], &endptr, 0);
	if ((endptr == argv[2]) || (*endptr != '\0') || (raw_ul > 0xFFFFUL)) {
		shell_error(sh, "<raw> must be 0..0xFFFF (supports 0x prefix)");
		return -EINVAL;
	}

	ret = adc_emul_const_raw_value_set(dev, (unsigned int)chan_ul, (uint32_t)raw_ul);
	if (ret != 0) {
		shell_error(sh, "adc_emul_const_raw_value_set failed: %d", ret);
		return ret;
	}

	shell_print(sh, "%s: ch%lu raw=0x%04lX (%lu)", dev->name, chan_ul, raw_ul, raw_ul);
	return 0;
}

static int cmd_adc_emul_mv(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	const struct device *dev = get_adc_emul(argv[-1]);
	char *endptr;
	unsigned long chan_ul;
	unsigned long mv_ul;
	int ret;

	if (!device_is_ready(dev)) {
		shell_error(sh, "ADC emulator device not ready");
		return -ENODEV;
	}

	endptr = NULL;
	chan_ul = strtoul(argv[1], &endptr, 0);
	if ((endptr == argv[1]) || (*endptr != '\0')) {
		shell_error(sh, "<channel> must be a number");
		return -EINVAL;
	}

	endptr = NULL;
	mv_ul = strtoul(argv[2], &endptr, 0);
	if ((endptr == argv[2]) || (*endptr != '\0')) {
		shell_error(sh, "<mv> must be a number");
		return -EINVAL;
	}

	ret = adc_emul_const_value_set(dev, (unsigned int)chan_ul, (uint32_t)mv_ul);
	if (ret != 0) {
		shell_error(sh, "adc_emul_const_value_set failed: %d", ret);
		return ret;
	}

	shell_print(sh, "%s: ch%lu mv=%lu", dev->name, chan_ul, mv_ul);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_adc_emul_cmds,
	SHELL_CMD_ARG(raw, NULL,
		"Set constant RAW code: raw <channel> <value> (decimal or 0x....)",
		cmd_adc_emul_raw, 3, 0),
	SHELL_CMD_ARG(mv, NULL,
		"Set constant input voltage in mV: mv <channel> <mv>",
		cmd_adc_emul_mv, 3, 0),
	SHELL_SUBCMD_SET_END);

static void cmd_adc_emul_dev_get(size_t idx, struct shell_static_entry *entry)
{
	if (idx < ARRAY_SIZE(adc_emul_list)) {
		entry->syntax = adc_emul_list[idx].dev->name;
		entry->handler = NULL;
		entry->subcmd = &sub_adc_emul_cmds;
		entry->help = "Select subcommand for ADC emulator device.";
	} else {
		entry->syntax = NULL;
	}
}

SHELL_DYNAMIC_CMD_CREATE(sub_adc_emul_dev, cmd_adc_emul_dev_get);

SHELL_CMD_REGISTER(adc_emul, &sub_adc_emul_dev, "ADC emulator commands", NULL);
