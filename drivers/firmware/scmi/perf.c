/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/firmware/scmi/perf.h>
#include <string.h>
#include <zephyr/kernel.h>

DT_SCMI_PROTOCOL_DEFINE_NODEV(DT_INST(0, arm_scmi_perf), NULL,
		SCMI_PERF_PROTOCOL_SUPPORTED_VERSION);

/* Reply buffer capacity for DESCRIBE_LEVELS. */
#define SCMI_PERF_OPPS_PER_MSG	CONFIG_ARM_SCMI_PERF_OPPS_PER_MSG

enum scmi_perf_message {
	PERF_PROTOCOL_ATTRIBUTES = 0x1,
	PERF_DOMAIN_ATTRIBUTES   = 0x3,
	PERF_DESCRIBE_LEVELS     = 0x4,
	PERF_LIMITS_SET          = 0x5,
	PERF_LIMITS_GET          = 0x6,
	PERF_LEVEL_SET           = 0x7,
	PERF_LEVEL_GET           = 0x8,
};

struct scmi_perf_proto_attrs_reply {
	int32_t  status;
	uint32_t attributes;
	uint32_t stats_addr_low;
	uint32_t stats_addr_high;
	uint32_t stats_size;
} __packed;

struct scmi_perf_domain_attrs_reply {
	int32_t  status;
	uint32_t flags;
	uint32_t rate_limit;
	uint32_t sustained_freq_khz;
	uint32_t sustained_perf_level;
	uint8_t  name[SCMI_SHORT_NAME_MAX_SIZE];
} __packed;

struct scmi_perf_describe_levels_req {
	uint32_t domain_id;
	uint32_t level_index;
};

/* v4 OPP wire format — indicative_freq and level_index appended */
struct scmi_perf_opp_wire {
	uint32_t perf_val;
	uint32_t power;
	uint16_t transition_latency_us;
	uint16_t reserved;
	uint32_t indicative_freq;
	uint32_t level_index;
} __packed;

struct scmi_perf_describe_levels_reply {
	int32_t  status;
	uint16_t num_returned;
	uint16_t num_remaining;
	struct scmi_perf_opp_wire opp[SCMI_PERF_OPPS_PER_MSG];
} __packed;

struct scmi_perf_level_set_req {
	uint32_t domain_id;
	uint32_t performance_level;
};

struct scmi_perf_level_get_reply {
	int32_t  status;
	uint32_t performance_level;
};

struct scmi_perf_limits_get_reply {
	int32_t  status;
	uint32_t max_level;
	uint32_t min_level;
};

struct scmi_perf_domain_cache {
	struct scmi_perf_opp opps[SCMI_PERF_MAX_OPPS];
	uint32_t             count;
	bool                 level_indexing_mode;
	bool                 valid;
};

static struct scmi_perf_domain_cache domain_cache[32];

static int scmi_perf_cache_fill(uint32_t domain_id);

static struct scmi_perf_domain_cache *scmi_perf_cache_get(uint32_t domain_id)
{
	if (domain_id >= ARRAY_SIZE(domain_cache)) {
		return NULL;
	}

	if (!domain_cache[domain_id].valid) {
		if (scmi_perf_cache_fill(domain_id) < 0) {
			return NULL;
		}
	}

	return &domain_cache[domain_id];
}

static int scmi_perf_cache_fill(uint32_t domain_id)
{
	struct scmi_protocol *proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_PERF);
	struct scmi_perf_domain_attrs_reply attrs_reply;
	struct scmi_perf_describe_levels_req req;
	struct scmi_perf_describe_levels_reply lvl_reply;
	struct scmi_message msg, reply;
	struct scmi_perf_domain_cache *cache = &domain_cache[domain_id];
	uint32_t total = 0, index = 0;
	int ret;

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(PERF_DOMAIN_ATTRIBUTES, SCMI_COMMAND,
					proto->id, 0x0);
	msg.len = sizeof(domain_id);
	msg.content = &domain_id;
	reply.hdr = msg.hdr;
	reply.len = sizeof(attrs_reply);
	reply.content = &attrs_reply;

	ret = scmi_send_message(proto, &msg, &reply, false);
	if (ret < 0) {
		return ret;
	}

	if (attrs_reply.status != SCMI_SUCCESS) {
		return scmi_status_to_errno(attrs_reply.status);
	}

	/* BIT 25 = SUPPORTS_LEVEL_INDEXING (ARM SCMI spec v4, §13.4.3).
	 * When set, LEVEL_GET returns an opaque index and LEVEL_SET expects an index.
	 */
	cache->level_indexing_mode = !!(attrs_reply.flags & BIT(25));

	do {
		req.domain_id   = domain_id;
		req.level_index = index;

		msg.hdr = SCMI_MESSAGE_HDR_MAKE(PERF_DESCRIBE_LEVELS, SCMI_COMMAND,
						proto->id, 0x0);
		msg.len = sizeof(req);
		msg.content = &req;
		reply.hdr = msg.hdr;
		reply.len = sizeof(lvl_reply);
		reply.content = &lvl_reply;

		ret = scmi_send_message(proto, &msg, &reply, false);
		if (ret < 0) {
			return ret;
		}

		if (lvl_reply.status != SCMI_SUCCESS) {
			return scmi_status_to_errno(lvl_reply.status);
		}

		for (uint16_t i = 0; i < lvl_reply.num_returned &&
				      total < SCMI_PERF_MAX_OPPS; i++) {
			cache->opps[total].perf_val_khz =
				lvl_reply.opp[i].perf_val;
			cache->opps[total].power_mw =
				lvl_reply.opp[i].power;
			cache->opps[total].transition_latency_us =
				lvl_reply.opp[i].transition_latency_us;
			cache->opps[total].indicative_freq_khz =
				lvl_reply.opp[i].indicative_freq;
			total++;
		}

		index += lvl_reply.num_returned;

	} while (lvl_reply.num_remaining > 0 && total < SCMI_PERF_MAX_OPPS);

	cache->count = total;
	cache->valid = true;

	return 0;
}

/* Translate level index → kHz (index mode only) */
static int scmi_perf_idx_to_khz(const struct scmi_perf_domain_cache *cache,
				uint32_t idx, uint32_t *khz)
{
	if (idx >= cache->count) {
		return -ERANGE;
	}

	*khz = cache->opps[idx].perf_val_khz;
	return 0;
}

/* Translate kHz → level index (index mode only) */
static int scmi_perf_khz_to_idx(const struct scmi_perf_domain_cache *cache,
				uint32_t khz, uint32_t *idx)
{
	for (uint32_t i = 0; i < cache->count; i++) {
		if (cache->opps[i].perf_val_khz == khz) {
			*idx = i;
			return 0;
		}
	}

	return -ENOENT;
}

int scmi_perf_num_domains_get(uint32_t *num_domains)
{
	struct scmi_protocol *proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_PERF);
	struct scmi_perf_proto_attrs_reply reply_buffer;
	struct scmi_message msg, reply;
	int ret;

	if (!proto || !num_domains) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_PERF) {
		return -EINVAL;
	}

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(PERF_PROTOCOL_ATTRIBUTES, SCMI_COMMAND,
					proto->id, 0x0);
	msg.len = 0;
	msg.content = NULL;

	reply.hdr = msg.hdr;
	reply.len = sizeof(reply_buffer);
	reply.content = &reply_buffer;

	ret = scmi_send_message(proto, &msg, &reply, false);
	if (ret < 0) {
		return ret;
	}

	if (reply_buffer.status != SCMI_SUCCESS) {
		return scmi_status_to_errno(reply_buffer.status);
	}

	*num_domains = reply_buffer.attributes & GENMASK(15, 0);

	return 0;
}

int scmi_perf_domain_attributes_get(uint32_t domain_id,
				    struct scmi_perf_domain_attrs *attrs)
{
	struct scmi_protocol *proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_PERF);
	struct scmi_perf_domain_attrs_reply reply_buffer;
	struct scmi_message msg, reply;
	int ret;

	if (!proto || !attrs) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_PERF) {
		return -EINVAL;
	}

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(PERF_DOMAIN_ATTRIBUTES, SCMI_COMMAND,
					proto->id, 0x0);
	msg.len = sizeof(domain_id);
	msg.content = &domain_id;

	reply.hdr = msg.hdr;
	reply.len = sizeof(reply_buffer);
	reply.content = &reply_buffer;

	ret = scmi_send_message(proto, &msg, &reply, false);
	if (ret < 0) {
		return ret;
	}

	if (reply_buffer.status != SCMI_SUCCESS) {
		return scmi_status_to_errno(reply_buffer.status);
	}

	attrs->flags = reply_buffer.flags;
	attrs->rate_limit_us = reply_buffer.rate_limit & SCMI_PERF_RATE_LIMIT_MASK;
	attrs->sustained_freq_khz = reply_buffer.sustained_freq_khz;
	attrs->sustained_perf_level = reply_buffer.sustained_perf_level;
	memcpy(attrs->name, reply_buffer.name, SCMI_SHORT_NAME_MAX_SIZE);

	return 0;
}

int scmi_perf_describe_levels(uint32_t domain_id,
			      struct scmi_perf_opp *opps,
			      uint32_t max_opps,
			      uint32_t *num_opps)
{
	struct scmi_protocol *proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_PERF);
	struct scmi_perf_describe_levels_req req;
	struct scmi_perf_describe_levels_reply reply_buffer;
	struct scmi_message msg, reply;
	uint32_t total = 0, index = 0;
	int ret;

	if (!proto || !num_opps) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_PERF) {
		return -EINVAL;
	}

	do {
		req.domain_id   = domain_id;
		req.level_index = index;

		msg.hdr = SCMI_MESSAGE_HDR_MAKE(PERF_DESCRIBE_LEVELS, SCMI_COMMAND,
						proto->id, 0x0);
		msg.len = sizeof(req);
		msg.content = &req;

		reply.hdr = msg.hdr;
		reply.len = sizeof(reply_buffer);
		reply.content = &reply_buffer;

		ret = scmi_send_message(proto, &msg, &reply, false);
		if (ret < 0) {
			return ret;
		}

		if (reply_buffer.status != SCMI_SUCCESS) {
			return scmi_status_to_errno(reply_buffer.status);
		}

		for (uint16_t i = 0; i < reply_buffer.num_returned; i++) {
			if (opps && total < max_opps) {
				opps[total].perf_val_khz =
					reply_buffer.opp[i].perf_val;
				opps[total].power_mw =
					reply_buffer.opp[i].power;
				opps[total].transition_latency_us =
					reply_buffer.opp[i].transition_latency_us;
				opps[total].indicative_freq_khz =
					reply_buffer.opp[i].indicative_freq;
			}
			total++;
		}

		index += reply_buffer.num_returned;

	} while (reply_buffer.num_remaining > 0 &&
		 (!opps || total < max_opps));

	*num_opps = total;

	return 0;
}

int scmi_perf_level_set(uint32_t domain_id, uint32_t perf_level_khz)
{
	struct scmi_protocol *proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_PERF);
	struct scmi_perf_level_set_req req;
	struct scmi_message msg, reply;
	uint32_t wire_value;
	int status, ret;

	if (!proto) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_PERF) {
		return -EINVAL;
	}

	/*
	 * Translate kHz → wire value depending on the domain's mode:
	 *
	 *   Value mode (BIT 25 = 0): wire value = perf_val in kHz
	 *     → pass through directly, no OPP cache needed
	 *
	 *   Index mode (BIT 25 = 1): wire value = level index
	 *     → look up kHz in OPP cache to find the matching index
	 */
	struct scmi_perf_domain_cache *cache = scmi_perf_cache_get(domain_id);

	if (!cache) {
		return -ENODEV;
	}

	if (cache->level_indexing_mode) {
		ret = scmi_perf_khz_to_idx(cache, perf_level_khz, &wire_value);
		if (ret < 0) {
			return ret;
		}
	} else {
		wire_value = perf_level_khz;
	}

	req.domain_id        = domain_id;
	req.performance_level = wire_value;

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(PERF_LEVEL_SET, SCMI_COMMAND,
					proto->id, 0x0);
	msg.len = sizeof(req);
	msg.content = &req;

	reply.hdr = msg.hdr;
	reply.len = sizeof(status);
	reply.content = &status;

	ret = scmi_send_message(proto, &msg, &reply, false);
	if (ret < 0) {
		return ret;
	}

	return scmi_status_to_errno(status);
}

int scmi_perf_level_get(uint32_t domain_id, uint32_t *perf_level_khz)
{
	struct scmi_protocol *proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_PERF);
	struct scmi_perf_level_get_reply reply_buffer;
	struct scmi_message msg, reply;
	int ret;

	if (!proto || !perf_level_khz) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_PERF) {
		return -EINVAL;
	}

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(PERF_LEVEL_GET, SCMI_COMMAND,
					proto->id, 0x0);
	msg.len = sizeof(domain_id);
	msg.content = &domain_id;

	reply.hdr = msg.hdr;
	reply.len = sizeof(reply_buffer);
	reply.content = &reply_buffer;

	ret = scmi_send_message(proto, &msg, &reply, false);
	if (ret < 0) {
		return ret;
	}

	if (reply_buffer.status != SCMI_SUCCESS) {
		return scmi_status_to_errno(reply_buffer.status);
	}

	/*
	 * Translate wire value → kHz depending on the domain's mode:
	 *
	 *   Value mode (BIT 25 = 0): wire value is already kHz
	 *   Index mode (BIT 25 = 1): wire value is a level index
	 */
	struct scmi_perf_domain_cache *cache = scmi_perf_cache_get(domain_id);

	if (!cache) {
		return -ENODEV;
	}

	if (cache->level_indexing_mode) {
		return scmi_perf_idx_to_khz(cache,
					    reply_buffer.performance_level,
					    perf_level_khz);
	}

	*perf_level_khz = reply_buffer.performance_level;

	return 0;
}

int scmi_perf_limits_get(uint32_t domain_id, struct scmi_perf_limits *limits)
{
	struct scmi_protocol *proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_PERF);
	struct scmi_perf_limits_get_reply reply_buffer;
	struct scmi_message msg, reply;
	int ret;

	if (!proto || !limits) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_PERF) {
		return -EINVAL;
	}

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(PERF_LIMITS_GET, SCMI_COMMAND,
					proto->id, 0x0);
	msg.len = sizeof(domain_id);
	msg.content = &domain_id;

	reply.hdr = msg.hdr;
	reply.len = sizeof(reply_buffer);
	reply.content = &reply_buffer;

	ret = scmi_send_message(proto, &msg, &reply, false);
	if (ret < 0) {
		return ret;
	}

	if (reply_buffer.status != SCMI_SUCCESS) {
		return scmi_status_to_errno(reply_buffer.status);
	}

	limits->max_level = reply_buffer.max_level;
	limits->min_level = reply_buffer.min_level;

	return 0;
}
