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

#define CMD_HELP_ACQ_TIME 			\
	"Configure acquisition time."		\
	"\nUsage: acq_time <time> <unit>"	\
	"\nunits: us, ns, ticks\n"

#define CMD_HELP_CHANNEL			\
	"Configure ADC channel\n"		\

#define CMD_HELP_CH_ID				\
	"Configure channel id\n"		\
	"Usage: id <channel_id>\n"

#define CMD_HELP_DIFF				\
	"Configure differential\n"		\
	"Usage: differential <0||1>\n"

#define CMD_HELP_CH_NEG				\
	"Configure channel negative input\n"	\
	"Usage: negative <negative_input_id>\n"

#define CMD_HELP_CH_POS				\
	"Configure channel positive input\n"	\
	"Usage: positive <positive_input_id>\n"

#define CMD_HELP_READ				\
	"Read adc value\n"			\
	"Usage: read <channel>\n"

#define CMD_HELP_RES				\
	"Configure resolution\n"		\
	"Usage: resolution <resolution>\n"

#define CMD_HELP_REF	"Configure reference\n"
#define CMD_HELP_GAIN	"Configure gain.\n"
#define CMD_HELP_PRINT	"Print current configuration"

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
	DT_FOREACH_STATUS_OKAY(atmel_sam_afec, ADC_HDL_LIST_ENTRY)
	DT_FOREACH_STATUS_OKAY(espressif_esp32_adc, ADC_HDL_LIST_ENTRY)
	DT_FOREACH_STATUS_OKAY(atmel_sam_adc, ADC_HDL_LIST_ENTRY)
	DT_FOREACH_STATUS_OKAY(atmel_sam0_adc, ADC_HDL_LIST_ENTRY)
	DT_FOREACH_STATUS_OKAY(ite_it8xxx2_adc, ADC_HDL_LIST_ENTRY)
	DT_FOREACH_STATUS_OKAY(microchip_xec_adc, ADC_HDL_LIST_ENTRY)
	DT_FOREACH_STATUS_OKAY(nordic_nrf_adc, ADC_HDL_LIST_ENTRY)
	DT_FOREACH_STATUS_OKAY(nordic_nrf_saadc, ADC_HDL_LIST_ENTRY)
	DT_FOREACH_STATUS_OKAY(nxp_mcux_12b1msps_sar, ADC_HDL_LIST_ENTRY)
	DT_FOREACH_STATUS_OKAY(nxp_kinetis_adc12, ADC_HDL_LIST_ENTRY)
	DT_FOREACH_STATUS_OKAY(nxp_kinetis_adc16, ADC_HDL_LIST_ENTRY)
	DT_FOREACH_STATUS_OKAY(nxp_vf610_adc, ADC_HDL_LIST_ENTRY)
	DT_FOREACH_STATUS_OKAY(st_stm32_adc, ADC_HDL_LIST_ENTRY)
	DT_FOREACH_STATUS_OKAY(nuvoton_npcx_adc, ADC_HDL_LIST_ENTRY)
	DT_FOREACH_STATUS_OKAY(ti_ads1112, ADC_HDL_LIST_ENTRY)
	DT_FOREACH_STATUS_OKAY(ti_ads1119, ADC_HDL_LIST_ENTRY)
	DT_FOREACH_STATUS_OKAY(ti_ads114s08, ADC_HDL_LIST_ENTRY)
	DT_FOREACH_STATUS_OKAY(ti_cc32xx_adc, ADC_HDL_LIST_ENTRY)
	DT_FOREACH_STATUS_OKAY(raspberrypi_pico_adc, ADC_HDL_LIST_ENTRY)
	DT_FOREACH_STATUS_OKAY(zephyr_adc_emul, ADC_HDL_LIST_ENTRY)
	DT_FOREACH_STATUS_OKAY(nxp_s32_adc_sar, ADC_HDL_LIST_ENTRY)
	DT_FOREACH_STATUS_OKAY(maxim_max11102, ADC_HDL_LIST_ENTRY)
	DT_FOREACH_STATUS_OKAY(maxim_max11103, ADC_HDL_LIST_ENTRY)
	DT_FOREACH_STATUS_OKAY(maxim_max11105, ADC_HDL_LIST_ENTRY)
	DT_FOREACH_STATUS_OKAY(maxim_max11106, ADC_HDL_LIST_ENTRY)
	DT_FOREACH_STATUS_OKAY(maxim_max11110, ADC_HDL_LIST_ENTRY)
	DT_FOREACH_STATUS_OKAY(maxim_max11111, ADC_HDL_LIST_ENTRY)
	DT_FOREACH_STATUS_OKAY(maxim_max11115, ADC_HDL_LIST_ENTRY)
	DT_FOREACH_STATUS_OKAY(maxim_max11116, ADC_HDL_LIST_ENTRY)
	DT_FOREACH_STATUS_OKAY(maxim_max11117, ADC_HDL_LIST_ENTRY)
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

	return retval;
}

static int cmd_adc_print(const struct shell *sh, size_t argc, char **argv)
{
	/* -1 index of ADC label name */
	struct adc_hdl *adc = get_adc(argv[-1]);

	shell_print(sh, "%s:\n"
			   "Gain: %s\n"
			   "Reference: %s\n"
			   "Acquisition Time: %u\n"
			   "Channel ID: %u\n"
			   "Differential: %u\n"
			   "Resolution: %u",
			   adc->dev->name,
			   chosen_gain,
			   chosen_reference,
			   adc->channel_config.acquisition_time,
			   adc->channel_config.channel_id,
			   adc->channel_config.differential,
			   adc->resolution);
#if CONFIG_ADC_CONFIGURABLE_INPUTS
	shell_print(sh, "Input positive: %u",
		    adc->channel_config.input_positive);
	if (adc->channel_config.differential != 0) {
		shell_print(sh, "Input negative: %u",
			    adc->channel_config.input_negative);
	}
#endif
	return 0;
}

SHELL_SUBCMD_DICT_SET_CREATE(sub_ref_cmds, cmd_adc_ref,
	(VDD_1, ADC_REF_VDD_1, "VDD"),
	(VDD_1_2, ADC_REF_VDD_1_2, "VDD/2"),
	(VDD_1_3, ADC_REF_VDD_1_3, "VDD/3"),
	(VDD_1_4, ADC_REF_VDD_1_4, "VDD/4"),
	(INTERNAL, ADC_REF_INTERNAL, "Internal"),
	(EXTERNAL_0, ADC_REF_EXTERNAL0, "External, input 0"),
	(EXTERNAL_1, ADC_REF_EXTERNAL1, "External, input 1")
);

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
	SHELL_CMD_ARG(read, NULL, CMD_HELP_READ, cmd_adc_read, 2, 0),
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
		entry->help    = "Select subcommand for ADC property label.";
	} else {
		entry->syntax  = NULL;
	}
}
SHELL_DYNAMIC_CMD_CREATE(sub_adc_dev, cmd_adc_dev_get);

SHELL_CMD_REGISTER(adc, &sub_adc_dev, "ADC commands", NULL);
