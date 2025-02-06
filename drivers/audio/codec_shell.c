/*
 * Copyright (c) 2023 Centralp
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/audio/codec.h>

#define CODEC_START_HELP                                                                           \
	"Start output audio playback. Syntax:\n"                                                   \
	"<device>"

#define CODEC_STOP_HELP                                                                            \
	"Stop output audio playback. Syntax:\n"                                                    \
	"<device>"

#define CODEC_SET_PROP_HELP                                                                        \
	"Set a codec property. Syntax:\n"                                                          \
	"<device> <property> <channel> <value>"

#define CODEC_APPLY_PROP_HELP                                                                      \
	"Apply any cached properties. Syntax:\n"                                                   \
	"<device>"

static const char *const codec_property_name[] = {
	[AUDIO_PROPERTY_OUTPUT_VOLUME] = "volume",
	[AUDIO_PROPERTY_OUTPUT_MUTE] = "mute",
};

static const char *const codec_channel_name[] = {
	[AUDIO_CHANNEL_FRONT_LEFT] = "front_left",
	[AUDIO_CHANNEL_FRONT_RIGHT] = "front_right",
	[AUDIO_CHANNEL_LFE] = "lfe",
	[AUDIO_CHANNEL_FRONT_CENTER] = "front_center",
	[AUDIO_CHANNEL_REAR_LEFT] = "rear_left",
	[AUDIO_CHANNEL_REAR_RIGHT] = "rear_right",
	[AUDIO_CHANNEL_REAR_CENTER] = "rear_center",
	[AUDIO_CHANNEL_SIDE_LEFT] = "side_left",
	[AUDIO_CHANNEL_SIDE_RIGHT] = "side_right",
	[AUDIO_CHANNEL_HEADPHONE_LEFT] = "headphone_left",
	[AUDIO_CHANNEL_HEADPHONE_RIGHT] = "headphone_right",
	[AUDIO_CHANNEL_ALL] = "all",
};

struct args_index {
	uint8_t device;
	uint8_t property;
	uint8_t channel;
	uint8_t value;
};

static const struct args_index args_indx = {
	.device = 1,
	.property = 2,
	.channel = 3,
	.value = 4,
};

static int parse_named_int(const char *name, const char *const keystack[], size_t count)
{
	char *endptr;
	int i;

	/* Attempt to parse name as a number first */
	i = strtoul(name, &endptr, 0);
	if (*endptr == '\0') {
		return i;
	}

	/* Name is not a number, look it up */
	for (i = 0; i < count; i++) {
		if (strcmp(name, keystack[i]) == 0) {
			return i;
		}
	}

	return -ENOTSUP;
}

static int cmd_start(const struct shell *sh, size_t argc, char *argv[])
{
	const struct device *dev;

	dev = shell_device_get_binding(argv[args_indx.device]);
	if (!dev) {
		shell_error(sh, "Audio Codec device not found");
		return -ENODEV;
	}
	audio_codec_start_output(dev);

	return 0;
}

static int cmd_stop(const struct shell *sh, size_t argc, char *argv[])
{
	const struct device *dev;

	dev = shell_device_get_binding(argv[args_indx.device]);
	if (!dev) {
		shell_error(sh, "Audio Codec device not found");
		return -ENODEV;
	}
	audio_codec_stop_output(dev);

	return 0;
}

static int cmd_set_prop(const struct shell *sh, size_t argc, char *argv[])
{
	const struct device *dev;
	int property;
	int channel;
	long value;
	char *endptr;
	audio_property_value_t property_value;

	dev = shell_device_get_binding(argv[args_indx.device]);
	if (!dev) {
		shell_error(sh, "Audio Codec device not found");
		return -ENODEV;
	}

	property = parse_named_int(argv[args_indx.property], codec_property_name,
				   ARRAY_SIZE(codec_property_name));
	if (property < 0) {
		shell_error(sh, "Property '%s' unknown", argv[args_indx.property]);
		return -EINVAL;
	}

	channel = parse_named_int(argv[args_indx.channel], codec_channel_name,
				  ARRAY_SIZE(codec_channel_name));
	if (channel < 0) {
		shell_error(sh, "Channel '%s' unknown", argv[args_indx.channel]);
		return -EINVAL;
	}

	value = strtol(argv[args_indx.value], &endptr, 0);
	if (*endptr != '\0') {
		return -EINVAL;
	}
	if (value > INT32_MAX || value < INT32_MIN) {
		return -EINVAL;
	}
	switch (property) {
	case AUDIO_PROPERTY_OUTPUT_VOLUME:
		property_value.vol = value;
		break;
	case AUDIO_PROPERTY_OUTPUT_MUTE:
		property_value.mute = value;
		break;
	default:
		return -EINVAL;
	}

	return audio_codec_set_property(dev, property, channel, property_value);
}

static int cmd_apply_prop(const struct shell *sh, size_t argc, char *argv[])
{
	const struct device *dev;

	dev = shell_device_get_binding(argv[args_indx.device]);
	if (!dev) {
		shell_error(sh, "Audio Codec device not found");
		return -ENODEV;
	}

	return audio_codec_apply_properties(dev);
}

/* Device name autocompletion support */
static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, NULL);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

/* clang-format off */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_codec,
	SHELL_CMD_ARG(start, &dsub_device_name, CODEC_START_HELP, cmd_start,
			2, 0),
	SHELL_CMD_ARG(stop, &dsub_device_name, CODEC_STOP_HELP, cmd_stop,
			2, 0),
	SHELL_CMD_ARG(set_prop, &dsub_device_name, CODEC_SET_PROP_HELP, cmd_set_prop,
			5, 0),
	SHELL_CMD_ARG(apply_prop, &dsub_device_name, CODEC_APPLY_PROP_HELP, cmd_apply_prop,
			2, 0),
	SHELL_SUBCMD_SET_END
);
/* clang-format on */

SHELL_CMD_REGISTER(codec, &sub_codec, "Audio Codec commands", NULL);
