/*
 * Copyright (c) 2024 Centro de Inovacao EDGE
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/uart.h>

/* ADC node from the devicetree. */
#define ADC_NODE DT_ALIAS(adc0)

/* Auxiliary macro to obtain channel vref, if available. */
#define CHANNEL_VREF(node_id) DT_PROP_OR(node_id, zephyr_vref_mv, 0)

/* Data of ADC device specified in devicetree. */
static const struct device *adc = DEVICE_DT_GET(ADC_NODE);

/* Data array of ADC channels for the specified ADC. */
static const struct adc_channel_cfg channel_cfgs[] = {
	DT_FOREACH_CHILD_SEP(ADC_NODE, ADC_CHANNEL_CFG_DT, (,))};

/* Data array of ADC channel voltage references. */
static uint32_t vrefs_mv[] = {DT_FOREACH_CHILD_SEP(ADC_NODE, CHANNEL_VREF, (,))};

/* Get the number of channels defined on the DTS. */
#define CHANNEL_COUNT ARRAY_SIZE(channel_cfgs)

#ifdef CONFIG_SEQUENCE_32BITS_REGISTERS
static uint32_t channel_reading[CONFIG_SEQUENCE_SAMPLES][CHANNEL_COUNT];
#else
static uint16_t channel_reading[CONFIG_SEQUENCE_SAMPLES][CHANNEL_COUNT];
#endif

/* Options for the sequence sampling. */
static const struct adc_sequence_options options = {
	.extra_samplings = CONFIG_SEQUENCE_SAMPLES - 1,
	.interval_us = 0,
};

/* Configure the sampling sequence to be made. */
static struct adc_sequence sequence = {
	.buffer = channel_reading,
	/* buffer size in bytes, not number of samples */
	.buffer_size = sizeof(channel_reading),
	.resolution = CONFIG_SEQUENCE_RESOLUTION,
	.options = &options,
};

static uint32_t count;

#ifndef CONFIG_SHELL_ADC_PS
static const struct device *const dev_console = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
#endif

#ifdef CONFIG_SHELL_ADC_PS
/* ADC shell command handlers */
static int cmd_adc_read(const struct shell *sh, size_t argc, char **argv)
{
	int err = 0;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "ADC sequence reading [%u]:", count++);

	err = adc_read(adc, &sequence);
	if (err < 0) {
		shell_print(sh, "Could not read (%d)", err);
		return -EIO;
	}

	for (size_t channel_index = 0U; channel_index < CHANNEL_COUNT; channel_index++) {
		int32_t val_mv;
		int32_t val_raw;

		shell_print(sh,
			    "- %s, channel %" PRId32 ", %" PRId32 " sequence samples (diff = %d):",
			    adc->name, channel_cfgs[channel_index].channel_id,
			    CONFIG_SEQUENCE_SAMPLES, channel_cfgs[channel_index].differential);
		for (size_t sample_index = 0U; sample_index < CONFIG_SEQUENCE_SAMPLES;
		     sample_index++) {
			val_raw = channel_reading[sample_index][channel_index];
			val_mv = val_raw;
			err = adc_raw_to_millivolts(vrefs_mv[channel_index],
						    channel_cfgs[channel_index].gain,
						    CONFIG_SEQUENCE_RESOLUTION, &val_mv);

			/* conversion to mV may not be supported, skip if not */
			if ((err < 0) || vrefs_mv[channel_index] == 0) {
				shell_print(sh, "- - %" PRId32 " (value in mV not available)",
					    val_raw);
			} else {
				shell_print(sh, "- - %" PRId32 " = %" PRId32 "mV", val_raw, val_mv);
			}
		}
	}
	shell_print(sh, "==== end of reading ===");

	return 0;
}

static int cmd_adc_status(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "%s", "\n");
	shell_print(sh, "==== start of adc features ===");
	shell_print(sh, "CHANNEL_COUNT: %d", CHANNEL_COUNT);
	shell_print(sh, "Resolution: %d", CONFIG_SEQUENCE_RESOLUTION);
	for (size_t channel_index = 0U; channel_index < CHANNEL_COUNT; channel_index++) {
		shell_print(sh,
			    "\tchannel_id %d features:", channel_cfgs[channel_index].channel_id);
		if (channel_cfgs[channel_index].differential) {
			shell_print(sh, "\t - is diff mode");
		} else {
			shell_print(sh, "\t - is single mode");
		}
		shell_print(sh, "\t - verf is %d mv", vrefs_mv[channel_index]);
	}
	shell_print(sh, "==== end of adc features ===");

	return 0;
}

/* ADC subcommands */
SHELL_STATIC_SUBCMD_SET_CREATE(adc_cmds,
			       SHELL_CMD_ARG(read, NULL, "Read ADC values", cmd_adc_read, 1, 0),
			       SHELL_CMD_ARG(status, NULL, "Show ADC status", cmd_adc_status, 1, 0),
			       SHELL_SUBCMD_SET_END);

/* Register main ADC command */
SHELL_CMD_REGISTER(adc, &adc_cmds, "ADC power shield commands", NULL);

#endif

int main(void)
{
	int err;

	if (!device_is_ready(adc)) {
		return 0;
	}

	/* Configure channels individually prior to sampling. */
	for (size_t i = 0U; i < CHANNEL_COUNT; i++) {
		sequence.channels |= BIT(channel_cfgs[i].channel_id);
		err = adc_channel_setup(adc, &channel_cfgs[i]);
		if (err < 0) {
			return 0;
		}
		if ((vrefs_mv[i] == 0) && (channel_cfgs[i].reference == ADC_REF_INTERNAL)) {
			vrefs_mv[i] = adc_ref_internal(adc);
		}
	}
	while (1) {
#ifndef CONFIG_SHELL_ADC_PS
		uint8_t c = 'A';

		err = uart_poll_in(dev_console, &c);
		if (err < 0) {
			k_msleep(100);
			continue;
		}
		k_msleep(100);

		/* print capacity when input is '\r'*/
		if (c == '\r') {
			printf("\r\n==== start of adc features ===\n");
			printf("CHANNEL_COUNT: %d\n", CHANNEL_COUNT);
			printf("Resolution: %d\n", CONFIG_SEQUENCE_RESOLUTION);
			for (size_t channel_index = 0U; channel_index < CHANNEL_COUNT;
			     channel_index++) {
				printf("\tchannel_id %d features:\n",
				       channel_cfgs[channel_index].channel_id);
				if (channel_cfgs[channel_index].differential) {
					printf("\t - is diff mode\n");
				} else {
					printf("\t - is single mode\n");
				}
				printf("\t - verf is %d mv\n", vrefs_mv[channel_index]);
			}
			printf("==== end of adc features ===\n");
			continue;
		}

		printf("ADC sequence reading [%u]:\n", count++);

		err = adc_read(adc, &sequence);
		if (err < 0) {
			printf("Could not read (%d)\n", err);
			continue;
		}

		for (size_t channel_index = 0U; channel_index < CHANNEL_COUNT; channel_index++) {
			int32_t val_mv;

			printf("- %s, channel %" PRId32 ", %" PRId32
			       " sequence samples (diff = %d):\n",
			       adc->name, channel_cfgs[channel_index].channel_id,
			       CONFIG_SEQUENCE_SAMPLES, channel_cfgs[channel_index].differential);
			for (size_t sample_index = 0U; sample_index < CONFIG_SEQUENCE_SAMPLES;
			     sample_index++) {

				val_mv = channel_reading[sample_index][channel_index];

				printf("- - %" PRId32, val_mv);
				err = adc_raw_to_millivolts(vrefs_mv[channel_index],
							    channel_cfgs[channel_index].gain,
							    CONFIG_SEQUENCE_RESOLUTION, &val_mv);

				/* conversion to mV may not be supported, skip if not */
				if ((err < 0) || vrefs_mv[channel_index] == 0) {
					continue;
				} else {
					printf(" = %" PRId32 "mV\n", val_mv);
				}
			}
		}
		printf("==== end of reading ===\n");
#else
		k_msleep(100);
#endif
	}

	return 0;
}
