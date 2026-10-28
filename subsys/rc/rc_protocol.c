/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/logging/log.h>
#include <zephyr/rc/rc.h>
#include <zephyr/rc/rc_protocol.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/iterable_sections.h>

#include "rc_internal.h"

LOG_MODULE_DECLARE(rc, CONFIG_RC_LOG_LEVEL);

static int rc_protocol_index(uint16_t proto)
{
	int idx = 0;

	STRUCT_SECTION_FOREACH(rc_protocol, p) {
		if ((proto >= p->proto_first) && (proto <= p->proto_last)) {
			return idx;
		}
		idx++;
	}

	return -ENOTSUP;
}

int rc_protocol_init(const struct device *dev, const uint16_t *protocols, size_t num_protocols)
{
	struct rc_dev_data *data = rc_data(dev);
	int count;

	STRUCT_SECTION_COUNT(rc_protocol, &count);
	if (count > RC_PROTOCOL_MAX) {
		LOG_ERR("%d protocols built, maximum %d", count, RC_PROTOCOL_MAX);
		return -ENOMEM;
	}

	if (protocols == NULL) {
		for (int i = 0; i < count; i++) {
			atomic_set_bit(data->protocols, i);
		}
		return 0;
	}

	for (size_t i = 0; i < num_protocols; i++) {
		int idx = rc_protocol_index(protocols[i]);

		if (idx < 0) {
			LOG_WRN("%s: protocol %u not built", dev->name, protocols[i]);
			continue;
		}
		atomic_set_bit(data->protocols, idx);
	}

	return 0;
}

int rc_protocol_enable(const struct device *dev, uint16_t proto)
{
	int idx = rc_protocol_index(proto);

	if (idx < 0) {
		return idx;
	}

	atomic_set_bit(rc_data(dev)->protocols, idx);

	return 0;
}

int rc_protocol_disable(const struct device *dev, uint16_t proto)
{
	int idx = rc_protocol_index(proto);

	if (idx < 0) {
		return idx;
	}

	atomic_clear_bit(rc_data(dev)->protocols, idx);

	return 0;
}

int rc_protocol_is_enabled(const struct device *dev, uint16_t proto)
{
	int idx = rc_protocol_index(proto);

	if (idx < 0) {
		return idx;
	}

	return atomic_test_bit(rc_data(dev)->protocols, idx) ? 1 : 0;
}
