/*
 * Copyright (c) 2018 Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include <zephyr/drivers/adc.h>
#include <ctype.h>
#include <zephyr/sys/util.h>
#include <zephyr/devicetree.h>


#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_shell);

#define CMD_HELP_ACQ_TIME \
	SHELL_HELP("Configure acquisition time", \
		   "<time> <unit>\nunits: us, ns, ticks")

#define CMD_HELP_CHANNEL \
	SHELL_HELP("Configure ADC channel", "<subcommand> [args]")

#define CMD_HELP_CH_ID \
	SHELL_HELP("Configure channel id", "<channel_id>")

#define CMD_HELP_DIFF \
	SHELL_HELP("Configure differential", "<0|1>")

#define CMD_HELP_CH_NEG \
	SHELL_HELP("Configure channel negative input", "<negative_input_id>")

#define CMD_HELP_CH_POS \
	SHELL_HELP("Configure channel positive input", "<positive_input_id>")

#define CMD_HELP_READ                                                                              \
	SHELL_HELP("Read adc value. Prints periodically if period is provided",                    \
		   "<channel> [period_ms]")

#define CMD_HELP_RES \
	SHELL_HELP("Configure resolution", "<resolution>")

#define CMD_HELP_REF	SHELL_HELP("Configure reference", NULL)
#define CMD_HELP_GAIN	SHELL_HELP("Configure gain", NULL)
#define CMD_HELP_PRINT	SHELL_HELP("Print current configuration", NULL)

static const char *cmd_adc_dev_get_help =
	SHELL_HELP("Select subcommand for ADC property label", NULL);

#define ADC_HDL_LIST_ENTRY(node_id)                                                                \
	{                                                                                          \
		.dev = DEVICE_DT_GET(node_id),                                                     \
		.channel_config =                                                                  \
			{                                                                          \
				.gain = ADC_GAIN_1,                                                \
				.reference = ADC_REF_INTERNAL,                                     \
				.acquisition_time = ADC_ACQ_TIME_DEFAULT,                          \
				.channel_id = 0,                                                   \
			},                                                                         \
		.resolution = 0,                                                                   \
	},

#define CHOSEN_STR_LEN 20
static char chosen_reference[CHOSEN_STR_LEN + 1] = "INTERNAL";
static char chosen_gain[CHOSEN_STR_LEN + 1] = "1";

static struct adc_hdl {
	const struct device *dev;
	struct adc_channel_cfg channel_config;
	uint8_t resolution;
} adc_list[] = {
	DT_FOREACH_CLASS_STATUS_OKAY(adc, ADC_HDL_LIST_ENTRY)
};

static struct adc_hdl *get_adc(const char *device_label)
{
	for (int i = 0; i < ARRAY_SIZE(adc_list); i++) {
		if (!strcmp(device_label, adc_list[i].dev->name)) {
			return &adc_list[i];
		}
	}

	/* This will never happen because ADC was prompted by shell */
	__ASSERT_NO_MSG(false);
	return NULL;
}

static int cmd_adc_ch_id(const struct shell *sh, size_t argc, char **argv)
{
	/* -2: index of ADC label name */
	struct adc_hdl *adc = get_adc(argv[-2]);
	int retval = 0;

	if (!device_is_ready(adc->dev)) {
		shell_error(sh, "ADC device not ready");
		return -ENODEV;
	}

	if (isdigit((unsigned char)argv[1][0]) == 0) {
		shell_error(sh, "<channel> must be digits");
		return -EINVAL;
	}

	adc->channel_config.channel_id = (uint8_t)strtol(argv[1], NULL, 10);
	retval = adc_channel_setup(adc->dev, &adc->channel_config);
	LOG_DBG("Channel setup returned %i", retval);

	return retval;
}

static int cmd_adc_ch_diff(const struct shell *sh, size_t argc, char **argv)
{
	/* -2: index of ADC label name */
	struct adc_hdl *adc = get_adc(argv[-2]);
	int retval = 0;
	char *endptr;
	long diff;

	if (!device_is_ready(adc->dev)) {
		shell_error(sh, "ADC device not ready");
		return -ENODEV;
	}

	endptr = argv[1];
	diff = strtol(argv[1], &endptr, 10);
	if ((endptr == argv[1]) || ((diff != 0) && (diff != 1))) {
		shell_error(sh, "<differential> must be 0 or 1");
		return -EINVAL;
	}

	adc->channel_config.differential = (uint8_t)diff;
	retval = adc_channel_setup(adc->dev, &adc->channel_config);
	LOG_DBG("Channel setup returned %i", retval);

	return retval;
}

static int cmd_adc_ch_neg(const struct shell *sh, size_t argc, char **argv)
{
#if CONFIG_ADC_CONFIGURABLE_INPUTS
	/* -2: index of ADC label name */
	struct adc_hdl *adc = get_adc(argv[-2]);
	int retval = 0;

	if (!device_is_ready(adc->dev)) {
		shell_error(sh, "ADC device not ready");
		return -ENODEV;
	}

	if (isdigit((unsigned char)argv[1][0]) == 0) {
		shell_error(sh, "<negative input> must be digits");
		return -EINVAL;
	}

	adc->channel_config.input_negative = (uint8_t)strtol(argv[1], NULL, 10);
	retval = adc_channel_setup(adc->dev, &adc->channel_config);
	LOG_DBG("Channel setup returned %i", retval);

	return retval;
#else
	return -EINVAL;
#endif
}

static int cmd_adc_ch_pos(const struct shell *sh, size_t argc, char **argv)
{
#if CONFIG_ADC_CONFIGURABLE_INPUTS
	/* -2: index of ADC label name */
	struct adc_hdl *adc = get_adc(argv[-2]);
	int retval = 0;

	if (!device_is_ready(adc->dev)) {
		shell_error(sh, "ADC device not ready");
		return -ENODEV;
	}

	if (isdigit((unsigned char)argv[1][0]) == 0) {
		shell_error(sh, "<positive input> must be digits");
		return -EINVAL;
	}

	adc->channel_config.input_positive = (uint8_t)strtol(argv[1], NULL, 10);
	retval = adc_channel_setup(adc->dev, &adc->channel_config);
	LOG_DBG("Channel setup returned %i", retval);

	return retval;
#else
	return -EINVAL;
#endif
}

static int cmd_adc_gain(const struct shell *sh, size_t argc, char **argv,
			void *data)
{
	/* -2: index of ADC label name */
	struct adc_hdl *adc = get_adc(argv[-2]);
	enum adc_gain gain = (enum adc_gain)data;
	int retval = -EINVAL;

	if (!device_is_ready(adc->dev)) {
		shell_error(sh, "ADC device not ready");
		return -ENODEV;
	}

	adc->channel_config.gain = gain;
	int len = strlen(argv[0]) > CHOSEN_STR_LEN ? CHOSEN_STR_LEN
						   : strlen(argv[0]);
	memcpy(chosen_gain, argv[0], len);
	chosen_gain[len] = '\0';
	retval = adc_channel_setup(adc->dev, &adc->channel_config);
	LOG_DBG("Channel setup returned %i", retval);

	return retval;
}

static int cmd_adc_acq(const struct shell *sh, size_t argc, char **argv)
{
	/* -1 index of ADC label name */
	struct adc_hdl *adc = get_adc(argv[-1]);
	uint16_t acq_time;
	int retval;

	if (!device_is_ready(adc->dev)) {
		shell_error(sh, "ADC device not ready");
		return -ENODEV;
	}

	if (isdigit((unsigned char)argv[1][0]) == 0) {
		shell_error(sh, "<time> must be digits");
		return -EINVAL;
	}

	acq_time = (uint16_t)strtol(argv[1], NULL, 10);
	if (!strcmp(argv[2], "us")) {
		adc->channel_config.acquisition_time =
			ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, acq_time);
	} else if (!strcmp(argv[2], "ns")) {
		adc->channel_config.acquisition_time =
			ADC_ACQ_TIME(ADC_ACQ_TIME_NANOSECONDS, acq_time);
	} else if (!strcmp(argv[2], "ticks")) {
		adc->channel_config.acquisition_time =
			ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, acq_time);
	} else {
		adc->channel_config.acquisition_time =
			ADC_ACQ_TIME_DEFAULT;
	}
	retval = adc_channel_setup(adc->dev, &adc->channel_config);
	LOG_DBG("Channel setup returned %i", retval);

	return retval;
}
static int cmd_adc_reso(const struct shell *sh, size_t argc, char **argv)
{
	/* -1 index of ADC label name */
	struct adc_hdl *adc = get_adc(argv[-1]);
	int retval;

	if (!device_is_ready(adc->dev)) {
		shell_error(sh, "ADC device not ready");
		return -ENODEV;
	}

	if (isdigit((unsigned char)argv[1][0]) == 0) {
		shell_error(sh, "<resolution> must be digits");
		return -EINVAL;
	}

	adc->resolution = (uint8_t)strtol(argv[1], NULL, 10);
	retval = adc_channel_setup(adc->dev, &adc->channel_config);

	return retval;
}

static int cmd_adc_ref(const struct shell *sh, size_t argc, char **argv,
		       void *data)
{
	/* -2 index of ADC label name */
	struct adc_hdl *adc = get_adc(argv[-2]);
	enum adc_reference reference = (enum adc_reference)data;
	int retval = -EINVAL;

	if (!device_is_ready(adc->dev)) {
		shell_error(sh, "ADC device not ready");
		return -ENODEV;
	}

	int len = strlen(argv[0]) > CHOSEN_STR_LEN ? CHOSEN_STR_LEN
						   : strlen(argv[0]);
	memcpy(chosen_reference, argv[0], len);
	chosen_reference[len] = '\0';

	adc->channel_config.reference = reference;
	retval = adc_channel_setup(adc->dev, &adc->channel_config);
	LOG_DBG("Channel setup returned %i", retval);

	return retval;
}

static void adc_shell_read_bypass_cb(const struct shell *sh, uint8_t *data, size_t len,
				     void *user_data)
{
	ARG_UNUSED(sh);
	ARG_UNUSED(data);
	ARG_UNUSED(len);

	*(bool *)user_data = true;
}

#define BUFFER_SIZE 1
static int cmd_adc_read(const struct shell *sh, size_t argc, char **argv)
{
	uint8_t adc_channel_id = strtol(argv[1], NULL, 10);
	/* -1 index of adc label name */
	struct adc_hdl *adc = get_adc(argv[-1]);
	int16_t m_sample_buffer[BUFFER_SIZE];
	int retval;

	if (!device_is_ready(adc->dev)) {
		shell_error(sh, "ADC device not ready");
		return -ENODEV;
	}

	adc->channel_config.channel_id = adc_channel_id;
	const struct adc_sequence sequence = {
		.channels	= BIT(adc->channel_config.channel_id),
		.buffer		= m_sample_buffer,
		.buffer_size	= sizeof(m_sample_buffer),
		.resolution	= adc->resolution,
	};

	retval = adc_read(adc->dev, &sequence);
	if (retval >= 0) {
		shell_print(sh, "read: %i", m_sample_buffer[0]);
	}

	if (argc == 2) { /* One-time print; Non-periodic */
		return retval;
	}

	/* Periodic print; argc=3 */
	int period_ms = strtol(argv[2], NULL, 10);

	if (period_ms < 1) {
		shell_error(sh, "<period_ms> must be at least 1");
		return -EINVAL;
	}

	bool stop = false;
	bool msg_one_shot = true;

	shell_set_bypass(sh, adc_shell_read_bypass_cb, &stop);

	while (!stop) {
		retval = adc_read(adc->dev, &sequence);
		if (retval >= 0) {
			shell_print(sh, "read: %i", m_sample_buffer[0]);
		} else {
			break;
		}

		if (msg_one_shot) {
			msg_one_shot = false;
			shell_print(sh, "Hit any key to exit");
		}

		k_msleep(period_ms);
	}

	shell_set_bypass(sh, NULL, NULL);

	return stop ? 0 : retval;
}

static void adc_shell_print_acq_time(const struct shell *sh, uint16_t acq_time)
{
	const char *unit;

	if (acq_time == ADC_ACQ_TIME_DEFAULT) {
		shell_print(sh, "Acquisition Time: default");
		return;
	}

	if (acq_time == ADC_ACQ_TIME_MAX) {
		shell_print(sh, "Acquisition Time: max");
		return;
	}

	switch (ADC_ACQ_TIME_UNIT(acq_time)) {
	case ADC_ACQ_TIME_MICROSECONDS:
		unit = "us";
		break;
	case ADC_ACQ_TIME_NANOSECONDS:
		unit = "ns";
		break;
	case ADC_ACQ_TIME_TICKS:
		unit = "ticks";
		break;
	default:
		shell_print(sh, "Acquisition Time: invalid (%u)", acq_time);
		return;
	}

	shell_print(sh, "Acquisition Time: %u %s", (unsigned int)ADC_ACQ_TIME_VALUE(acq_time),
		    unit);
}

static int cmd_adc_print(const struct shell *sh, size_t argc, char **argv)
{
	/* -1 index of ADC label name */
	struct adc_hdl *adc = get_adc(argv[-1]);

	shell_print(sh,
		    "%s:\n"
		    "Gain: %s\n"
		    "Reference: %s",
		    adc->dev->name, chosen_gain, chosen_reference);

	adc_shell_print_acq_time(sh, adc->channel_config.acquisition_time);

	shell_print(sh,
		    "Channel ID: %u\n"
		    "Differential: %u\n"
		    "Resolution: %u",
		    adc->channel_config.channel_id, adc->channel_config.differential,
		    adc->resolution);
#if CONFIG_ADC_CONFIGURABLE_INPUTS
	shell_print(sh, "Input positive: %u", adc->channel_config.input_positive);
	if (adc->channel_config.differential != 0) {
		shell_print(sh, "Input negative: %u", adc->channel_config.input_negative);
	}
#endif
	return 0;
}

SHELL_SUBCMD_DICT_SET_CREATE(sub_ref_cmds, cmd_adc_ref, (VDD_1, ADC_REF_VDD_1, "VDD"),
			     (VDD_1_2, ADC_REF_VDD_1_2, "VDD/2"),
			     (VDD_1_3, ADC_REF_VDD_1_3, "VDD/3"),
			     (VDD_1_4, ADC_REF_VDD_1_4, "VDD/4"),
			     (INTERNAL, ADC_REF_INTERNAL, "Internal"),
			     (EXTERNAL_0, ADC_REF_EXTERNAL0, "External, input 0"),
			     (EXTERNAL_1, ADC_REF_EXTERNAL1, "External, input 1"));

SHELL_SUBCMD_DICT_SET_CREATE(sub_gain_cmds, cmd_adc_gain,
	(GAIN_1_6, ADC_GAIN_1_6, "x 1/6"),
	(GAIN_1_5, ADC_GAIN_1_5, "x 1/5"),
	(GAIN_1_4, ADC_GAIN_1_4, "x 1/4"),
	(GAIN_1_3, ADC_GAIN_1_3, "x 1/3"),
	(GAIN_1_2, ADC_GAIN_1_2, "x 1/2"),
	(GAIN_2_3, ADC_GAIN_2_3, "x 2/3"),
	(GAIN_1, ADC_GAIN_1, "x 1"),
	(GAIN_2, ADC_GAIN_2, "x 2"),
	(GAIN_3, ADC_GAIN_3, "x 3"),
	(GAIN_4, ADC_GAIN_4, "x 4"),
	(GAIN_8, ADC_GAIN_8, "x 8"),
	(GAIN_16, ADC_GAIN_16, "x 16"),
	(GAIN_32, ADC_GAIN_32, "x 32"),
	(GAIN_64, ADC_GAIN_64, "x 64")
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_channel_cmds,
	SHELL_CMD_ARG(id, NULL, CMD_HELP_CH_ID, cmd_adc_ch_id, 2, 0),
	SHELL_CMD_ARG(differential, NULL, CMD_HELP_DIFF, cmd_adc_ch_diff, 2, 0),
	SHELL_COND_CMD_ARG(CONFIG_ADC_CONFIGURABLE_INPUTS,
		negative, NULL, CMD_HELP_CH_NEG, cmd_adc_ch_neg, 2, 0),
	SHELL_COND_CMD_ARG(CONFIG_ADC_CONFIGURABLE_INPUTS,
		positive, NULL, CMD_HELP_CH_POS, cmd_adc_ch_pos, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_adc_cmds,
	/* Alphabetically sorted. */
	SHELL_CMD_ARG(acq_time, NULL, CMD_HELP_ACQ_TIME, cmd_adc_acq, 3, 0),
	SHELL_CMD_ARG(channel, &sub_channel_cmds, CMD_HELP_CHANNEL, NULL, 3, 0),
	SHELL_CMD(gain, &sub_gain_cmds, CMD_HELP_GAIN, NULL),
	SHELL_CMD_ARG(print, NULL, CMD_HELP_PRINT, cmd_adc_print, 1, 0),
	SHELL_CMD_ARG(read, NULL, CMD_HELP_READ, cmd_adc_read, 2, 1),
	SHELL_CMD(reference, &sub_ref_cmds, CMD_HELP_REF, NULL),
	SHELL_CMD_ARG(resolution, NULL, CMD_HELP_RES, cmd_adc_reso, 2, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

static void cmd_adc_dev_get(size_t idx, struct shell_static_entry *entry)
{
	if (idx < ARRAY_SIZE(adc_list)) {
		entry->syntax  = adc_list[idx].dev->name;
		entry->handler = NULL;
		entry->subcmd  = &sub_adc_cmds;
		entry->help    = cmd_adc_dev_get_help;
	} else {
		entry->syntax  = NULL;
	}
}
SHELL_DYNAMIC_CMD_CREATE(sub_adc_dev, cmd_adc_dev_get);

SHELL_CMD_REGISTER(adc, &sub_adc_dev, SHELL_HELP("ADC commands", NULL), NULL);
