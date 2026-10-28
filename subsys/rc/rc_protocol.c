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

#define RC_STATE_ALIGN 8U

static uint16_t state_offset[RC_PROTOCOL_MAX];

static int rc_protocol_layout(void)
{
	size_t offset = 0U;
	int idx = 0;

	STRUCT_SECTION_FOREACH(rc_protocol, p) {
		state_offset[idx] = (uint16_t)offset;
		offset += ROUND_UP(p->state_size, RC_STATE_ALIGN);
		idx++;
	}

	if (offset > CONFIG_RC_DECODER_STATE_SIZE) {
		LOG_ERR("decoder state %zu bytes, CONFIG_RC_DECODER_STATE_SIZE %d", offset,
			CONFIG_RC_DECODER_STATE_SIZE);
		return -ENOMEM;
	}

	return 0;
}

static void *rc_protocol_state(const struct device *dev, int idx)
{
	return &rc_data(dev)->decoder_state[state_offset[idx]];
}

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
	int ret;

	STRUCT_SECTION_COUNT(rc_protocol, &count);
	if (count > RC_PROTOCOL_MAX) {
		LOG_ERR("%d protocols built, maximum %d", count, RC_PROTOCOL_MAX);
		return -ENOMEM;
	}

	ret = rc_protocol_layout();
	if (ret != 0) {
		return ret;
	}

	rc_protocol_reset_all(dev);

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
	struct rc_dev_data *data = rc_data(dev);
	int idx = rc_protocol_index(proto);

	if (idx < 0) {
		return idx;
	}

	/* The rc receive work queue resets the state before the next word */
	if (!atomic_test_bit(data->protocols, idx)) {
		atomic_set_bit(data->reset_pending, idx);
		atomic_set_bit(data->protocols, idx);
	}

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

static void rc_protocol_reset(const struct rc_protocol *p, const struct device *dev, int idx)
{
	atomic_clear_bit(rc_data(dev)->reset_pending, idx);
	if (p->reset != NULL) {
		p->reset(rc_protocol_state(dev, idx));
	}
}

void rc_protocol_reset_all(const struct device *dev)
{
	int idx = 0;

	STRUCT_SECTION_FOREACH(rc_protocol, p) {
		rc_protocol_reset(p, dev, idx);
		idx++;
	}
}

void rc_protocol_decode(const struct device *dev, uint32_t word)
{
	struct rc_dev_data *data = rc_data(dev);
	int idx = 0;

	STRUCT_SECTION_FOREACH(rc_protocol, p) {
		struct rc_decode_result res;

		if ((p->decode != NULL) && atomic_test_bit(data->protocols, idx)) {
			if (atomic_test_bit(data->reset_pending, idx)) {
				rc_protocol_reset(p, dev, idx);
			}
			if (p->decode(rc_protocol_state(dev, idx), word, &res) == 1) {
				LOG_DBG("%s: %s proto %u scancode 0x%08x flags 0x%x", dev->name,
					p->name, res.proto, res.scancode, res.flags);
			}
		}
		idx++;
	}
}
