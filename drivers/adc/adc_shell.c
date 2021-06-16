/*
 * Copyright (c) 2018 Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <shell/shell.h>
#include <stdlib.h>
#include <drivers/adc.h>
#include <ctype.h>
#include <sys/util.h>
#include <devicetree.h>

#if DT_HAS_COMPAT_STATUS_OKAY(atmel_sam_afec)
#define DT_DRV_COMPAT atmel_sam_afec
#elif DT_HAS_COMPAT_STATUS_OKAY(atmel_sam0_adc)
#define DT_DRV_COMPAT atmel_sam0_adc
#elif DT_HAS_COMPAT_STATUS_OKAY(ite_it8xxx2_adc)
#define DT_DRV_COMPAT ite_it8xxx2_adc
#elif DT_HAS_COMPAT_STATUS_OKAY(microchip_xec_adc)
#define DT_DRV_COMPAT microchip_xec_adc
#elif DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_adc)
#define DT_DRV_COMPAT nordic_nrf_adc
#elif DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_saadc)
#define DT_DRV_COMPAT nordic_nrf_saadc
#elif DT_HAS_COMPAT_STATUS_OKAY(nxp_kinetis_adc12)
#define DT_DRV_COMPAT nxp_kinetis_adc12
#elif DT_HAS_COMPAT_STATUS_OKAY(nxp_kinetis_adc16)
#define DT_DRV_COMPAT nxp_kinetis_adc16
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_adc)
#define DT_DRV_COMPAT st_stm32_adc
#elif DT_HAS_COMPAT_STATUS_OKAY(nuvoton_npcx_adc)
#define DT_DRV_COMPAT nuvoton_npcx_adc
#elif DT_HAS_COMPAT_STATUS_OKAY(ti_cc32xx_adc)
#define DT_DRV_COMPAT ti_cc32xx_adc
#elif DT_HAS_COMPAT_STATUS_OKAY(zephyr_adc_emul)
#define DT_DRV_COMPAT zephyr_adc_emul
#else
#error No known devicetree compatible match for ADC shell
#endif

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
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

#define NODE_LABELS(n) DT_INST_LABEL(n),
#define ADC_HDL_LIST_ENTRY(label)					\
	{								\
		.device_label = label,					\
		.channel_config = {					\
			.gain = ADC_GAIN_1,				\
			.reference = ADC_REF_INTERNAL,			\
			.acquisition_time = ADC_ACQ_TIME_DEFAULT,	\
			.channel_id = 0,				\
		},							\
		.resolution = 0,					\
	}

#define INIT_MACRO() DT_INST_FOREACH_STATUS_OKAY(NODE_LABELS) "NA"

#define CHOSEN_STR_LEN 20
static char chosen_reference[CHOSEN_STR_LEN + 1] = "INTERNAL";
static char chosen_gain[CHOSEN_STR_LEN + 1] = "1";

/* This table size is = ADC devices count + 1 (NA). */
static struct adc_hdl {
	char *device_label;
	struct adc_channel_cfg channel_config;
	uint8_t resolution;
} adc_list[] = {
	FOR_EACH(ADC_HDL_LIST_ENTRY, (,), INIT_MACRO())
};

static struct adc_hdl *get_adc(const char *device_label)
{
	for (int i = 0; i < ARRAY_SIZE(adc_list); i++) {
		if (!strcmp(device_label, adc_list[i].device_label)) {
			return &adc_list[i];
		}
	}

	/* This will never happen because ADC was prompted by shell */
	__ASSERT_NO_MSG(false);
	return NULL;
}

static int cmd_adc_ch_id(const struct shell *shell, size_t argc, char **argv)
{
	/* -2: index of ADC label name */
	struct adc_hdl *adc = get_adc(argv[-2]);
	const struct device *adc_dev;
	int retval = 0;

	adc_dev = device_get_binding(adc->device_label);
	if (adc_dev == NULL) {
		shell_error(shell, "ADC device not found");
		return -ENODEV;
	}

	if (!isdigit((unsigned char)argv[1][0])) {
		shell_error(shell, "<channel> must be digits");
		return -EINVAL;
	}

	adc->channel_config.channel_id = (uint8_t)strtol(argv[1], NULL, 10);
	retval = adc_channel_setup(adc_dev, &adc->channel_config);
	LOG_DBG("Channel setup returned %i\n", retval);

	return retval;
}

static int cmd_adc_ch_neg(const struct shell *shell, size_t argc, char **argv)
{
#if CONFIG_ADC_CONFIGURABLE_INPUTS
	/* -2: index of ADC label name */
	struct adc_hdl *adc = get_adc(argv[-2]);
	const struct device *adc_dev;
	int retval = 0;

	adc_dev = device_get_binding(adc->device_label);
	if (adc_dev == NULL) {
		shell_error(shell, "ADC device not found");
		return -ENODEV;
	}

	if (!isdigit((unsigned char)argv[1][0])) {
		shell_error(shell, "<negative input> must be digits");
		return -EINVAL;
	}

	adc->channel_config.input_negative = (uint8_t)strtol(argv[1], NULL, 10);
	retval = adc_channel_setup(adc_dev, &adc->channel_config);
	LOG_DBG("Channel setup returned %i\n", retval);

	return retval;
#else
	return -EINVAL;
#endif
}

static int cmd_adc_ch_pos(const struct shell *shell, size_t argc, char **argv)
{
#if CONFIG_ADC_CONFIGURABLE_INPUTS
	/* -2: index of ADC label name */
	struct adc_hdl *adc = get_adc(argv[-2]);
	const struct device *adc_dev;
	int retval = 0;

	adc_dev = device_get_binding(adc->device_label);
	if (adc_dev == NULL) {
		shell_error(shell, "ADC device not found");
		return -ENODEV;
	}

	if (!isdigit((unsigned char)argv[1][0])) {
		shell_error(shell, "<positive input> must be digits");
		return -EINVAL;
	}

	adc->channel_config.input_positive = (uint8_t)strtol(argv[1], NULL, 10);
	retval = adc_channel_setup(adc_dev, &adc->channel_config);
	LOG_DBG("Channel setup returned %i\n", retval);

	return retval;
#else
	return -EINVAL;
#endif
}

static int cmd_adc_gain(const struct shell *shell, size_t argc, char **argv,
			void *data)
{
	/* -2: index of ADC label name */
	struct adc_hdl *adc = get_adc(argv[-2]);
	enum adc_gain gain = (enum adc_gain)data;
	const struct device *adc_dev;
	int retval = -EINVAL;

	adc_dev = device_get_binding(adc->device_label);
	if (adc_dev == NULL) {
		shell_error(shell, "ADC device not found");
		return -ENODEV;
	}

	adc->channel_config.gain = gain;
	int len = strlen(argv[0]) > CHOSEN_STR_LEN ? CHOSEN_STR_LEN
						   : strlen(argv[0]);
	memcpy(chosen_gain, argv[0], len);
	chosen_gain[len] = '\0';
	retval = adc_channel_setup(adc_dev, &adc->channel_config);
	LOG_DBG("Channel setup returned %i\n", retval);

	return retval;
}

static int cmd_adc_acq(const struct shell *shell, size_t argc, char **argv)
{
	/* -1 index of ADC label name */
	struct adc_hdl *adc = get_adc(argv[-1]);
	const struct device *adc_dev;
	uint16_t acq_time;
	int retval;

	adc_dev = device_get_binding(adc->device_label);
	if (adc_dev == NULL) {
		shell_error(shell, "ADC device not found");
		return -ENODEV;
	}

	if (!isdigit((unsigned char)argv[1][0])) {
		shell_error(shell, "<time> must be digits");
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
	retval = adc_channel_setup(adc_dev, &adc->channel_config);
	LOG_DBG("Channel setup returned %i\n", retval);

	return retval;
}
static int cmd_adc_reso(const struct shell *shell, size_t argc, char **argv)
{
	/* -1 index of ADC label name */
	struct adc_hdl *adc = get_adc(argv[-1]);
	const struct device *adc_dev;
	int retval;

	adc_dev = device_get_binding(adc->device_label);
	if (adc_dev == NULL) {
		shell_error(shell, "ADC device not found");
		return -ENODEV;
	}

	if (!isdigit((unsigned char)argv[1][0])) {
		shell_error(shell, "<resolution> must be digits");
		return -EINVAL;
	}

	adc->resolution = (uint8_t)strtol(argv[1], NULL, 10);
	retval = adc_channel_setup(adc_dev, &adc->channel_config);

	return retval;
}

static int cmd_adc_ref(const struct shell *shell, size_t argc, char **argv,
		       void *data)
{
	/* -2 index of ADC label name */
	struct adc_hdl *adc = get_adc(argv[-2]);
	enum adc_reference reference = (enum adc_reference)data;
	const struct device *adc_dev;
	int retval = -EINVAL;

	adc_dev = device_get_binding(adc->device_label);
	if (adc_dev == NULL) {
		shell_error(shell, "ADC device not found");
		return -ENODEV;
	}

	int len = strlen(argv[0]) > CHOSEN_STR_LEN ? CHOSEN_STR_LEN
						   : strlen(argv[0]);
	memcpy(chosen_reference, argv[0], len);
	chosen_reference[len] = '\0';

	adc->channel_config.reference = reference;
	retval = adc_channel_setup(adc_dev, &adc->channel_config);
	LOG_DBG("Channel setup returned %i\n", retval);

	return retval;
}

#define BUFFER_SIZE 1
static int cmd_adc_read(const struct shell *shell, size_t argc, char **argv)
{
	uint8_t adc_channel_id = strtol(argv[1], NULL, 10);
	/* -1 index of adc label name */
	struct adc_hdl *adc = get_adc(argv[-1]);
	uint16_t m_sample_buffer[BUFFER_SIZE];
	const struct device *adc_dev;
	int retval;

	adc_dev = device_get_binding(adc->device_label);
	if (adc_dev == NULL) {
		shell_error(shell, "adc device not found");
		return -ENODEV;
	}

	adc->channel_config.channel_id = adc_channel_id;
	const struct adc_sequence sequence = {
		.channels	= BIT(adc->channel_config.channel_id),
		.buffer		= m_sample_buffer,
		.buffer_size	= sizeof(m_sample_buffer),
		.resolution	= adc->resolution,
	};

	retval = adc_read(adc_dev, &sequence);
	if (retval >= 0) {
		shell_print(shell, "read: %i", m_sample_buffer[0]);
	}

	return retval;
}

static int cmd_adc_print(const struct shell *shell, size_t argc, char **argv)
{
	/* -1 index of ADC label name */
	struct adc_hdl *adc = get_adc(argv[-1]);

	shell_print(shell, "%s:\n"
			   "Gain: %s\n"
			   "Reference: %s\n"
			   "Acquisition Time: %u\n"
			   "Channel ID: %u\n"
			   "Resolution: %u",
			   adc->device_label,
			   chosen_gain,
			   chosen_reference,
			   adc->channel_config.acquisition_time,
			   adc->channel_config.channel_id,
			   adc->resolution);
	return 0;
}

SHELL_SUBCMD_DICT_SET_CREATE(sub_ref_cmds, cmd_adc_ref,
	(VDD_1, ADC_REF_VDD_1),
	(VDD_1_2, ADC_REF_VDD_1_2),
	(VDD_1_3, ADC_REF_VDD_1_3),
	(VDD_1_4, ADC_REF_VDD_1_4),
	(INTERNAL, ADC_REF_INTERNAL),
	(EXTERNAL_0, ADC_REF_EXTERNAL0),
	(EXTERNAL_1, ADC_REF_EXTERNAL1)
);

SHELL_SUBCMD_DICT_SET_CREATE(sub_gain_cmds, cmd_adc_gain,
	(GAIN_1_6, ADC_GAIN_1_6),
	(GAIN_1_5, ADC_GAIN_1_5),
	(GAIN_1_4, ADC_GAIN_1_4),
	(GAIN_1_3, ADC_GAIN_1_3),
	(GAIN_1_2, ADC_GAIN_1_2),
	(GAIN_2_3, ADC_GAIN_2_3),
	(GAIN_1, ADC_GAIN_1),
	(GAIN_2, ADC_GAIN_2),
	(GAIN_3, ADC_GAIN_3),
	(GAIN_4, ADC_GAIN_4),
	(GAIN_8, ADC_GAIN_8),
	(GAIN_16, ADC_GAIN_16),
	(GAIN_32, ADC_GAIN_32),
	(GAIN_64, ADC_GAIN_64)
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_channel_cmds,
	SHELL_CMD_ARG(id, NULL, CMD_HELP_CH_ID, cmd_adc_ch_id, 2, 0),
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
	/* -1 because the last element in the list is a "list terminator" */
	if (idx < ARRAY_SIZE(adc_list) - 1) {
		entry->syntax  = adc_list[idx].device_label;
		entry->handler = NULL;
		entry->subcmd  = &sub_adc_cmds;
		entry->help    = "Select subcommand for ADC property label.\n";
	} else {
		entry->syntax  = NULL;
	}
}
SHELL_DYNAMIC_CMD_CREATE(sub_adc_dev, cmd_adc_dev_get);

SHELL_CMD_REGISTER(adc, &sub_adc_dev, "ADC commands", NULL);
