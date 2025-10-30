/*
 * Copyright (c) 2024 Astrolight
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>
#include <zephyr/devicetree.h>

#define TXRX_ARGV_SPI_DEV (1)
#define TXRX_ARGV_BYTES   (2)

#define CONF_ARGV_SPI_DEV   (1)
#define CONF_ARGV_FREQUENCY (2)
#define CONF_ARGV_SETTINGS  (3)

#define CS_ARGV_SPI_DEV    (1)
#define CS_ARGV_GPIO_DEV   (2)
#define CS_ARGV_GPIO_PIN   (3)
#define CS_ARGV_GPIO_FLAGS (4)

/* Maximum bytes we can write and read at once */
#define MAX_SPI_BYTES MIN((CONFIG_SHELL_ARGC_MAX - TXRX_ARGV_BYTES), 32)

/* Runs the given fn only if the node_id belongs to a spi device, which is on an okay spi bus. */
#define RUN_FN_ON_SPI_DEVICE(node_id, fn)                                                          \
	COND_CODE_1(DT_ON_BUS(node_id, spi), \
	(COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(DT_BUS(node_id)), \
	(fn), ())), ())

/* Create specified number of empty structs, separated by ',' */
#define _EMPTY_STRUCT_INST(idx, list) {0}
#define CREATE_NUM_EMPTY_STRUCTS(num) LISTIFY(num, _EMPTY_STRUCT_INST, (,))

/* Struct representing either a spi bus or a spi device. */
struct spi_shell_device {
	/* Device name. Either a spi bus or a spi device. */
	const char *name;
	struct spi_dt_spec spec;
};

/* Struct used to map a label to a name of the associated spi_shell_device. */
struct map {
	/* Either nodelabel or device name of either spi bus or spi device. */
	const char *label;
	/* Device name. This is the same as the name of the associated spi_shell_device struct. */
	const char *name;
};

#define INST_SPI_SHELL_DEVICE_AS_SPI_DEV(node_id)                                                  \
	{                                                                                          \
		.name = DEVICE_DT_NAME(node_id),                                                   \
		.spec = SPI_DT_SPEC_GET(node_id, 0),                                               \
	},

#define INST_SPI_SHELL_DEVICE_AS_SPI_DEV_AS_SPI_BUS(dev)                                           \
	(struct spi_shell_device)                                                                  \
	{                                                                                          \
		.name = dev->name,                                                                 \
		.spec = {                                                                          \
			.bus = dev,                                                                \
			.config =                                                                  \
				{                                                                  \
					.frequency = 1000000,                                      \
					.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8),         \
				},                                                                 \
		},                                                                                 \
	}

#define INST_MAP(nodelabel, node_id)                                                               \
	{.label = STRINGIFY(nodelabel), .name = DEVICE_DT_NAME(node_id)},

#define INST_MAP_FROM_NODE_ID(node_id)                                                             \
	{.label = DEVICE_DT_NAME(node_id), .name = DEVICE_DT_NAME(node_id)},

/* Instantiate spi_shell_device struct from node_id, if node_id is a spi device. */
#define INST_ALL_SPI_DEVICES_AS_SPI_SHELL_DEVICES(node_id)                                         \
	RUN_FN_ON_SPI_DEVICE(node_id, INST_SPI_SHELL_DEVICE_AS_SPI_DEV(node_id))

/* List of spi shell devices. At compile time we instantiate structs for all spi devices.
 * Additional empty space is reserved for the spi buses, which are added in spi_buses_init at
 * runtime.
 */
static struct spi_shell_device spi_shell_devices[] = {
	DT_FOREACH_STATUS_OKAY_NODE(INST_ALL_SPI_DEVICES_AS_SPI_SHELL_DEVICES)
		CREATE_NUM_EMPTY_STRUCTS(CONFIG_SPI_SHELL_MAX_DEVICE_SLOTS)};
static size_t num_spi_shell_devices =
	ARRAY_SIZE(spi_shell_devices) - CONFIG_SPI_SHELL_MAX_DEVICE_SLOTS;

#define INST_MAPS_FROM_SPI_DEVICE_NODELABELS(node_id)                                              \
	RUN_FN_ON_SPI_DEVICE(node_id, DT_FOREACH_NODELABEL_VARGS(node_id, INST_MAP, node_id))

#define INST_MAPS_FROM_SPI_DEVICE_NODE_ID(node_id)                                                 \
	RUN_FN_ON_SPI_DEVICE(node_id, INST_MAP_FROM_NODE_ID(node_id))

/* A list of maps. At compile time we create maps for all nodelabels and node_ids of spi devices.
 * Additional empty space is reserved for the spi buses, which are added in spi_buses_init at
 * runtime.
 */
static struct map maps[] = {
	DT_FOREACH_STATUS_OKAY_NODE(INST_MAPS_FROM_SPI_DEVICE_NODELABELS)
		DT_FOREACH_STATUS_OKAY_NODE(INST_MAPS_FROM_SPI_DEVICE_NODE_ID)
			CREATE_NUM_EMPTY_STRUCTS(CONFIG_SPI_SHELL_MAX_DEVICE_SLOTS)};
static size_t num_maps = ARRAY_SIZE(maps) - CONFIG_SPI_SHELL_MAX_DEVICE_SLOTS;

static bool device_is_spi(const struct device *dev)
{
	return DEVICE_API_IS(spi, dev);
}

static bool device_is_gpio(const struct device *dev)
{
	return DEVICE_API_IS(gpio, dev);
}

/**
 * @brief Initialize spi buses at runtime.
 *
 * Since Zephyr currently doesn't support getting a device for all spi buses at compile time in a
 * generic way, we do it at runtime.
 *
 * For each spi bus device we:
 * - add an entry to spi_shell_devices array
 * - add an entry to maps array by its name
 * - add an entry to maps array for each it's nodelabel
 */
static int spi_buses_init(void)
{
	int idx = 0;

	while (1) {
		const struct device *dev = shell_device_filter(idx, device_is_spi);

		idx++;

		if (dev == NULL) {
			break;
		}

		if (num_spi_shell_devices == ARRAY_SIZE(spi_shell_devices)) {
			printk("ERROR: not enough space in spi_shell_devices array\n");
			printk("Increase CONFIG_SPI_SHELL_MAX_DEVICE_SLOTS.\n");
			break;
		}

		spi_shell_devices[num_spi_shell_devices++] =
			INST_SPI_SHELL_DEVICE_AS_SPI_DEV_AS_SPI_BUS(dev);

		maps[num_maps++] = (struct map){
			.label = dev->name,
			.name = dev->name,
		};

#ifdef CONFIG_DEVICE_DT_METADATA
		const struct device_dt_nodelabels *nl = device_get_dt_nodelabels(dev);

		if (nl == NULL) {
			/* No nodelabel for this device, so we can skip the rest. */
			continue;
		}

		if (num_maps + nl->num_nodelabels > ARRAY_SIZE(maps)) {
			printk("ERROR: not enough space in maps array\n");
			printk("Increase CONFIG_SPI_SHELL_MAX_DEVICE_SLOTS.\n");
			break;
		}

		for (size_t i = 0; i < nl->num_nodelabels; i++) {
			maps[num_maps++] = (struct map){
				.label = nl->nodelabels[i],
				.name = dev->name,
			};
		}
#endif
	}

	if (num_spi_shell_devices == 0) {
		printk("ERROR: no spi devices or spi buses are enabled, check devicetree.\n");
	}

	return 0;
}

/**
 * @brief Find spi_dt_spec by label (either nodelabel or nodename).
 *
 * The label can belong to either a spi bus or a spi device. We first look up the name
 * associated with the given label in the maps array. If the name is found, we then search
 * the spi_shell_devices array for a matching name and return the corresponding spi_dt_spec.
 *
 * @param[in] label
 *
 * @return Pointer to spi_dt_spec if found, NULL otherwise.
 */
static struct spi_dt_spec *find_spec_by_label(const char *label)
{
	const char *name = NULL;
	static bool initialized;

	if (!initialized) {
		spi_buses_init();
		initialized = true;
	}

	for (size_t i = 0; i < num_maps; i++) {
		if (strcmp(label, maps[i].label) == 0) {
			name = maps[i].name;
			break;
		}
	}

	if (name == NULL) {
		return NULL;
	}

	for (size_t i = 0; i < num_spi_shell_devices; i++) {
		if (strcmp(name, spi_shell_devices[i].name) == 0) {
			return &spi_shell_devices[i].spec;
		}
	}

	return NULL;
}

static void get_gpio_device_name(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_filter(idx, device_is_gpio);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_get_gpio_device_name, get_gpio_device_name);

static void get_spi_shell_device_name_and_set_gpio_dsub(size_t idx,
							struct shell_static_entry *entry)
{
	if (idx >= num_maps) {
		entry->syntax = NULL;
		return;
	}

	entry->syntax = maps[idx].label;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = &dsub_get_gpio_device_name;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_get_spi_shell_device_name_and_set_gpio_dsub,
			 get_spi_shell_device_name_and_set_gpio_dsub);

static void get_spi_shell_device_name(size_t idx, struct shell_static_entry *entry)
{
	if (idx >= num_maps) {
		entry->syntax = NULL;
		return;
	}

	entry->syntax = maps[idx].label;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_get_spi_shell_device_name, get_spi_shell_device_name);

static int cmd_spi_transceive(const struct shell *ctx, size_t argc, char **argv)
{
	uint8_t rx_buffer[MAX_SPI_BYTES] = {0};
	uint8_t tx_buffer[MAX_SPI_BYTES] = {0};

	struct spi_dt_spec *spec = find_spec_by_label(argv[TXRX_ARGV_SPI_DEV]);

	if (spec == NULL) {
		shell_error(ctx, "device %s not found.", argv[TXRX_ARGV_SPI_DEV]);
		return -ENODEV;
	}

	int bytes_to_send = argc - TXRX_ARGV_BYTES;

	for (int i = 0; i < bytes_to_send; i++) {
		tx_buffer[i] = strtol(argv[TXRX_ARGV_BYTES + i], NULL, 16);
	}

	const struct spi_buf tx_buffers = {.buf = tx_buffer, .len = bytes_to_send};
	const struct spi_buf rx_buffers = {.buf = rx_buffer, .len = bytes_to_send};

	const struct spi_buf_set tx_buf_set = {.buffers = &tx_buffers, .count = 1};
	const struct spi_buf_set rx_buf_set = {.buffers = &rx_buffers, .count = 1};

	int ret = spi_transceive_dt(spec, &tx_buf_set, &rx_buf_set);

	if (ret < 0) {
		shell_error(ctx, "spi_transceive returned %d", ret);
		return ret;
	}

	shell_print(ctx, "TX:");
	shell_hexdump(ctx, tx_buffer, bytes_to_send);

	shell_print(ctx, "RX:");
	shell_hexdump(ctx, rx_buffer, bytes_to_send);

	return ret;
}

static int cmd_spi_conf(const struct shell *ctx, size_t argc, char **argv)
{
	spi_operation_t operation = SPI_WORD_SET(8) | SPI_OP_MODE_MASTER;

	struct spi_dt_spec *spec = find_spec_by_label(argv[CONF_ARGV_SPI_DEV]);

	if (spec == NULL) {
		shell_error(ctx, "device %s not found.", argv[CONF_ARGV_SPI_DEV]);
		return -ENODEV;
	}

	uint32_t frequency = strtol(argv[CONF_ARGV_FREQUENCY], NULL, 10);

	if (!IN_RANGE(frequency, 100 * 1000, 80 * 1000 * 1000)) {
		shell_error(ctx, "frequency must be between 100000  and 80000000");
		return -EINVAL;
	}

	/* no settings */
	if (argc == (CONF_ARGV_FREQUENCY + 1)) {
		goto out;
	}

	char *opts = argv[CONF_ARGV_SETTINGS];
	bool all_opts_is_valid = true;

	while (*opts != '\0') {
		switch (*opts) {
		case 'o':
			operation |= SPI_MODE_CPOL;
			break;
		case 'h':
			operation |= SPI_MODE_CPHA;
			break;
		case 'l':
			operation |= SPI_TRANSFER_LSB;
			break;
		case 'T':
			operation |= SPI_FRAME_FORMAT_TI;
			break;
		default:
			all_opts_is_valid = false;
			shell_error(ctx, "invalid setting %c", *opts);
		}
		opts++;
	}

	if (!all_opts_is_valid) {
		return -EINVAL;
	}

out:
	spec->config.frequency = frequency;
	spec->config.operation = operation;

	return 0;
}

static int cmd_spi_conf_cs(const struct shell *ctx, size_t argc, char **argv)
{
	struct spi_dt_spec *spec = find_spec_by_label(argv[CS_ARGV_SPI_DEV]);

	if (spec == NULL) {
		shell_error(ctx, "device %s not found.", argv[CS_ARGV_SPI_DEV]);
		return -ENODEV;
	}

	struct device *dev = (struct device *)shell_device_get_binding(argv[CS_ARGV_GPIO_DEV]);
	char *endptr = NULL;

	if (dev == NULL) {
		shell_error(ctx, "device %s not found.", argv[CS_ARGV_GPIO_DEV]);
		return -ENODEV;
	}

	int pin = strtol(argv[CS_ARGV_GPIO_PIN], &endptr, 10);

	if (endptr == argv[CS_ARGV_GPIO_PIN] || (pin < 0)) {
		shell_error(ctx, "invalid pin number: %s", argv[CS_ARGV_GPIO_PIN]);
		return -EINVAL;
	}

	spec->config.cs.gpio.port = dev;
	spec->config.cs.gpio.pin = pin;

	/* Include flags if provided */
	if (argc == (CS_ARGV_GPIO_FLAGS + 1)) {
		uint32_t flags = strtol(argv[CS_ARGV_GPIO_FLAGS], &endptr, 16);

		if (endptr == argv[CS_ARGV_GPIO_FLAGS]) {
			shell_error(ctx, "invalid gpio flags: %s", argv[CS_ARGV_GPIO_FLAGS]);
			return -EINVAL;
		}

		spec->config.cs.gpio.dt_flags = flags;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_spi_cmds,
	SHELL_CMD_ARG(conf, &dsub_get_spi_shell_device_name,
		      "Configure SPI\n"
		      "Usage: spi conf <spi-device> <frequency> [<settings>]\n"
		      "<settings> - any sequence of letters:\n"
		      "o - SPI_MODE_CPOL\n"
		      "h - SPI_MODE_CPHA\n"
		      "l - SPI_TRANSFER_LSB\n"
		      "T - SPI_FRAME_FORMAT_TI\n"
		      "example: spi conf spi1 1000000 ol",
		      cmd_spi_conf, 3, 1),
	SHELL_CMD_ARG(cs, &dsub_get_spi_shell_device_name_and_set_gpio_dsub,
		      "Assign CS GPIO to SPI device\n"
		      "Usage: spi cs <spi-device> <gpio-device> <pin> [<gpio flags>]\n"
		      "example: spi cs spi1 gpio1 3 0x01",
		      cmd_spi_conf_cs, 4, 1),
	SHELL_CMD_ARG(transceive, &dsub_get_spi_shell_device_name,
		      "Transceive data to and from an SPI device\n"
		      "Usage: spi transceive <spi-device> <TX byte 1> [<TX byte 2> ...]\n"
		      "example: spi transceive spi1 0x00 0x01",
		      cmd_spi_transceive, 3, MAX_SPI_BYTES - 1),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(spi, &sub_spi_cmds, "SPI commands", NULL);
