/*
 * Copyright (c) 2026 Siratul Islam <siratul.islam@linux.dev>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/biometrics.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>
#include <string.h>

static const struct device *const biometrics = DEVICE_DT_GET(DT_ALIAS(biometrics));

static const char *modality_name(uint32_t modality)
{
	switch (modality) {
	case BIOMETRIC_MODALITY_FINGERPRINT:
		return "fingerprint";
	case BIOMETRIC_MODALITY_IRIS:
		return "iris";
	case BIOMETRIC_MODALITY_FACE:
		return "face";
	case BIOMETRIC_MODALITY_VOICE:
		return "voice";
	case BIOMETRIC_MODALITY_PALM:
		return "palm";
	default:
		return "unknown modality";
	}
}

static void on_event(const struct device *dev, const struct biometric_event *event, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	switch (event->type) {
	case BIOMETRIC_EVENT_MATCH:
		printk("Matched %s ID %u\n", modality_name(event->match.modality),
		       event->match.template_id);
		break;
	case BIOMETRIC_EVENT_NO_MATCH:
		printk("No match: %d\n", event->status);
		break;
	case BIOMETRIC_EVENT_ENROLL_COMPLETE:
		printk("Enrolled %s ID %u\n", modality_name(event->enrollment.modality),
		       event->enrollment.template_id);
		break;
	case BIOMETRIC_EVENT_ERROR:
		printk("Operation failed: %d\n", event->status);
		break;
	case BIOMETRIC_EVENT_STOPPED:
		printk("Operation ended: %d\n", event->status);
		break;
	}
}

static int check_operation(const struct shell *sh, uint32_t operation, const char *name)
{
	struct biometric_capabilities caps;
	int ret;

	if (!device_is_ready(biometrics)) {
		shell_error(sh, "Biometrics device is unavailable");
		return -ENODEV;
	}

	ret = biometric_get_capabilities(biometrics, &caps);
	if (ret != 0) {
		shell_error(sh, "Cannot query capabilities: %d", ret);
		return ret;
	}

	if ((caps.async_operations & operation) == 0U) {
		shell_error(sh, "%s is not supported by this device", name);
		return -ENOTSUP;
	}

	return 0;
}

static int cmd_enroll(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	ret = check_operation(sh, BIOMETRIC_ASYNC_ENROLL, "Asynchronous enrollment");
	if (ret != 0) {
		return ret;
	}

	ret = biometric_enroll_async(biometrics, BIOMETRIC_ID_AUTO, K_SECONDS(20));
	if (ret == 0) {
		shell_print(sh, "Present a biometric sample within 20 seconds.");
	} else {
		shell_error(sh, "Cannot start enrollment: %d", ret);
	}

	return ret;
}

static int cmd_identify(const struct shell *sh, size_t argc, char **argv)
{
	bool continuous = argc == 1U;
	int ret;

	if (!continuous && strcmp(argv[1], "once") != 0) {
		shell_error(sh, "Usage: biometrics_async identify [once]");
		return -EINVAL;
	}

	ret = check_operation(sh, BIOMETRIC_ASYNC_IDENTIFY, "Asynchronous identification");
	if (ret != 0) {
		return ret;
	}

	ret = biometric_match_async(biometrics, BIOMETRIC_MATCH_IDENTIFY, 0, continuous,
				    K_SECONDS(10));
	if (ret == 0) {
		if (continuous) {
			shell_print(sh, "Recognition active. Use 'biometrics_async stop' to stop.");
		} else {
			shell_print(sh, "Present a biometric sample within 10 seconds.");
		}
	} else {
		shell_error(sh, "Cannot start identification: %d", ret);
	}

	return ret;
}

static int cmd_stop(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	ret = device_is_ready(biometrics) ? biometric_async_stop(biometrics) : -ENODEV;
	if (ret != 0) {
		shell_error(sh, "Cannot stop operation: %d", ret);
	}

	return ret;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	async_commands,
	SHELL_CMD_ARG(enroll, NULL, "Enroll a biometric template.", cmd_enroll, 1, 0),
	SHELL_CMD_ARG(identify, NULL, "Identify continuously, or 'once' for one attempt.",
		      cmd_identify, 1, 1),
	SHELL_CMD_ARG(stop, NULL, "Stop the active operation.", cmd_stop, 1, 0),
	SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(biometrics_async, &async_commands, "Asynchronous biometric operations", NULL);

int main(void)
{
	struct biometric_capabilities caps;
	int ret;

	if (!device_is_ready(biometrics)) {
		printk("Biometrics device is unavailable\n");
		return 0;
	}

	ret = biometric_get_capabilities(biometrics, &caps);
	if (ret != 0) {
		printk("Cannot query capabilities: %d\n", ret);
		return 0;
	}

	if ((caps.async_operations & (BIOMETRIC_ASYNC_ENROLL | BIOMETRIC_ASYNC_IDENTIFY)) == 0U) {
		printk("Device supports neither asynchronous enrollment nor identification\n");
		return 0;
	}

	ret = biometric_callback_set(biometrics, on_event, NULL);
	if (ret != 0) {
		printk("Cannot register callback: %d\n", ret);
		return 0;
	}

	printk("Asynchronous enrollment: %s\n",
	       (caps.async_operations & BIOMETRIC_ASYNC_ENROLL) != 0U ? "supported"
								      : "unsupported");
	printk("Asynchronous identification: %s\n",
	       (caps.async_operations & BIOMETRIC_ASYNC_IDENTIFY) != 0U ? "supported"
									: "unsupported");
	printk("Use 'biometrics_async enroll', 'biometrics_async identify', "
	       "and 'biometrics_async stop'.\n");
	printk("Device name for biometric database commands: %s\n", biometrics->name);

	return 0;
}
