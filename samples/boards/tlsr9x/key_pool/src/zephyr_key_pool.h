/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_KEY_POOL_H
#define ZEPHYR_KEY_POOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* Data types */

typedef void (*key_pool_on_button_change_t)(size_t button, bool pressed, void *context);

struct key_pool_data {
	const struct gpio_dt_spec    *inp;
	const size_t                  inp_len;
	uint8_t                      *buttons;
	void                         *aux;
	key_pool_on_button_change_t   on_button_change;
	void                         *context;
	struct k_work_delayable       work;
};

/*
 * Declare struct key_pool_data variable base on data from .dts.
 * The name of variable should correspond to .dts node name.
 * .dts fragment example:
 * name {
 *     compatible = "gpio-keys";
 *     inp {
 *         gpios = <&gpioc 3 (GPIO_PULL_UP | GPIO_ACTIVE_LOW)>,
 *                 <&gpioc 1 (GPIO_PULL_UP | GPIO_ACTIVE_LOW)>;
 *     };
 * };
 */
#define KEY_POOL_DEFINE(name)                                                    \
	struct key_pool_data name = {                                                \
		.inp = (const struct gpio_dt_spec []) {                                  \
			COND_CODE_1(                                                         \
			 DT_NODE_HAS_PROP(DT_PATH_INTERNAL(DT_CHILD(name, inp)), gpios),     \
			(DT_FOREACH_PROP_ELEM_SEP(DT_PATH_INTERNAL(DT_CHILD(name, inp)),     \
				gpios, GPIO_DT_SPEC_GET_BY_IDX, (,))),                           \
			())                                                                  \
		},                                                                       \
		.inp_len = COND_CODE_1(                                                  \
			 DT_NODE_HAS_PROP(DT_PATH_INTERNAL(DT_CHILD(name, inp)), gpios),     \
			(DT_PROP_LEN(DT_PATH_INTERNAL(DT_CHILD(name, inp)), gpios)),         \
			(0)),                                                                \
		.buttons = (uint8_t [COND_CODE_1(                                        \
			 DT_NODE_HAS_PROP(DT_PATH_INTERNAL(DT_CHILD(name, inp)), gpios),     \
			(DIV_ROUND_UP(                                                       \
				DT_PROP_LEN(DT_PATH_INTERNAL(DT_CHILD(name, inp)), gpios), 8)),  \
			(0))]) {},                                                           \
	}

/* Public APIs */

bool key_pool_init(struct key_pool_data *key_pool);
void key_pool_set_callback(struct key_pool_data *key_pool,
	key_pool_on_button_change_t on_button_change, void *context);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_KEY_POOL_H */
