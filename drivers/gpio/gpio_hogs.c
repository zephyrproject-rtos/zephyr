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
#ifdef CONFIG_GPIO_HOGS_LINE_NAMES
	const char *name;
#endif
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
		IF_ENABLED(CONFIG_GPIO_HOGS_LINE_NAMES,                                         \
			   (.name = DT_GPIO_HOG_LINE_NAME_BY_IDX(node_id, idx),))               \
	}

/* Expands to 1 if node_id is a GPIO controller, 0 otherwise */
#define GPIO_HOGS_NODE_IS_GPIO_CTLR(node_id)			\
	DT_PROP_OR(node_id, gpio_controller, 0)

/* Expands to to 1 if node_id is a GPIO hog, empty otherwise */
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

const struct gpio_hogs gpio_hogs[] = {
	DT_FOREACH_STATUS_OKAY_NODE(GPIO_HOGS_COND_INIT)
};

/* TODO: move to a separate file */
// #ifdef CONFIG_GPIO_HOGS_SHELL
#if 1
#include <zephyr/shell/shell.h>

#define ARGV_NAME	1
#define ARGV_VALUE	2

static bool get_gpio_hog(const char *name,
			 const struct device **port,
			 const struct gpio_hog_dt_spec **hog_dt_spec)
{
	const struct gpio_hogs *gpio_hog;

	gpio_hog = &gpio_hogs[0];
	for (size_t i = 0; i < ARRAY_SIZE(gpio_hogs); i++, gpio_hog++) {
		for (size_t pin = 0; pin < gpio_hog->num_specs; pin++) {
			if (!strcmp(name, gpio_hog->specs[pin].name)) {
				*port = gpio_hog->port;
				*hog_dt_spec = &gpio_hog->specs[pin];
				return true;
			}
		}
	}

	return false;
}

static int cmd_gpio_hogs_get(const struct shell *shell_ctx, size_t argc, char **argv)
{
	const struct device *port;
	const struct gpio_hog_dt_spec *spec;
	int level;

	if (!get_gpio_hog(argv[ARGV_NAME], &port, &spec)) {
		shell_error(shell_ctx, "GPIO Hog: %s not found.", argv[ARGV_NAME]);
		return -ENODEV;
	}

	level = gpio_pin_get(port, spec->pin);
	if (level >= 0) {
		shell_print(shell_ctx, " %d %s", level, spec->name);
	} else {
		shell_error(shell_ctx, "Error %d reading value", level);
		return -EIO;
	}

	return 0;
}


static void cmd_gpio_hog_get_name(size_t idx, struct shell_static_entry *entry)
{
	size_t hog_idx = 0;
	const struct gpio_hogs *gpio_hog;

	gpio_hog = &gpio_hogs[0];
	for (size_t i = 0; i < ARRAY_SIZE(gpio_hogs); i++, gpio_hog++) {
		for (size_t pin = 0; pin < gpio_hog->num_specs; pin++, hog_idx++) {
			if (hog_idx == idx) {
				entry->syntax = gpio_hog->specs[pin].name;
				entry->handler = NULL;
				entry->help = NULL;
				entry->subcmd = NULL;
				return;
			}
		}
		if (hog_idx == idx) {

		}
	}

	/* Reached the end of all GPIO hogs */
	entry->syntax = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(sub_gpio_hog_port_name, cmd_gpio_hog_get_name);


SHELL_STATIC_SUBCMD_SET_CREATE(sub_gpio_hogs,
			       SHELL_CMD_ARG(get,
			       		     &sub_gpio_hog_port_name,
					     "Get GPIO value",
					     cmd_gpio_hogs_get,
					     2,
					     0),
			//        SHELL_CMD_ARG(set, NULL, "Set GPIO", cmd_gpio_hogs_set, 4, 0),
			       SHELL_SUBCMD_SET_END /* Array terminated. */
			       );

SHELL_CMD_REGISTER(gpio_hogs, &sub_gpio_hogs, "GPIO Hogs commands", NULL);

#endif

int gpio_hogs_configure(const struct device *port, gpio_flags_t mask)
{
	const struct gpio_hogs *hogs;
	const struct gpio_hog_dt_spec *spec;
	gpio_flags_t flags;
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

                        if (port && port != hogs->port) {
                                continue;
                        }

			/*
			 * Always skip configuring any pin that doesn't specify
			 * input or output direction. This path allows applications
			 * to add GPIO hogs for use with the shell commands.
			 */
			if ((spec->flags & GPIO_DIR_MASK) == 0) {
				continue;
			}

			flags = spec->flags & mask;

			err = gpio_pin_configure(hogs->port, spec->pin, flags);
			if (err < 0) {
				LOG_ERR("failed to configure GPIO hog for port %s pin %u (err %d)",
					hogs->port->name, spec->pin, err);
				return err;
			}
		}
	}

	return 0;
}

static int gpio_hogs_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	if (!IS_ENABLED(CONFIG_GPIO_HOGS_INITIALIZE_BY_APPLICATION)) {
		return gpio_hogs_configure(NULL, GPIO_FLAGS_ALL);
	}

	return 0;
}

SYS_INIT(gpio_hogs_init, POST_KERNEL, CONFIG_GPIO_HOGS_INIT_PRIORITY);
