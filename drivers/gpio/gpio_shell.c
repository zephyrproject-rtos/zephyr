/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2021 Dennis Ruffer <daruffer@gmail.com>
 * Copyright (c) 2023 Nick Ward <nix.ward@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/shell/shell.h>

#include <stdio.h>

#define ARGV_DEV 1
#define ARGV_PIN 2
#define ARGV_CONF 3
#define ARGV_VALUE 3
#define ARGV_VENDOR_SPECIFIC 4

#define NGPIOS_UNKNOWN -1
#define PIN_NOT_FOUND UINT8_MAX

/* Pin syntax maximum length */
#define PIN_SYNTAX_MAX 32
#define PIN_NUM_MAX 4

struct gpio_ctrl {
	const struct device *dev;
	int8_t ngpios;
	gpio_port_pins_t reserved_mask;
	const char **line_names;
	uint8_t line_names_len;
	const union shell_cmd_entry *subcmd;
};

struct sh_gpio {
	const struct device *dev;
	gpio_pin_t pin;
};
/*
 * Find idx-th pin reference from the set of non reserved
 * pin numbers and provided line names.
 */
static void port_pin_get(gpio_port_pins_t reserved_mask, const char **line_names,
			 uint8_t line_names_len, size_t idx, struct shell_static_entry *entry)
{
	static char pin_syntax[PIN_SYNTAX_MAX];
	static char pin_num[PIN_NUM_MAX];
	const char *name;
	gpio_pin_t pin;
	bool reserved;

	entry->handler = NULL;

	/* Find allowed numeric pin reference */
	for (pin = 0; pin < GPIO_MAX_PINS_PER_PORT; pin++) {
		reserved = ((BIT64(pin) & reserved_mask) != 0);
		if (!reserved) {
			if (idx == 0) {
				break;
			}
			idx--;
		}
	}

	if (pin < GPIO_MAX_PINS_PER_PORT) {
		sprintf(pin_num, "%u", pin);
		if ((pin < line_names_len) && (strlen(line_names[pin]) > 0)) {
			/* pin can be specified by line name */
			name = line_names[pin];
			for (int i = 0; i < (sizeof(pin_syntax) - 1); i++) {
				/*
				 * For line-name tab completion to work replace any
				 * space characters with '_'.
				 */
				pin_syntax[i] = (name[i] != ' ') ? name[i] : '_';
				if (name[i] == '\0') {
					break;
				}
			}
			pin_syntax[sizeof(pin_syntax) - 1] = '\0';
			entry->syntax = pin_syntax;
			entry->help = pin_num;
		} else {
			/* fallback to pin specified by pin number */
			entry->syntax = pin_num;
			entry->help = NULL;
		}
	} else {
		/* No more pins */
		entry->syntax = NULL;
		entry->help = NULL;
	}
}

#define GPIO_DT_RESERVED_RANGES_NGPIOS_SHELL(node_id)                                              \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, ngpios),                                             \
		    (GPIO_DT_RESERVED_RANGES_NGPIOS(node_id, DT_PROP(node_id, ngpios))),           \
		    (GPIO_MAX_PINS_PER_PORT))

#define GPIO_CTRL_PIN_GET_FN(node_id)                                                              \
	static const char *node_id##line_names[] = DT_PROP_OR(node_id, gpio_line_names, {NULL});   \
                                                                                                   \
	static void node_id##cmd_gpio_pin_get(size_t idx, struct shell_static_entry *entry);       \
                                                                                                   \
	SHELL_DYNAMIC_CMD_CREATE(node_id##sub_gpio_pin, node_id##cmd_gpio_pin_get);                \
                                                                                                   \
	static void node_id##cmd_gpio_pin_get(size_t idx, struct shell_static_entry *entry)        \
	{                                                                                          \
		gpio_port_pins_t reserved_mask = GPIO_DT_RESERVED_RANGES_NGPIOS_SHELL(node_id);    \
		uint8_t line_names_len = DT_PROP_LEN_OR(node_id, gpio_line_names, 0);              \
                                                                                                   \
		port_pin_get(reserved_mask, node_id##line_names, line_names_len, idx, entry);      \
		entry->subcmd = NULL;                                                              \
	}

#define IS_GPIO_CTRL_PIN_GET(node_id)                                                              \
	COND_CODE_1(DT_PROP(node_id, gpio_controller), (GPIO_CTRL_PIN_GET_FN(node_id)), ())

DT_FOREACH_STATUS_OKAY_NODE(IS_GPIO_CTRL_PIN_GET)

#define GPIO_CTRL_LIST_ENTRY(node_id)                                                              \
	{                                                                                          \
		.dev = DEVICE_DT_GET(node_id),                                                     \
		.ngpios = DT_PROP_OR(node_id, ngpios, NGPIOS_UNKNOWN),                             \
		.reserved_mask = GPIO_DT_RESERVED_RANGES_NGPIOS_SHELL(node_id),                    \
		.line_names = node_id##line_names,                                                 \
		.line_names_len = DT_PROP_LEN_OR(node_id, gpio_line_names, 0),                     \
		.subcmd = &node_id##sub_gpio_pin,                                                  \
	},

#define IS_GPIO_CTRL_LIST(node_id)                                                                 \
	COND_CODE_1(DT_PROP(node_id, gpio_controller), (GPIO_CTRL_LIST_ENTRY(node_id)), ())

static const struct gpio_ctrl gpio_list[] = {DT_FOREACH_STATUS_OKAY_NODE(IS_GPIO_CTRL_LIST)};

static const struct gpio_ctrl *get_gpio_ctrl(const char *name)
{
	size_t i;
	const struct device *dev = shell_device_get_binding(name);

	if (dev == NULL) {
		return NULL;
	}

	for (i = 0; i < ARRAY_SIZE(gpio_list); i++) {
		if (gpio_list[i].dev == dev) {
			return &gpio_list[i];
		}
	}

	return NULL;
}

int line_cmp(const char *input, const char *line_name)
{
	int i = 0;

	while (true) {
		if ((input[i] == '_') && (line_name[i] == ' ')) {
			/* Allow input underscore to match line_name space */
		} else if (input[i] != line_name[i]) {
			return (input[i] > line_name[i]) ? 1 : -1;
		} else if (line_name[i] == '\0') {
			return 0;
		}
		i++;
	}
}

static int get_gpio_pin(const struct shell *sh, const struct gpio_ctrl *ctrl, char *line_name)
{
	gpio_pin_t pin = PIN_NOT_FOUND;
	gpio_pin_t i;
	int result;

	for (i = 0; i < ctrl->ngpios; i++) {
		result = line_cmp(line_name, ctrl->line_names[i]);
		if (result == 0) {
			if ((BIT64(i) & ctrl->reserved_mask) != 0) {
				shell_error(sh, "Reserved pin");
				return -EACCES;
			} else if (pin == PIN_NOT_FOUND) {
				pin = i;
			} else {
				shell_error(sh, "Line name ambiguous");
				return -EFAULT;
			}
		}
	}

	if (pin == PIN_NOT_FOUND) {
		shell_error(sh, "Line name not found: '%s'", line_name);
		return -ENOENT;
	}

	return pin;
}

static int get_sh_gpio(const struct shell *sh, char **argv, struct sh_gpio *gpio)
{
	const struct gpio_ctrl *ctrl;
	int ret = 0;
	int pin;

	ctrl = get_gpio_ctrl(argv[ARGV_DEV]);
	if (ctrl == NULL) {
		shell_error(sh, "unknown gpio controller: %s", argv[ARGV_DEV]);
		return -EINVAL;
	}
	gpio->dev = ctrl->dev;
	pin = shell_strtoul(argv[ARGV_PIN], 0, &ret);
	if (ret != 0) {
		pin = get_gpio_pin(sh, ctrl, argv[ARGV_PIN]);
		if (pin < 0) {
			return pin;
		}
	} else if ((BIT64(pin) & ctrl->reserved_mask) != 0) {
		shell_error(sh, "Reserved pin");
		return -EACCES;
	}
	gpio->pin = pin;

	return 0;
}

static int cmd_gpio_conf(const struct shell *sh, size_t argc, char **argv, void *data)
{
	gpio_flags_t flags = 0;
	gpio_flags_t vendor_specific;
	struct sh_gpio gpio;
	int ret = 0;

	ret = get_sh_gpio(sh, argv, &gpio);
	if (ret != 0) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	for (int i = 0; i < strlen(argv[ARGV_CONF]); i++) {
		switch (argv[ARGV_CONF][i]) {
		case 'i':
			flags |= GPIO_INPUT;
			break;
		case 'o':
			flags |= GPIO_OUTPUT;
			break;
		case 'u':
			flags |= GPIO_PULL_UP;
			break;
		case 'd':
			flags |= GPIO_PULL_DOWN;
			break;
		case 'h':
			flags |= GPIO_ACTIVE_HIGH;
			break;
		case 'l':
			flags |= GPIO_ACTIVE_LOW;
			break;
		case '0':
			flags |= GPIO_OUTPUT_INIT_LOGICAL | GPIO_OUTPUT_INIT_LOW;
			break;
		case '1':
			flags |= GPIO_OUTPUT_INIT_LOGICAL | GPIO_OUTPUT_INIT_HIGH;
			break;
		default:
			shell_error(sh, "Unknown: '%c'", argv[ARGV_CONF][i]);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
	}

	if (((flags & GPIO_INPUT) != 0) == ((flags & GPIO_OUTPUT) != 0)) {
		shell_error(sh, "must be either input or output");
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	if (((flags & GPIO_PULL_UP) != 0) && ((flags & GPIO_PULL_DOWN) != 0)) {
		shell_error(sh, "cannot be pull up and pull down");
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	if (((flags & GPIO_ACTIVE_LOW) != 0) && ((flags & GPIO_ACTIVE_HIGH) != 0)) {
		shell_error(sh, "cannot be active low and active high");
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	if ((flags & GPIO_OUTPUT) != 0) {
		/* Default to active high if not specified */
		if ((flags & (GPIO_ACTIVE_LOW | GPIO_ACTIVE_HIGH)) == 0) {
			flags |= GPIO_ACTIVE_HIGH;
		}
		/* Default to initialisation to logic 0 if not specified */
		if ((flags & GPIO_OUTPUT_INIT_LOGICAL) == 0) {
			flags |= GPIO_OUTPUT_INIT_LOGICAL | GPIO_OUTPUT_INIT_LOW;
		}
	}

	if (((flags & GPIO_INPUT) != 0) && ((flags & GPIO_OUTPUT_INIT_LOGICAL) != 0)) {
		shell_error(sh, "an input cannot be initialised to a logic level");
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	if (((flags & GPIO_OUTPUT_INIT_LOW) != 0) && ((flags & GPIO_OUTPUT_INIT_HIGH) != 0)) {
		shell_error(sh, "cannot initialise to logic 0 and logic 1");
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	if (argc == 5) {
		vendor_specific = shell_strtoul(argv[ARGV_VENDOR_SPECIFIC], 0, &ret);
		if ((ret == 0) && ((vendor_specific & ~(0xFF00U)) == 0)) {
			flags |= vendor_specific;
		} else {
			/*
			 * See include/zephyr/dt-bindings/gpio/ for the
			 * available flags for your vendor.
			 */
			shell_error(sh, "vendor specific flags must be within "
					"the mask 0xFF00");
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
	}

	ret = gpio_pin_configure(gpio.dev, gpio.pin, flags);
	if (ret != 0) {
		shell_error(sh, "error: %d", ret);
		return ret;
	}

	return 0;
}

static int cmd_gpio_get(const struct shell *sh, size_t argc, char **argv)
{
	struct sh_gpio gpio;
	int value;
	int ret;

	ret = get_sh_gpio(sh, argv, &gpio);
	if (ret != 0) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	value = gpio_pin_get(gpio.dev, gpio.pin);
	if (value >= 0) {
		shell_print(sh, "%u", value);
	} else {
		shell_error(sh, "error: %d", value);
		return value;
	}

	return 0;
}

static int cmd_gpio_set(const struct shell *sh, size_t argc, char **argv)
{
	struct sh_gpio gpio;
	unsigned long value;
	int ret = 0;

	ret = get_sh_gpio(sh, argv, &gpio);
	if (ret != 0) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	value = shell_strtoul(argv[ARGV_VALUE], 0, &ret);
	if (ret != 0) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	ret = gpio_pin_set(gpio.dev, gpio.pin, value != 0);
	if (ret != 0) {
		shell_error(sh, "error: %d", ret);
		return ret;
	}

	return 0;
}

static int cmd_gpio_toggle(const struct shell *sh, size_t argc, char **argv)
{
	struct sh_gpio gpio;
	int ret = 0;

	ret = get_sh_gpio(sh, argv, &gpio);
	if (ret != 0) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	ret = gpio_pin_toggle(gpio.dev, gpio.pin);
	if (ret != 0) {
		shell_error(sh, "error: %d", ret);
		return ret;
	}

	return 0;
}

static int cmd_gpio_devices(const struct shell *sh, size_t argc, char **argv)
{
	size_t i;

	shell_fprintf(sh, SHELL_NORMAL, "%-16s Other names\n", "Device");

	for (i = 0; i < ARRAY_SIZE(gpio_list); i++) {
		const struct device *dev = gpio_list[i].dev;

		shell_fprintf(sh, SHELL_NORMAL, "%-16s", dev->name);

#ifdef CONFIG_DEVICE_DT_METADATA
		const struct device_dt_nodelabels *nl = device_get_dt_nodelabels(dev);

		if (nl != NULL && nl->num_nodelabels > 0) {
			for (size_t j = 0; j < nl->num_nodelabels; j++) {
				const char *nodelabel = nl->nodelabels[j];

				shell_fprintf(sh, SHELL_NORMAL, " %s", nodelabel);
			}
		}
#endif

		shell_fprintf(sh, SHELL_NORMAL, "\n");
	}

	return 0;
}

/* 500 msec = 1/2 sec */
#define SLEEP_TIME_MS   500

static int cmd_gpio_blink(const struct shell *sh, size_t argc, char **argv)
{
	bool msg_one_shot = true;
	struct sh_gpio gpio;
	size_t count;
	char data;
	int ret;

	ret = get_sh_gpio(sh, argv, &gpio);
	if (ret != 0) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	/* dummy read to clear any pending input */
	(void)sh->iface->api->read(sh->iface, &data, sizeof(data), &count);

	while (true) {
		(void)sh->iface->api->read(sh->iface, &data, sizeof(data), &count);
		if (count != 0) {
			break;
		}
		ret = gpio_pin_toggle(gpio.dev, gpio.pin);
		if (ret != 0) {
			shell_error(sh, "%d", ret);
			break;
		} else if (msg_one_shot) {
			msg_one_shot = false;
			shell_print(sh, "Hit any key to exit");
		}
		k_msleep(SLEEP_TIME_MS);
	}

	return 0;
}

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	if (idx >= ARRAY_SIZE(gpio_list)) {
		entry->syntax = NULL;
		return;
	}

	entry->syntax = gpio_list[idx].dev->name;
	entry->handler = NULL;
	entry->help = "Device";
	entry->subcmd = gpio_list[idx].subcmd;
}

SHELL_DYNAMIC_CMD_CREATE(sub_gpio_dev, device_name_get);

struct pin_info {
	const struct device *dev;
	bool reserved;
	gpio_pin_t pin;
	const char *line_name;
};

struct pin_order_user_data {
	const struct shell *sh;
	struct pin_info prev;
	struct pin_info next;
};

typedef void (*pin_foreach_func_t)(const struct pin_info *info, void *user_data);

static void print_gpio_ctrl_info(const struct shell *sh, const struct gpio_ctrl *ctrl)
{
	gpio_pin_t pin;
	bool reserved;
	const char *line_name;

	shell_print(sh, " ngpios: %u", ctrl->ngpios);
	shell_print(sh, " Reserved pin mask: 0x%08X", ctrl->reserved_mask);

	shell_print(sh, "");

	shell_print(sh, " Reserved  Pin  Line Name");
	for (pin = 0; pin < ctrl->ngpios; pin++) {
		reserved = (BIT64(pin) & ctrl->reserved_mask) != 0;
		if (pin < ctrl->line_names_len) {
			line_name = ctrl->line_names[pin];
		} else {
			line_name = "";
		}
		shell_print(sh, "     %c     %2u    %s", reserved ? '*' : ' ', pin, line_name);
	}
}

static void foreach_pin(pin_foreach_func_t func, void *user_data)
{
	gpio_port_pins_t reserved_mask;
	struct pin_info info;
	gpio_pin_t pin;
	size_t i;

	for (i = 0; i < ARRAY_SIZE(gpio_list); i++) {
		for (pin = 0; pin < gpio_list[i].ngpios; pin++) {
			info.dev = gpio_list[i].dev;
			reserved_mask = gpio_list[i].reserved_mask;
			info.reserved = (BIT64(pin) & reserved_mask) != 0;
			info.pin = pin;
			if (pin < gpio_list[i].line_names_len) {
				info.line_name = gpio_list[i].line_names[pin];
			} else {
				info.line_name = "";
			}
			func(&info, user_data);
		}
	}
}

static int pin_cmp(const struct pin_info *a, const struct pin_info *b)
{
	int result = strcmp(a->line_name, b->line_name);

	if (result != 0) {
		return result;
	}
	result = strcmp(a->dev->name, b->dev->name);
	if (result != 0) {
		return result;
	}
	result = (int)a->pin - (int)b->pin;

	return result;
}

static void pin_get_next(const struct pin_info *info, void *user_data)
{
	struct pin_order_user_data *data = user_data;
	int result;

	if (data->prev.line_name != NULL) {
		result = pin_cmp(info, &data->prev);
	} else {
		result = 1;
	}
	if (result > 0) {
		if (data->next.line_name == NULL) {
			data->next = *info;
			return;
		}
		result = pin_cmp(info, &data->next);
		if (result < 0) {
			data->next = *info;
		}
	}
}

static void pin_ordered(const struct pin_info *info, void *user_data)
{
	struct pin_order_user_data *data = user_data;

	ARG_UNUSED(info);

	foreach_pin(pin_get_next, data);

	shell_print(data->sh, "   %-12s %-8c %-16s %2u",
		    data->next.line_name,
		    data->next.reserved ? '*' : ' ',
		    data->next.dev->name,
		    data->next.pin);

	data->prev = data->next;
	data->next.line_name = NULL;
}

static void print_ordered_info(const struct shell *sh)
{
	struct pin_order_user_data data = {0};

	data.sh = sh;

	shell_print(sh, "  %-12s %-8s %-16s %-3s",
		"Line", "Reserved", "Device", "Pin");

	foreach_pin(pin_ordered, &data);
}

static int cmd_gpio_info(const struct shell *sh, size_t argc, char **argv)
{
	const struct gpio_ctrl *ctrl = get_gpio_ctrl(argv[ARGV_DEV]);

	if (ctrl == NULL) {
		/* No device specified */
		print_ordered_info(sh);
		return 0;
	}

	print_gpio_ctrl_info(sh, ctrl);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_gpio,
	SHELL_CMD_ARG(conf, &sub_gpio_dev,
		"Configure GPIO pin\n"
		"Usage: gpio conf <device> <pin> <configuration <i|o>[u|d][h|l][0|1]> [vendor specific]\n"
		"<i|o> - input|output\n"
		"[u|d] - pull up|pull down, otherwise open\n"
		"[h|l] - active high|active low, otherwise defaults to active high\n"
		"[0|1] - initialise to logic 0|logic 1, otherwise defaults to logic 0\n"
		"[vendor specific] - configuration flags within the mask 0xFF00\n"
		"                    see include/zephyr/dt-bindings/gpio/",
		cmd_gpio_conf, 4, 1),
	SHELL_CMD_ARG(get, &sub_gpio_dev,
		"Get GPIO pin value\n"
		"Usage: gpio get <device> <pin>", cmd_gpio_get, 3, 0),
	SHELL_CMD_ARG(set, &sub_gpio_dev,
		"Set GPIO pin value\n"
		"Usage: gpio set <device> <pin> <level 0|1>", cmd_gpio_set, 4, 0),
	SHELL_COND_CMD_ARG(CONFIG_GPIO_SHELL_TOGGLE_CMD, toggle, &sub_gpio_dev,
		"Toggle GPIO pin\n"
		"Usage: gpio toggle <device> <pin>", cmd_gpio_toggle, 3, 0),
	SHELL_CMD(devices, NULL,
		"List all GPIO devices\n"
		"Usage: gpio devices", cmd_gpio_devices),
	SHELL_COND_CMD_ARG(CONFIG_GPIO_SHELL_BLINK_CMD, blink, &sub_gpio_dev,
		"Blink GPIO pin\n"
		"Usage: gpio blink <device> <pin>", cmd_gpio_blink, 3, 0),
	SHELL_COND_CMD_ARG(CONFIG_GPIO_SHELL_INFO_CMD, info, &sub_gpio_dev,
		"GPIO Information\n"
		"Usage: gpio info [device]", cmd_gpio_info, 1, 1),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(gpio, &sub_gpio, "GPIO commands", NULL);
