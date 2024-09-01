/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/comparator.h>
#include <zephyr/shell/shell.h>
#include <zephyr/kernel.h>

#include <stdlib.h>

#define AWAIT_TRIGGER_DEFAULT_TIMEOUT \
	CONFIG_COMPARATOR_SHELL_AWAIT_TRIGGER_DEFAULT_TIMEOUT

#define AWAIT_TRIGGER_MAX_TIMEOUT \
	CONFIG_COMPARATOR_SHELL_AWAIT_TRIGGER_MAX_TIMEOUT

/* Mapped 1-1 to enum comparator_trigger */
static const char *const trigger_lookup[] = {
	"NONE",
	"RISING_EDGE",
	"FALLING_EDGE",
	"BOTH_EDGES",
};

static K_SEM_DEFINE(triggered_sem, 0, 1);

static int get_device_from_str(const struct shell *sh,
			       const char *dev_str,
			       const struct device **dev)
{
	*dev = device_get_binding(dev_str);

	if (*dev == NULL) {
		shell_error(sh, "%s not %s", dev_str, "found");
		return -ENODEV;
	}

	if (!device_is_ready(*dev)) {
		shell_error(sh, "%s not %s", dev_str, "ready");
		return -ENODEV;
	}

	return 0;
}

static int cmd_get_output(const struct shell *sh, size_t argc, char **argv)
{
	int ret;
	const char *dev_str;
	const struct device *dev;

	ARG_UNUSED(argc);

	dev_str = argv[1];
	ret = get_device_from_str(sh, dev_str, &dev);
	if (ret < 0) {
		return ret;
	}

	ret = comparator_get_output(dev);
	if (ret < 0) {
		shell_error(sh, "failed to %s %s", "get", "output");
		return -EIO;
	}

	shell_print(sh, "%i", ret);
	return 0;
}

static int get_trigger_from_str(const struct shell *sh,
				const char *trigger_str,
				enum comparator_trigger *trigger)
{
	ARRAY_FOR_EACH(trigger_lookup, i) {
		if (strcmp(trigger_lookup[i], trigger_str) == 0) {
			*trigger = (enum comparator_trigger)i;
			return 0;
		}
	}

	shell_error(sh, "%s not %s", trigger_str, "valid");
	return -EINVAL;
}

static int cmd_set_trigger(const struct shell *sh, size_t argc, char **argv)
{
	const char *dev_str;
	const char *trigger_str;
	int ret;
	const struct device *dev;
	enum comparator_trigger trigger;

	ARG_UNUSED(argc);

	dev_str = argv[1];
	ret = get_device_from_str(sh, dev_str, &dev);
	if (ret < 0) {
		return ret;
	}

	trigger_str = argv[2];
	ret = get_trigger_from_str(sh, trigger_str, &trigger);
	if (ret < 0) {
		return ret;
	}

	ret = comparator_set_trigger(dev, trigger);
	if (ret < 0) {
		shell_error(sh, "failed to %s %s", "set", "trigger");
		return -EIO;
	}

	return 0;
}

static int get_timeout_from_str(const struct shell *sh,
				const char *timeout_str,
				k_timeout_t *timeout)
{
	long seconds;
	char *end;

	seconds = strtol(timeout_str, &end, 10);
	if ((*end != '\0') ||
	    (seconds < 1) ||
	    (seconds > AWAIT_TRIGGER_MAX_TIMEOUT)) {
		shell_error(sh, "%s not %s", timeout_str, "valid");
		return -EINVAL;
	}

	*timeout = K_SECONDS(seconds);
	return 0;
}

static void trigger_cb(const struct device *dev, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	k_sem_give(&triggered_sem);
}

static int cmd_await_trigger(const struct shell *sh, size_t argc, char **argv)
{
	const char *dev_str;
	const char *timeout_str;
	int ret;
	const struct device *dev;
	k_timeout_t timeout;

	dev_str = argv[1];
	ret = get_device_from_str(sh, dev_str, &dev);
	if (ret < 0) {
		return ret;
	}

	if (argc == 3) {
		timeout_str = argv[2];
		ret = get_timeout_from_str(sh, timeout_str, &timeout);
		if (ret < 0) {
			return ret;
		}
	} else {
		timeout = K_SECONDS(AWAIT_TRIGGER_DEFAULT_TIMEOUT);
	}

	k_sem_reset(&triggered_sem);

	ret = comparator_set_trigger_callback(dev, trigger_cb, NULL);
	if (ret < 0) {
		shell_error(sh, "failed to %s %s", "set", "trigger callback");
		return -EIO;
	}

	ret = k_sem_take(&triggered_sem, timeout);
	if (ret == 0) {
		shell_print(sh, "triggered");
	} else if (ret == -EAGAIN) {
		shell_print(sh, "timed out");
	} else {
		shell_error(sh, "internal error");
	}

	ret = comparator_set_trigger_callback(dev, NULL, NULL);
	if (ret < 0) {
		shell_error(sh, "failed to %s %s", "clear", "trigger callback");
		return -EIO;
	}

	return 0;
}

static int cmd_trigger_is_pending(const struct shell *sh, size_t argc, char **argv)
{
	int ret;
	const char *dev_str;
	const struct device *dev;

	ARG_UNUSED(argc);

	dev_str = argv[1];
	ret = get_device_from_str(sh, dev_str, &dev);
	if (ret < 0) {
		return ret;
	}

	ret = comparator_trigger_is_pending(dev);
	if (ret < 0) {
		shell_error(sh, "failed to %s %s", "get", "trigger status");
		return -EIO;
	}

	shell_print(sh, "%i", ret);
	return 0;
}

static void dsub_set_trigger_lookup_1(size_t idx, struct shell_static_entry *entry)
{
	entry->syntax = (idx < ARRAY_SIZE(trigger_lookup)) ? trigger_lookup[idx] : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_set_trigger_1, dsub_set_trigger_lookup_1);

static void dsub_set_trigger_lookup_0(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, NULL);

	entry->syntax = dev != NULL ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = &dsub_set_trigger_1;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_set_trigger_0, dsub_set_trigger_lookup_0);

static void dsub_device_lookup_0(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, NULL);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_0, dsub_device_lookup_0);

#define GET_OUTPUT_HELP \
	("comp get_output <device>")

#define SET_TRIGGER_HELP \
	("comp set_trigger <device> <NONE | RISING_EDGE | FALLING_EDGE | BOTH_EDGES>")

#define AWAIT_TRIGGER_HELP								\
	("comp await_trigger <device> [timeout] (default "				\
	 STRINGIFY(AWAIT_TRIGGER_DEFAULT_TIMEOUT)					\
	 "s, max "									\
	 STRINGIFY(AWAIT_TRIGGER_MAX_TIMEOUT)						\
	 "s)")

#define TRIGGER_PENDING_HELP \
	("comp trigger_is_pending <device>")

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_comp,
	SHELL_CMD_ARG(get_output, &dsub_device_0, GET_OUTPUT_HELP, cmd_get_output, 2, 0),
	SHELL_CMD_ARG(set_trigger, &dsub_set_trigger_0, SET_TRIGGER_HELP, cmd_set_trigger, 3, 0),
	SHELL_CMD_ARG(await_trigger, &dsub_device_0, AWAIT_TRIGGER_HELP, cmd_await_trigger, 2, 1),
	SHELL_CMD_ARG(trigger_is_pending, &dsub_device_0, TRIGGER_PENDING_HELP,
		      cmd_trigger_is_pending, 2, 1),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(comp, &sub_comp, "Comparator device commands", NULL);
