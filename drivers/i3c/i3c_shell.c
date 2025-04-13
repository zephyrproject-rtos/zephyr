/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i3c.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i3c_shell, CONFIG_LOG_DEFAULT_LEVEL);

#define MAX_BYTES_FOR_REGISTER_INDEX 4
#define ARGV_DEV                     1
#define ARGV_TDEV                    2
#define ARGV_REG                     3

/* Maximum bytes we can write or read at once */
#define MAX_I3C_BYTES 16

struct i3c_ctrl {
	const struct device *dev;
	const union shell_cmd_entry *i3c_attached_dev_subcmd;
	const union shell_cmd_entry *i3c_list_dev_subcmd;
};

#define I3C_ATTACHED_DEV_GET_FN(node_id)                                                           \
	static void node_id##cmd_i3c_attached_get(size_t idx, struct shell_static_entry *entry);   \
                                                                                                   \
	SHELL_DYNAMIC_CMD_CREATE(node_id##sub_i3c_attached, node_id##cmd_i3c_attached_get);        \
                                                                                                   \
	static void node_id##cmd_i3c_attached_get(size_t idx, struct shell_static_entry *entry)    \
	{                                                                                          \
		const struct device *dev = DEVICE_DT_GET(node_id);                                 \
		struct i3c_device_desc *i3c_desc;                                                  \
		size_t cnt = 0;                                                                    \
                                                                                                   \
		entry->syntax = NULL;                                                              \
		entry->handler = NULL;                                                             \
		entry->subcmd = NULL;                                                              \
		entry->help = NULL;                                                                \
                                                                                                   \
		I3C_BUS_FOR_EACH_I3CDEV(dev, i3c_desc) {                                           \
			if (cnt == idx) {                                                          \
				entry->syntax = i3c_desc->dev->name;                               \
				return;                                                            \
			}                                                                          \
			cnt++;                                                                     \
		}                                                                                  \
	}

#define I3C_LIST_DEV_GET_FN(node_id)                                                               \
	static void node_id##cmd_i3c_list_get(size_t idx, struct shell_static_entry *entry);       \
                                                                                                   \
	SHELL_DYNAMIC_CMD_CREATE(node_id##sub_i3c_list, node_id##cmd_i3c_list_get);                \
                                                                                                   \
	static void node_id##cmd_i3c_list_get(size_t idx, struct shell_static_entry *entry)        \
	{                                                                                          \
		const struct device *dev = DEVICE_DT_GET(node_id);                                 \
		struct i3c_driver_config *config;                                                  \
                                                                                                   \
		entry->syntax = NULL;                                                              \
		entry->handler = NULL;                                                             \
		entry->subcmd = NULL;                                                              \
		entry->help = NULL;                                                                \
                                                                                                   \
		config = (struct i3c_driver_config *)dev->config;                                  \
		if (idx < config->dev_list.num_i3c) {                                              \
			entry->syntax = config->dev_list.i3c[idx].dev->name;                       \
		}                                                                                  \
	}

#define I3C_CTRL_FN(node_id)                                                                       \
	I3C_ATTACHED_DEV_GET_FN(node_id)                                                           \
	I3C_LIST_DEV_GET_FN(node_id)

/* zephyr-keep-sorted-start */
DT_FOREACH_STATUS_OKAY(cdns_i3c, I3C_CTRL_FN)
DT_FOREACH_STATUS_OKAY(nuvoton_npcx_i3c, I3C_CTRL_FN)
DT_FOREACH_STATUS_OKAY(nxp_mcux_i3c, I3C_CTRL_FN)
DT_FOREACH_STATUS_OKAY(snps_designware_i3c, I3C_CTRL_FN)
DT_FOREACH_STATUS_OKAY(st_stm32_i3c, I3C_CTRL_FN)
/* zephyr-keep-sorted-stop */

#define I3C_CTRL_LIST_ENTRY(node_id)                                                               \
	{                                                                                          \
		.dev = DEVICE_DT_GET(node_id),                                                     \
		.i3c_attached_dev_subcmd = &node_id##sub_i3c_attached,                             \
		.i3c_list_dev_subcmd = &node_id##sub_i3c_list,                                     \
	},

const struct i3c_ctrl i3c_list[] = {
	/* zephyr-keep-sorted-start */
	DT_FOREACH_STATUS_OKAY(cdns_i3c, I3C_CTRL_LIST_ENTRY)
	DT_FOREACH_STATUS_OKAY(nuvoton_npcx_i3c, I3C_CTRL_LIST_ENTRY)
	DT_FOREACH_STATUS_OKAY(nxp_mcux_i3c, I3C_CTRL_LIST_ENTRY)
	DT_FOREACH_STATUS_OKAY(snps_designware_i3c, I3C_CTRL_LIST_ENTRY)
	DT_FOREACH_STATUS_OKAY(st_stm32_i3c, I3C_CTRL_LIST_ENTRY)
	/* zephyr-keep-sorted-stop */
};

static int get_bytes_count_for_hex(char *arg)
{
	int length = (strlen(arg) + 1) / 2;

	if (length > 1 && arg[0] == '0' && (arg[1] == 'x' || arg[1] == 'X')) {
		length -= 1;
	}

	return MIN(MAX_BYTES_FOR_REGISTER_INDEX, length);
}

static struct i3c_device_desc *get_i3c_list_desc_from_dev_name(const struct device *dev,
							       const char *tdev_name)
{
	struct i3c_driver_config *config;
	uint8_t i;

	config = (struct i3c_driver_config *)dev->config;
	for (i = 0; i < config->dev_list.num_i3c; i++) {
		if (strcmp(config->dev_list.i3c[i].dev->name, tdev_name) == 0) {
			/* only look for a device with the same name */
			return &config->dev_list.i3c[i];
		}
	}

	return NULL;
}

static struct i3c_device_desc *get_i3c_attached_desc_from_dev_name(const struct device *dev,
								   const char *tdev_name)
{
	struct i3c_device_desc *i3c_desc;

	I3C_BUS_FOR_EACH_I3CDEV(dev, i3c_desc) {
		/* only look for a device with the same name */
		if (strcmp(i3c_desc->dev->name, tdev_name) == 0) {
			return i3c_desc;
		}
	}

	return NULL;
}

static int i3c_parse_args(const struct shell *sh, char **argv, const struct device **dev,
			  const struct device **tdev, struct i3c_device_desc **desc)
{
	*dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!*dev) {
		shell_error(sh, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}
	*tdev = shell_device_get_binding(argv[ARGV_TDEV]);
	if (!*tdev) {
		shell_error(sh, "I3C: Device driver %s not found.", argv[ARGV_TDEV]);
		return -ENODEV;
	}
	*desc = get_i3c_attached_desc_from_dev_name(*dev, (*tdev)->name);
	if (!*desc) {
		shell_error(sh, "I3C: Device %s not attached to bus.", (*tdev)->name);
		return -ENODEV;
	}

	return 0;
}

/* i3c info <device> [<target>] */
static int cmd_i3c_info(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_driver_data *data;
	struct i3c_device_desc *desc;
	struct i3c_i2c_device_desc *i2c_desc;
	bool found = false;

	dev = shell_device_get_binding(argv[ARGV_DEV]);

	if (!dev) {
		shell_error(sh, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}
	data = (struct i3c_driver_data *)dev->data;

	if (argc == 3) {
		tdev = shell_device_get_binding(argv[ARGV_TDEV]);
		if (!tdev) {
			shell_error(sh, "I3C: Target Device driver %s not found.", argv[ARGV_TDEV]);
			return -ENODEV;
		}
		if (!sys_slist_is_empty(&data->attached_dev.devices.i3c)) {
			I3C_BUS_FOR_EACH_I3CDEV(dev, desc) {
				/* only look for a device with the same name */
				if (strcmp(desc->dev->name, tdev->name) == 0) {
					shell_print(sh,
						    "name: %s\n"
						    "\tpid: 0x%012llx\n"
						    "\tstatic_addr: 0x%02x\n"
						    "\tdynamic_addr: 0x%02x\n"
						    "\tbcr: 0x%02x\n"
						    "\tdcr: 0x%02x\n"
						    "\tmaxrd: 0x%02x\n"
						    "\tmaxwr: 0x%02x\n"
						    "\tmax_read_turnaround: 0x%06x\n"
						    "\tmrl: 0x%04x\n"
						    "\tmwl: 0x%04x\n"
						    "\tmax_ibi: 0x%02x\n"
						    "\tcrhdly1: 0x%02x\n"
						    "\tgetcaps: 0x%02x; 0x%02x; 0x%02x; 0x%02x\n"
						    "\tcrcaps: 0x%02x; 0x%02x",
						    desc->dev->name, (uint64_t)desc->pid,
						    desc->static_addr, desc->dynamic_addr,
						    desc->bcr, desc->dcr, desc->data_speed.maxrd,
						    desc->data_speed.maxwr,
						    desc->data_speed.max_read_turnaround,
						    desc->data_length.mrl, desc->data_length.mwl,
						    desc->data_length.max_ibi, desc->crhdly1,
						    desc->getcaps.getcap1, desc->getcaps.getcap2,
						    desc->getcaps.getcap3, desc->getcaps.getcap4,
						    desc->crcaps.crcaps1, desc->crcaps.crcaps2);
					found = true;
					break;
				}
			}
		} else {
			shell_print(sh, "I3C: No devices found.");
			return -ENODEV;
		}
		if (found == false) {
			shell_error(sh, "I3C: Target device not found.");
			return -ENODEV;
		}
	} else if (argc == 2) {
		/* This gets all "currently attached" I3C and I2C devices */
		if (!sys_slist_is_empty(&data->attached_dev.devices.i3c)) {
			shell_print(sh, "I3C: Devices found:");
			I3C_BUS_FOR_EACH_I3CDEV(dev, desc) {
				shell_print(sh,
					    "name: %s\n"
					    "\tpid: 0x%012llx\n"
					    "\tstatic_addr: 0x%02x\n"
					    "\tdynamic_addr: 0x%02x\n"
					    "\tbcr: 0x%02x\n"
					    "\tdcr: 0x%02x\n"
					    "\tmaxrd: 0x%02x\n"
					    "\tmaxwr: 0x%02x\n"
					    "\tmax_read_turnaround: 0x%06x\n"
					    "\tmrl: 0x%04x\n"
					    "\tmwl: 0x%04x\n"
					    "\tmax_ibi: 0x%02x\n"
					    "\tcrhdly1: 0x%02x\n"
					    "\tgetcaps: 0x%02x; 0x%02x; 0x%02x; 0x%02x\n"
					    "\tcrcaps: 0x%02x; 0x%02x",
					    desc->dev->name, (uint64_t)desc->pid, desc->static_addr,
					    desc->dynamic_addr,
					    desc->bcr, desc->dcr, desc->data_speed.maxrd,
					    desc->data_speed.maxwr,
					    desc->data_speed.max_read_turnaround,
					    desc->data_length.mrl, desc->data_length.mwl,
					    desc->data_length.max_ibi, desc->crhdly1,
					    desc->getcaps.getcap1, desc->getcaps.getcap2,
					    desc->getcaps.getcap3, desc->getcaps.getcap4,
					    desc->crcaps.crcaps1, desc->crcaps.crcaps2);
			}
		} else {
			shell_print(sh, "I3C: No devices found.");
		}
		if (!sys_slist_is_empty(&data->attached_dev.devices.i2c)) {
			shell_print(sh, "I2C: Devices found:");
			I3C_BUS_FOR_EACH_I2CDEV(dev, i2c_desc) {
				shell_print(sh,
					    "addr: 0x%02x\n"
					    "\tlvr: 0x%02x",
					    i2c_desc->addr, i2c_desc->lvr);
			}
		} else {
			shell_print(sh, "I2C: No devices found.");
		}
	} else {
		shell_error(sh, "Invalid number of arguments.");
	}

	return 0;
}

/* i3c speed <device> <speed> */
static int cmd_i3c_speed(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	struct i3c_config_controller config;
	uint32_t speed;
	int ret;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "I3C: Device driver %s not found.", argv[1]);
		return -ENODEV;
	}

	speed = strtol(argv[ARGV_DEV + 1], NULL, 10);

	ret = i3c_config_get(dev, I3C_CONFIG_CONTROLLER, &config);
	if (ret != 0) {
		shell_error(sh, "I3C: Failed to retrieve configuration");
		return ret;
	}

	config.scl.i3c = speed;

	ret = i3c_configure(dev, I3C_CONFIG_CONTROLLER, &config);
	if (ret != 0) {
		shell_error(sh, "I3C: Failed to configure device");
		return ret;
	}

	return ret;
}

/* i3c recover <device> */
static int cmd_i3c_recover(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int err;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "I3C: Device driver %s not found.", argv[1]);
		return -ENODEV;
	}

	err = i3c_recover_bus(dev);
	if (err) {
		shell_error(sh, "I3C: Bus recovery failed (err %d)", err);
		return err;
	}

	return 0;
}

static int i3c_write_from_buffer(const struct shell *sh, char *s_dev_name, char *s_tdev_name,
				 char *s_reg_addr, char **data, uint8_t data_length)
{
	/* This buffer must preserve 4 bytes for register address, as it is
	 * filled using put_be32 function and we don't want to lower available
	 * space when using 1 byte address.
	 */
	uint8_t buf[MAX_I3C_BYTES + MAX_BYTES_FOR_REGISTER_INDEX - 1];
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	int reg_addr_bytes;
	int reg_addr;
	int ret;
	int i;

	dev = shell_device_get_binding(s_dev_name);
	if (!dev) {
		shell_error(sh, "I3C: Device driver %s not found.", s_dev_name);
		return -ENODEV;
	}
	tdev = shell_device_get_binding(s_tdev_name);
	if (!tdev) {
		shell_error(sh, "I3C: Device driver %s not found.", s_tdev_name);
		return -ENODEV;
	}
	desc = get_i3c_attached_desc_from_dev_name(dev, tdev->name);
	if (!desc) {
		shell_error(sh, "I3C: Device %s not attached to bus.", tdev->name);
		return -ENODEV;
	}

	reg_addr = strtol(s_reg_addr, NULL, 16);

	reg_addr_bytes = get_bytes_count_for_hex(s_reg_addr);
	sys_put_be32(reg_addr, buf);

	if (data_length + reg_addr_bytes > MAX_I3C_BYTES) {
		data_length = MAX_I3C_BYTES - reg_addr_bytes;
		shell_info(sh, "Too many bytes provided, limit is %d",
			   MAX_I3C_BYTES - reg_addr_bytes);
	}

	for (i = 0; i < data_length; i++) {
		buf[MAX_BYTES_FOR_REGISTER_INDEX + i] = (uint8_t)strtol(data[i], NULL, 16);
	}

	ret = i3c_write(desc, buf + MAX_BYTES_FOR_REGISTER_INDEX - reg_addr_bytes,
			reg_addr_bytes + data_length);
	if (ret < 0) {
		shell_error(sh, "Failed to write to device: %s", tdev->name);
		return -EIO;
	}

	return 0;
}

/* i3c write <device> <dev_addr> <reg_addr> [<byte1>, ...] */
static int cmd_i3c_write(const struct shell *sh, size_t argc, char **argv)
{
	return i3c_write_from_buffer(sh, argv[ARGV_DEV], argv[ARGV_TDEV], argv[ARGV_REG], &argv[4],
				     argc - 4);
}

/* i3c write_byte <device> <dev_addr> <reg_addr> <value> */
static int cmd_i3c_write_byte(const struct shell *sh, size_t argc, char **argv)
{
	return i3c_write_from_buffer(sh, argv[ARGV_DEV], argv[ARGV_TDEV], argv[ARGV_REG], &argv[4],
				     1);
}

static int i3c_read_to_buffer(const struct shell *sh, char *s_dev_name, char *s_tdev_name,
			      char *s_reg_addr, uint8_t *buf, uint8_t buf_length)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	uint8_t reg_addr_buf[MAX_BYTES_FOR_REGISTER_INDEX];
	int reg_addr_bytes;
	int reg_addr;
	int ret;

	dev = shell_device_get_binding(s_dev_name);
	if (!dev) {
		shell_error(sh, "I3C: Device driver %s not found.", s_dev_name);
		return -ENODEV;
	}
	tdev = shell_device_get_binding(s_tdev_name);
	if (!tdev) {
		shell_error(sh, "I3C: Device driver %s not found.", s_dev_name);
		return -ENODEV;
	}
	desc = get_i3c_attached_desc_from_dev_name(dev, tdev->name);
	if (!desc) {
		shell_error(sh, "I3C: Device %s not attached to bus.", tdev->name);
		return -ENODEV;
	}

	reg_addr = strtol(s_reg_addr, NULL, 16);

	reg_addr_bytes = get_bytes_count_for_hex(s_reg_addr);
	sys_put_be32(reg_addr, reg_addr_buf);

	ret = i3c_write_read(desc, reg_addr_buf + MAX_BYTES_FOR_REGISTER_INDEX - reg_addr_bytes,
			     reg_addr_bytes, buf, buf_length);
	if (ret < 0) {
		shell_error(sh, "Failed to read from device: %s", tdev->name);
		return -EIO;
	}

	return 0;
}

/* i3c read_byte <device> <target> <reg_addr> */
static int cmd_i3c_read_byte(const struct shell *sh, size_t argc, char **argv)
{
	uint8_t out;
	int ret;

	ret = i3c_read_to_buffer(sh, argv[ARGV_DEV], argv[ARGV_TDEV], argv[ARGV_REG], &out, 1);
	if (ret == 0) {
		shell_print(sh, "Output: 0x%x", out);
	}

	return ret;
}

/* i3c read <device> <target> <reg_addr> [<numbytes>] */
static int cmd_i3c_read(const struct shell *sh, size_t argc, char **argv)
{
	uint8_t buf[MAX_I3C_BYTES];
	int num_bytes;
	int ret;

	if (argc > 4) {
		num_bytes = strtol(argv[4], NULL, 16);
		if (num_bytes > MAX_I3C_BYTES) {
			num_bytes = MAX_I3C_BYTES;
		}
	} else {
		num_bytes = MAX_I3C_BYTES;
	}

	ret = i3c_read_to_buffer(sh, argv[ARGV_DEV], argv[ARGV_TDEV], argv[ARGV_REG], buf,
				 num_bytes);
	if (ret == 0) {
		shell_hexdump(sh, buf, num_bytes);
	}

	return ret;
}

/* i3c hdr ddr read <device> <target> <7b cmd> [<byte1>, ...] */
static int cmd_i3c_hdr_ddr_write(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	uint8_t buf[MAX_I3C_BYTES];
	uint8_t cmd;
	uint8_t data_length;
	uint8_t i;
	int ret;

	ret = i3c_parse_args(sh, argv, &dev, &tdev, &desc);
	if (ret != 0) {
		return ret;
	}

	cmd = strtol(argv[3], NULL, 16);

	data_length = argc - 4;
	if (data_length > MAX_I3C_BYTES) {
		shell_info(sh, "Too many bytes provided, limit is %d", MAX_I3C_BYTES);
	}

	for (i = 0; i < data_length; i++) {
		buf[i] = (uint8_t)strtol(argv[4 + i], NULL, 16);
	}

	ret = i3c_hdr_ddr_write(desc, cmd, buf, data_length);
	if (ret != 0) {
		shell_error(sh, "I3C: unable to perform HDR DDR write.");
		return ret;
	}

	return ret;
}

/* i3c hdr ddr read <device> <target> <7b cmd> [<numbytes>] */
static int cmd_i3c_hdr_ddr_read(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	uint8_t buf[MAX_I3C_BYTES];
	int num_bytes;
	uint8_t cmd;
	int ret;

	ret = i3c_parse_args(sh, argv, &dev, &tdev, &desc);
	if (ret != 0) {
		return ret;
	}

	cmd = strtol(argv[3], NULL, 16);

	if (argc > 4) {
		num_bytes = strtol(argv[4], NULL, 16);
		if (num_bytes > MAX_I3C_BYTES) {
			num_bytes = MAX_I3C_BYTES;
		}
	} else {
		num_bytes = MAX_I3C_BYTES;
	}

	ret = i3c_hdr_ddr_read(desc, cmd, buf, num_bytes);
	if (ret != 0) {
		shell_error(sh, "I3C: unable to perform HDR DDR read.");
		return ret;
	}

	shell_hexdump(sh, buf, num_bytes);

	return ret;
}

/* i3c ccc rstdaa <device> */
static int cmd_i3c_ccc_rstdaa(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	struct i3c_device_desc *desc;
	int ret;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}

	ret = i3c_ccc_do_rstdaa_all(dev);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC RSTDAA.");
		return ret;
	}

	/* reset all devices DA */
	I3C_BUS_FOR_EACH_I3CDEV(dev, desc) {
		desc->dynamic_addr = 0;
		shell_print(sh, "Reset dynamic address for device %s", desc->dev->name);
	}

	return ret;
}

/* i3c ccc entdaa <device> */
static int cmd_i3c_ccc_entdaa(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}

	return i3c_do_daa(dev);
}

/* i3c ccc setaasa <device> */
static int cmd_i3c_ccc_setaasa(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	struct i3c_device_desc *desc;
	int ret;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}

	ret = i3c_ccc_do_setaasa_all(dev);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC SETAASA.");
		return ret;
	}

	/* set all devices DA to SA */
	I3C_BUS_FOR_EACH_I3CDEV(dev, desc) {
		if ((desc->supports_setaasa) && (desc->dynamic_addr == 0) &&
		    (desc->static_addr != 0)) {
			desc->dynamic_addr = desc->static_addr;
		}
	}

	return ret;
}

/* i3c ccc setdasa <device> <target> <dynamic address> */
static int cmd_i3c_ccc_setdasa(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	struct i3c_driver_data *data;
	struct i3c_ccc_address da;
	uint8_t dynamic_addr;
	int ret;

	ret = i3c_parse_args(sh, argv, &dev, &tdev, &desc);
	if (ret != 0) {
		return ret;
	}

	data = (struct i3c_driver_data *)dev->data;
	dynamic_addr = strtol(argv[3], NULL, 16);
	da.addr = dynamic_addr << 1;
	/* check if the addressed is free */
	if (!i3c_addr_slots_is_free(&data->attached_dev.addr_slots, dynamic_addr)) {
		shell_error(sh, "I3C: Address 0x%02x is already in use.", dynamic_addr);
		return -EINVAL;
	}
	ret = i3c_ccc_do_setdasa(desc, da);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC SETDASA.");
		return ret;
	}

	/* update the target's dynamic address */
	desc->dynamic_addr = dynamic_addr;
	if (desc->dynamic_addr != desc->static_addr) {
		ret = i3c_reattach_i3c_device(desc, desc->static_addr);
		if (ret < 0) {
			shell_error(sh, "I3C: unable to reattach device");
			return ret;
		}
	}

	return ret;
}

/* i3c ccc setnewda <device> <target> <dynamic address> */
static int cmd_i3c_ccc_setnewda(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	struct i3c_driver_data *data;
	struct i3c_ccc_address new_da;
	uint8_t dynamic_addr;
	uint8_t old_da;
	int ret;

	ret = i3c_parse_args(sh, argv, &dev, &tdev, &desc);
	if (ret != 0) {
		return ret;
	}

	data = (struct i3c_driver_data *)dev->data;
	dynamic_addr = strtol(argv[3], NULL, 16);
	new_da.addr = dynamic_addr << 1;
	/* check if the addressed is free */
	if (!i3c_addr_slots_is_free(&data->attached_dev.addr_slots, dynamic_addr)) {
		shell_error(sh, "I3C: Address 0x%02x is already in use.", dynamic_addr);
		return -EINVAL;
	}

	ret = i3c_ccc_do_setnewda(desc, new_da);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC SETDASA.");
		return ret;
	}

	/* reattach device address */
	old_da = desc->dynamic_addr;
	desc->dynamic_addr = dynamic_addr;
	ret = i3c_reattach_i3c_device(desc, old_da);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to reattach device");
		return ret;
	}

	return ret;
}

/* i3c ccc getbcr <device> <target> */
static int cmd_i3c_ccc_getbcr(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	struct i3c_ccc_getbcr bcr;
	int ret;

	ret = i3c_parse_args(sh, argv, &dev, &tdev, &desc);
	if (ret != 0) {
		return ret;
	}

	ret = i3c_ccc_do_getbcr(desc, &bcr);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC GETBCR.");
		return ret;
	}

	shell_print(sh, "BCR: 0x%02x", bcr.bcr);
	desc->bcr = bcr.bcr;

	return ret;
}

/* i3c ccc getdcr <device> <target> */
static int cmd_i3c_ccc_getdcr(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	struct i3c_ccc_getdcr dcr;
	int ret;

	ret = i3c_parse_args(sh, argv, &dev, &tdev, &desc);
	if (ret != 0) {
		return ret;
	}

	ret = i3c_ccc_do_getdcr(desc, &dcr);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC GETDCR.");
		return ret;
	}

	shell_print(sh, "DCR: 0x%02x", dcr.dcr);
	desc->dcr = dcr.dcr;

	return ret;
}

/* i3c ccc getpid <device> <target> */
static int cmd_i3c_ccc_getpid(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	struct i3c_ccc_getpid pid;
	int ret;

	ret = i3c_parse_args(sh, argv, &dev, &tdev, &desc);
	if (ret != 0) {
		return ret;
	}

	ret = i3c_ccc_do_getpid(desc, &pid);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC GETPID.");
		return ret;
	}

	shell_print(sh, "PID: 0x%012llx", sys_get_be48(pid.pid));

	return ret;
}

/* i3c ccc getmrl <device> <target> */
static int cmd_i3c_ccc_getmrl(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	struct i3c_ccc_mrl mrl;
	int ret;

	ret = i3c_parse_args(sh, argv, &dev, &tdev, &desc);
	if (ret != 0) {
		return ret;
	}

	ret = i3c_ccc_do_getmrl(desc, &mrl);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC GETMRL.");
		return ret;
	}

	desc->data_length.mrl = mrl.len;
	if (desc->bcr & I3C_BCR_IBI_PAYLOAD_HAS_DATA_BYTE) {
		shell_print(sh, "MRL: 0x%04x; IBI Length:0x%02x", mrl.len, mrl.ibi_len);
		desc->data_length.max_ibi = mrl.ibi_len;
	} else {
		shell_print(sh, "MRL: 0x%04x", mrl.len);
		desc->data_length.max_ibi = 0;
	}

	return ret;
}

/* i3c ccc getmwl <device> <target> */
static int cmd_i3c_ccc_getmwl(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	struct i3c_ccc_mwl mwl;
	int ret;

	ret = i3c_parse_args(sh, argv, &dev, &tdev, &desc);
	if (ret != 0) {
		return ret;
	}

	ret = i3c_ccc_do_getmwl(desc, &mwl);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC GETMWL.");
		return ret;
	}

	shell_print(sh, "MWL: 0x%04x", mwl.len);
	desc->data_length.mwl = mwl.len;

	return ret;
}

/* i3c ccc setmrl <device> <target> <max read length> [<max ibi length>] */
static int cmd_i3c_ccc_setmrl(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	struct i3c_ccc_mrl mrl;
	int ret;

	ret = i3c_parse_args(sh, argv, &dev, &tdev, &desc);
	if (ret != 0) {
		return ret;
	}

	/* IBI length is required if the ibi payload bit is set */
	if ((desc->bcr & I3C_BCR_IBI_PAYLOAD_HAS_DATA_BYTE) && (argc < 5)) {
		shell_error(sh, "I3C: Missing IBI length.");
		return -EINVAL;
	}

	mrl.len = strtol(argv[3], NULL, 16);
	if (argc > 4) {
		mrl.ibi_len = strtol(argv[4], NULL, 16);
	}

	ret = i3c_ccc_do_setmrl(desc, &mrl);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC SETMRL.");
		return ret;
	}

	desc->data_length.mrl = mrl.len;
	if (argc > 4) {
		desc->data_length.max_ibi = mrl.ibi_len;
	}

	return ret;
}

/* i3c ccc setmwl <device> <target> <max write length> */
static int cmd_i3c_ccc_setmwl(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	struct i3c_ccc_mwl mwl;
	int ret;

	ret = i3c_parse_args(sh, argv, &dev, &tdev, &desc);
	if (ret != 0) {
		return ret;
	}

	mwl.len = strtol(argv[3], NULL, 16);

	ret = i3c_ccc_do_setmwl(desc, &mwl);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC SETMWL.");
		return ret;
	}

	desc->data_length.mwl = mwl.len;

	return ret;
}

/* i3c ccc setmrl_bc <device> <max read length> [<max ibi length>] */
static int cmd_i3c_ccc_setmrl_bc(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	struct i3c_device_desc *desc;
	struct i3c_ccc_mrl mrl;
	int ret;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}

	mrl.len = strtol(argv[2], NULL, 16);
	if (argc > 3) {
		mrl.ibi_len = strtol(argv[3], NULL, 16);
	}

	ret = i3c_ccc_do_setmrl_all(dev, &mrl, argc > 3);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC SETMRL BC.");
		return ret;
	}

	I3C_BUS_FOR_EACH_I3CDEV(dev, desc) {
		desc->data_length.mrl = mrl.len;
		if ((argc > 3) && (desc->bcr & I3C_BCR_IBI_PAYLOAD_HAS_DATA_BYTE)) {
			desc->data_length.max_ibi = mrl.ibi_len;
		}
	}

	return ret;
}

/* i3c ccc setmwl_bc <device> <max write length> */
static int cmd_i3c_ccc_setmwl_bc(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	struct i3c_device_desc *desc;
	struct i3c_ccc_mwl mwl;
	int ret;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}

	mwl.len = strtol(argv[2], NULL, 16);

	ret = i3c_ccc_do_setmwl_all(dev, &mwl);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC SETMWL BC.");
		return ret;
	}

	I3C_BUS_FOR_EACH_I3CDEV(dev, desc) {
		desc->data_length.mwl = mwl.len;
	}

	return ret;
}

/* i3c ccc deftgts <device> */
static int cmd_i3c_ccc_deftgts(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int ret;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}

	if (!i3c_bus_has_sec_controller(dev)) {
		shell_error(sh, "I3C: No secondary controller on the bus");
		return -ENXIO;
	}

	ret = i3c_bus_deftgts(dev);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC DEFTGTS.");
		return ret;
	}

	return ret;
}

/* i3c ccc enttm <device> <defining byte> */
static int cmd_i3c_ccc_enttm(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	enum i3c_ccc_enttm_defbyte defbyte;
	int ret;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}

	defbyte = strtol(argv[2], NULL, 16);

	ret = i3c_ccc_do_enttm(dev, defbyte);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC ENTTM.");
		return ret;
	}

	return ret;
}

/* i3c ccc getacccr <device> <target> */
static int cmd_i3c_ccc_getacccr(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	struct i3c_ccc_address handoff_address;
	int ret;

	ret = i3c_parse_args(sh, argv, &dev, &tdev, &desc);
	if (ret != 0) {
		return ret;
	}

	if (!i3c_device_is_controller_capable(desc)) {
		shell_error(sh, "I3C: Not a Controller Capable Device");
		return -EINVAL;
	}

	ret = i3c_ccc_do_getacccr(desc, &handoff_address);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC GETACCCR.");
		return ret;
	}

	/* Verify Odd Parity and Correct Dynamic Address Reply */
	if ((i3c_odd_parity(handoff_address.addr >> 1) != (handoff_address.addr & BIT(0))) ||
	    (handoff_address.addr >> 1 != desc->dynamic_addr)) {
		shell_error(sh, "I3C: invalid returned address 0x%02x; expected 0x%02x",
			    handoff_address.addr, desc->dynamic_addr);
		return -EIO;
	}

	shell_print(sh, "I3C: Controller Handoff successful");

	return ret;
}

/* i3c ccc rstact_bc <device> <defining byte> */
static int cmd_i3c_ccc_rstact_bc(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	enum i3c_ccc_rstact_defining_byte action;
	int ret;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}

	action = strtol(argv[2], NULL, 16);

	ret = i3c_ccc_do_rstact_all(dev, action);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC RSTACT BC.");
		return ret;
	}

	return ret;
}

/* i3c ccc rstact <device> <target> <"set"/"get"> <defining byte> */
static int cmd_i3c_ccc_rstact(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	enum i3c_ccc_rstact_defining_byte action;
	int ret;
	uint8_t data;

	ret = i3c_parse_args(sh, argv, &dev, &tdev, &desc);
	if (ret != 0) {
		return ret;
	}

	action = strtol(argv[4], NULL, 16);

	if (strcmp(argv[3], "get") == 0) {
		ret = i3c_ccc_do_rstact_fmt3(desc, action, &data);
	} else if (strcmp(argv[3], "set") == 0) {
		ret = i3c_ccc_do_rstact_fmt2(desc, action);
	} else {
		shell_error(sh, "I3C: invalid parameter");
		return -EINVAL;
	}

	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC RSTACT.");
		return ret;
	}

	if (action >= 0x80) {
		shell_print(sh, "RSTACT Returned Data: 0x%02x", data);
	}

	return ret;
}


/* i3c ccc enec_bc <device> <defining byte> */
static int cmd_i3c_ccc_enec_bc(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	struct i3c_ccc_events events;
	int ret;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}

	events.events = strtol(argv[2], NULL, 16);

	ret = i3c_ccc_do_events_all_set(dev, true, &events);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC ENEC BC.");
		return ret;
	}

	return ret;
}

/* i3c ccc disec_bc <device> <defining byte> */
static int cmd_i3c_ccc_disec_bc(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	struct i3c_ccc_events events;
	int ret;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}

	events.events = strtol(argv[2], NULL, 16);

	ret = i3c_ccc_do_events_all_set(dev, false, &events);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC DISEC BC.");
		return ret;
	}

	return ret;
}

/* i3c ccc enec <device> <target> <defining byte> */
static int cmd_i3c_ccc_enec(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	struct i3c_ccc_events events;
	int ret;

	ret = i3c_parse_args(sh, argv, &dev, &tdev, &desc);
	if (ret != 0) {
		return ret;
	}

	events.events = strtol(argv[3], NULL, 16);

	ret = i3c_ccc_do_events_set(desc, true, &events);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC ENEC.");
		return ret;
	}

	return ret;
}

/* i3c ccc disec <device> <target> <defining byte> */
static int cmd_i3c_ccc_disec(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	struct i3c_ccc_events events;
	int ret;

	ret = i3c_parse_args(sh, argv, &dev, &tdev, &desc);
	if (ret != 0) {
		return ret;
	}

	events.events = strtol(argv[3], NULL, 16);

	ret = i3c_ccc_do_events_set(desc, false, &events);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC DISEC.");
		return ret;
	}

	return ret;
}

/* i3c ccc entas0_bc <device> */
static int cmd_i3c_ccc_entas0_bc(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	struct i3c_driver_data *data;
	int ret;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}
	data = (struct i3c_driver_data *)dev->data;

	ret = i3c_ccc_do_entas0_all(dev);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC ENTAS0 BC.");
		return ret;
	}

	return ret;
}

/* i3c ccc entas1_bc <device> */
static int cmd_i3c_ccc_entas1_bc(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	struct i3c_driver_data *data;
	int ret;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}
	data = (struct i3c_driver_data *)dev->data;

	ret = i3c_ccc_do_entas1_all(dev);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC ENTAS1 BC.");
		return ret;
	}

	return ret;
}

/* i3c ccc entas2_bc <device> */
static int cmd_i3c_ccc_entas2_bc(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	struct i3c_driver_data *data;
	int ret;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}
	data = (struct i3c_driver_data *)dev->data;

	ret = i3c_ccc_do_entas2_all(dev);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC ENTAS2 BC.");
		return ret;
	}

	return ret;
}

/* i3c ccc entas3_bc <device> */
static int cmd_i3c_ccc_entas3_bc(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	struct i3c_driver_data *data;
	int ret;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}
	data = (struct i3c_driver_data *)dev->data;

	ret = i3c_ccc_do_entas3_all(dev);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC ENTAS3 BC.");
		return ret;
	}

	return ret;
}

/* i3c ccc entas0 <device> <target> */
static int cmd_i3c_ccc_entas0(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	int ret;

	ret = i3c_parse_args(sh, argv, &dev, &tdev, &desc);
	if (ret != 0) {
		return ret;
	}

	ret = i3c_ccc_do_entas0(desc);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC ENTAS0.");
		return ret;
	}

	return ret;
}

/* i3c ccc entas1 <device> <target> */
static int cmd_i3c_ccc_entas1(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	int ret;

	ret = i3c_parse_args(sh, argv, &dev, &tdev, &desc);
	if (ret != 0) {
		return ret;
	}

	ret = i3c_ccc_do_entas1(desc);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC ENTAS1.");
		return ret;
	}

	return ret;
}

/* i3c ccc entas2 <device> <target> */
static int cmd_i3c_ccc_entas2(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	int ret;

	ret = i3c_parse_args(sh, argv, &dev, &tdev, &desc);
	if (ret != 0) {
		return ret;
	}

	ret = i3c_ccc_do_entas2(desc);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC ENTAS2.");
		return ret;
	}

	return ret;
}

/* i3c ccc entas3 <device> <target> */
static int cmd_i3c_ccc_entas3(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	int ret;

	ret = i3c_parse_args(sh, argv, &dev, &tdev, &desc);
	if (ret != 0) {
		return ret;
	}

	ret = i3c_ccc_do_entas3(desc);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC ENTAS3.");
		return ret;
	}

	return ret;
}

/* i3c ccc getstatus <device> <target> [<defining byte>] */
static int cmd_i3c_ccc_getstatus(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	union i3c_ccc_getstatus status;
	enum i3c_ccc_getstatus_fmt fmt;
	enum i3c_ccc_getstatus_defbyte defbyte = GETSTATUS_FORMAT_2_INVALID;
	int ret;

	ret = i3c_parse_args(sh, argv, &dev, &tdev, &desc);
	if (ret != 0) {
		return ret;
	}

	/* If there is a defining byte, then it is assumed to be Format 2*/
	if (argc > 3) {
		fmt = GETSTATUS_FORMAT_2;
		defbyte = strtol(argv[3], NULL, 16);
		if (defbyte != GETSTATUS_FORMAT_2_TGTSTAT && defbyte != GETSTATUS_FORMAT_2_PRECR) {
			shell_error(sh, "Invalid defining byte.");
			return -EINVAL;
		}
	} else {
		fmt = GETSTATUS_FORMAT_1;
	}

	ret = i3c_ccc_do_getstatus(desc, &status, fmt, defbyte);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC GETSTATUS.");
		return ret;
	}

	if (fmt == GETSTATUS_FORMAT_2) {
		if (defbyte == GETSTATUS_FORMAT_2_TGTSTAT) {
			shell_print(sh, "TGTSTAT: 0x%04x", status.fmt2.tgtstat);
		} else if (defbyte == GETSTATUS_FORMAT_2_PRECR) {
			shell_print(sh, "PRECR: 0x%04x", status.fmt2.precr);
		}
	} else {
		shell_print(sh, "Status: 0x%04x", status.fmt1.status);
	}

	return ret;
}

/* i3c ccc getcaps <device> <target> [<defining byte>] */
static int cmd_i3c_ccc_getcaps(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	union i3c_ccc_getcaps caps;
	enum i3c_ccc_getcaps_fmt fmt;
	enum i3c_ccc_getcaps_defbyte defbyte = GETCAPS_FORMAT_2_INVALID;
	int ret;

	ret = i3c_parse_args(sh, argv, &dev, &tdev, &desc);
	if (ret != 0) {
		return ret;
	}

	/* If there is a defining byte, then it is assumed to be Format 2 */
	if (argc > 3) {
		fmt = GETCAPS_FORMAT_2;
		defbyte = strtol(argv[3], NULL, 16);
		if (defbyte != GETCAPS_FORMAT_2_TGTCAPS && defbyte != GETCAPS_FORMAT_2_TESTPAT &&
		    defbyte != GETCAPS_FORMAT_2_CRCAPS && defbyte != GETCAPS_FORMAT_2_VTCAPS &&
		    defbyte != GETCAPS_FORMAT_2_DBGCAPS) {
			shell_error(sh, "Invalid defining byte.");
			return -EINVAL;
		}
	} else {
		fmt = GETCAPS_FORMAT_1;
	}

	ret = i3c_ccc_do_getcaps(desc, &caps, fmt, defbyte);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC GETCAPS.");
		return ret;
	}

	if (fmt == GETCAPS_FORMAT_2) {
		if (defbyte == GETCAPS_FORMAT_2_TGTCAPS) {
			shell_print(sh, "TGTCAPS: 0x%02x; 0x%02x; 0x%02x; 0x%02x",
				    caps.fmt2.tgtcaps[0], caps.fmt2.tgtcaps[1],
				    caps.fmt2.tgtcaps[2], caps.fmt2.tgtcaps[3]);
		} else if (defbyte == GETCAPS_FORMAT_2_TESTPAT) {
			shell_print(sh, "TESTPAT: 0x%08x", caps.fmt2.testpat);
		} else if (defbyte == GETCAPS_FORMAT_2_CRCAPS) {
			shell_print(sh, "CRCAPS: 0x%02x; 0x%02x", caps.fmt2.crcaps[0],
				    caps.fmt2.crcaps[1]);
			memcpy(&desc->crcaps, &caps, sizeof(desc->crcaps));
		} else if (defbyte == GETCAPS_FORMAT_2_VTCAPS) {
			shell_print(sh, "VTCAPS: 0x%02x; 0x%02x", caps.fmt2.vtcaps[0],
				    caps.fmt2.vtcaps[1]);
		}
	} else {
		shell_print(sh, "GETCAPS: 0x%02x; 0x%02x; 0x%02x; 0x%02x", caps.fmt1.getcaps[0],
			    caps.fmt1.getcaps[1], caps.fmt1.getcaps[2], caps.fmt1.getcaps[3]);
		memcpy(&desc->getcaps, &caps, sizeof(desc->getcaps));
	}

	return ret;
}

/* i3c ccc getvendor <device> <target> <id> [<defining byte>] */
static int cmd_i3c_ccc_getvendor(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	uint8_t buf[MAX_I3C_BYTES] = {0};
	uint8_t defbyte;
	size_t num_xfer;
	uint8_t id;
	int err = 0;
	int ret;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}

	tdev = shell_device_get_binding(argv[ARGV_TDEV]);
	if (!tdev) {
		shell_error(sh, "I3C: Device driver %s not found.", argv[ARGV_TDEV]);
		return -ENODEV;
	}
	desc = get_i3c_attached_desc_from_dev_name(dev, tdev->name);
	if (!desc) {
		shell_error(sh, "I3C: Device %s not attached to bus.", tdev->name);
		return -ENODEV;
	}

	id = (uint8_t)shell_strtoul(argv[3], 0, &err);
	if (err != 0) {
		shell_error(sh, "I3C: Invalid ID.");
		return -EINVAL;
	}

	if (argc > 4) {
		defbyte = strtol(argv[4], NULL, 16);
		ret = i3c_ccc_do_getvendor_defbyte(desc, id, defbyte, buf, MAX_I3C_BYTES,
						   &num_xfer);
	} else {
		ret = i3c_ccc_do_getvendor(desc, id, buf, MAX_I3C_BYTES, &num_xfer);
	}

	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC VENDOR.");
		return ret;
	}

	shell_hexdump(sh, buf, num_xfer);

	return ret;
}

/* i3c ccc setvendor <device> <target> <id> [<bytes>] */
static int cmd_i3c_ccc_setvendor(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	struct i3c_driver_data *data;
	uint8_t buf[MAX_I3C_BYTES] = {0};
	uint8_t data_length;
	uint8_t id;
	int err = 0;
	int ret;
	int i;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}

	tdev = shell_device_get_binding(argv[ARGV_TDEV]);
	if (!tdev) {
		shell_error(sh, "I3C: Device driver %s not found.", argv[ARGV_TDEV]);
		return -ENODEV;
	}
	desc = get_i3c_attached_desc_from_dev_name(dev, tdev->name);
	if (!desc) {
		shell_error(sh, "I3C: Device %s not attached to bus.", tdev->name);
		return -ENODEV;
	}
	data = (struct i3c_driver_data *)dev->data;

	id = (uint8_t)shell_strtoul(argv[3], 0, &err);
	if (err != 0) {
		shell_error(sh, "I3C: Invalid ID.");
		return -EINVAL;
	}

	data_length = argc - 4;
	for (i = 0; i < data_length; i++) {
		buf[i] = (uint8_t)strtol(argv[4 + i], NULL, 16);
	}

	ret = i3c_ccc_do_setvendor(desc, id, buf, data_length);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC VENDOR.");
		return ret;
	}

	return ret;
}

/* i3c ccc setvendor_bc <device> <id> [<bytes>] */
static int cmd_i3c_ccc_setvendor_bc(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uint8_t buf[MAX_I3C_BYTES] = {0};
	uint8_t data_length;
	uint8_t id;
	int err = 0;
	int ret;
	int i;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}

	id = (uint8_t)shell_strtoul(argv[2], 0, &err);
	if (err != 0) {
		shell_error(sh, "I3C: Invalid ID.");
		return -EINVAL;
	}

	data_length = argc - 3;
	for (i = 0; i < data_length; i++) {
		buf[i] = (uint8_t)strtol(argv[3 + i], NULL, 16);
	}

	ret = i3c_ccc_do_setvendor_all(dev, id, buf, data_length);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC VENDOR.");
		return ret;
	}

	return ret;
}

/* i3c ccc setbuscon <device> <context> [<optional bytes>] */
static int cmd_i3c_ccc_setbuscon(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uint8_t buf[MAX_I3C_BYTES] = {0};
	uint8_t data_length;
	int ret;
	int i;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}

	data_length = argc - 2;
	for (i = 0; i < data_length; i++) {
		buf[i] = (uint8_t)strtol(argv[2 + i], NULL, 16);
	}

	ret = i3c_ccc_do_setbuscon(dev, buf, data_length);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC SETBUSCON.");
		return ret;
	}

	return ret;
}

/* i3c ccc getmxds <device> <target> [<defining byte>] */
static int cmd_i3c_ccc_getmxds(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	union i3c_ccc_getmxds mxds;
	enum i3c_ccc_getmxds_fmt fmt;
	enum i3c_ccc_getmxds_defbyte defbyte = GETMXDS_FORMAT_3_INVALID;
	int ret;

	ret = i3c_parse_args(sh, argv, &dev, &tdev, &desc);
	if (ret != 0) {
		return ret;
	}

	if (!(desc->bcr & I3C_BCR_MAX_DATA_SPEED_LIMIT)) {
		shell_error(sh, "I3C: Device %s does not support max data speed limit",
			    desc->dev->name);
		return -ENOTSUP;
	}

	/* If there is a defining byte, then it is assumed to be Format 3 */
	if (argc > 3) {
		fmt = GETMXDS_FORMAT_3;
		defbyte = strtol(argv[3], NULL, 16);
		if (defbyte != GETMXDS_FORMAT_3_CRHDLY && defbyte != GETMXDS_FORMAT_3_WRRDTURN) {
			shell_error(sh, "Invalid defining byte.");
			return -EINVAL;
		}
	} else {
		fmt = GETMXDS_FORMAT_2;
	}

	ret = i3c_ccc_do_getmxds(desc, &mxds, fmt, defbyte);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to send CCC GETMXDS.");
		return ret;
	}

	if (fmt == GETMXDS_FORMAT_3) {
		if (defbyte == GETMXDS_FORMAT_3_WRRDTURN) {
			shell_print(sh, "WRRDTURN: maxwr 0x%02x; maxrd 0x%02x; maxrdturn 0x%06x",
				    mxds.fmt3.wrrdturn[0], mxds.fmt3.wrrdturn[1],
				    sys_get_le24(&mxds.fmt3.wrrdturn[2]));
			/* Update values in descriptor */
			desc->data_speed.maxwr = mxds.fmt3.wrrdturn[0];
			desc->data_speed.maxrd = mxds.fmt3.wrrdturn[1];
			desc->data_speed.max_read_turnaround = sys_get_le24(&mxds.fmt3.wrrdturn[2]);
		} else if (defbyte == GETMXDS_FORMAT_3_CRHDLY) {
			shell_print(sh, "CRHDLY1: 0x%02x", mxds.fmt3.crhdly1);
			desc->crhdly1 = mxds.fmt3.crhdly1;
		}
	} else {
		shell_print(sh, "GETMXDS: maxwr 0x%02x; maxrd 0x%02x; maxrdturn 0x%06x",
			    mxds.fmt2.maxwr, mxds.fmt2.maxrd, sys_get_le24(mxds.fmt2.maxrdturn));
		/* Update values in descriptor */
		desc->data_speed.maxwr = mxds.fmt2.maxwr;
		desc->data_speed.maxrd = mxds.fmt2.maxrd;
		desc->data_speed.max_read_turnaround = sys_get_le24(mxds.fmt2.maxrdturn);
	}

	return ret;
}

static int cmd_i3c_attach(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	int ret;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}
	tdev = shell_device_get_binding(argv[ARGV_TDEV]);
	if (!tdev) {
		shell_error(sh, "I3C: Device driver %s not found.", argv[ARGV_TDEV]);
		return -ENODEV;
	}
	desc = get_i3c_list_desc_from_dev_name(dev, tdev->name);
	if (!desc) {
		shell_error(sh, "I3C: Device %s not attached to bus.", tdev->name);
		return -ENODEV;
	}

	ret = i3c_attach_i3c_device(desc);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to attach device %s.", tdev->name);
	}

	return ret;
}

static int cmd_i3c_reattach(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	uint8_t old_dyn_addr = 0;
	int ret;

	ret = i3c_parse_args(sh, argv, &dev, &tdev, &desc);
	if (ret != 0) {
		return ret;
	}

	if (argc > 2) {
		old_dyn_addr = strtol(argv[2], NULL, 16);
	}

	ret = i3c_reattach_i3c_device(desc, old_dyn_addr);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to reattach device %s.", tdev->name);
	}

	return ret;
}

static int cmd_i3c_detach(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	int ret;

	ret = i3c_parse_args(sh, argv, &dev, &tdev, &desc);
	if (ret != 0) {
		return ret;
	}

	ret = i3c_detach_i3c_device(desc);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to detach device %s.", tdev->name);
	}

	return ret;
}

static int cmd_i3c_i2c_attach(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	struct i3c_i2c_device_desc *desc;
	uint16_t addr = 0;
	int ret;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}
	addr = strtol(argv[2], NULL, 16);
	desc = i3c_dev_list_i2c_addr_find(dev, addr);
	if (!desc) {
		shell_error(sh, "I3C: I2C addr 0x%02x not listed with the bus.", addr);
		return -ENODEV;
	}

	ret = i3c_attach_i2c_device(desc);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to attach I2C addr 0x%02x.", addr);
	}

	return ret;
}

static int cmd_i3c_i2c_detach(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	struct i3c_i2c_device_desc *desc;
	uint16_t addr = 0;
	int ret;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}
	addr = strtol(argv[2], NULL, 16);
	desc = i3c_dev_list_i2c_addr_find(dev, addr);
	if (!desc) {
		shell_error(sh, "I3C: I2C addr 0x%02x not listed with the bus.", addr);
		return -ENODEV;
	}

	ret = i3c_detach_i2c_device(desc);
	if (ret < 0) {
		shell_error(sh, "I3C: unable to detach I2C addr 0x%02x.", addr);
	}

	return ret;
}

/*
 * This is a workaround command to perform an I2C Scan which is not as
 * simple on an I3C bus as it is with the I2C Shell.
 *
 * This will print "I3" if an address is already assigned for an I3C
 * device and it will print "I2" if an address is already assigned for
 * an I2C device. It will print RS, if the address is reserved according
 * to section 5.1.2.2.5 I3C Target Address Restrictions in I3C v1.1.1.
 *
 * This sends I2C messages without any data (i.e. stop condition after
 * sending just the address). If there is an ACK for the address, it
 * is assumed there is a device present.
 *
 * WARNING: As there is no standard I2C detection command, this code
 * uses arbitrary SMBus commands (namely SMBus quick write and SMBus
 * receive byte) to probe for devices.  This operation can confuse
 * your I2C bus, cause data loss, and is known to corrupt the Atmel
 * AT24RF08 EEPROM found on many IBM Thinkpad laptops.
 *
 * https://manpages.debian.org/buster/i2c-tools/i2cdetect.8.en.html
 */
/* i3c i2c_scan <device> */
static int cmd_i3c_i2c_scan(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	struct i3c_driver_data *data;
	enum i3c_addr_slot_status slot;
	uint8_t cnt = 0, first = 0x04, last = 0x77;

	dev = shell_device_get_binding(argv[ARGV_DEV]);

	if (!dev) {
		shell_error(sh, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}

	data = (struct i3c_driver_data *)dev->data;

	shell_print(sh, "     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f");
	for (uint8_t i = 0; i <= last; i += 16) {
		shell_fprintf_normal(sh, "%02x: ", i);
		for (uint8_t j = 0; j < 16; j++) {
			if (i + j < first || i + j > last) {
				shell_fprintf_normal(sh, "   ");
				continue;
			}

			slot = i3c_addr_slots_status(&data->attached_dev.addr_slots, i + j);
			if (slot == I3C_ADDR_SLOT_STATUS_FREE) {
				struct i2c_msg msgs[1];
				uint8_t dst;
				int ret;
				struct i3c_i2c_device_desc desc = {
					.bus = dev,
					.addr = i + j,
					.lvr = 0x00,
				};

				ret = i3c_attach_i2c_device(&desc);
				if (ret < 0) {
					shell_error(sh, "I3C: unable to attach I2C addr 0x%02x.",
						    desc.addr);
				}

				/* Send the address to read from */
				msgs[0].buf = &dst;
				msgs[0].len = 0U;
				msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;
				if (i2c_transfer(dev, &msgs[0], 1, i + j) == 0) {
					shell_fprintf_normal(sh, "%02x ", i + j);
					++cnt;
				} else {
					shell_fprintf_normal(sh, "-- ");
				}

				ret = i3c_detach_i2c_device(&desc);
				if (ret < 0) {
					shell_error(sh, "I3C: unable to detach I2C addr 0x%02x.",
						    desc.addr);
				}
			} else if (slot == I3C_ADDR_SLOT_STATUS_I3C_DEV) {
				shell_fprintf_normal(sh, "I3 ");
			} else if (slot == I3C_ADDR_SLOT_STATUS_I2C_DEV) {
				shell_fprintf_normal(sh, "I2 ");
			} else if (slot == I3C_ADDR_SLOT_STATUS_RSVD) {
				shell_fprintf_normal(sh, "RS ");
			} else {
				shell_fprintf_normal(sh, "-- ");
			}
		}
		shell_print(sh, "");
	}

	shell_print(sh, "%u additional devices found on %s", cnt, argv[ARGV_DEV]);

	return 0;
}

#ifdef CONFIG_I3C_USE_IBI
/* i3c ibi hj_response <device> <"ack"/"nack"> */
static int cmd_i3c_ibi_hj_response(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	bool ack;
	int ret;

	dev = device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}

	if (strcmp(argv[2], "ack") == 0) {
		ack = true;
	} else if (strcmp(argv[2], "nack") == 0) {
		ack = false;
	} else {
		shell_error(sh, "I3C: invalid parameter");
		return -EINVAL;
	}

	ret = i3c_ibi_hj_response(dev, ack);
	if (ret != 0) {
		shell_error(sh, "I3C: Unable to set IBI HJ Response");
		return ret;
	}

	shell_print(sh, "I3C: Set IBI HJ Response");

	return 0;
}

/* i3c ibi hj <device> */
static int cmd_i3c_ibi_hj(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	struct i3c_ibi request;
	int ret;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}

	request.ibi_type = I3C_IBI_HOTJOIN;
	ret = i3c_ibi_raise(dev, &request);
	if (ret != 0) {
		shell_error(sh, "I3C: Unable to issue IBI HJ");
		return ret;
	}

	shell_print(sh, "I3C: Issued IBI HJ");

	return 0;
}

/* i3c ibi cr <device> */
static int cmd_i3c_ibi_cr(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	struct i3c_ibi request;
	int ret;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}

	request.ibi_type = I3C_IBI_CONTROLLER_ROLE_REQUEST;
	ret = i3c_ibi_raise(dev, &request);
	if (ret != 0) {
		shell_error(sh, "I3C: Unable to issue IBI CR");
		return ret;
	}

	shell_print(sh, "I3C: Issued IBI CR");

	return 0;
}

/* i3c ibi tir <device> [<bytes>]*/
static int cmd_i3c_ibi_tir(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	struct i3c_ibi request;
	uint16_t data_length;
	uint8_t buf[MAX_I3C_BYTES];
	int ret;
	uint8_t i;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}

	data_length = argc - 2;
	for (i = 0; i < data_length; i++) {
		buf[i] = (uint8_t)strtol(argv[2 + i], NULL, 16);
	}

	request.ibi_type = I3C_IBI_TARGET_INTR;
	request.payload = buf;
	request.payload_len = data_length;

	ret = i3c_ibi_raise(dev, &request);
	if (ret != 0) {
		shell_error(sh, "I3C: Unable to issue IBI TIR");
		return ret;
	}

	shell_print(sh, "I3C: Issued IBI TIR");

	return 0;
}

/* i3c ibi enable <device> <target> */
static int cmd_i3c_ibi_enable(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	int ret;

	ret = i3c_parse_args(sh, argv, &dev, &tdev, &desc);
	if (ret != 0) {
		return ret;
	}

	ret = i3c_ibi_enable(desc);
	if (ret != 0) {
		shell_error(sh, "I3C: Unable to enable IBI");
		return ret;
	}

	shell_print(sh, "I3C: Enabled IBI");

	return 0;
}

/* i3c ibi disable <device> <target> */
static int cmd_i3c_ibi_disable(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	int ret;

	ret = i3c_parse_args(sh, argv, &dev, &tdev, &desc);
	if (ret != 0) {
		return ret;
	}

	ret = i3c_ibi_disable(desc);
	if (ret != 0) {
		shell_error(sh, "I3C: Unable to disable IBI");
		return ret;
	}

	shell_print(sh, "I3C: Disabled IBI");

	return 0;
}
#endif

static void i3c_device_list_target_name_get(size_t idx, struct shell_static_entry *entry)
{
	if (idx < ARRAY_SIZE(i3c_list)) {
		entry->syntax = i3c_list[idx].dev->name;
		entry->handler = NULL;
		entry->help = NULL;
		entry->subcmd = i3c_list[idx].i3c_list_dev_subcmd;
	} else {
		entry->syntax = NULL;
	}
}

SHELL_DYNAMIC_CMD_CREATE(dsub_i3c_device_list_name, i3c_device_list_target_name_get);

static void i3c_device_attached_target_name_get(size_t idx, struct shell_static_entry *entry)
{
	if (idx < ARRAY_SIZE(i3c_list)) {
		entry->syntax = i3c_list[idx].dev->name;
		entry->handler = NULL;
		entry->help = NULL;
		entry->subcmd = i3c_list[idx].i3c_attached_dev_subcmd;
	} else {
		entry->syntax = NULL;
	}
}

SHELL_DYNAMIC_CMD_CREATE(dsub_i3c_device_attached_name, i3c_device_attached_target_name_get);

static void i3c_device_name_get(size_t idx, struct shell_static_entry *entry)
{
	if (idx < ARRAY_SIZE(i3c_list)) {
		entry->syntax = i3c_list[idx].dev->name;
		entry->handler = NULL;
		entry->help = NULL;
		entry->subcmd = NULL;
	} else {
		entry->syntax = NULL;
	}
}

SHELL_DYNAMIC_CMD_CREATE(dsub_i3c_device_name, i3c_device_name_get);

#ifdef CONFIG_I3C_USE_IBI
/* L2 I3C IBI Shell Commands*/
SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_i3c_ibi_cmds,
	SHELL_CMD_ARG(hj_response, &dsub_i3c_device_name,
		      "Set IBI HJ Response\n"
		      "Usage: ibi hj_response <device> <\"ack\"/\"nack\">",
		      cmd_i3c_ibi_hj_response, 3, 0),
	SHELL_CMD_ARG(hj, &dsub_i3c_device_name,
		      "Send IBI HJ\n"
		      "Usage: ibi hj <device>",
		      cmd_i3c_ibi_hj, 2, 0),
	SHELL_CMD_ARG(tir, &dsub_i3c_device_name,
		      "Send IBI TIR\n"
		      "Usage: ibi tir <device> [<byte1>, ...]",
		      cmd_i3c_ibi_tir, 2, MAX_I3C_BYTES),
	SHELL_CMD_ARG(cr, &dsub_i3c_device_name,
		      "Send IBI CR\n"
		      "Usage: ibi cr <device>",
		      cmd_i3c_ibi_cr, 2, 0),
	SHELL_CMD_ARG(enable, &dsub_i3c_device_attached_name,
		      "Enable receiving IBI from target\n"
		      "Usage: ibi enable <device> <target>",
		      cmd_i3c_ibi_enable, 3, 0),
	SHELL_CMD_ARG(disable, &dsub_i3c_device_attached_name,
		      "Disable receiving IBI from target\n"
		      "Usage: ibi disable <device> <target>",
		      cmd_i3c_ibi_disable, 3, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
#endif

/* L3 I3C HDR DDR Shell Commands*/
SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_i3c_hdr_ddr_cmds,
	SHELL_CMD_ARG(write, &dsub_i3c_device_attached_name,
		      "Send HDR DDR Write\n"
		      "Usage: hdr ddr write <device> <target> <7b cmd> [<byte1>, ...]",
		      cmd_i3c_hdr_ddr_write, 4, MAX_I3C_BYTES),
	SHELL_CMD_ARG(read, &dsub_i3c_device_attached_name,
		      "Send HDR DDR Read\n"
		      "Usage: hdr ddr read <device> <target> <7b cmd> <bytes>",
		      cmd_i3c_hdr_ddr_read, 5, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

/* L2 I3C HDR Shell Commands*/
SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_i3c_hdr_cmds,
	SHELL_CMD_ARG(ddr, &sub_i3c_hdr_ddr_cmds,
		      "Send HDR DDR\n"
		      "Usage: hdr ddr <sub cmd>",
		      NULL, 2, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

/* L2 I3C CCC Shell Commands*/
SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_i3c_ccc_cmds,
	SHELL_CMD_ARG(rstdaa, &dsub_i3c_device_name,
		      "Send CCC RSTDAA\n"
		      "Usage: ccc rstdaa <device>",
		      cmd_i3c_ccc_rstdaa, 2, 0),
	SHELL_CMD_ARG(entdaa, &dsub_i3c_device_name,
		      "Send CCC ENTDAA\n"
		      "Usage: ccc entdaa <device>",
		      cmd_i3c_ccc_entdaa, 2, 0),
	SHELL_CMD_ARG(setaasa, &dsub_i3c_device_name,
		      "Send CCC SETAASA\n"
		      "Usage: ccc setaasa <device>",
		      cmd_i3c_ccc_setaasa, 2, 0),
	SHELL_CMD_ARG(setdasa, &dsub_i3c_device_attached_name,
		      "Send CCC SETDASA\n"
		      "Usage: ccc setdasa <device> <target> <dynamic address>",
		      cmd_i3c_ccc_setdasa, 4, 0),
	SHELL_CMD_ARG(setnewda, &dsub_i3c_device_attached_name,
		      "Send CCC SETNEWDA\n"
		      "Usage: ccc setnewda <device> <target> <dynamic address>",
		      cmd_i3c_ccc_setnewda, 4, 0),
	SHELL_CMD_ARG(getbcr, &dsub_i3c_device_attached_name,
		      "Send CCC GETBCR\n"
		      "Usage: ccc getbcr <device> <target>",
		      cmd_i3c_ccc_getbcr, 3, 0),
	SHELL_CMD_ARG(getdcr, &dsub_i3c_device_attached_name,
		      "Send CCC GETDCR\n"
		      "Usage: ccc getdcr <device> <target>",
		      cmd_i3c_ccc_getdcr, 3, 0),
	SHELL_CMD_ARG(getpid, &dsub_i3c_device_attached_name,
		      "Send CCC GETPID\n"
		      "Usage: ccc getpid <device> <target>",
		      cmd_i3c_ccc_getpid, 3, 0),
	SHELL_CMD_ARG(getmrl, &dsub_i3c_device_attached_name,
		      "Send CCC GETMRL\n"
		      "Usage: ccc getmrl <device> <target>",
		      cmd_i3c_ccc_getmrl, 3, 0),
	SHELL_CMD_ARG(getmwl, &dsub_i3c_device_attached_name,
		      "Send CCC GETMWL\n"
		      "Usage: ccc getmwl <device> <target>",
		      cmd_i3c_ccc_getmwl, 3, 0),
	SHELL_CMD_ARG(setmrl, &dsub_i3c_device_attached_name,
		      "Send CCC SETMRL\n"
		      "Usage: ccc setmrl <device> <target> <max read length> [<max ibi length>]",
		      cmd_i3c_ccc_setmrl, 4, 1),
	SHELL_CMD_ARG(setmwl, &dsub_i3c_device_attached_name,
		      "Send CCC SETMWL\n"
		      "Usage: ccc setmwl <device> <target> <max write length>",
		      cmd_i3c_ccc_setmwl, 4, 0),
	SHELL_CMD_ARG(setmrl_bc, &dsub_i3c_device_name,
		      "Send CCC SETMRL BC\n"
		      "Usage: ccc setmrl_bc <device> <max read length> [<max ibi length>]",
		      cmd_i3c_ccc_setmrl_bc, 3, 1),
	SHELL_CMD_ARG(setmwl_bc, &dsub_i3c_device_name,
		      "Send CCC SETMWL BC\n"
		      "Usage: ccc setmwl_bc <device> <max write length>",
		      cmd_i3c_ccc_setmwl_bc, 3, 0),
	SHELL_CMD_ARG(deftgts, &dsub_i3c_device_name,
		      "Send CCC DEFTGTS\n"
		      "Usage: ccc deftgts <device>",
		      cmd_i3c_ccc_deftgts, 2, 0),
	SHELL_CMD_ARG(enttm, &dsub_i3c_device_name,
		      "Send CCC ENTTM\n"
		      "Usage: ccc enttm <device> <defining byte>",
		      cmd_i3c_ccc_enttm, 3, 0),
	SHELL_CMD_ARG(rstact, &dsub_i3c_device_attached_name,
		      "Send CCC RSTACT\n"
		      "Usage: ccc rstact <device> <target> <\"set\"/\"get\"> <defining byte>",
		      cmd_i3c_ccc_rstact, 5, 0),
	SHELL_CMD_ARG(getacccr, &dsub_i3c_device_attached_name,
		      "Send CCC GETACCCR\n"
		      "Usage: ccc getacccr <device> <target>",
		      cmd_i3c_ccc_getacccr, 3, 0),
	SHELL_CMD_ARG(rstact_bc, &dsub_i3c_device_name,
		      "Send CCC RSTACT BC\n"
		      "Usage: ccc rstact_bc <device> <defining byte>",
		      cmd_i3c_ccc_rstact_bc, 3, 0),
	SHELL_CMD_ARG(enec_bc, &dsub_i3c_device_name,
		      "Send CCC ENEC BC\n"
		      "Usage: ccc enec_bc <device> <defining byte>",
		      cmd_i3c_ccc_enec_bc, 3, 0),
	SHELL_CMD_ARG(disec_bc, &dsub_i3c_device_name,
		      "Send CCC DISEC BC\n"
		      "Usage: ccc disec_bc <device> <defining byte>",
		      cmd_i3c_ccc_disec_bc, 3, 0),
	SHELL_CMD_ARG(enec, &dsub_i3c_device_attached_name,
		      "Send CCC ENEC\n"
		      "Usage: ccc enec <device> <target> <defining byte>",
		      cmd_i3c_ccc_enec, 4, 0),
	SHELL_CMD_ARG(disec, &dsub_i3c_device_attached_name,
		      "Send CCC DISEC\n"
		      "Usage: ccc disec <device> <target> <defining byte>",
		      cmd_i3c_ccc_disec, 4, 0),
	SHELL_CMD_ARG(entas0_bc, &dsub_i3c_device_name,
		      "Send CCC ENTAS0 BC\n"
		      "Usage: ccc entas0 <device>",
		      cmd_i3c_ccc_entas0_bc, 2, 0),
	SHELL_CMD_ARG(entas1_bc, &dsub_i3c_device_name,
		      "Send CCC ENTAS1 BC\n"
		      "Usage: ccc entas1 <device>",
		      cmd_i3c_ccc_entas1_bc, 2, 0),
	SHELL_CMD_ARG(entas2_bc, &dsub_i3c_device_name,
		      "Send CCC ENTAS2 BC\n"
		      "Usage: ccc entas2 <device>",
		      cmd_i3c_ccc_entas2_bc, 2, 0),
	SHELL_CMD_ARG(entas3_bc, &dsub_i3c_device_name,
		      "Send CCC ENTAS3 BC\n"
		      "Usage: ccc entas3 <device>",
		      cmd_i3c_ccc_entas3_bc, 2, 0),
	SHELL_CMD_ARG(entas0, &dsub_i3c_device_attached_name,
		      "Send CCC ENTAS0\n"
		      "Usage: ccc entas0 <device> <target>",
		      cmd_i3c_ccc_entas0, 3, 0),
	SHELL_CMD_ARG(entas1, &dsub_i3c_device_attached_name,
		      "Send CCC ENTAS1\n"
		      "Usage: ccc entas1 <device> <target>",
		      cmd_i3c_ccc_entas1, 3, 0),
	SHELL_CMD_ARG(entas2, &dsub_i3c_device_attached_name,
		      "Send CCC ENTAS2\n"
		      "Usage: ccc entas2 <device> <target>",
		      cmd_i3c_ccc_entas2, 3, 0),
	SHELL_CMD_ARG(entas3, &dsub_i3c_device_attached_name,
		      "Send CCC ENTAS3\n"
		      "Usage: ccc entas3 <device> <target>",
		      cmd_i3c_ccc_entas3, 3, 0),
	SHELL_CMD_ARG(getstatus, &dsub_i3c_device_attached_name,
		      "Send CCC GETSTATUS\n"
		      "Usage: ccc getstatus <device> <target> [<defining byte>]",
		      cmd_i3c_ccc_getstatus, 3, 1),
	SHELL_CMD_ARG(getcaps, &dsub_i3c_device_attached_name,
		      "Send CCC GETCAPS\n"
		      "Usage: ccc getcaps <device> <target> [<defining byte>]",
		      cmd_i3c_ccc_getcaps, 3, 1),
	SHELL_CMD_ARG(getmxds, &dsub_i3c_device_attached_name,
		      "Send CCC GETMXDS\n"
		      "Usage: ccc getmxds <device> <target> [<defining byte>]",
		      cmd_i3c_ccc_getmxds, 3, 1),
	SHELL_CMD_ARG(setbuscon, &dsub_i3c_device_name,
		      "Send CCC SETBUSCON\n"
		      "Usage: ccc setbuscon <device> <context> [<optional bytes>]",
		      cmd_i3c_ccc_setbuscon, 3, MAX_I3C_BYTES - 1),
	SHELL_CMD_ARG(getvendor, &dsub_i3c_device_attached_name,
		      "Send CCC GETVENDOR\n"
		      "Usage: ccc getvendor <device> <target> <id> [<defining byte>]",
		      cmd_i3c_ccc_getvendor, 4, 1),
	SHELL_CMD_ARG(setvendor, &dsub_i3c_device_attached_name,
		      "Send CCC SETVENDOR\n"
		      "Usage: ccc setvendor <device> <target> <id> [<bytes>]",
		      cmd_i3c_ccc_setvendor, 4, MAX_I3C_BYTES),
	SHELL_CMD_ARG(setvendor_bc, &dsub_i3c_device_name,
		      "Send CCC SETVENDOR BC\n"
		      "Usage: ccc setvendor_bc <device> <id> [<bytes>]",
		      cmd_i3c_ccc_setvendor_bc, 3, MAX_I3C_BYTES),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

/* L1 I3C Shell Commands*/
SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_i3c_cmds,
	SHELL_CMD_ARG(info, &dsub_i3c_device_attached_name,
		      "Get I3C device info\n"
		      "Usage: info <device> [<target>]",
		      cmd_i3c_info, 2, 1),
	SHELL_CMD_ARG(speed, &dsub_i3c_device_name,
		      "Set I3C device speed\n"
		      "Usage: speed <device> <speed>",
		      cmd_i3c_speed, 3, 0),
	SHELL_CMD_ARG(recover, &dsub_i3c_device_name,
		      "Recover I3C bus\n"
		      "Usage: recover <device>",
		      cmd_i3c_recover, 2, 0),
	SHELL_CMD_ARG(read, &dsub_i3c_device_attached_name,
		      "Read bytes from an I3C device\n"
		      "Usage: read <device> <target> <reg> [<bytes>]",
		      cmd_i3c_read, 4, 1),
	SHELL_CMD_ARG(read_byte, &dsub_i3c_device_attached_name,
		      "Read a byte from an I3C device\n"
		      "Usage: read_byte <device> <target> <reg>",
		      cmd_i3c_read_byte, 4, 0),
	SHELL_CMD_ARG(write, &dsub_i3c_device_attached_name,
		      "Write bytes to an I3C device\n"
		      "Usage: write <device> <target> <reg> [<byte1>, ...]",
		      cmd_i3c_write, 4, MAX_I3C_BYTES),
	SHELL_CMD_ARG(write_byte, &dsub_i3c_device_attached_name,
		      "Write a byte to an I3C device\n"
		      "Usage: write_byte <device> <target> <reg> <value>",
		      cmd_i3c_write_byte, 5, 0),
	SHELL_CMD_ARG(i3c_attach, &dsub_i3c_device_list_name,
		      "Attach I3C device from the bus\n"
		      "Usage: i3c_attach <device> <target>",
		      cmd_i3c_attach, 3, 0),
	SHELL_CMD_ARG(i3c_reattach, &dsub_i3c_device_attached_name,
		      "Reattach I3C device from the bus\n"
		      "Usage: i3c_reattach <device> <target> [<old dynamic address>]",
		      cmd_i3c_reattach, 3, 1),
	SHELL_CMD_ARG(i3c_detach, &dsub_i3c_device_attached_name,
		      "Detach I3C device from the bus\n"
		      "Usage: i3c_detach <device> <target>",
		      cmd_i3c_detach, 3, 0),
	SHELL_CMD_ARG(i2c_attach, &dsub_i3c_device_name,
		      "Attach I2C device from the bus\n"
		      "Usage: i2c_attach <device> <addr>",
		      cmd_i3c_i2c_attach, 3, 0),
	SHELL_CMD_ARG(i2c_detach, &dsub_i3c_device_name,
		      "Detach I2C device from the bus\n"
		      "Usage: i2c_detach <device> <addr>",
		      cmd_i3c_i2c_detach, 3, 0),
	SHELL_CMD_ARG(i2c_scan, &dsub_i3c_device_name,
		      "Scan I2C devices\n"
		      "Usage: i2c_scan <device>",
		      cmd_i3c_i2c_scan, 2, 0),
	SHELL_CMD_ARG(ccc, &sub_i3c_ccc_cmds,
		      "Send I3C CCC\n"
		      "Usage: ccc <sub cmd>",
		      NULL, 3, 0),
	SHELL_CMD_ARG(hdr, &sub_i3c_hdr_cmds,
		      "Send I3C HDR\n"
		      "Usage: hdr <sub cmd>",
		      NULL, 3, 0),
#ifdef CONFIG_I3C_USE_IBI
	SHELL_CMD_ARG(ibi, &sub_i3c_ibi_cmds,
		      "Send I3C IBI\n"
		      "Usage: ibi <sub cmd>",
		      NULL, 3, 0),
#endif
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(i3c, &sub_i3c_cmds, "I3C commands", NULL);
