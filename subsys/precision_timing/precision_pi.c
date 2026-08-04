/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/precision_timing/precision_pi.h>
#include <zephyr/precision_timing/precision_time.h>

static precision_time_t precision_abs_sat(precision_time_t value)
{
	if (value == PRECISION_TIME_MIN) {
		return PRECISION_TIME_MAX;
	}

	return value < 0 ? -value : value;
}

static bool precision_time_domain_valid(const struct precision_time_domain *domain)
{
	return domain != NULL && domain->type != PRECISION_TIME_DOMAIN_INVALID;
}

static int64_t precision_add_sat(int64_t a, int64_t b)
{
	if (b > 0 && a > INT64_MAX - b) {
		return INT64_MAX;
	}

	if (b < 0 && a < INT64_MIN - b) {
		return INT64_MIN;
	}

	return a + b;
}

static int64_t precision_mul_div_sat(int64_t value, int32_t multiplier, uint32_t divisor)
{
	int64_t quotient;
	int64_t remainder;
	int64_t scaled_quotient;
	int64_t scaled_remainder;

	if (divisor == 0) {
		return value < 0 ? INT64_MIN : INT64_MAX;
	}

	quotient = value / (int64_t)divisor;
	remainder = value % (int64_t)divisor;

	if (multiplier == -1 && quotient == INT64_MIN) {
		scaled_quotient = INT64_MAX;
	} else if (multiplier > 0 && quotient > INT64_MAX / multiplier) {
		scaled_quotient = INT64_MAX;
	} else if (multiplier > 0 && quotient < INT64_MIN / multiplier) {
		scaled_quotient = INT64_MIN;
	} else if (multiplier < -1 && quotient > INT64_MIN / multiplier) {
		scaled_quotient = INT64_MIN;
	} else if (multiplier < -1 && quotient < INT64_MAX / multiplier) {
		scaled_quotient = INT64_MAX;
	} else {
		scaled_quotient = quotient * (int64_t)multiplier;
	}

	if (scaled_quotient == INT64_MAX || scaled_quotient == INT64_MIN) {
		/* Quotient and remainder have the same sign, so the remainder term
		 * can only move an overflowing result farther toward saturation.
		 */
		return scaled_quotient;
	}

	/* This multiplication cannot overflow: |remainder| is less than a
	 * uint32_t divisor and |multiplier| is at most 2^31.
	 */
	scaled_remainder = (remainder * (int64_t)multiplier) / (int64_t)divisor;

	return precision_add_sat(scaled_quotient, scaled_remainder);
}

static int32_t precision_clamp_ppb(int64_t value, const struct precision_pi_config *config)
{
	int64_t min_rate = config->min_rate_ppb;
	int64_t max_rate = config->max_rate_ppb;

	if (min_rate > max_rate) {
		min_rate = INT32_MIN;
		max_rate = INT32_MAX;
	}

	return (int32_t)CLAMP(value, min_rate, max_rate);
}

static void precision_pi_result_set(const struct precision_pi_discipline *discipline,
				    struct precision_discipline_result *result,
				    enum precision_discipline_action action,
				    precision_time_t offset_ns)
{
	if (result == NULL) {
		return;
	}

	result->action = action;
	result->state = discipline->state;
	result->offset_ns = offset_ns;
	result->phase_correction_ns = action == PRECISION_DISCIPLINE_STEP ? offset_ns : 0;
	result->rate_ppb = discipline->frequency_correction_ppb;
	result->rejected_observations = discipline->rejected_observations;
}

int precision_pi_init(struct precision_pi_discipline *discipline,
		      const struct precision_pi_config *config)
{
	if ((discipline == NULL) || (config == NULL) || (config->gain_den == 0) ||
	    (config->step_threshold_ns < 0) || (config->lock_threshold_ns < 0) ||
	    (config->outlier_threshold_ns < 0) || (config->max_uncertainty_ns < 0) ||
	    (config->source_timeout_ns < 0) || (config->holdover_ns < 0) ||
	    (config->min_rate_ppb > config->max_rate_ppb) ||
	    !precision_time_domain_valid(&config->source_domain) ||
	    !precision_time_domain_valid(&config->local_domain)) {
		return -EINVAL;
	}

	/*
	 * Outlier rejection needs a non-zero sample count; otherwise the first
	 * outlier seen while locked would immediately reset the servo.
	 */
	if ((config->outlier_threshold_ns > 0) && (config->outlier_sample_count == 0U)) {
		return -EINVAL;
	}

	memset(discipline, 0, sizeof(*discipline));
	discipline->config = *config;
	discipline->state = PRECISION_SYNC_UNSYNCED;

	return 0;
}

void precision_pi_reset(struct precision_pi_discipline *discipline)
{
	struct precision_pi_config config;

	if (discipline == NULL) {
		return;
	}

	config = discipline->config;
	memset(discipline, 0, sizeof(*discipline));
	discipline->config = config;
	discipline->state = PRECISION_SYNC_UNSYNCED;
}

void precision_pi_fault(struct precision_pi_discipline *discipline)
{
	if (discipline == NULL) {
		return;
	}

	discipline->state = PRECISION_SYNC_FAULT;
	discipline->drift_ppb = 0;
	discipline->frequency_correction_ppb = 0;
	discipline->lock_samples = 0U;
	discipline->outlier_samples = 0U;
}

int precision_pi_process(struct precision_pi_discipline *discipline,
			 const struct precision_time_observation *observation,
			 struct precision_discipline_result *result)
{
	precision_time_t offset_ns;
	precision_time_t abs_offset_ns;
	int64_t proportional_ppb;
	int64_t integral_ppb;
	int64_t next_drift_ppb;
	int64_t correction_ppb;
	int32_t clamped_correction_ppb;
	int ret;

	if (discipline == NULL || observation == NULL ||
	    (observation->flags &
	     (PRECISION_OBSERVATION_SOURCE_VALID | PRECISION_OBSERVATION_LOCAL_VALID)) !=
		    (PRECISION_OBSERVATION_SOURCE_VALID | PRECISION_OBSERVATION_LOCAL_VALID)) {
		return -EINVAL;
	}

	if (discipline->state == PRECISION_SYNC_FAULT) {
		precision_pi_result_set(discipline, result, PRECISION_DISCIPLINE_IGNORE,
					discipline->last_offset_ns);
		return -EIO;
	}

	if (!precision_time_domain_valid(&discipline->config.source_domain) ||
	    !precision_time_domain_valid(&discipline->config.local_domain) ||
	    !precision_time_domain_valid(&observation->source.domain) ||
	    !precision_time_domain_valid(&observation->local.domain) ||
	    !precision_time_domain_equal(&discipline->config.source_domain,
					 &observation->source.domain) ||
	    !precision_time_domain_equal(&discipline->config.local_domain,
					 &observation->local.domain)) {
		discipline->rejected_observations++;
		precision_pi_result_set(discipline, result, PRECISION_DISCIPLINE_IGNORE, 0);
		return -EINVAL;
	}

	if (discipline->config.max_uncertainty_ns > 0 &&
	    observation->uncertainty_ns > discipline->config.max_uncertainty_ns) {
		discipline->rejected_observations++;
		precision_pi_result_set(discipline, result, PRECISION_DISCIPLINE_IGNORE, 0);
		return -ESTALE;
	}

	if (discipline->has_last_update && observation->local.time <= discipline->last_update_ns) {
		discipline->rejected_observations++;
		precision_pi_result_set(discipline, result, PRECISION_DISCIPLINE_IGNORE, 0);
		return -EINVAL;
	}

	ret = precision_time_sub(observation->source.time, observation->local.time, &offset_ns);
	if (ret < 0) {
		discipline->rejected_observations++;
		precision_pi_result_set(discipline, result, PRECISION_DISCIPLINE_IGNORE, 0);
		return ret;
	}

	abs_offset_ns = precision_abs_sat(offset_ns);

	if (discipline->config.step_threshold_ns > 0 &&
	    abs_offset_ns > discipline->config.step_threshold_ns) {
		precision_pi_reset(discipline);
		precision_pi_result_set(discipline, result, PRECISION_DISCIPLINE_STEP, offset_ns);
		return 0;
	}

	if (discipline->state == PRECISION_SYNC_LOCKED &&
	    discipline->config.outlier_threshold_ns > 0 &&
	    abs_offset_ns > discipline->config.outlier_threshold_ns) {
		discipline->outlier_samples++;
		discipline->rejected_observations++;

		if (discipline->outlier_samples >= discipline->config.outlier_sample_count) {
			precision_pi_reset(discipline);
			precision_pi_result_set(discipline, result, PRECISION_DISCIPLINE_RESET,
						offset_ns);
		} else {
			precision_pi_result_set(discipline, result, PRECISION_DISCIPLINE_IGNORE,
						offset_ns);
		}

		return 0;
	}

	discipline->outlier_samples = 0U;
	integral_ppb = precision_mul_div_sat(offset_ns, discipline->config.ki_num,
					     discipline->config.gain_den);
	proportional_ppb = precision_mul_div_sat(offset_ns, discipline->config.kp_num,
						 discipline->config.gain_den);
	next_drift_ppb = precision_add_sat(discipline->drift_ppb, integral_ppb);
	correction_ppb = precision_add_sat(proportional_ppb, next_drift_ppb);
	clamped_correction_ppb = precision_clamp_ppb(correction_ppb, &discipline->config);

	/* Do not integrate an error that would drive a saturated output farther out of range. */
	if (!((correction_ppb > clamped_correction_ppb && integral_ppb > 0) ||
	      (correction_ppb < clamped_correction_ppb && integral_ppb < 0))) {
		discipline->drift_ppb = next_drift_ppb;
	}

	discipline->frequency_correction_ppb = clamped_correction_ppb;
	discipline->last_offset_ns = offset_ns;
	discipline->last_update_ns = observation->local.time;
	discipline->has_last_update = true;

	if (discipline->config.lock_sample_count == 0U) {
		discipline->state = PRECISION_SYNC_LOCKED;
	} else if (abs_offset_ns > discipline->config.lock_threshold_ns) {
		discipline->lock_samples = 0U;
		discipline->state = PRECISION_SYNC_ACQUIRING;
	} else {
		if (discipline->lock_samples < discipline->config.lock_sample_count) {
			discipline->lock_samples++;
		}

		discipline->state = discipline->lock_samples >= discipline->config.lock_sample_count
					    ? PRECISION_SYNC_LOCKED
					    : PRECISION_SYNC_ACQUIRING;
	}

	precision_pi_result_set(discipline, result, PRECISION_DISCIPLINE_ADJUST_RATE, offset_ns);

	return 0;
}

int precision_pi_check_source_timeout(struct precision_pi_discipline *discipline,
				      precision_time_t now_local_ns,
				      struct precision_discipline_result *result)
{
	enum precision_discipline_action action = PRECISION_DISCIPLINE_IGNORE;
	precision_time_t last_offset_ns;
	precision_time_t age_ns;
	int ret;

	if (discipline == NULL) {
		return -EINVAL;
	}

	if (discipline->state == PRECISION_SYNC_FAULT) {
		precision_pi_result_set(discipline, result, PRECISION_DISCIPLINE_IGNORE,
					discipline->last_offset_ns);
		return -EIO;
	}

	if (!discipline->has_last_update || discipline->config.source_timeout_ns <= 0) {
		precision_pi_result_set(discipline, result, PRECISION_DISCIPLINE_IGNORE,
					discipline->last_offset_ns);
		return -EAGAIN;
	}

	ret = precision_time_sub(now_local_ns, discipline->last_update_ns, &age_ns);
	if (ret < 0) {
		precision_pi_result_set(discipline, result, PRECISION_DISCIPLINE_IGNORE,
					discipline->last_offset_ns);
		return ret;
	}

	if (age_ns <= discipline->config.source_timeout_ns) {
		precision_pi_result_set(discipline, result, PRECISION_DISCIPLINE_IGNORE,
					discipline->last_offset_ns);
		return 0;
	}

	last_offset_ns = discipline->last_offset_ns;
	discipline->state = PRECISION_SYNC_HOLDOVER;

	if (discipline->config.holdover_ns > 0 &&
	    age_ns > precision_add_sat(discipline->config.source_timeout_ns,
				       discipline->config.holdover_ns)) {
		precision_pi_reset(discipline);
		action = PRECISION_DISCIPLINE_RESET;
	}

	precision_pi_result_set(discipline, result, action, last_offset_ns);

	return -ESTALE;
}

int precision_pi_get_config(const struct precision_pi_discipline *discipline,
			    struct precision_pi_config *config)
{
	if (discipline == NULL || config == NULL) {
		return -EINVAL;
	}

	*config = discipline->config;

	return 0;
}

int precision_pi_get_status(const struct precision_pi_discipline *discipline,
			    struct precision_pi_status *status)
{
	if (discipline == NULL || status == NULL) {
		return -EINVAL;
	}

	*status = (struct precision_pi_status){
		.state = discipline->state,
		.last_offset_ns = discipline->last_offset_ns,
		.last_update_ns = discipline->last_update_ns,
		.frequency_correction_ppb = discipline->frequency_correction_ppb,
		.rejected_observations = discipline->rejected_observations,
		.lock_samples = discipline->lock_samples,
		.outlier_samples = discipline->outlier_samples,
		.has_observation = discipline->has_last_update,
	};

	return 0;
}

int precision_pi_set_rate_limits(struct precision_pi_discipline *discipline, int32_t min_rate_ppb,
				 int32_t max_rate_ppb)
{
	if (discipline == NULL || min_rate_ppb > max_rate_ppb) {
		return -EINVAL;
	}

	discipline->config.min_rate_ppb = min_rate_ppb;
	discipline->config.max_rate_ppb = max_rate_ppb;

	return 0;
}

int precision_pi_set_source_timeout(struct precision_pi_discipline *discipline,
				    precision_time_t source_timeout_ns,
				    precision_time_t holdover_ns)
{
	if (discipline == NULL || source_timeout_ns < 0 || holdover_ns < 0) {
		return -EINVAL;
	}

	discipline->config.source_timeout_ns = source_timeout_ns;
	discipline->config.holdover_ns = holdover_ns;

	return 0;
}

int precision_pi_set_local_domain(struct precision_pi_discipline *discipline,
				  struct precision_time_domain local_domain)
{
	if (discipline == NULL || !precision_time_domain_valid(&local_domain)) {
		return -EINVAL;
	}

	if (precision_time_domain_equal(&discipline->config.local_domain, &local_domain)) {
		return 0;
	}

	precision_pi_reset(discipline);
	discipline->config.local_domain = local_domain;

	return 0;
}

int precision_pi_time_to_expiry(const struct precision_pi_discipline *discipline,
				precision_time_t now_local_ns, precision_time_t *remaining_ns)
{
	precision_time_t expiry_ns;
	precision_time_t age_ns;
	int ret;

	if (discipline == NULL || remaining_ns == NULL) {
		return -EINVAL;
	}

	if (!discipline->has_last_update || discipline->config.source_timeout_ns <= 0) {
		return -EAGAIN;
	}

	ret = precision_time_sub(now_local_ns, discipline->last_update_ns, &age_ns);
	if (ret < 0) {
		return ret;
	}

	if (age_ns < 0) {
		/* The local clock was stepped backwards. Restart the full interval. */
		*remaining_ns = discipline->config.source_timeout_ns;
		return 0;
	}

	expiry_ns = discipline->config.source_timeout_ns;

	if (age_ns > expiry_ns) {
		if (discipline->config.holdover_ns <= 0) {
			/* Holdover never expires, so there is nothing left to poll. */
			return -EAGAIN;
		}

		expiry_ns = precision_add_sat(expiry_ns, discipline->config.holdover_ns);

		if (age_ns > expiry_ns) {
			return -EAGAIN;
		}
	}

	/* check_source_timeout() acts strictly above the threshold, so aim just
	 * past it to avoid a poll that finds the deadline not yet reached.
	 */
	*remaining_ns = expiry_ns - age_ns;
	if (*remaining_ns < PRECISION_TIME_MAX) {
		(*remaining_ns)++;
	}

	return 0;
}
