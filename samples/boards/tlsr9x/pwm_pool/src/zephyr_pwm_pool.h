/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_PWM_POOL_H
#define ZEPHYR_PWM_POOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>

/* Zephyr PWM device tree compatibility macro */

#define PWM_DT_SPEC_GET_BY_IDX_COMPAT(node_id, prop, idx)                        \
	PWM_DT_SPEC_GET_BY_IDX(node_id, idx)

/* Defines */

#ifndef PERMILLE_MAX
#define PERMILLE_MAX                         1000
#endif /* PERMILLE_MAX */

/* Data types */

enum pwm_state {
	PWM_OFF = 0,
	PWM_ON,
	PWM_FIXED,
	PWM_BLINK,
	PWM_BREATH
};

struct pwm_pool_data {
	const struct pwm_dt_spec     *out;
	const size_t                  out_len;
	void                         *aux;
	struct k_work_delayable       work;
};

/*
 * Declare struct led_pool_data variable base on data from .dts.
 * The name of variable should correspond to .dts node name.
 * .dts fragment example:
 * name {
 *     compatible = "pwm-leds";
 *     out {
 *         pwms = <&pwm0 0 PWM_MSEC(20) PWM_POLARITY_NORMAL>,
 *                <&pwm0 1 PWM_MSEC(20) PWM_POLARITY_NORMAL>,
 *                <&pwm0 2 PWM_MSEC(20) PWM_POLARITY_NORMAL>;
 *     };
 * };
 */
#define PWM_POOL_DEFINE(name)                                                    \
	struct pwm_pool_data name = {                                                \
		.out = (const struct pwm_dt_spec []) {                                   \
			COND_CODE_1(                                                         \
			 DT_NODE_HAS_PROP(DT_PATH_INTERNAL(DT_CHILD(name, out)), pwms),      \
			(DT_FOREACH_PROP_ELEM_SEP(DT_PATH_INTERNAL(DT_CHILD(name, out)),     \
				pwms, PWM_DT_SPEC_GET_BY_IDX_COMPAT, (,))),                      \
			())                                                                  \
		},                                                                       \
		.out_len = COND_CODE_1(                                                  \
			 DT_NODE_HAS_PROP(DT_PATH_INTERNAL(DT_CHILD(name, out)), pwms),      \
			(DT_PROP_LEN(DT_PATH_INTERNAL(DT_CHILD(name, out)), pwms)),          \
			(0)),                                                                \
	}

/* Public APIs */

bool pwm_pool_init(struct pwm_pool_data *pwm_pool);
bool pwm_pool_set(struct pwm_pool_data *pwm_pool,
	size_t pwm, enum pwm_state state, ...);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_KEY_POOL_H */
