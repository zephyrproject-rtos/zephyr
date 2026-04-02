/*
 * Copyright (c) 2022-2023 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gpio_hogs, CONFIG_GPIO_LOG_LEVEL);

struct gpio_hog_dt_spec {
	gpio_pin_t pin;
	gpio_flags_t flags;
};

struct gpio_hogs {
	const struct device *port;
	const struct gpio_hog_dt_spec *specs;
	size_t num_specs;
};

/* Static initializer for a struct gpio_hog_dt_spec */
#define GPIO_HOG_DT_SPEC_GET_BY_IDX(node_id, idx)						\
	{											\
		.pin = DT_GPIO_HOG_PIN_BY_IDX(node_id, idx),					\
		.flags = DT_GPIO_HOG_FLAGS_BY_IDX(node_id, idx) |				\
			 COND_CODE_1(DT_PROP(node_id, input), (GPIO_INPUT),			\
				     (COND_CODE_1(DT_PROP(node_id, output_low),			\
						 (GPIO_OUTPUT_INACTIVE),			\
						 (COND_CODE_1(DT_PROP(node_id, output_high),	\
							     (GPIO_OUTPUT_ACTIVE), (0)))))),	\
	}

/* Expands to 1 if node_id is a GPIO controller, 0 otherwise */
#define GPIO_HOGS_NODE_IS_GPIO_CTLR(node_id)			\
	DT_PROP_OR(node_id, gpio_controller, 0)

/* Expands to 1 if node_id is a GPIO hog, empty otherwise */
#define GPIO_HOGS_NODE_IS_GPIO_HOG(node_id)			\
	IF_ENABLED(DT_PROP_OR(node_id, gpio_hog, 0), 1)

/* Expands to 1 if GPIO controller node_id has GPIO hog children, 0 otherwise */
#define GPIO_HOGS_GPIO_CTLR_HAS_HOGS(node_id)			\
	COND_CODE_0(						\
		IS_EMPTY(DT_FOREACH_CHILD_STATUS_OKAY(node_id,	\
			GPIO_HOGS_NODE_IS_GPIO_HOG)),		\
		(1), (0))

/* Called for GPIO hog indexes */
#define GPIO_HOGS_INIT_GPIO_HOG_BY_IDX(idx, node_id)		\
	GPIO_HOG_DT_SPEC_GET_BY_IDX(node_id, idx)

/* Called for GPIO hog dts nodes */
#define GPIO_HOGS_INIT_GPIO_HOGS(node_id)			\
	LISTIFY(DT_NUM_GPIO_HOGS(node_id),			\
		GPIO_HOGS_INIT_GPIO_HOG_BY_IDX, (,), node_id),

/* Called for GPIO controller dts node children */
#define GPIO_HOGS_COND_INIT_GPIO_HOGS(node_id)			\
	COND_CODE_0(IS_EMPTY(GPIO_HOGS_NODE_IS_GPIO_HOG(node_id)),	\
		    (GPIO_HOGS_INIT_GPIO_HOGS(node_id)), ())

/* Called for each GPIO controller dts node which has GPIO hog children */
#define GPIO_HOGS_INIT_GPIO_CTLR(node_id)				\
	{								\
		.port = DEVICE_DT_GET(node_id),				\
		.specs = (const struct gpio_hog_dt_spec []) {		\
			DT_FOREACH_CHILD_STATUS_OKAY(node_id,		\
				GPIO_HOGS_COND_INIT_GPIO_HOGS)		\
		},							\
		.num_specs =						\
			DT_FOREACH_CHILD_STATUS_OKAY_SEP(node_id,	\
				DT_NUM_GPIO_HOGS, (+)),			\
	},

/* Called for each GPIO controller dts node */
#define GPIO_HOGS_COND_INIT_GPIO_CTLR(node_id)			\
	IF_ENABLED(GPIO_HOGS_GPIO_CTLR_HAS_HOGS(node_id),	\
		   (GPIO_HOGS_INIT_GPIO_CTLR(node_id)))

/* Called for each dts node */
#define GPIO_HOGS_COND_INIT(node_id)				\
	IF_ENABLED(GPIO_HOGS_NODE_IS_GPIO_CTLR(node_id),	\
		   (GPIO_HOGS_COND_INIT_GPIO_CTLR(node_id)))

static const struct gpio_hogs gpio_hogs[] = {
	DT_FOREACH_STATUS_OKAY_NODE(GPIO_HOGS_COND_INIT)
};

static int gpio_hogs_init(void)
{
	const struct gpio_hogs *hogs;
	const struct gpio_hog_dt_spec *spec;
	int err;
	int i;
	int j;


	for (i = 0; i < ARRAY_SIZE(gpio_hogs); i++) {
		hogs = &gpio_hogs[i];

		if (!device_is_ready(hogs->port)) {
			LOG_ERR("GPIO port %s not ready", hogs->port->name);
			return -ENODEV;
		}

		for (j = 0; j < hogs->num_specs; j++) {
			spec = &hogs->specs[j];

			err = gpio_pin_configure(hogs->port, spec->pin, spec->flags);
			if (err < 0) {
				LOG_ERR("failed to configure GPIO hog for port %s pin %u (err %d)",
					hogs->port->name, spec->pin, err);
				return err;
			}
		}
	}

	return 0;
}

SYS_INIT(gpio_hogs_init, POST_KERNEL, CONFIG_GPIO_HOGS_INIT_PRIORITY);
