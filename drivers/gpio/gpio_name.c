/*
 * Copyright (c) 2022 Google Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <devicetree.h>
#include <drivers/gpio/gpio_name.h>
#include <string.h>
#include <logging/log.h>

#ifdef CONFIG_GPIO_SHELL
#include <shell/shell.h>
#include <stdio.h>
#endif

/*
 * Try to determine the base node containing the GPIO nodes.
 * Mostly the GPIO controllers are child nodes of the
 * 'soc' node.
 * A node label 'gpio_controller_parent' may be set
 * on the parent node.
 */

#if DT_NODE_EXISTS(DT_ALIAS(gpio_controller_parent))

#define GPIO_BASE_NODE DT_ALIAS(gpio_controller_parent)

#elif DT_NODE_EXISTS(DT_PATH(soc))

#define GPIO_BASE_NODE DT_PATH(soc)

#elif DT_HAS_COMPAT_STATUS_OKAY(zephyr_posix)

#define GPIO_BASE_NODE DT_ROOT

#else

#error "Unable to determine node containing GPIO controllers"

#endif

/*
 * For this GPIO controller node, create a string array containing the
 * gpio-line-names strings, and name the array using the node ID.
 */
#define NAME_DEFN(id) \
	static const char *const id[] = DT_PROP(id, gpio_line_names)

/*
 * For GPIO controller nodes that have the gpio-line-names property,
 * create the string array for the names.
 */
#define NAME_ENTRY(id)					   \
	COND_CODE_1(DT_NODE_HAS_PROP(id, gpio_line_names), \
		    (NAME_DEFN(id);),			   \
		    ())

/*
 * Walk through all the child nodes of the base node, and
 * for each node that contains a gpio-line-names property,
 * generated an array of names from this property.
 */
DT_FOREACH_CHILD_STATUS_OKAY(GPIO_BASE_NODE, NAME_ENTRY)

/*
 * Holds a summary of the GPIO controller data captured from DTS
 * nodes that have the gpio-line-names property.
 */
struct gpio_names {
	const struct device *port;      /* GPIO controller device */
	const char *const *names;       /* pointer to array of names */
	uint8_t name_count;             /* Count of names in array */
};

/*
 * Init one entry for a GPIO controller.
 */
#define CONTR_ENTRY(id)						\
	{							\
		.port = DEVICE_DT_GET(id),			\
		.names = id,					\
		.name_count = DT_PROP_LEN(id, gpio_line_names),	\
	},

/*
 * Conditionally create a gpio_names entry if the
 * node has the gpio-line-names property.
 */
#define GPIO_CONTR(id)					   \
	COND_CODE_1(DT_NODE_HAS_PROP(id, gpio_line_names), \
		    (CONTR_ENTRY(id)),			   \
		    ())

/*
 * Table of gpio_names, one entry for each GPIO controller.
 */
static const struct gpio_names gpio_names[] = {
	DT_FOREACH_CHILD_STATUS_OKAY(GPIO_BASE_NODE, GPIO_CONTR)
};

/*
 * Search for a name of a GPIO, and return the
 * controller (port) and pin number. The pin number
 * is the array index of the matching name within the
 * controller.
 * Return the index of the controller, or -1 if not
 * found. Also store the pin number in the pointer passed.
 */
static int gpio_find_by_name(const char *name, gpio_pin_t *pin)
{
	for (int i = 0; i < ARRAY_SIZE(gpio_names); i++) {
		const char *const *n = gpio_names[i].names;

		for (int j = 0; j < gpio_names[i].name_count; j++) {
			if (n[j] != NULL && strcmp(n[j], name) == 0) {
				*pin = j;
				return i;
			}
		}
	}
	*pin = 0;
	return -1;
}

#ifdef CONFIG_GPIO_SHELL
/*
 * Array of last displayed values for GPIOs.
 */
static gpio_port_pins_t gpio_last_value[ARRAY_SIZE(gpio_names)];

/*
 * Print the details of this GPIO. The raw value is
 * compared against the last value displayed, and '*' displayed
 * for changed values. 'L' is displayed for inverted values (active low).
 */
static void gpio_print(const struct shell *sh, int index, gpio_pin_t pin)
{
	int value;
	char changed, polarity;
	uint32_t last = gpio_last_value[index];
	const struct gpio_names *gp = &gpio_names[index];
	struct gpio_driver_data *data;

	if (pin >= gp->name_count) {
		return;
	}
	data = (struct gpio_driver_data *)gp->port->data;
	/* Get current state of GPIO */
	value = gpio_pin_get_raw(gp->port, pin);
	if (value != ((last >> pin) & 1)) {
		changed = '*';
		/* Update remembered value */
		gpio_last_value[index] =
			(last & ~(1 << pin)) | (value << pin);
	} else {
		changed = ' ';
	}
	/* Get the polarity (active low/high) of the pin */
	polarity = (data->invert & (1 << pin)) ? 'L' : ' ';
	shell_print(sh, " %d%c %c %s", value, changed, polarity, gp->names[pin]);
}

int cmd_gpio_name_show(const struct shell *sh, const char *name)
{
	/* Display a single GPIO */
	gpio_pin_t pin;
	int index = gpio_find_by_name(name, &pin);

	if (index < 0) {
		return -ENOENT;
	}
	gpio_print(sh, index, pin);
	return 0;
}

void cmd_gpio_name_show_all(const struct shell *sh)
{
	/* Display all GPIOs with names */
	for (int i = 0; i < ARRAY_SIZE(gpio_names); i++) {
		for (int j = 0; j < gpio_names[i].name_count; j++) {
			const char *np = gpio_names[i].names[j];
			/* Don't attempt to print missing or empty names */
			if (np != NULL && *np != 0) {
				gpio_print(sh, i, j);
			}
		}
	}
}
#endif

const char *gpio_pin_get_name(const struct device *port, gpio_pin_t pin)
{
	const char *name;

	/* Search table for matching port */
	for (int i = 0; i < ARRAY_SIZE(gpio_names); i++) {
		if (gpio_names[i].port == port) {
			/* Check pin is within range */
			if (pin >= gpio_names[i].name_count) {
				return NULL;
			}
			name = gpio_names[i].names[pin];
			/* Make sure it is not NULL or empty */
			if (name != NULL && *name != 0) {
				return name;
			}
			return NULL;
		}
	}
	return NULL;
}

int gpio_pin_by_name(const char *name,
		     const struct device **port, gpio_pin_t *pin)
{
	int index = gpio_find_by_name(name, pin);

	if (index < 0) {
		*port = NULL;
		return -ENOENT;
	}
	*port = gpio_names[index].port;
	return 0;
}
