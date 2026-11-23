/*
 * Copyright (c) 2025 Siratul Islam <email@sirat.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/biometrics.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

LOG_MODULE_REGISTER(biometrics_shell, CONFIG_BIOMETRICS_LOG_LEVEL);

#define BIOMETRIC_INFO_HELP SHELL_HELP("Get biometric device info.", "<device_name>")

#define BIOMETRIC_ATTR_GET_HELP SHELL_HELP("Get biometric attribute.", "<device_name> <attribute>")

#define BIOMETRIC_ATTR_SET_HELP                                                                    \
	SHELL_HELP("Set biometric attribute.", "<device_name> <attribute> <value>")

#define BIOMETRIC_ENROLL_HELP                                                                      \
	SHELL_HELP("Full enrollment flow.", "<device_name> <template_id> [timeout_ms]")

#define BIOMETRIC_ENROLL_START_HELP SHELL_HELP("Start enrollment.", "<device_name> <template_id>")

#define BIOMETRIC_ENROLL_CAPTURE_HELP                                                              \
	SHELL_HELP("Capture enrollment sample.", "<device_name> [timeout_ms]")

#define BIOMETRIC_ENROLL_FINALIZE_HELP SHELL_HELP("Finalize enrollment.", "<device_name>")

#define BIOMETRIC_ENROLL_ABORT_HELP SHELL_HELP("Abort enrollment.", "<device_name>")

#define BIOMETRIC_TEMPLATE_LIST_HELP SHELL_HELP("List stored templates.", "<device_name>")

#define BIOMETRIC_TEMPLATE_DELETE_HELP                                                             \
	SHELL_HELP("Delete a template.", "<device_name> <template_id>")

#define BIOMETRIC_TEMPLATE_DELETE_ALL_HELP SHELL_HELP("Delete all templates.", "<device_name>")

#define BIOMETRIC_MATCH_HELP                                                                       \
	SHELL_HELP("Match biometric.\n"                                                            \
		   "For verify mode, template_id is required.",                                    \
		   "<device_name> <verify|identify> [template_id] [timeout_ms]")

#define BIOMETRIC_LED_HELP SHELL_HELP("Control LED.", "<device_name> <off|on|blink|breathe>")

#define BIOMETRIC_DEV_NOT_FOUND "Biometric device not found: \"%s\""

static bool biometric_device_check(const struct device *dev)
{
	return DEVICE_API_IS(biometric, dev);
}

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_filter(idx, biometric_device_check);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

static const char *const sensor_type_names[] = {
	[BIOMETRIC_TYPE_FINGERPRINT] = "fingerprint",
	[BIOMETRIC_TYPE_IRIS] = "iris",
	[BIOMETRIC_TYPE_FACE] = "face",
	[BIOMETRIC_TYPE_VOICE] = "voice",
};

static const char *const match_mode_names[] = {
	[BIOMETRIC_MATCH_VERIFY] = "verify",
	[BIOMETRIC_MATCH_IDENTIFY] = "identify",
};

static const char *const led_state_names[] = {
	[BIOMETRIC_LED_OFF] = "off",
	[BIOMETRIC_LED_ON] = "on",
	[BIOMETRIC_LED_BLINK] = "blink",
	[BIOMETRIC_LED_BREATHE] = "breathe",
};

static const char *const attr_names[] = {
	[BIOMETRIC_ATTR_MATCH_THRESHOLD] = "match_threshold",
	[BIOMETRIC_ATTR_ENROLLMENT_QUALITY] = "enrollment_quality",
	[BIOMETRIC_ATTR_SECURITY_LEVEL] = "security_level",
	[BIOMETRIC_ATTR_TIMEOUT_MS] = "timeout_ms",
	[BIOMETRIC_ATTR_ANTI_SPOOF_LEVEL] = "anti_spoof_level",
	[BIOMETRIC_ATTR_IMAGE_QUALITY] = "image_quality",
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

static int parse_timeout(k_timeout_t *timeout, const struct shell *sh, const char *arg)
{
	long ms;
	int ret;

	ret = parse_long(&ms, sh, arg);
	if (ret < 0) {
		return ret;
	}

	if (ms < 0) {
		*timeout = K_FOREVER;
	} else {
		*timeout = K_MSEC(ms);
	}

	return 0;
}

static int parse_led_state(enum biometric_led_state *state, const struct shell *sh, const char *arg)
{
	for (size_t i = 0; i < ARRAY_SIZE(led_state_names); i++) {
		if (strcmp(arg, led_state_names[i]) == 0) {
			*state = (enum biometric_led_state)i;
			return 0;
		}
	}

	shell_error(sh, "Unknown LED state: %s", arg);
	return -EINVAL;
}

static int parse_match_mode(enum biometric_match_mode *mode, const struct shell *sh,
			    const char *arg)
{
	for (size_t i = 0; i < ARRAY_SIZE(match_mode_names); i++) {
		if (strcmp(arg, match_mode_names[i]) == 0) {
			*mode = (enum biometric_match_mode)i;
			return 0;
		}
	}

	shell_error(sh, "Unknown match mode: %s", arg);
	return -EINVAL;
}

static int parse_attr(enum biometric_attribute *attr, const struct shell *sh, const char *arg)
{
	for (size_t i = 0; i < BIOMETRIC_ATTR_COMMON_COUNT; i++) {
		if (strcmp(arg, attr_names[i]) == 0) {
			*attr = (enum biometric_attribute)i;
			return 0;
		}
	}

	/* Try parsing as a number for private attributes */
	char *eptr;
	long val = strtol(arg, &eptr, 0);

	if (*eptr == '\0' && val >= 0 && val <= BIOMETRIC_ATTR_MAX) {
		*attr = (enum biometric_attribute)val;
		return 0;
	}

	shell_error(sh, "Unknown attribute: %s", arg);
	return -EINVAL;
}

static int cmd_biometric_info(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	struct biometric_capabilities caps;
	int ret;

	dev = shell_device_get_binding(argv[1]);
	if (!dev) {
		shell_error(sh, BIOMETRIC_DEV_NOT_FOUND, argv[1]);
		return -ENODEV;
	}

	ret = biometric_get_capabilities(dev, &caps);
	if (ret < 0) {
		shell_error(sh, "Failed to get capabilities: %d", ret);
		return ret;
	}

	shell_print(sh, "Device: %s", dev->name);
	shell_print(sh, "  Type: %s",
		    caps.type < ARRAY_SIZE(sensor_type_names) ? sensor_type_names[caps.type]
							      : "unknown");
	shell_print(sh, "  Max templates: %u", caps.max_templates);
	shell_print(sh, "  Template size: %u bytes", caps.template_size);
	shell_print(sh, "  Storage modes: %s%s",
		    (caps.storage_modes & BIOMETRIC_STORAGE_DEVICE) ? "device " : "",
		    (caps.storage_modes & BIOMETRIC_STORAGE_HOST) ? "host" : "");
	shell_print(sh, "  Enrollment samples: %u", caps.enrollment_samples_required);

	return 0;
}

static int cmd_biometric_attr_get(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	enum biometric_attribute attr;
	int32_t val;
	int ret;

	dev = shell_device_get_binding(argv[1]);
	if (!dev) {
		shell_error(sh, BIOMETRIC_DEV_NOT_FOUND, argv[1]);
		return -ENODEV;
	}

	ret = parse_attr(&attr, sh, argv[2]);
	if (ret < 0) {
		return ret;
	}

	ret = biometric_attr_get(dev, attr, &val);
	if (ret < 0) {
		shell_error(sh, "Failed to get attribute [%d]", ret);
		return ret;
	}

	if (attr < BIOMETRIC_ATTR_COMMON_COUNT) {
		shell_print(sh, "%s = %d", attr_names[attr], val);
	} else {
		shell_print(sh, "attr[%d] = %d", attr, val);
	}

	return 0;
}

static int cmd_biometric_attr_set(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	enum biometric_attribute attr;
	long val;
	int ret;

	dev = shell_device_get_binding(argv[1]);
	if (!dev) {
		shell_error(sh, BIOMETRIC_DEV_NOT_FOUND, argv[1]);
		return -ENODEV;
	}

	ret = parse_attr(&attr, sh, argv[2]);
	if (ret < 0) {
		return ret;
	}

	ret = parse_long(&val, sh, argv[3]);
	if (ret < 0) {
		return ret;
	}

	ret = biometric_attr_set(dev, attr, (int32_t)val);
	if (ret < 0) {
		shell_error(sh, "Failed to set attribute [%d]", ret);
		return ret;
	}

	return 0;
}

static int cmd_biometric_enroll_start(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	long template_id;
	int ret;

	dev = shell_device_get_binding(argv[1]);
	if (!dev) {
		shell_error(sh, BIOMETRIC_DEV_NOT_FOUND, argv[1]);
		return -ENODEV;
	}

	ret = parse_long(&template_id, sh, argv[2]);
	if (ret < 0) {
		return ret;
	}

	if (template_id < 1 || template_id > UINT16_MAX) {
		shell_error(sh, "Template ID out of range (1-%d)", UINT16_MAX);
		return -EINVAL;
	}

	ret = biometric_enroll_start(dev, (uint16_t)template_id);
	if (ret < 0) {
		shell_error(sh, "Failed to start enrollment [%d]", ret);
		return ret;
	}

	shell_print(sh, "Enrollment started, ID: %ld", template_id);
	return 0;
}

static int cmd_biometric_enroll_capture(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	struct biometric_capture_result result;
	k_timeout_t timeout = K_SECONDS(10);
	int ret;

	dev = shell_device_get_binding(argv[1]);
	if (!dev) {
		shell_error(sh, BIOMETRIC_DEV_NOT_FOUND, argv[1]);
		return -ENODEV;
	}

	if (argc > 2) {
		ret = parse_timeout(&timeout, sh, argv[2]);
		if (ret < 0) {
			return ret;
		}
	}

	ret = biometric_enroll_capture(dev, timeout, &result);
	if (ret < 0) {
		shell_error(sh, "Capture failed [%d]", ret);
		return ret;
	}

	shell_print(sh, "Sample %u/%u captured (quality: %u)", result.samples_captured,
		    result.samples_required, result.quality);
	return 0;
}

static int cmd_biometric_enroll_finalize(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int ret;

	dev = shell_device_get_binding(argv[1]);
	if (!dev) {
		shell_error(sh, BIOMETRIC_DEV_NOT_FOUND, argv[1]);
		return -ENODEV;
	}

	ret = biometric_enroll_finalize(dev);
	if (ret < 0) {
		shell_error(sh, "Finalize failed [%d]", ret);
		return ret;
	}

	shell_print(sh, "Enrollment finalized");
	return 0;
}

static int cmd_biometric_enroll_abort(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int ret;

	dev = shell_device_get_binding(argv[1]);
	if (!dev) {
		shell_error(sh, BIOMETRIC_DEV_NOT_FOUND, argv[1]);
		return -ENODEV;
	}

	ret = biometric_enroll_abort(dev);
	if (ret < 0) {
		shell_error(sh, "Abort failed [%d]", ret);
		return ret;
	}

	return 0;
}

static int cmd_biometric_template_list(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uint16_t ids[64];
	size_t count;
	int ret;

	dev = shell_device_get_binding(argv[1]);
	if (!dev) {
		shell_error(sh, BIOMETRIC_DEV_NOT_FOUND, argv[1]);
		return -ENODEV;
	}

	ret = biometric_template_list(dev, ids, ARRAY_SIZE(ids), &count);
	if (ret < 0) {
		shell_error(sh, "Failed to list templates [%d]", ret);
		return ret;
	}

	shell_print(sh, "Templates (%zu):", count);
	for (size_t i = 0; i < count; i++) {
		shell_print(sh, "  %u", ids[i]);
	}

	return 0;
}

static int cmd_biometric_template_delete(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	long template_id;
	int ret;

	dev = shell_device_get_binding(argv[1]);
	if (!dev) {
		shell_error(sh, BIOMETRIC_DEV_NOT_FOUND, argv[1]);
		return -ENODEV;
	}

	ret = parse_long(&template_id, sh, argv[2]);
	if (ret < 0) {
		return ret;
	}

	if (template_id < 1 || template_id > UINT16_MAX) {
		shell_error(sh, "Template ID out of range (1-%d)", UINT16_MAX);
		return -EINVAL;
	}

	ret = biometric_template_delete(dev, (uint16_t)template_id);
	if (ret < 0) {
		shell_error(sh, "Delete failed [%d]", ret);
		return ret;
	}

	return 0;
}

static int cmd_biometric_template_delete_all(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int ret;

	dev = shell_device_get_binding(argv[1]);
	if (!dev) {
		shell_error(sh, BIOMETRIC_DEV_NOT_FOUND, argv[1]);
		return -ENODEV;
	}

	ret = biometric_template_delete_all(dev);
	if (ret < 0) {
		shell_error(sh, "Delete all failed [%d]", ret);
		return ret;
	}

	return 0;
}

static int cmd_biometric_match(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	struct biometric_match_result result;
	enum biometric_match_mode mode;
	k_timeout_t timeout = K_SECONDS(10);
	long template_id = 0;
	int ret;

	dev = shell_device_get_binding(argv[1]);
	if (!dev) {
		shell_error(sh, BIOMETRIC_DEV_NOT_FOUND, argv[1]);
		return -ENODEV;
	}

	ret = parse_match_mode(&mode, sh, argv[2]);
	if (ret < 0) {
		return ret;
	}

	if (mode == BIOMETRIC_MATCH_VERIFY) {
		if (argc < 4) {
			shell_error(sh, "Verify mode requires template_id");
			return -EINVAL;
		}
		ret = parse_long(&template_id, sh, argv[3]);
		if (ret < 0) {
			return ret;
		}
		if (argc > 4) {
			ret = parse_timeout(&timeout, sh, argv[4]);
			if (ret < 0) {
				return ret;
			}
		}
	} else {
		/* Identify mode */
		if (argc > 3) {
			ret = parse_timeout(&timeout, sh, argv[3]);
			if (ret < 0) {
				return ret;
			}
		}
	}

	ret = biometric_match(dev, mode, (uint16_t)template_id, timeout, &result);
	if (ret < 0) {
		if (ret == -ENOENT) {
			shell_print(sh, "No match");
		} else {
			shell_error(sh, "Match failed [%d]", ret);
		}
		return ret;
	}

	shell_print(sh, "Match! ID: %u, confidence: %d, quality: %u", result.template_id,
		    result.confidence, result.image_quality);

	return 0;
}

static int cmd_biometric_led(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	enum biometric_led_state state;
	int ret;

	dev = shell_device_get_binding(argv[1]);
	if (!dev) {
		shell_error(sh, BIOMETRIC_DEV_NOT_FOUND, argv[1]);
		return -ENODEV;
	}

	ret = parse_led_state(&state, sh, argv[2]);
	if (ret < 0) {
		return ret;
	}

	ret = biometric_led_control(dev, state);
	if (ret < 0) {
		shell_error(sh, "LED control failed [%d]", ret);
		return ret;
	}

	return 0;
}

static int cmd_biometric_enroll(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	struct biometric_capabilities caps;
	struct biometric_capture_result result;
	k_timeout_t timeout = K_SECONDS(10);
	long template_id;
	int ret;

	dev = shell_device_get_binding(argv[1]);
	if (!dev) {
		shell_error(sh, BIOMETRIC_DEV_NOT_FOUND, argv[1]);
		return -ENODEV;
	}

	ret = parse_long(&template_id, sh, argv[2]);
	if (ret < 0) {
		return ret;
	}

	if (template_id < 1 || template_id > UINT16_MAX) {
		shell_error(sh, "Template ID out of range (1-%d)", UINT16_MAX);
		return -EINVAL;
	}

	if (argc > 3) {
		ret = parse_timeout(&timeout, sh, argv[3]);
		if (ret < 0) {
			return ret;
		}
	}

	ret = biometric_get_capabilities(dev, &caps);
	if (ret < 0) {
		shell_error(sh, "Failed to get capabilities [%d]", ret);
		return ret;
	}

	shell_print(sh, "Enrolling ID %ld (%d samples)", template_id,
		    caps.enrollment_samples_required);

	ret = biometric_enroll_start(dev, (uint16_t)template_id);
	if (ret < 0) {
		shell_error(sh, "Start failed [%d]", ret);
		return ret;
	}

	for (uint8_t i = 0; i < caps.enrollment_samples_required; i++) {
		shell_print(sh, "Capture %d/%d...", i + 1, caps.enrollment_samples_required);
		ret = biometric_enroll_capture(dev, timeout, &result);
		if (ret < 0) {
			shell_error(sh, "Capture failed [%d]", ret);
			biometric_enroll_abort(dev);
			return ret;
		}
		shell_print(sh, "  Quality: %u", result.quality);
	}

	ret = biometric_enroll_finalize(dev);
	if (ret < 0) {
		shell_error(sh, "Finalize failed [%d]", ret);
		return ret;
	}

	shell_print(sh, "Enrollment complete");
	return 0;
}

/* clang-format off */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_biometrics,
	SHELL_CMD_ARG(info, &dsub_device_name, BIOMETRIC_INFO_HELP,
		cmd_biometric_info, 2, 0),
	SHELL_CMD_ARG(attr_get, &dsub_device_name, BIOMETRIC_ATTR_GET_HELP,
		cmd_biometric_attr_get, 3, 0),
	SHELL_CMD_ARG(attr_set, &dsub_device_name, BIOMETRIC_ATTR_SET_HELP,
		cmd_biometric_attr_set, 4, 0),
	SHELL_CMD_ARG(enroll, &dsub_device_name, BIOMETRIC_ENROLL_HELP,
		cmd_biometric_enroll, 3, 1),
	SHELL_CMD_ARG(enroll_start, &dsub_device_name, BIOMETRIC_ENROLL_START_HELP,
		cmd_biometric_enroll_start, 3, 0),
	SHELL_CMD_ARG(enroll_capture, &dsub_device_name, BIOMETRIC_ENROLL_CAPTURE_HELP,
		cmd_biometric_enroll_capture, 2, 1),
	SHELL_CMD_ARG(enroll_finalize, &dsub_device_name, BIOMETRIC_ENROLL_FINALIZE_HELP,
		cmd_biometric_enroll_finalize, 2, 0),
	SHELL_CMD_ARG(enroll_abort, &dsub_device_name, BIOMETRIC_ENROLL_ABORT_HELP,
		cmd_biometric_enroll_abort, 2, 0),
	SHELL_CMD_ARG(template_list, &dsub_device_name, BIOMETRIC_TEMPLATE_LIST_HELP,
		cmd_biometric_template_list, 2, 0),
	SHELL_CMD_ARG(template_delete, &dsub_device_name, BIOMETRIC_TEMPLATE_DELETE_HELP,
		cmd_biometric_template_delete, 3, 0),
	SHELL_CMD_ARG(template_delete_all, &dsub_device_name, BIOMETRIC_TEMPLATE_DELETE_ALL_HELP,
		cmd_biometric_template_delete_all, 2, 0),
	SHELL_CMD_ARG(match, &dsub_device_name, BIOMETRIC_MATCH_HELP,
		cmd_biometric_match, 3, 2),
	SHELL_CMD_ARG(led, &dsub_device_name, BIOMETRIC_LED_HELP,
		cmd_biometric_led, 3, 0),
	SHELL_SUBCMD_SET_END
);
/* clang-format on */

SHELL_CMD_REGISTER(biometrics, &sub_biometrics, "Biometrics commands", NULL);
