/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/pm/event_domain.h>
#include <zephyr/devicetree.h>
#include <zephyr/shell/shell.h>

#include <string.h>

#define EDS_DT_EVENT_DOMAIN_BY_IDX(_idx) \
	DT_NODE_CHILD_IDX(DT_PATH(event_domains), _idx)

#define EDS_DT_EVENT_DOMAIN_NAME_BY_IDX(_idx) \
	DT_NODE_FULL_NAME(EDS_DT_EVENT_DOMAIN_BY_IDX(_idx))

#define EDS_DT_EVENT_DOMAIN_LEN \
	DT_CHILD_NUM(DT_PATH(event_domains))

struct eds_domain {
	const char *name;
	struct pm_event_domain_event *event;
	bool requested;
	uint32_t max_latency_us;
};

#define EDS_DOMAIN_SYM(_idx) \
	_CONCAT(eds_event_, _idx)

#define EDS_EVENT_DT_DEFINE(_node)								\
	PM_EVENT_DOMAIN_EVENT_DT_DEFINE(							\
		EDS_DOMAIN_SYM(DT_NODE_CHILD_IDX(_node)),					\
		_node										\
	)

#define EDS_DOMAIN_DT_INIT(_node)								\
	{											\
		.name = DT_NODE_FULL_NAME(_node),						\
		.event = &EDS_DOMAIN_SYM(DT_NODE_CHILD_IDX(_node)),				\
	}

DT_FOREACH_CHILD_SEP(DT_PATH(event_domains), EDS_EVENT_DT_DEFINE, (;));

static struct eds_domain domains[EDS_DT_EVENT_DOMAIN_LEN] = {
	DT_FOREACH_CHILD_SEP(DT_PATH(event_domains), EDS_DOMAIN_DT_INIT, (,))
};

static int get_domain_from_str(const struct shell *sh,
			       const char *domain_str,
			       struct eds_domain **domain)
{
	*domain = NULL;

	for (size_t i = 0; i < EDS_DT_EVENT_DOMAIN_LEN; i++) {
		if (strcmp(domain_str, domains[i].name)) {
			continue;
		}

		*domain = &domains[i];
		break;
	}

	if (*domain == NULL) {
		shell_error(sh, "%s not %s", domain_str, "found");
		return -ENODEV;
	}

	return 0;
}

static int get_max_latency_us_from_str(const struct shell *sh,
				       const char *max_latency_us_str,
				       uint32_t *max_latency_us)
{
	int ret;
	long res;

	*max_latency_us = 0;

	ret = 0;
	res = shell_strtoul(max_latency_us_str, 10, &ret);
	if (ret) {
		shell_error(sh, "%s not %s", max_latency_us_str, "valid");
		return ret;
	}

	if (res > UINT32_MAX) {
		shell_error(sh, "%s not %s", max_latency_us_str, "within range");
		return ret;
	}

	*max_latency_us = (uint32_t)res;
	return 0;
}

static int cmd_request_latency(const struct shell *sh, size_t argc, char **argv)
{
	int ret;
	const char *domain_str;
	struct eds_domain *domain;
	const char *max_latency_us_str;
	uint32_t max_latency_us;
	int64_t effective_uptime_ticks;
	struct pm_event_domain_event *event;

	ARG_UNUSED(argc);

	domain_str = argv[1];
	ret = get_domain_from_str(sh, domain_str, &domain);
	if (ret) {
		return ret;
	}

	max_latency_us_str = argv[2];
	ret = get_max_latency_us_from_str(sh, max_latency_us_str, &max_latency_us);
	if (ret) {
		return ret;
	}

	event = domain->event;
	if (domain->requested) {
		effective_uptime_ticks = pm_event_domain_rerequest_event(event,
									 max_latency_us);
	} else {
		effective_uptime_ticks = pm_event_domain_request_event(event,
								       max_latency_us);
	}

	k_sleep(K_TIMEOUT_ABS_TICKS(effective_uptime_ticks));
	domain->requested = true;
	domain->max_latency_us = max_latency_us;
	return 0;
}

static int cmd_release_latency(const struct shell *sh, size_t argc, char **argv)
{
	int ret;
	const char *domain_str;
	struct eds_domain *domain;
	struct pm_event_domain_event *event;

	ARG_UNUSED(argc);

	domain_str = argv[1];
	ret = get_domain_from_str(sh, domain_str, &domain);
	if (ret) {
		return ret;
	}

	if (!domain->requested) {
		shell_error(sh, "%s not %s", domain_str, "requested");
		return -EPERM;
	}

	event = domain->event;
	pm_event_domain_release_event(event);
	domain->requested = false;
	return 0;
}

static int cmd_status(const struct shell *sh, size_t argc, char **argv)
{
	struct eds_domain *domain;

	ARG_UNUSED(argc);

	for (size_t i = 0; i < EDS_DT_EVENT_DOMAIN_LEN; i++) {
		domain = &domains[i];

		if (domain->requested) {
			shell_print(sh,
				    "%s: %uus requested",
				    domain->name,
				    domain->max_latency_us);
		} else {
			shell_print(sh, "%s: %s", domain->name, "released");
		}
	}

	return 0;
}

static void dsub_domain_lookup_0(size_t idx, struct shell_static_entry *entry)
{
	struct eds_domain *domain = &domains[idx];

	entry->syntax = (idx < EDS_DT_EVENT_DOMAIN_LEN) ? domain->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_domain_0, dsub_domain_lookup_0);

#define CMD_REQUEST_HELP \
	("pm_event_domain request_latency <domain> <max_latency_us>")

#define CMD_RELEASE_HELP \
	("pm_event_domain release_latency <domain>")

#define CMD_STATUS_HELP \
	("pm_event_domain status")

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_pm_event_domain,
	SHELL_CMD_ARG(
		request_latency,
		&dsub_domain_0,
		CMD_REQUEST_HELP,
		cmd_request_latency,
		3,
		0
	),
	SHELL_CMD_ARG(
		release_latency,
		&dsub_domain_0,
		CMD_RELEASE_HELP,
		cmd_release_latency,
		2,
		0
	),
	SHELL_CMD_ARG(
		status,
		NULL,
		CMD_STATUS_HELP,
		cmd_status,
		1,
		0
	),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(
	pm_event_domain,
	&sub_pm_event_domain,
	"PM event domain commands",
	NULL
);
