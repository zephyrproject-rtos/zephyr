/*
 * Copyright (c) 2018 Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <shell/shell.h>
#include <stdlib.h>
#include <adc.h>
#include <ctype.h>
#include <misc/util.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(adc_shell);

struct adc_hdl {
	char *device_name;
	struct adc_channel_cfg channel_config;
	u8_t resolution;
};

struct adc_hdl adc_list[] = {
#ifdef CONFIG_ADC_0
	{
		.device_name = DT_ADC_0_NAME,
		.channel_config = {
			.gain = ADC_GAIN_1,
			.reference = ADC_REF_INTERNAL,
			.acquisition_time = ADC_ACQ_TIME_DEFAULT,
			.channel_id = 0,
		},
		.resolution = 0,
	},
#endif
#ifdef CONFIG_ADC_1
	{
		.device_name = DT_ADC_1_NAME,
		.channel_config = {
			.gain = ADC_GAIN_1,
			.reference = ADC_REF_INTERNAL,
			.acquisition_time = ADC_ACQ_TIME_DEFAULT,
			.channel_id = 0,
		},
		.resolution = 0,
	},
#endif
#ifdef CONFIG_ADC_2
	{
		.device_name = DT_ADC_2_NAME,
		.channel_config = {
			.gain = ADC_GAIN_1,
			.reference = ADC_REF_INTERNAL,
			.acquisition_time = ADC_ACQ_TIME_DEFAULT,
			.channel_id = 0,
		},
		.resolution = 0,
	},
#endif
};

struct args_index {
	s8_t adc;
	s8_t parent_adc;
	u8_t channel;
	u8_t conf;
	u8_t acq_unit;
};

struct args_number {
	u8_t help;
	u8_t channel;
	u8_t acq_time;
	u8_t resolution;
	u8_t read;
};

static const struct args_index args_indx = {
	.adc = -1,
	.parent_adc = -2,
	.channel = 1,
	.conf = 1,
	.acq_unit = 2,
};

static const struct args_number args_no = {
	.help = 1,
	.channel = 2,
	.acq_time = 3,
	.resolution = 2,
	.read = 2,
};

/** get_adc_from_list returns the number entry of the adc in the adc_list,
 * returns -ENODEV if it doesn't exist
 */
static int get_adc_from_list(char *name)
{
	int adc_idx;

	for (adc_idx = 0; adc_idx < ARRAY_SIZE(adc_list); adc_idx++) {
		if (!strcmp(name, adc_list[adc_idx].device_name)) {
			return adc_idx;
		}
	}
	return -ENODEV;
}

static int cmd_adc_channel(const struct shell *shell, size_t argc, char **argv)
{
	int retval = 0;
	struct device *adc_dev;
	int chosen_adc;

	if (argc != args_no.channel) {
		shell_fprintf(shell, SHELL_NORMAL,
				"Usage: channel <channel_id>\n");
		return 0;
	}

	chosen_adc = get_adc_from_list(argv[args_indx.adc]);
	if (chosen_adc < 0) {
		shell_error(shell, "Device not in device list");
		return -EINVAL;
	}

	adc_dev = device_get_binding(adc_list[chosen_adc].device_name);
	if (adc_dev == NULL) {
		shell_error(shell, "ADC device not found");
		return -ENODEV;
	}
	if (!isdigit(argv[args_indx.conf][0])) {
		shell_error(shell, "<channel> must be digits");
		return -EINVAL;
	}
	adc_list[chosen_adc].channel_config.channel_id =
		(u8_t)strtol(argv[args_indx.conf], NULL, 10);
	retval = adc_channel_setup(adc_dev,
			&adc_list[chosen_adc].channel_config);
	LOG_DBG("Channel setup returned %i\n", retval);
	return retval;
}

struct gain_string_to_enum {
	char *string;
	enum adc_gain gain;
};

static const struct gain_string_to_enum gain_list[] = {
	{ .string = "ADC_GAIN_1_6", .gain = ADC_GAIN_1_6 },
	{ .string = "ADC_GAIN_1_5", .gain = ADC_GAIN_1_5 },
	{ .string = "ADC_GAIN_1_4", .gain = ADC_GAIN_1_4 },
	{ .string = "ADC_GAIN_1_3", .gain = ADC_GAIN_1_3 },
	{ .string = "ADC_GAIN_1_2", .gain = ADC_GAIN_1_2 },
	{ .string = "ADC_GAIN_2_3", .gain = ADC_GAIN_2_3 },
	{ .string = "ADC_GAIN_1", .gain = ADC_GAIN_1 },
	{ .string = "ADC_GAIN_2", .gain = ADC_GAIN_2 },
	{ .string = "ADC_GAIN_3", .gain = ADC_GAIN_3 },
	{ .string = "ADC_GAIN_4", .gain = ADC_GAIN_4 },
	{ .string = "ADC_GAIN_8", .gain = ADC_GAIN_8 },
	{ .string = "ADC_GAIN_16", .gain = ADC_GAIN_16 },
	{ .string = "ADC_GAIN_32", .gain = ADC_GAIN_32 },
	{ .string = "ADC_GAIN_64", .gain = ADC_GAIN_64 }
};

static int cmd_adc_gain(const struct shell *shell, size_t argc, char **argv)
{
	int retval = -EINVAL;
	struct device *adc_dev;
	int chosen_adc;
	int i;

	chosen_adc = get_adc_from_list(argv[args_indx.parent_adc]);
	if (chosen_adc < 0) {
		shell_error(shell, "Device not in device list");
		return -EINVAL;
	}

	adc_dev = device_get_binding(adc_list[chosen_adc].device_name);
	if (adc_dev == NULL) {
		shell_error(shell, "ADC device not found");
		return -ENODEV;
	}
	for (i = 0; i < ARRAY_SIZE(gain_list); i++) {
		if (!strcmp(argv[0], gain_list[i].string)) {
			adc_list[chosen_adc].channel_config.gain =
				gain_list[i].gain;
			retval = adc_channel_setup(adc_dev,
					&adc_list[chosen_adc].channel_config);
			LOG_DBG("Channel setup returned %i\n", retval);
			break;
		}
	}
	return retval;
}


static int cmd_adc_acq(const struct shell *shell, size_t argc, char **argv)
{
	int retval = 0;
	struct device *adc_dev;
	int chosen_adc;
	u16_t acq_time;

	if (argc != args_no.acq_time) {
		shell_fprintf(shell, SHELL_NORMAL,
				"Usage: acq_time <time> <unit>\nunits: us, ns, ticks\n");
		return 0;
	}

	chosen_adc = get_adc_from_list(argv[args_indx.adc]);
	if (chosen_adc < 0) {
		shell_error(shell, "Device not in device list");
		return -EINVAL;
	}

	adc_dev = device_get_binding(adc_list[chosen_adc].device_name);
	if (adc_dev == NULL) {
		shell_error(shell, "ADC device not found");
		return -ENODEV;
	}

	if (!isdigit(argv[args_indx.conf][0])) {
		shell_error(shell, "<time> must be digits");
		return -EINVAL;
	}
	acq_time = (u16_t)strtol(argv[args_indx.conf], NULL, 10);
	if (!strcmp(argv[args_indx.acq_unit], "us")) {
		adc_list[chosen_adc].channel_config.acquisition_time =
			ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, acq_time);
	} else if (!strcmp(argv[args_indx.acq_unit], "ns")) {
		adc_list[chosen_adc].channel_config.acquisition_time =
			ADC_ACQ_TIME(ADC_ACQ_TIME_NANOSECONDS, acq_time);
	} else if (!strcmp(argv[args_indx.acq_unit], "ticks")) {
		adc_list[chosen_adc].channel_config.acquisition_time =
			ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, acq_time);
	} else {
		adc_list[chosen_adc].channel_config.acquisition_time =
			ADC_ACQ_TIME_DEFAULT;
	}
	retval = adc_channel_setup(adc_dev,
			&adc_list[chosen_adc].channel_config);
	LOG_DBG("Channel setup returned %i\n", retval);
	return retval;
}
static int cmd_adc_reso(const struct shell *shell, size_t argc, char **argv)
{
	int retval = 0;
	struct device *adc_dev;
	int chosen_adc;

	if (argc != args_no.resolution ||
			!isdigit((unsigned char)argv[args_indx.conf][0])) {
		shell_fprintf(shell, SHELL_NORMAL,
				"Usage: resolution <resolution>\n");
		return 0;
	}

	chosen_adc = get_adc_from_list(argv[args_indx.adc]);
	if (chosen_adc < 0) {
		shell_error(shell, "Device not in device list");
		return -EINVAL;
	}

	adc_dev = device_get_binding(adc_list[chosen_adc].device_name);
	if (adc_dev == NULL) {
		shell_error(shell, "ADC device not found");
		return -ENODEV;
	}
	if (!isdigit(argv[args_indx.conf][0])) {
		shell_error(shell, "<resolution> must be digits");
		return -EINVAL;
	}
	adc_list[chosen_adc].resolution =
		(u8_t)strtol(argv[args_indx.conf], NULL, 10);
	retval = adc_channel_setup(adc_dev,
			&adc_list[chosen_adc].channel_config);
	return retval;
}

struct reference_string_to_enum {
	char *string;
	enum adc_reference reference;
};

static const struct reference_string_to_enum reference_list[] = {
	{ .string = "VDD_1", .reference = ADC_REF_VDD_1 },
	{ .string = "VDD_1_2", .reference = ADC_REF_VDD_1_2 },
	{ .string = "VDD_1_3", .reference = ADC_REF_VDD_1_3 },
	{ .string = "VDD_1_4", .reference = ADC_REF_VDD_1_4 },
	{ .string = "INT", .reference = ADC_REF_INTERNAL },
	{ .string = "EXT0", .reference = ADC_REF_EXTERNAL0 },
	{ .string = "EXT1", .reference = ADC_REF_EXTERNAL1 }
};

static int cmd_adc_ref(const struct shell *shell, size_t argc, char **argv)
{
	int retval = -EINVAL;
	struct device *adc_dev;
	int chosen_adc;
	int i;

	chosen_adc = get_adc_from_list(argv[args_indx.parent_adc]);
	if (chosen_adc < 0) {
		shell_error(shell, "Device not in device list");
		return -EINVAL;
	}

	adc_dev = device_get_binding(adc_list[chosen_adc].device_name);
	if (adc_dev == NULL) {
		shell_error(shell, "ADC device not found");
		return -ENODEV;
	}
	for (i = 0; i < ARRAY_SIZE(reference_list); i++) {
		if (!strcmp(argv[0], reference_list[i].string)) {
			adc_list[chosen_adc].channel_config.reference =
				reference_list[i].reference;
			retval = adc_channel_setup(adc_dev,
					&adc_list[chosen_adc].channel_config);
			LOG_DBG("Channel setup returned %i\n", retval);
			break;
		}
	}
	retval = adc_channel_setup(adc_dev, &adc_list[chosen_adc].channel_config);
	return retval;
}

#define BUFFER_SIZE 1
static int cmd_adc_read(const struct shell *shell, size_t argc, char **argv)
{
	int retval = 0;
	int chosen_adc = -1;
	struct device *adc_dev;
	u16_t m_sample_buffer[BUFFER_SIZE];

	if (argc != args_no.read) {
		shell_fprintf(shell, SHELL_NORMAL,
				"Usage: read <channel>\n");
		return 0;
	}
	chosen_adc = get_adc_from_list(argv[args_indx.adc]);
	if (chosen_adc < 0) {
		shell_error(shell, "Device not in device list");
		return 0;
	}
	u8_t adc_channel_id = strtol(argv[args_indx.channel], NULL, 10);

	adc_dev = device_get_binding(adc_list[chosen_adc].device_name);
	if (adc_dev == NULL) {
		shell_error(shell, "ADC device not found");
		return -ENODEV;
	}
	adc_list[chosen_adc].channel_config.channel_id = adc_channel_id;
	const struct adc_sequence sequence = {
		.channels	=
			BIT(adc_list[chosen_adc].channel_config.channel_id),
		.buffer		= m_sample_buffer,
		.buffer_size	= sizeof(m_sample_buffer),
		.resolution	= adc_list[chosen_adc].resolution,
	};
	retval = adc_read(adc_dev, &sequence);
	if (retval >= 0) {
		shell_fprintf(shell, SHELL_NORMAL,
				"Read: %i\n", m_sample_buffer[0]);
	}
	return retval;
}

static int cmd_adc_print(const struct shell *shell, size_t argc, char **argv)
{
	int chosen_adc = -1;
	int i;
	char *gain = "1";
	char *ref = "INTERNAL";
	u16_t acq_time;
	u8_t channel_id;
	u8_t resolution;

	chosen_adc = get_adc_from_list(argv[args_indx.adc]);
	if (chosen_adc < 0) {
		shell_error(shell, "Device not in device list");
		return 0;
	}
	for (i = 0; i < ARRAY_SIZE(gain_list); i++) {
		if (gain_list[i].gain ==
				adc_list[chosen_adc].channel_config.gain) {
			gain = gain_list[i].string;
		}
	}
	for (i = 0; i < ARRAY_SIZE(reference_list); i++) {
		if (reference_list[i].reference ==
				adc_list[chosen_adc].channel_config.reference) {
			ref = reference_list[i].string;
		}
	}
	acq_time = adc_list[chosen_adc].channel_config.acquisition_time;
	channel_id = adc_list[chosen_adc].channel_config.channel_id;
	resolution = adc_list[chosen_adc].resolution;
	shell_fprintf(shell, SHELL_NORMAL, "%s:\n"
			"Gain: %s\n"
			"Reference: %s\n"
			"Acquisition Time: %u\n"
			"Channel ID: %u\n"
			"Resolution: %u\n",
			argv[args_indx.adc],
			gain,
			ref,
			acq_time,
			channel_id,
			resolution);
	return 0;
}
SHELL_STATIC_SUBCMD_SET_CREATE(sub_ref_cmds,
	/* Alphabetically sorted. */
	SHELL_CMD(VDD_1, NULL, "VDD", cmd_adc_ref),
	SHELL_CMD(VDD_1_2, NULL, "VDD/2", cmd_adc_ref),
	SHELL_CMD(VDD_1_3, NULL, "VDD/3", cmd_adc_ref),
	SHELL_CMD(VDD_1_4, NULL, "VDD/4", cmd_adc_ref),
	SHELL_CMD(INT, NULL, "Internal", cmd_adc_ref),
	SHELL_CMD(EXT0, NULL, "EXT0", cmd_adc_ref),
	SHELL_CMD(EXT1, NULL, "EXT1", cmd_adc_ref),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);


SHELL_STATIC_SUBCMD_SET_CREATE(sub_gain_cmds,
	/* Alphabetically sorted. */
	SHELL_CMD(ADC_GAIN_1_6, NULL, "Gain: 1/6", cmd_adc_gain),
	SHELL_CMD(ADC_GAIN_1_5, NULL, "Gain: 1/5", cmd_adc_gain),
	SHELL_CMD(ADC_GAIN_1_4, NULL, "Gain: 1/4", cmd_adc_gain),
	SHELL_CMD(ADC_GAIN_1_3, NULL, "Gain: 1/3", cmd_adc_gain),
	SHELL_CMD(ADC_GAIN_1_2, NULL, "Gain: 1/2", cmd_adc_gain),
	SHELL_CMD(ADC_GAIN_2_3, NULL, "Gain: 2/3", cmd_adc_gain),
	SHELL_CMD(ADC_GAIN_1, NULL, "Gain: 1", cmd_adc_gain),
	SHELL_CMD(ADC_GAIN_2, NULL, "Gain: 2", cmd_adc_gain),
	SHELL_CMD(ADC_GAIN_3, NULL, "Gain: 3", cmd_adc_gain),
	SHELL_CMD(ADC_GAIN_4, NULL, "Gain: 4", cmd_adc_gain),
	SHELL_CMD(ADC_GAIN_8, NULL, "Gain: 8", cmd_adc_gain),
	SHELL_CMD(ADC_GAIN_16, NULL, "Gain: 16", cmd_adc_gain),
	SHELL_CMD(ADC_GAIN_32, NULL, "Gain: 32", cmd_adc_gain),
	SHELL_CMD(ADC_GAIN_64, NULL, "Gain: 64", cmd_adc_gain),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);


SHELL_STATIC_SUBCMD_SET_CREATE(sub_adc_cmds,
	/* Alphabetically sorted. */
	SHELL_CMD(acq_time, NULL, "Configure acquisition time", cmd_adc_acq),
	SHELL_CMD(channel_id, NULL, "Configure channel id", cmd_adc_channel),
	SHELL_CMD(gain, &sub_gain_cmds, "Configure gain", NULL),
	SHELL_CMD(print, NULL, "Print current configuration", cmd_adc_print),
	SHELL_CMD(read, NULL, "Read adc value", cmd_adc_read),
	SHELL_CMD(reference, &sub_ref_cmds, "Configure reference", NULL),
	SHELL_CMD(resolution, NULL, "Configure resolution", cmd_adc_reso),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_adc,
#ifdef CONFIG_ADC_0
	SHELL_CMD(ADC_0, &sub_adc_cmds, "ADC_0", NULL),
#endif
#ifdef CONFIG_ADC_1
	SHELL_CMD(ADC_1, &sub_adc_cmds, "ADC_1", NULL),
#endif
#ifdef CONFIG_ADC_2
	SHELL_CMD(ADC_2, &sub_adc_cmds, "ADC_2", NULL),
#endif
	SHELL_SUBCMD_SET_END /* Array terminated. */
);


SHELL_CMD_REGISTER(adc, &sub_adc, "ADC commands", NULL);
