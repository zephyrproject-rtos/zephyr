/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr_pwm_pool.h"
#include <stdlib.h>
#include <stdarg.h>

/* Auxiliary data to support blink & breath */
struct pwm_pool_aux_data {
	/* common */
	enum pwm_state  state;
	k_ticks_t       t_event;
	bool            current_state;
	union {
		struct blink {
			k_timeout_t     t_on;
			k_timeout_t     t_off;
		} blink;
		struct breath {
			uint32_t        period;
			uint32_t        current_pulse;
			uint32_t        step_pulse;
		} breath;
	} mode;
};

/* Get timeout to next event in auxiliary data */
static k_timeout_t pwm_pool_aux_timeout(const struct pwm_pool_data *pwm_pool)
{
	k_timeout_t timeout = {
		.ticks = K_TICKS_FOREVER
	};
	const struct pwm_pool_aux_data *pwm_pool_aux =
		(const struct pwm_pool_aux_data *)pwm_pool->aux;
	size_t pwm_pool_aux_len = pwm_pool->out_len;

	for (size_t i = 0; i < pwm_pool_aux_len; i++) {
		if (pwm_pool_aux[i].state == PWM_BLINK) {
			k_ticks_t t_next = pwm_pool_aux[i].t_event +
				(pwm_pool_aux[i].current_state ?
				pwm_pool_aux[i].mode.blink.t_on.ticks :
				pwm_pool_aux[i].mode.blink.t_off.ticks);
			k_ticks_t t_now = sys_clock_tick_get();
			k_timeout_t this = {
				.ticks = t_next > t_now ? t_next - t_now : 0
			};
			if (timeout.ticks == K_TICKS_FOREVER || this.ticks < timeout.ticks) {
				timeout.ticks = this.ticks;
			}
		} else if (pwm_pool_aux[i].state == PWM_BREATH) {
			k_ticks_t t_next = pwm_pool_aux[i].t_event +
				K_NSEC(pwm_pool_aux[i].mode.breath.period).ticks;
						k_ticks_t t_now = sys_clock_tick_get();
			k_timeout_t this = {
				.ticks = t_next > t_now ? t_next - t_now : 0
			};
			if (timeout.ticks == K_TICKS_FOREVER || this.ticks < timeout.ticks) {
				timeout.ticks = this.ticks;
			}
		}
	}
	return timeout;
}

/* Process events in auxiliary data */
static void pwm_pool_aux_update(const struct pwm_pool_data *pwm_pool)
{
	struct pwm_pool_aux_data *pwm_pool_aux =
		(struct pwm_pool_aux_data *)pwm_pool->aux;
	size_t pwm_pool_aux_len = pwm_pool->out_len;

	for (size_t i = 0; i < pwm_pool_aux_len; i++) {
		if (pwm_pool_aux[i].state == PWM_BLINK) {
			k_ticks_t t_next = pwm_pool_aux[i].t_event +
				(pwm_pool_aux[i].current_state ?
				pwm_pool_aux[i].mode.blink.t_on.ticks :
				pwm_pool_aux[i].mode.blink.t_off.ticks);
			k_ticks_t t_now = sys_clock_tick_get();

			if (t_next <= t_now) {
				pwm_pool_aux[i].t_event = t_now;
				pwm_pool_aux[i].current_state = !pwm_pool_aux[i].current_state;
				(void) pwm_set_dt(&pwm_pool->out[i], pwm_pool->out[i].period,
					pwm_pool_aux[i].current_state ?
					pwm_pool->out[i].period : 0);
			}
		} else if (pwm_pool_aux[i].state == PWM_BREATH) {
			k_ticks_t t_next = pwm_pool_aux[i].t_event +
				K_NSEC(pwm_pool_aux[i].mode.breath.period).ticks;
			k_ticks_t t_now = sys_clock_tick_get();

			if (t_next <= t_now) {
				pwm_pool_aux[i].t_event = t_now;
				if (pwm_pool_aux[i].current_state) {
					if (pwm_pool_aux[i].mode.breath.current_pulse <=
						pwm_pool_aux[i].mode.breath.period -
						pwm_pool_aux[i].mode.breath.step_pulse) {
						pwm_pool_aux[i].mode.breath.current_pulse +=
							pwm_pool_aux[i].mode.breath.step_pulse;
					} else {
						pwm_pool_aux[i].mode.breath.current_pulse =
							pwm_pool_aux[i].mode.breath.period;
						pwm_pool_aux[i].current_state = false;
					}
				} else {
					if (pwm_pool_aux[i].mode.breath.current_pulse >=
						pwm_pool_aux[i].mode.breath.step_pulse) {
						pwm_pool_aux[i].mode.breath.current_pulse -=
							pwm_pool_aux[i].mode.breath.step_pulse;
					} else {
						pwm_pool_aux[i].mode.breath.current_pulse = 0;
						pwm_pool_aux[i].current_state = true;
					}
				}
				(void) pwm_set_dt(&pwm_pool->out[i], pwm_pool->out[i].period,
					pwm_pool_aux[i].mode.breath.current_pulse);
			}
		}
	}
}

/* Pwm pool worker */
static void pwm_pool_event_work(struct k_work *item)
{
	struct pwm_pool_data *pwm_pool =
		CONTAINER_OF(k_work_delayable_from_work(item), struct pwm_pool_data, work);

	pwm_pool_aux_update(pwm_pool);
	(void) k_work_reschedule(&pwm_pool->work, pwm_pool_aux_timeout(pwm_pool));
}

/* Public APIs */

bool pwm_pool_init(struct pwm_pool_data *pwm_pool)
{
	bool result = true;

	do {
		if (!pwm_pool->out_len) {
			result = false;
			break;
		}
		/* check if all PWMs are ready */
		for (size_t i = 0; i < pwm_pool->out_len; i++) {
			if (!device_is_ready(pwm_pool->out[i].dev)) {
				result = false;
				break;
			}
		}
		if (!result) {
			break;
		}
		/* init all PWMs are ready */
		for (size_t i = 0; i < pwm_pool->out_len; i++) {
			if (pwm_set_dt(&pwm_pool->out[i], pwm_pool->out[i].period, 0)) {
				result = false;
				break;
			}
		}
		if (!result) {
			break;
		}
		/* set auxiliary blink/breath structure */
		struct pwm_pool_aux_data *pwm_pool_aux = (struct pwm_pool_aux_data *)
			malloc(sizeof(struct pwm_pool_aux_data) * pwm_pool->out_len);

		if (!pwm_pool_aux) {
			result = false;
			break;
		}

		for (size_t i = 0; i < pwm_pool->out_len; i++) {
			pwm_pool_aux[i].state = PWM_OFF;
		}
		pwm_pool->aux = pwm_pool_aux;
		/* init work */
		k_work_init_delayable(&pwm_pool->work, pwm_pool_event_work);
		if (k_work_reschedule(&pwm_pool->work, pwm_pool_aux_timeout(pwm_pool)) < 0) {
			result = false;
			break;
		}
		/* all done */
	} while (0);

	return result;
}

bool pwm_pool_set(struct pwm_pool_data *pwm_pool,
	size_t pwm, enum pwm_state state, ...)
{
	bool result = false;

	if (pwm < pwm_pool->out_len) {
		if (state == PWM_ON || state == PWM_OFF) {
			if (!pwm_set_dt(&pwm_pool->out[pwm], pwm_pool->out[pwm].period,
				state ? pwm_pool->out[pwm].period : 0)) {
				struct pwm_pool_aux_data *pwm_pool_aux = pwm_pool->aux;

				pwm_pool_aux[pwm].state = state;
				if (k_work_reschedule(&pwm_pool->work,
					pwm_pool_aux_timeout(pwm_pool)) >= 0) {
					result = true;
				}
			}
		} else if (state == PWM_FIXED) {
			va_list argptr;

			va_start(argptr, state);
			uint32_t permille = va_arg(argptr, uint32_t);

			va_end(argptr);
			if (permille <= PERMILLE_MAX) {
				if (!pwm_set_dt(&pwm_pool->out[pwm], pwm_pool->out[pwm].period,
					((uint64_t)permille * pwm_pool->out[pwm].period) /
					PERMILLE_MAX)) {
					struct pwm_pool_aux_data *pwm_pool_aux = pwm_pool->aux;

					pwm_pool_aux[pwm].state = state;
					if (k_work_reschedule(&pwm_pool->work,
						pwm_pool_aux_timeout(pwm_pool)) >= 0) {
						result = true;
					}
				}
			}
		} else if (state == PWM_BLINK) {
			va_list argptr;

			va_start(argptr, state);
			k_timeout_t  t_on = va_arg(argptr, k_timeout_t);
			k_timeout_t  t_off = va_arg(argptr, k_timeout_t);

			va_end(argptr);
			if (t_on.ticks >= K_NSEC(pwm_pool->out[pwm].period).ticks &&
				t_off.ticks >= K_NSEC(pwm_pool->out[pwm].period).ticks) {
				if (!pwm_set_dt(&pwm_pool->out[pwm], pwm_pool->out[pwm].period,
					pwm_pool->out[pwm].period)) {
					struct pwm_pool_aux_data *pwm_pool_aux = pwm_pool->aux;

					pwm_pool_aux[pwm].state            = state;
					pwm_pool_aux[pwm].t_event          = sys_clock_tick_get();
					pwm_pool_aux[pwm].current_state    = true;
					pwm_pool_aux[pwm].mode.blink.t_on  = t_on;
					pwm_pool_aux[pwm].mode.blink.t_off = t_off;

					if (k_work_reschedule(&pwm_pool->work,
						pwm_pool_aux_timeout(pwm_pool)) >= 0) {
						result = true;
					}
				}
			}
		} else if (state == PWM_BREATH) {
			va_list argptr;

			va_start(argptr, state);
			k_timeout_t  t_breath = va_arg(argptr, k_timeout_t);

			va_end(argptr);
			if (t_breath.ticks >=
				K_NSEC(pwm_pool->out[pwm].period).ticks * 2) {
				if (!pwm_set_dt(
					&pwm_pool->out[pwm], pwm_pool->out[pwm].period, 0)) {
					struct pwm_pool_aux_data *pwm_pool_aux = pwm_pool->aux;

					pwm_pool_aux[pwm].state            = state;
					pwm_pool_aux[pwm].t_event          = sys_clock_tick_get();
					/*
					 * set new value now because pwm will be updated
					 * with its value when current cycle will finished
					 */
					if (pwm_pool_aux[pwm].t_event >
						K_NSEC(pwm_pool->out[pwm].period).ticks) {
						pwm_pool_aux[pwm].t_event -=
							K_NSEC(pwm_pool->out[pwm].period).ticks;
					} else {
						pwm_pool_aux[pwm].t_event = 0;
					}
					pwm_pool_aux[pwm].current_state    = true;
					pwm_pool_aux[pwm].mode.breath.period =
						pwm_pool->out[pwm].period;
					pwm_pool_aux[pwm].mode.breath.current_pulse = 0;
					pwm_pool_aux[pwm].mode.breath.step_pulse    =
						((uint64_t) pwm_pool->out[pwm].period *
						pwm_pool->out[pwm].period * 2) /
						k_ticks_to_ns_ceil64(t_breath.ticks);
					if (k_work_reschedule(&pwm_pool->work,
						pwm_pool_aux_timeout(pwm_pool)) >= 0) {
						result = true;
					}
				}
			}
		}
	}
	return result;
}
