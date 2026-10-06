/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>
#include <zephyr/precision_timing/precision_clock.h>
#include <zephyr/precision_timing/precision_time.h>
#include <zephyr/precision_timing/precision_timing_shell.h>

#include "precision_timing_shell_internal.h"

struct precision_timing_shell_entry {
	char name[PRECISION_TIMING_SHELL_NAME_MAX + 1];
	const struct precision_clock *clock;
	size_t active_operations;
	bool unregistering;
};

static struct precision_timing_shell_entry registry[CONFIG_PRECISION_TIMING_SHELL_MAX_INSTANCES];
static size_t registry_count;
static K_MUTEX_DEFINE(registry_lock);
static K_CONDVAR_DEFINE(registry_idle);

static bool name_is_valid(const char *name)
{
	size_t length;

	if (name == NULL) {
		return false;
	}

	length = strlen(name);
	if (length == 0 || length > PRECISION_TIMING_SHELL_NAME_MAX) {
		return false;
	}

	if (strcmp(name, "-h") == 0 || strcmp(name, "--help") == 0) {
		return false;
	}

	for (size_t i = 0; i < length; i++) {
		unsigned char character = name[i];

		if (character > 0x7f || (!isalnum(character) && character != '.' &&
					 character != '-' && character != '_')) {
			return false;
		}
	}

	return true;
}

static size_t registry_lower_bound(const char *name, bool *found)
{
	size_t left = 0;
	size_t right = registry_count;

	while (left < right) {
		size_t middle = left + (right - left) / 2;
		int comparison = strcmp(registry[middle].name, name);

		if (comparison < 0) {
			left = middle + 1;
		} else {
			right = middle;
		}
	}

	*found = left < registry_count && strcmp(registry[left].name, name) == 0;
	return left;
}

int precision_timing_shell_register(const char *name, const struct precision_clock *precision_clk)
{
	struct precision_timing_shell_entry *entry;
	size_t name_length;
	size_t index;
	bool found;

	if (!name_is_valid(name) || precision_clk == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&registry_lock, K_FOREVER);
	index = registry_lower_bound(name, &found);
	if (found) {
		k_mutex_unlock(&registry_lock);
		return -EEXIST;
	}

	if (registry_count == ARRAY_SIZE(registry)) {
		k_mutex_unlock(&registry_lock);
		return -ENOSPC;
	}

	memmove(&registry[index + 1], &registry[index],
		(registry_count - index) * sizeof(registry[0]));
	entry = &registry[index];
	name_length = strlen(name);
	memcpy(entry->name, name, name_length + 1);
	entry->clock = precision_clk;
	entry->active_operations = 0;
	entry->unregistering = false;
	registry_count++;
	k_mutex_unlock(&registry_lock);

	return 0;
}

int precision_timing_shell_unregister(const char *name)
{
	size_t index;
	bool found;

	if (!name_is_valid(name)) {
		return -EINVAL;
	}

	k_mutex_lock(&registry_lock, K_FOREVER);
	index = registry_lower_bound(name, &found);
	if (!found || registry[index].unregistering) {
		k_mutex_unlock(&registry_lock);
		return -ENOENT;
	}

	registry[index].unregistering = true;
	while (registry[index].active_operations != 0) {
		(void)k_condvar_wait(&registry_idle, &registry_lock, K_FOREVER);
		index = registry_lower_bound(name, &found);
		__ASSERT_NO_MSG(found && registry[index].unregistering);
	}

	registry_count--;
	memmove(&registry[index], &registry[index + 1],
		(registry_count - index) * sizeof(registry[0]));
	memset(&registry[registry_count], 0, sizeof(registry[0]));
	k_mutex_unlock(&registry_lock);

	return 0;
}

int precision_timing_shell_operation_acquire(const char *name,
					     struct precision_timing_shell_operation *operation)
{
	size_t index;
	bool found;

	if (operation == NULL || !name_is_valid(name)) {
		return -EINVAL;
	}

	k_mutex_lock(&registry_lock, K_FOREVER);
	index = registry_lower_bound(name, &found);
	if (!found || registry[index].unregistering) {
		k_mutex_unlock(&registry_lock);
		return -ENOENT;
	}

	operation->clock = registry[index].clock;
	memcpy(operation->name, registry[index].name, sizeof(operation->name));
	registry[index].active_operations++;
	k_mutex_unlock(&registry_lock);

	return 0;
}

void precision_timing_shell_operation_release(struct precision_timing_shell_operation *operation)
{
	size_t index;
	bool found;

	if (operation == NULL) {
		return;
	}

	k_mutex_lock(&registry_lock, K_FOREVER);
	index = registry_lower_bound(operation->name, &found);
	__ASSERT_NO_MSG(found);
	if (found) {
		registry[index].active_operations--;
		if (registry[index].active_operations == 0 && registry[index].unregistering) {
			k_condvar_broadcast(&registry_idle);
		}
	}
	k_mutex_unlock(&registry_lock);
}

#if defined(CONFIG_PRECISION_CLOCK_OUTPUT)
static int report_argument_error(const struct shell *sh, const char *argument, const char *value,
				 int error)
{
	shell_error(sh, "invalid %s '%s': %d", argument, value, error);
	return error;
}

static int parse_u32(const struct shell *sh, const char *argument, const char *value,
		     uint32_t *result)
{
	unsigned long long parsed;
	int ret = 0;

	parsed = shell_strtoull(value, 10, &ret);
	if (ret == 0 && parsed > UINT32_MAX) {
		ret = -ERANGE;
	}
	if (ret < 0) {
		return report_argument_error(sh, argument, value, ret);
	}

	*result = (uint32_t)parsed;
	return 0;
}

static int parse_nonnegative_time(const struct shell *sh, const char *argument,
				  const char *value, precision_time_t *result)
{
	unsigned long long parsed;
	int ret = 0;

	parsed = shell_strtoull(value, 10, &ret);
	if (ret == 0 && parsed > PRECISION_TIME_MAX) {
		ret = -ERANGE;
	}
	if (ret < 0) {
		return report_argument_error(sh, argument, value, ret);
	}

	*result = (precision_time_t)parsed;
	return 0;
}

static int parse_time(const struct shell *sh, const char *argument, const char *value,
		      precision_time_t *result)
{
	const bool negative = value[0] == '-';
	const char *magnitude_string = negative ? &value[1] : value;
	const uint64_t minimum_magnitude = (uint64_t)PRECISION_TIME_MAX + 1U;
	unsigned long long magnitude;
	int ret = 0;

	magnitude = shell_strtoull(magnitude_string, 10, &ret);
	if (ret == 0 && ((!negative && magnitude > PRECISION_TIME_MAX) ||
			 (negative && magnitude > minimum_magnitude))) {
		ret = -ERANGE;
	}
	if (ret < 0) {
		return report_argument_error(sh, argument, value, ret);
	}

	if (negative && magnitude == minimum_magnitude) {
		*result = PRECISION_TIME_MIN;
	} else if (negative) {
		*result = -(precision_time_t)magnitude;
	} else {
		*result = (precision_time_t)magnitude;
	}

	return 0;
}

static const char *yes_no(bool value)
{
	return value ? "yes" : "no";
}

static const char *width_policy_name(enum precision_clock_output_width_policy policy)
{
	switch (policy) {
	case PRECISION_CLOCK_OUTPUT_WIDTH_PROVIDER_DEFAULT:
		return "default";
	case PRECISION_CLOCK_OUTPUT_WIDTH_EXACT:
		return "exact";
	default:
		return "unknown";
	}
}
#endif /* CONFIG_PRECISION_CLOCK_OUTPUT */

static int report_error(const struct shell *sh, const char *name, int error)
{
	shell_error(sh, "precision clock '%s': %d", name, error);
	return error;
}

static int cmd_list(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	k_mutex_lock(&registry_lock, K_FOREVER);
	for (size_t i = 0; i < registry_count; i++) {
		if (!registry[i].unregistering) {
			shell_print(sh, "%s", registry[i].name);
		}
	}
	k_mutex_unlock(&registry_lock);

	return 0;
}

static int cmd_get(const struct shell *sh, size_t argc, char **argv)
{
	struct precision_timing_shell_operation operation;
	precision_time_t tm;
	int ret;

	ARG_UNUSED(argc);

	ret = precision_timing_shell_operation_acquire(argv[1], &operation);
	if (ret == 0) {
		ret = precision_clock_read(operation.clock, &tm);
		precision_timing_shell_operation_release(&operation);
	}
	if (ret < 0) {
		return report_error(sh, argv[1], ret);
	}

	shell_print(sh, "time_ns=%lld", (long long)tm);
	return 0;
}

#if defined(CONFIG_PRECISION_CLOCK_OUTPUT)
static int cmd_output_caps(const struct shell *sh, size_t argc, char **argv)
{
	struct precision_timing_shell_operation operation;
	struct precision_clock_output_caps caps;
	uint32_t channel;
	int ret;

	ARG_UNUSED(argc);

	ret = parse_u32(sh, "channel", argv[2], &channel);
	if (ret < 0) {
		return ret;
	}

	ret = precision_timing_shell_operation_acquire(argv[1], &operation);
	if (ret == 0) {
		ret = precision_clock_output_get_caps(operation.clock, channel, &caps);
		precision_timing_shell_operation_release(&operation);
	}
	if (ret < 0) {
		return report_error(sh, argv[1], ret);
	}

	shell_print(sh, "waveform=%s programmable_width=%s channel_count=%u",
		    yes_no((caps.flags & PRECISION_CLOCK_OUTPUT_CAP_WAVEFORM) != 0),
		    yes_no((caps.flags & PRECISION_CLOCK_OUTPUT_CAP_PROGRAMMABLE_WIDTH) != 0),
		    caps.channel_count);
	shell_print(sh, "hardware_active=%s",
		    yes_no((caps.flags & PRECISION_CLOCK_OUTPUT_CAP_HARDWARE_ACTIVE) != 0));
	shell_print(sh, "resolution_ns=%lld min_lead_time_ns=%lld",
		    (long long)caps.resolution_ns, (long long)caps.min_lead_time_ns);
	shell_print(sh, "min_period_ns=%lld max_period_ns=%lld", (long long)caps.min_period_ns,
		    (long long)caps.max_period_ns);
	shell_print(sh, "min_pulse_width_ns=%lld max_pulse_width_ns=%lld",
		    (long long)caps.min_pulse_width_ns, (long long)caps.max_pulse_width_ns);
	return 0;
}

static void print_output_status(const struct shell *sh,
				const struct precision_clock_output_status *status)
{
	const char *hardware_active;

	shell_print(sh, "configured=%s", yes_no(status->configured));
	if (status->configured && status->kind == PRECISION_CLOCK_OUTPUT_KIND_WAVEFORM) {
		const struct precision_clock_output_waveform_config *waveform =
			&status->config.waveform;

		shell_print(sh, "kind=waveform");
		shell_print(sh, "first_rising_time_ns=%lld",
			    (long long)waveform->first_rising_time);
		if (waveform->width_policy == PRECISION_CLOCK_OUTPUT_WIDTH_EXACT) {
			shell_print(sh, "period_ns=%lld width_policy=%s pulse_width_ns=%lld",
				    (long long)waveform->period_ns,
				    width_policy_name(waveform->width_policy),
				    (long long)waveform->pulse_width_ns);
		} else {
			shell_print(sh, "period_ns=%lld width_policy=%s",
				    (long long)waveform->period_ns,
				    width_policy_name(waveform->width_policy));
		}
	}

	if (!status->hardware_active_valid) {
		hardware_active = "unknown";
	} else {
		hardware_active = yes_no(status->hardware_active);
	}
	shell_print(sh, "hardware_active=%s", hardware_active);
}

static int cmd_output_get(const struct shell *sh, size_t argc, char **argv)
{
	struct precision_timing_shell_operation operation;
	struct precision_clock_output_status status;
	uint32_t channel;
	int ret;

	ARG_UNUSED(argc);

	ret = parse_u32(sh, "channel", argv[2], &channel);
	if (ret < 0) {
		return ret;
	}

	ret = precision_timing_shell_operation_acquire(argv[1], &operation);
	if (ret == 0) {
		ret = precision_clock_output_get_status(operation.clock, channel, &status);
		precision_timing_shell_operation_release(&operation);
	}
	if (ret < 0) {
		return report_error(sh, argv[1], ret);
	}

	print_output_status(sh, &status);
	return 0;
}

static int cmd_output_waveform(const struct shell *sh, size_t argc, char **argv)
{
	struct precision_timing_shell_operation operation;
	struct precision_clock_output_waveform_config config = {
		.width_policy = PRECISION_CLOCK_OUTPUT_WIDTH_PROVIDER_DEFAULT,
	};
	uint32_t channel;
	int ret;

	ret = parse_u32(sh, "channel", argv[2], &channel);
	if (ret == 0) {
		ret = parse_time(sh, "first rising time", argv[3], &config.first_rising_time);
	}
	if (ret == 0) {
		ret = parse_nonnegative_time(sh, "period", argv[4], &config.period_ns);
	}
	if (ret == 0 && argc == 6) {
		config.width_policy = PRECISION_CLOCK_OUTPUT_WIDTH_EXACT;
		ret = parse_nonnegative_time(sh, "pulse width", argv[5], &config.pulse_width_ns);
	}
	if (ret < 0) {
		return ret;
	}

	ret = precision_timing_shell_operation_acquire(argv[1], &operation);
	if (ret == 0) {
		ret = precision_clock_output_start_waveform(operation.clock, channel, &config);
		precision_timing_shell_operation_release(&operation);
	}
	if (ret < 0) {
		return report_error(sh, argv[1], ret);
	}

	return 0;
}

static int cmd_output_stop(const struct shell *sh, size_t argc, char **argv)
{
	struct precision_timing_shell_operation operation;
	uint32_t channel;
	int ret;

	ARG_UNUSED(argc);

	ret = parse_u32(sh, "channel", argv[2], &channel);
	if (ret < 0) {
		return ret;
	}

	ret = precision_timing_shell_operation_acquire(argv[1], &operation);
	if (ret == 0) {
		ret = precision_clock_output_stop(operation.clock, channel);
		precision_timing_shell_operation_release(&operation);
	}
	if (ret < 0) {
		return report_error(sh, argv[1], ret);
	}

	return 0;
}

#define PRECISION_TIMING_PPS_PERIOD_NS           ((precision_time_t)NSEC_PER_SEC)
#define PRECISION_TIMING_PPS_DEFAULT_WIDTH_NS    (200 * (precision_time_t)NSEC_PER_MSEC)
#define PRECISION_TIMING_PPS_SCHEDULING_GUARD_NS (2 * (precision_time_t)NSEC_PER_SEC)
#define PRECISION_TIMING_PPS_START_ATTEMPTS      2U

static int cmd_pps_start(const struct shell *sh, size_t argc, char **argv)
{
	struct precision_clock_output_waveform_config config = {
		.period_ns = PRECISION_TIMING_PPS_PERIOD_NS,
		.width_policy = PRECISION_CLOCK_OUTPUT_WIDTH_EXACT,
		.pulse_width_ns = PRECISION_TIMING_PPS_DEFAULT_WIDTH_NS,
	};
	struct precision_clock_output_caps caps;
	struct precision_timing_shell_operation operation;
	precision_time_t now;
	precision_time_t lead_time_ns;
	uint32_t channel;
	uint32_t attempt;
	int ret;

	ret = parse_u32(sh, "channel", argv[2], &channel);
	if (ret == 0 && argc == 4) {
		ret = parse_nonnegative_time(sh, "pulse width", argv[3], &config.pulse_width_ns);
	}
	if (ret < 0) {
		return ret;
	}

	ret = precision_timing_shell_operation_acquire(argv[1], &operation);
	if (ret == 0) {
		ret = precision_clock_output_get_caps(operation.clock, channel, &caps);
		if (ret == 0 && argc != 4 &&
		    (((caps.flags & PRECISION_CLOCK_OUTPUT_CAP_PROGRAMMABLE_WIDTH) == 0U) ||
		     PRECISION_TIMING_PPS_DEFAULT_WIDTH_NS < caps.min_pulse_width_ns ||
		     PRECISION_TIMING_PPS_DEFAULT_WIDTH_NS > caps.max_pulse_width_ns ||
		     caps.resolution_ns <= 0 ||
		     (PRECISION_TIMING_PPS_DEFAULT_WIDTH_NS % caps.resolution_ns) != 0)) {
			config.width_policy = PRECISION_CLOCK_OUTPUT_WIDTH_PROVIDER_DEFAULT;
			config.pulse_width_ns = 0;
		}
		if (ret == 0) {
			if (caps.min_lead_time_ns < 0) {
				ret = -ERANGE;
			} else {
				ret = precision_time_add(caps.min_lead_time_ns,
							 PRECISION_TIMING_PPS_SCHEDULING_GUARD_NS,
							 &lead_time_ns);
			}
		}
		if (ret == 0) {
			for (attempt = 0U; attempt < PRECISION_TIMING_PPS_START_ATTEMPTS;
			     attempt++) {
				ret = precision_clock_read(operation.clock, &now);
				if (ret == 0) {
					ret = precision_clock_output_next_start_time(
						now, config.period_ns, lead_time_ns,
						&config.first_rising_time);
				}
				if (ret < 0) {
					break;
				}
				ret = precision_clock_output_start_waveform(
					operation.clock, channel, &config);
				if (ret != -ETIME ||
				    attempt + 1U == PRECISION_TIMING_PPS_START_ATTEMPTS) {
					break;
				}
				ret = precision_clock_output_stop(operation.clock, channel);
				if (ret < 0) {
					break;
				}
			}
		}
		precision_timing_shell_operation_release(&operation);
	}
	if (ret < 0) {
		return report_error(sh, argv[1], ret);
	}

	if (config.width_policy == PRECISION_CLOCK_OUTPUT_WIDTH_EXACT) {
		shell_print(sh,
			    "first_rising_time_ns=%lld period_ns=%lld width_policy=exact "
			    "pulse_width_ns=%lld",
			    (long long)config.first_rising_time, (long long)config.period_ns,
			    (long long)config.pulse_width_ns);
	} else {
		shell_print(
			sh,
			"first_rising_time_ns=%lld period_ns=%lld width_policy=provider_default",
			(long long)config.first_rising_time, (long long)config.period_ns);
	}
	return 0;
}

static int cmd_pps_stop(const struct shell *sh, size_t argc, char **argv)
{
	return cmd_output_stop(sh, argc, argv);
}
#endif /* CONFIG_PRECISION_CLOCK_OUTPUT */

#if defined(CONFIG_PRECISION_CLOCK_OUTPUT)
SHELL_STATIC_SUBCMD_SET_CREATE(
	precision_clock_output_commands,
	SHELL_CMD_ARG(caps, NULL, "Show output capabilities. Usage: caps <name> <channel>",
		      cmd_output_caps, 3, 0),
	SHELL_CMD_ARG(get, NULL, "Show output status. Usage: get <name> <channel>", cmd_output_get,
		      3, 0),
	SHELL_CMD_ARG(waveform, NULL,
		      "Start a periodic output waveform. Usage: waveform <name> <channel> "
		      "<first_rising_time_ns> <period_ns> [pulse_width_ns]",
		      cmd_output_waveform, 5, 1),
	SHELL_CMD_ARG(stop, NULL, "Stop output. Usage: stop <name> <channel>", cmd_output_stop, 3,
		      0),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	precision_clock_pps_commands,
	SHELL_CMD_ARG(start, NULL,
		      "Arm a 1 Hz PPS waveform once; no autonomous monitoring or rearming.\n"
		      "Usage: start <name> <channel> [pulse_width_ns]",
		      cmd_pps_start, 3, 1),
	SHELL_CMD_ARG(stop, NULL, "Stop PPS. Usage: stop <name> <channel>", cmd_pps_stop, 3, 0),
	SHELL_SUBCMD_SET_END);
#endif /* CONFIG_PRECISION_CLOCK_OUTPUT */

SHELL_STATIC_SUBCMD_SET_CREATE(
	precision_clock_commands,
	SHELL_CMD_ARG(list, NULL, "List registered precision clocks.", cmd_list, 1, 0),
	SHELL_CMD_ARG(get, NULL, "Read a precision clock. Usage: get <name>", cmd_get, 2, 0),
#if defined(CONFIG_PRECISION_CLOCK_OUTPUT)
	SHELL_CMD(output, &precision_clock_output_commands, "Inspect and control clock output.",
		  NULL),
	SHELL_CMD(pps, &precision_clock_pps_commands, "Control a 1 Hz PPS output.", NULL),
#endif /* CONFIG_PRECISION_CLOCK_OUTPUT */
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(precision_clock, &precision_clock_commands,
		   "Inspect and control registered precision clocks.", NULL);
