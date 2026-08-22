/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/precision_timing/precision_mapping.h>
#include <zephyr/precision_timing/precision_time.h>

/*
 * Both mapping inputs use the same nanosecond unit, so only a unit rate ratio
 * is needed. Using NSEC_PER_SEC for both rates would make timeutil_sync multiply
 * nanosecond deltas by NSEC_PER_SEC before dividing by the equal rate, which
 * overflows int64_t for deltas greater than about nine seconds.
 *
 * Keeping the configuration in static storage (rather than embedding it in
 * each mapping and self-referencing it from state.cfg) makes struct
 * precision_time_mapping trivially copyable.
 */
static const struct timeutil_sync_config precision_mapping_sync_cfg = {
	.ref_Hz = 1,
	.local_Hz = 1,
};

static bool precision_time_domain_valid(const struct precision_time_domain *domain)
{
	return domain != NULL && domain->type != PRECISION_TIME_DOMAIN_INVALID;
}

static int precision_time_to_timeutil(precision_time_t time, precision_time_t bias, uint64_t *out)
{
	precision_time_t biased;
	int ret;

	ret = precision_time_add(time, bias, &biased);
	if (ret < 0 || biased < 0) {
		return -ERANGE;
	}

	*out = (uint64_t)biased;

	return 0;
}

static int precision_time_bias_for(precision_time_t time, precision_time_t *bias)
{
	if (bias == NULL) {
		return -EINVAL;
	}

	if (time > 0) {
		*bias = 0;
		return 0;
	}

	return precision_time_sub(1, time, bias);
}

static int precision_time_mapping_check_local_range(const struct precision_time_mapping *mapping,
						    uint64_t source_biased)
{
	precision_time_t source_delta;
	precision_time_t local_delta;
	precision_time_t local_biased;
	int ret;

	if (mapping->state.base.ref == 0 || !(mapping->state.skew > 0.0f)) {
		return -EINVAL;
	}

	if (source_biased > (uint64_t)PRECISION_TIME_MAX ||
	    mapping->state.base.ref > (uint64_t)PRECISION_TIME_MAX ||
	    mapping->state.base.local > (uint64_t)PRECISION_TIME_MAX) {
		return -ERANGE;
	}

	ret = precision_time_sub((precision_time_t)source_biased,
				 (precision_time_t)mapping->state.base.ref, &source_delta);
	if (ret < 0) {
		return ret;
	}

	/* The mapping uses a unit rate ratio, so this mirrors the affine delta
	 * calculated by timeutil_sync_local_from_ref(). Check it before that
	 * helper performs its unchecked signed addition to the local base.
	 */
	local_delta = source_delta;
#ifdef CONFIG_TIMEUTIL_APPLY_SKEW
	if (mapping->state.skew != 1.0f) {
		double adjusted_delta = (double)local_delta / (double)mapping->state.skew;

		if (adjusted_delta < (double)PRECISION_TIME_MIN ||
		    adjusted_delta >= -(double)PRECISION_TIME_MIN) {
			return -ERANGE;
		}

		local_delta = (precision_time_t)adjusted_delta;
	}
#endif /* CONFIG_TIMEUTIL_APPLY_SKEW */

	return precision_time_add((precision_time_t)mapping->state.base.local, local_delta,
				  &local_biased);
}

static int precision_time_mapping_check_source_range(const struct precision_time_mapping *mapping,
						     uint64_t local_biased)
{
	precision_time_t local_delta;
	precision_time_t source_delta;
	precision_time_t source_biased;
	int ret;

	if (mapping->state.base.ref == 0 || !(mapping->state.skew > 0.0f)) {
		return -EINVAL;
	}

	if (local_biased > (uint64_t)PRECISION_TIME_MAX ||
	    mapping->state.base.ref > (uint64_t)PRECISION_TIME_MAX ||
	    mapping->state.base.local > (uint64_t)PRECISION_TIME_MAX) {
		return -ERANGE;
	}

	ret = precision_time_sub((precision_time_t)local_biased,
				 (precision_time_t)mapping->state.base.local, &local_delta);
	if (ret < 0) {
		return ret;
	}

	/* The mapping uses a unit rate ratio, so this mirrors the affine delta
	 * calculated by timeutil_sync_ref_from_local(). Check it before that
	 * helper performs its unchecked signed addition to the source base.
	 */
	source_delta = local_delta;
#ifdef CONFIG_TIMEUTIL_APPLY_SKEW
	if (mapping->state.skew != 1.0f) {
		double adjusted_delta = (double)source_delta * (double)mapping->state.skew;

		if (adjusted_delta < (double)PRECISION_TIME_MIN ||
		    adjusted_delta >= -(double)PRECISION_TIME_MIN) {
			return -ERANGE;
		}

		source_delta = (precision_time_t)adjusted_delta;
	}
#endif /* CONFIG_TIMEUTIL_APPLY_SKEW */

	return precision_time_add((precision_time_t)mapping->state.base.ref, source_delta,
				  &source_biased);
}

void precision_time_mapping_init(struct precision_time_mapping *mapping,
				 struct precision_time_domain source_domain,
				 struct precision_time_domain local_domain)
{
	if (mapping == NULL) {
		return;
	}

	memset(mapping, 0, sizeof(*mapping));
	mapping->source_domain = source_domain;
	mapping->local_domain = local_domain;
	mapping->state.cfg = &precision_mapping_sync_cfg;
}

void precision_time_mapping_invalidate(struct precision_time_mapping *mapping)
{
	struct precision_time_domain source_domain;
	struct precision_time_domain local_domain;

	if (mapping == NULL) {
		return;
	}

	source_domain = mapping->source_domain;
	local_domain = mapping->local_domain;
	precision_time_mapping_init(mapping, source_domain, local_domain);
}

int precision_time_mapping_update(struct precision_time_mapping *mapping,
				  const struct precision_time_observation *observation)
{
	struct timeutil_sync_instant instant;
	int ret;

	if (mapping == NULL || observation == NULL ||
	    (observation->flags &
	     (PRECISION_OBSERVATION_SOURCE_VALID | PRECISION_OBSERVATION_LOCAL_VALID)) !=
		    (PRECISION_OBSERVATION_SOURCE_VALID | PRECISION_OBSERVATION_LOCAL_VALID)) {
		return -EINVAL;
	}

	if (!precision_time_domain_valid(&mapping->source_domain) ||
	    !precision_time_domain_valid(&mapping->local_domain) ||
	    !precision_time_domain_valid(&observation->source.domain) ||
	    !precision_time_domain_valid(&observation->local.domain) ||
	    !precision_time_domain_equal(&mapping->source_domain, &observation->source.domain) ||
	    !precision_time_domain_equal(&mapping->local_domain, &observation->local.domain)) {
		return -EINVAL;
	}

	if (!mapping->valid) {
		ret = precision_time_bias_for(observation->source.time, &mapping->source_bias);
		if (ret < 0) {
			return ret;
		}

		ret = precision_time_bias_for(observation->local.time, &mapping->local_bias);
		if (ret < 0) {
			return ret;
		}
	}

	ret = precision_time_to_timeutil(observation->source.time, mapping->source_bias,
					 &instant.ref);
	if (ret < 0) {
		return ret;
	}

	ret = precision_time_to_timeutil(observation->local.time, mapping->local_bias,
					 &instant.local);
	if (ret < 0) {
		return ret;
	}

	ret = timeutil_sync_state_update(&mapping->state, &instant);
	if (ret < 0) {
		return ret;
	}

	mapping->valid = true;

	if (ret > 0) {
		float skew = timeutil_sync_estimate_skew(&mapping->state);

		if (skew > 0.0f) {
			(void)timeutil_sync_state_set_skew(&mapping->state, skew, NULL);
		}
	}

	return 0;
}

int precision_time_mapping_source_to_local(const struct precision_time_mapping *mapping,
					   const struct precision_time_point *source,
					   struct precision_time_point *local)
{
	uint64_t source_biased;
	int64_t local_biased;
	precision_time_t local_time;
	int ret;

	if (mapping == NULL || source == NULL || local == NULL) {
		return -EINVAL;
	}

	if (!precision_time_domain_valid(&mapping->source_domain) ||
	    !precision_time_domain_valid(&mapping->local_domain) ||
	    !precision_time_domain_valid(&source->domain)) {
		return -EINVAL;
	}

	if (!mapping->valid) {
		return -EAGAIN;
	}

	if (!precision_time_domain_equal(&mapping->source_domain, &source->domain)) {
		return -EINVAL;
	}

	ret = precision_time_to_timeutil(source->time, mapping->source_bias, &source_biased);
	if (ret < 0) {
		return ret;
	}

	ret = precision_time_mapping_check_local_range(mapping, source_biased);
	if (ret < 0) {
		return ret;
	}

	ret = timeutil_sync_local_from_ref(&mapping->state, source_biased, &local_biased);
	if (ret < 0) {
		return ret;
	}

	ret = precision_time_sub((precision_time_t)local_biased, mapping->local_bias, &local_time);
	if (ret < 0) {
		return ret;
	}

	local->time = local_time;
	local->domain = mapping->local_domain;

	return 0;
}

int precision_time_mapping_local_to_source(const struct precision_time_mapping *mapping,
					   const struct precision_time_point *local,
					   struct precision_time_point *source)
{
	uint64_t local_biased;
	uint64_t source_biased;
	precision_time_t source_time;
	int ret;

	if (mapping == NULL || source == NULL || local == NULL) {
		return -EINVAL;
	}

	if (!precision_time_domain_valid(&mapping->source_domain) ||
	    !precision_time_domain_valid(&mapping->local_domain) ||
	    !precision_time_domain_valid(&local->domain)) {
		return -EINVAL;
	}

	if (!mapping->valid) {
		return -EAGAIN;
	}

	if (!precision_time_domain_equal(&mapping->local_domain, &local->domain)) {
		return -EINVAL;
	}

	ret = precision_time_to_timeutil(local->time, mapping->local_bias, &local_biased);
	if (ret < 0) {
		return ret;
	}

	ret = precision_time_mapping_check_source_range(mapping, local_biased);
	if (ret < 0) {
		return ret;
	}

	ret = timeutil_sync_ref_from_local(&mapping->state, local_biased, &source_biased);
	if (ret < 0) {
		return ret;
	}

	if (source_biased > (uint64_t)PRECISION_TIME_MAX) {
		return -ERANGE;
	}

	ret = precision_time_sub((precision_time_t)source_biased, mapping->source_bias,
				 &source_time);
	if (ret < 0) {
		return ret;
	}

	source->time = source_time;
	source->domain = mapping->source_domain;

	return 0;
}
