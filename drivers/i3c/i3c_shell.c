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
		struct i3c_driver_data *data;                                                      \
		sys_snode_t *node;                                                                 \
		size_t cnt = 0;                                                                    \
                                                                                                   \
		entry->syntax = NULL;                                                              \
		entry->handler = NULL;                                                             \
		entry->subcmd = NULL;                                                              \
		entry->help = NULL;                                                                \
                                                                                                   \
		data = (struct i3c_driver_data *)dev->data;                                        \
		if (!sys_slist_is_empty(&data->attached_dev.devices.i3c)) {                        \
			SYS_SLIST_FOR_EACH_NODE(&data->attached_dev.devices.i3c, node) {           \
				if (cnt == idx) {                                                  \
					struct i3c_device_desc *desc =                             \
						CONTAINER_OF(node, struct i3c_device_desc, node);  \
					entry->syntax = desc->dev->name;                           \
					return;                                                    \
				}                                                                  \
				cnt++;                                                             \
			}                                                                          \
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

static struct i3c_i2c_device_desc *get_i3c_i2c_list_desc_from_addr(const struct device *dev,
								   uint16_t addr)
{
	struct i3c_driver_config *config;
	uint8_t i;

	config = (struct i3c_driver_config *)dev->config;
	for (i = 0; i < config->dev_list.num_i2c; i++) {
		if (config->dev_list.i2c[i].addr == addr) {
			/* only look for a device with the addr */
			return &config->dev_list.i2c[i];
		}
	}

	return NULL;
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
	struct i3c_driver_data *data;
	sys_snode_t *node;

	data = (struct i3c_driver_data *)dev->data;
	if (!sys_slist_is_empty(&data->attached_dev.devices.i3c)) {
		SYS_SLIST_FOR_EACH_NODE(&data->attached_dev.devices.i3c, node) {
			struct i3c_device_desc *desc =
				CONTAINER_OF(node, struct i3c_device_desc, node);
			/* only look for a device with the same name */
			if (strcmp(desc->dev->name, tdev_name) == 0) {
				return desc;
			}
		}
	}

	return NULL;
}

/* i3c info <device> [<target>] */
static int cmd_i3c_info(const struct shell *shell_ctx, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_driver_data *data;
	sys_snode_t *node;
	bool found = false;

	dev = device_get_binding(argv[ARGV_DEV]);

	if (!dev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}
	data = (struct i3c_driver_data *)dev->data;

	if (argc == 3) {
		/* TODO: is this needed? */
		tdev = device_get_binding(argv[ARGV_TDEV]);
		if (!tdev) {
			shell_error(shell_ctx, "I3C: Target Device driver %s not found.",
				    argv[ARGV_DEV]);
			return -ENODEV;
		}

		if (!sys_slist_is_empty(&data->attached_dev.devices.i3c)) {
			SYS_SLIST_FOR_EACH_NODE(&data->attached_dev.devices.i3c, node) {
				struct i3c_device_desc *desc =
					CONTAINER_OF(node, struct i3c_device_desc, node);
				/* only look for a device with the same name */
				if (strcmp(desc->dev->name, tdev->name) == 0) {
					shell_print(shell_ctx,
						    "name: %s\n"
						    "\tpid: 0x%012llx\n"
						    "\tstatic_addr: 0x%02x\n"
						    "\tdynamic_addr: 0x%02x\n"
#if defined(CONFIG_I3C_USE_GROUP_ADDR)
						    "\tgroup_addr: 0x%02x\n"
#endif
						    "\tbcr: 0x%02x\n"
						    "\tdcr: 0x%02x\n"
						    "\tmaxrd: 0x%02x\n"
						    "\tmaxwr: 0x%02x\n"
						    "\tmax_read_turnaround: 0x%08x\n"
						    "\tmrl: 0x%04x\n"
						    "\tmwl: 0x%04x\n"
						    "\tmax_ibi: 0x%02x\n"
						    "\tgetcaps: 0x%02x; 0x%02x; 0x%02x; 0x%02x",
						    desc->dev->name, (uint64_t)desc->pid,
						    desc->static_addr, desc->dynamic_addr,
#if defined(CONFIG_I3C_USE_GROUP_ADDR)
						    desc->group_addr,
#endif
						    desc->bcr, desc->dcr, desc->data_speed.maxrd,
						    desc->data_speed.maxwr,
						    desc->data_speed.max_read_turnaround,
						    desc->data_length.mrl, desc->data_length.mwl,
						    desc->data_length.max_ibi,
						    desc->getcaps.getcap1, desc->getcaps.getcap2,
						    desc->getcaps.getcap3, desc->getcaps.getcap4);
					found = true;
					break;
				}
			}
		} else {
			shell_print(shell_ctx, "I3C: No devices found.");
			return -ENODEV;
		}
		if (found == false) {
			shell_error(shell_ctx, "I3C: Target device not found.");
			return -ENODEV;
		}
	} else if (argc == 2) {
		/* This gets all "currently attached" I3C and I2C devices */
		if (!sys_slist_is_empty(&data->attached_dev.devices.i3c)) {
			shell_print(shell_ctx, "I3C: Devices found:");
			SYS_SLIST_FOR_EACH_NODE(&data->attached_dev.devices.i3c, node) {
				struct i3c_device_desc *desc =
					CONTAINER_OF(node, struct i3c_device_desc, node);
				shell_print(shell_ctx,
					    "name: %s\n"
					    "\tpid: 0x%012llx\n"
					    "\tstatic_addr: 0x%02x\n"
					    "\tdynamic_addr: 0x%02x\n"
#if defined(CONFIG_I3C_USE_GROUP_ADDR)
					    "\tgroup_addr: 0x%02x\n"
#endif
					    "\tbcr: 0x%02x\n"
					    "\tdcr: 0x%02x\n"
					    "\tmaxrd: 0x%02x\n"
					    "\tmaxwr: 0x%02x\n"
					    "\tmax_read_turnaround: 0x%08x\n"
					    "\tmrl: 0x%04x\n"
					    "\tmwl: 0x%04x\n"
					    "\tmax_ibi: 0x%02x\n"
					    "\tgetcaps: 0x%02x; 0x%02x; 0x%02x; 0x%02x",
					    desc->dev->name, (uint64_t)desc->pid, desc->static_addr,
					    desc->dynamic_addr,
#if defined(CONFIG_I3C_USE_GROUP_ADDR)
					    desc->group_addr,
#endif
					    desc->bcr, desc->dcr, desc->data_speed.maxrd,
					    desc->data_speed.maxwr,
					    desc->data_speed.max_read_turnaround,
					    desc->data_length.mrl, desc->data_length.mwl,
					    desc->data_length.max_ibi, desc->getcaps.getcap1,
					    desc->getcaps.getcap2, desc->getcaps.getcap3,
					    desc->getcaps.getcap4);
			}
		} else {
			shell_print(shell_ctx, "I3C: No devices found.");
		}
		if (!sys_slist_is_empty(&data->attached_dev.devices.i2c)) {
			shell_print(shell_ctx, "I2C: Devices found:");
			SYS_SLIST_FOR_EACH_NODE(&data->attached_dev.devices.i2c, node) {
				struct i3c_i2c_device_desc *desc =
					CONTAINER_OF(node, struct i3c_i2c_device_desc, node);
				shell_print(shell_ctx,
					    "addr: 0x%02x\n"
					    "\tlvr: 0x%02x",
					    desc->addr, desc->lvr);
			}
		} else {
			shell_print(shell_ctx, "I2C: No devices found.");
		}
	} else {
		shell_error(shell_ctx, "Invalid number of arguments.");
	}

	return 0;
}

/* i3c recover <device> */
static int cmd_i3c_recover(const struct shell *shell_ctx, size_t argc, char **argv)
{
	const struct device *dev;
	int err;

	dev = device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[1]);
		return -ENODEV;
	}

	err = i3c_recover_bus(dev);
	if (err) {
		shell_error(shell_ctx, "I3C: Bus recovery failed (err %d)", err);
		return err;
	}

	return 0;
}

static int i3c_write_from_buffer(const struct shell *shell_ctx, char *s_dev_name, char *s_tdev_name,
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

	dev = device_get_binding(s_dev_name);
	if (!dev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", s_dev_name);
		return -ENODEV;
	}
	tdev = device_get_binding(s_tdev_name);
	if (!tdev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", s_tdev_name);
		return -ENODEV;
	}
	desc = get_i3c_attached_desc_from_dev_name(dev, tdev->name);
	if (!desc) {
		shell_error(shell_ctx, "I3C: Device %s not attached to bus.", tdev->name);
		return -ENODEV;
	}

	reg_addr = strtol(s_reg_addr, NULL, 16);

	reg_addr_bytes = get_bytes_count_for_hex(s_reg_addr);
	sys_put_be32(reg_addr, buf);

	if (data_length + reg_addr_bytes > MAX_I3C_BYTES) {
		data_length = MAX_I3C_BYTES - reg_addr_bytes;
		shell_info(shell_ctx, "Too many bytes provided, limit is %d",
			   MAX_I3C_BYTES - reg_addr_bytes);
	}

	for (i = 0; i < data_length; i++) {
		buf[MAX_BYTES_FOR_REGISTER_INDEX + i] = (uint8_t)strtol(data[i], NULL, 16);
	}

	ret = i3c_write(desc, buf + MAX_BYTES_FOR_REGISTER_INDEX - reg_addr_bytes,
			reg_addr_bytes + data_length);
	if (ret < 0) {
		shell_error(shell_ctx, "Failed to write to device: %s", tdev->name);
		return -EIO;
	}

	return 0;
}

/* i3c write <device> <dev_addr> <reg_addr> [<byte1>, ...] */
static int cmd_i3c_write(const struct shell *shell_ctx, size_t argc, char **argv)
{
	return i3c_write_from_buffer(shell_ctx, argv[ARGV_DEV], argv[ARGV_TDEV], argv[ARGV_REG],
				     &argv[4], argc - 4);
}

/* i3c write_byte <device> <dev_addr> <reg_addr> <value> */
static int cmd_i3c_write_byte(const struct shell *shell_ctx, size_t argc, char **argv)
{
	return i3c_write_from_buffer(shell_ctx, argv[ARGV_DEV], argv[ARGV_TDEV], argv[ARGV_REG],
				     &argv[4], 1);
}

static int i3c_read_to_buffer(const struct shell *shell_ctx, char *s_dev_name, char *s_tdev_name,
			      char *s_reg_addr, uint8_t *buf, uint8_t buf_length)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	uint8_t reg_addr_buf[MAX_BYTES_FOR_REGISTER_INDEX];
	int reg_addr_bytes;
	int reg_addr;
	int ret;

	dev = device_get_binding(s_dev_name);
	if (!dev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", s_dev_name);
		return -ENODEV;
	}
	tdev = device_get_binding(s_tdev_name);
	if (!tdev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", s_dev_name);
		return -ENODEV;
	}
	desc = get_i3c_attached_desc_from_dev_name(dev, tdev->name);
	if (!desc) {
		shell_error(shell_ctx, "I3C: Device %s not attached to bus.", tdev->name);
		return -ENODEV;
	}

	reg_addr = strtol(s_reg_addr, NULL, 16);

	reg_addr_bytes = get_bytes_count_for_hex(s_reg_addr);
	sys_put_be32(reg_addr, reg_addr_buf);

	ret = i3c_write_read(desc, reg_addr_buf + MAX_BYTES_FOR_REGISTER_INDEX - reg_addr_bytes,
			     reg_addr_bytes, buf, buf_length);
	if (ret < 0) {
		shell_error(shell_ctx, "Failed to read from device: %s", tdev->name);
		return -EIO;
	}

	return 0;
}

/* i3c read_byte <device> <target> <reg_addr> */
static int cmd_i3c_read_byte(const struct shell *shell_ctx, size_t argc, char **argv)
{
	uint8_t out;
	int ret;

	ret = i3c_read_to_buffer(shell_ctx, argv[ARGV_DEV], argv[ARGV_TDEV], argv[ARGV_REG], &out,
				 1);
	if (ret == 0) {
		shell_print(shell_ctx, "Output: 0x%x", out);
	}

	return ret;
}

/* i3c read <device> <target> <reg_addr> [<numbytes>] */
static int cmd_i3c_read(const struct shell *shell_ctx, size_t argc, char **argv)
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

	ret = i3c_read_to_buffer(shell_ctx, argv[ARGV_DEV], argv[ARGV_TDEV], argv[ARGV_REG], buf,
				 num_bytes);
	if (ret == 0) {
		shell_hexdump(shell_ctx, buf, num_bytes);
	}

	return ret;
}

/* i3c ccc rstdaa <device> */
static int cmd_i3c_ccc_rstdaa(const struct shell *shell_ctx, size_t argc, char **argv)
{
	const struct device *dev;
	struct i3c_driver_data *data;
	sys_snode_t *node;
	int ret;

	dev = device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}
	data = (struct i3c_driver_data *)dev->data;

	ret = i3c_ccc_do_rstdaa_all(dev);
	if (ret < 0) {
		shell_error(shell_ctx, "I3C: unable to send CCC RSTDAA.");
		return ret;
	}

	/* reset all devices DA */
	if (!sys_slist_is_empty(&data->attached_dev.devices.i3c)) {
		SYS_SLIST_FOR_EACH_NODE(&data->attached_dev.devices.i3c, node) {
			struct i3c_device_desc *desc =
				CONTAINER_OF(node, struct i3c_device_desc, node);
			desc->dynamic_addr = 0;
			shell_print(shell_ctx, "Reset dynamic address for device %s",
				    desc->dev->name);
		}
	}

	return ret;
}

/* i3c ccc entdaa <device> */
static int cmd_i3c_ccc_entdaa(const struct shell *shell_ctx, size_t argc, char **argv)
{
	const struct device *dev;

	dev = device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}

	return i3c_do_daa(dev);
}

/* i3c ccc setdasa <device> <target> */
static int cmd_i3c_ccc_setdasa(const struct shell *shell_ctx, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	int ret;

	dev = device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}
	tdev = device_get_binding(argv[ARGV_TDEV]);
	if (!tdev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_TDEV]);
		return -ENODEV;
	}
	desc = get_i3c_attached_desc_from_dev_name(dev, tdev->name);
	if (!desc) {
		shell_error(shell_ctx, "I3C: Device %s not attached to bus.", tdev->name);
		return -ENODEV;
	}

	ret = i3c_ccc_do_setdasa(desc);
	if (ret < 0) {
		shell_error(shell_ctx, "I3C: unable to send CCC SETDASA.");
		return ret;
	}

	/* update the target's dynamic address */
	desc->dynamic_addr = desc->init_dynamic_addr ? desc->init_dynamic_addr : desc->static_addr;

	return ret;
}

/* i3c ccc setnewda <device> <target> <dynamic address>*/
static int cmd_i3c_ccc_setnewda(const struct shell *shell_ctx, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	struct i3c_driver_data *data;
	struct i3c_ccc_address new_da;
	uint8_t old_da;
	int ret;

	dev = device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}
	tdev = device_get_binding(argv[ARGV_TDEV]);
	if (!tdev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_TDEV]);
		return -ENODEV;
	}
	desc = get_i3c_attached_desc_from_dev_name(dev, tdev->name);
	if (!desc) {
		shell_error(shell_ctx, "I3C: Device %s not attached to bus.", tdev->name);
		return -ENODEV;
	}

	data = (struct i3c_driver_data *)dev->data;
	new_da.addr = strtol(argv[3], NULL, 16);
	/* check if the addressed is free */
	if (!i3c_addr_slots_is_free(&data->attached_dev.addr_slots, new_da.addr)) {
		shell_error(shell_ctx, "I3C: Address 0x%02x is already in use.", new_da.addr);
		return -EINVAL;
	}

	ret = i3c_ccc_do_setnewda(desc, new_da);
	if (ret < 0) {
		shell_error(shell_ctx, "I3C: unable to send CCC SETDASA.");
		return ret;
	}

	/* reattach device address */
	old_da = desc->dynamic_addr;
	desc->dynamic_addr = new_da.addr;
	ret = i3c_reattach_i3c_device(desc, old_da);
	if (ret < 0) {
		shell_error(shell_ctx, "I3C: unable to reattach device");
		return ret;
	}

	return ret;
}

/* i3c ccc getbcr <device> <target> */
static int cmd_i3c_ccc_getbcr(const struct shell *shell_ctx, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	struct i3c_ccc_getbcr bcr;
	int ret;

	dev = device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}
	tdev = device_get_binding(argv[ARGV_TDEV]);
	if (!tdev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_TDEV]);
		return -ENODEV;
	}
	desc = get_i3c_attached_desc_from_dev_name(dev, tdev->name);
	if (!desc) {
		shell_error(shell_ctx, "I3C: Device %s not attached to bus.", tdev->name);
		return -ENODEV;
	}

	ret = i3c_ccc_do_getbcr(desc, &bcr);
	if (ret < 0) {
		shell_error(shell_ctx, "I3C: unable to send CCC GETBCR.");
		return ret;
	}

	shell_print(shell_ctx, "BCR: 0x%02x", bcr.bcr);

	return ret;
}

/* i3c ccc getdcr <device> <target> */
static int cmd_i3c_ccc_getdcr(const struct shell *shell_ctx, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	struct i3c_ccc_getdcr dcr;
	int ret;

	dev = device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}
	tdev = device_get_binding(argv[ARGV_TDEV]);
	if (!tdev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_TDEV]);
		return -ENODEV;
	}
	desc = get_i3c_attached_desc_from_dev_name(dev, tdev->name);
	if (!desc) {
		shell_error(shell_ctx, "I3C: Device %s not attached to bus.", tdev->name);
		return -ENODEV;
	}

	ret = i3c_ccc_do_getdcr(desc, &dcr);
	if (ret < 0) {
		shell_error(shell_ctx, "I3C: unable to send CCC GETDCR.");
		return ret;
	}

	shell_print(shell_ctx, "DCR: 0x%02x", dcr.dcr);

	return ret;
}

/* i3c ccc getpid <device> <target> */
static int cmd_i3c_ccc_getpid(const struct shell *shell_ctx, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	struct i3c_ccc_getpid pid;
	int ret;

	dev = device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}
	tdev = device_get_binding(argv[ARGV_TDEV]);
	if (!tdev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_TDEV]);
		return -ENODEV;
	}
	desc = get_i3c_attached_desc_from_dev_name(dev, tdev->name);
	if (!desc) {
		shell_error(shell_ctx, "I3C: Device %s not attached to bus.", tdev->name);
		return -ENODEV;
	}

	ret = i3c_ccc_do_getpid(desc, &pid);
	if (ret < 0) {
		shell_error(shell_ctx, "I3C: unable to send CCC GETPID.");
		return ret;
	}

	shell_print(shell_ctx, "PID: 0x%012llx", sys_get_be48(pid.pid));

	return ret;
}

/* i3c ccc getmrl <device> <target> */
static int cmd_i3c_ccc_getmrl(const struct shell *shell_ctx, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	struct i3c_ccc_mrl mrl;
	int ret;

	dev = device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}
	tdev = device_get_binding(argv[ARGV_TDEV]);
	if (!tdev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_TDEV]);
		return -ENODEV;
	}
	desc = get_i3c_attached_desc_from_dev_name(dev, tdev->name);
	if (!desc) {
		shell_error(shell_ctx, "I3C: Device %s not attached to bus.", tdev->name);
		return -ENODEV;
	}

	ret = i3c_ccc_do_getmrl(desc, &mrl);
	if (ret < 0) {
		shell_error(shell_ctx, "I3C: unable to send CCC GETMRL.");
		return ret;
	}

	if (desc->bcr & I3C_BCR_IBI_PAYLOAD_HAS_DATA_BYTE) {
		shell_print(shell_ctx, "MRL: 0x%04x; IBI Length:0x%02x", mrl.len, mrl.ibi_len);
	} else {
		shell_print(shell_ctx, "MRL: 0x%04x", mrl.len);
	}

	return ret;
}

/* i3c ccc getmwl <device> <target> */
static int cmd_i3c_ccc_getmwl(const struct shell *shell_ctx, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	struct i3c_ccc_mwl mwl;
	int ret;

	dev = device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}
	tdev = device_get_binding(argv[ARGV_TDEV]);
	if (!tdev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_TDEV]);
		return -ENODEV;
	}
	desc = get_i3c_attached_desc_from_dev_name(dev, tdev->name);
	if (!desc) {
		shell_error(shell_ctx, "I3C: Device %s not attached to bus.", tdev->name);
		return -ENODEV;
	}

	ret = i3c_ccc_do_getmwl(desc, &mwl);
	if (ret < 0) {
		shell_error(shell_ctx, "I3C: unable to send CCC GETMWL.");
		return ret;
	}

	shell_print(shell_ctx, "MWL: 0x%04x", mwl.len);

	return ret;
}

/* i3c ccc setmrl <device> <target> <max read length> [<max ibi length>] */
static int cmd_i3c_ccc_setmrl(const struct shell *shell_ctx, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	struct i3c_ccc_mrl mrl;
	int ret;

	dev = device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}
	tdev = device_get_binding(argv[ARGV_TDEV]);
	if (!tdev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_TDEV]);
		return -ENODEV;
	}
	desc = get_i3c_attached_desc_from_dev_name(dev, tdev->name);
	if (!desc) {
		shell_error(shell_ctx, "I3C: Device %s not attached to bus.", tdev->name);
		return -ENODEV;
	}

	/* IBI length is required if the ibi payload bit is set */
	if ((desc->bcr & I3C_BCR_IBI_PAYLOAD_HAS_DATA_BYTE) && (argc < 4)) {
		shell_error(shell_ctx, "I3C: Missing IBI length.");
		return -EINVAL;
	}

	mrl.len = strtol(argv[3], NULL, 16);
	if (argc > 3) {
		mrl.ibi_len = strtol(argv[4], NULL, 16);
	}

	ret = i3c_ccc_do_setmrl(desc, &mrl);
	if (ret < 0) {
		shell_error(shell_ctx, "I3C: unable to send CCC SETMRL.");
		return ret;
	}

	return ret;
}

/* i3c ccc setmwl <device> <target> <max write length> */
static int cmd_i3c_ccc_setmwl(const struct shell *shell_ctx, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	struct i3c_ccc_mwl mwl;
	int ret;

	dev = device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}
	tdev = device_get_binding(argv[ARGV_TDEV]);
	if (!tdev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_TDEV]);
		return -ENODEV;
	}
	desc = get_i3c_attached_desc_from_dev_name(dev, tdev->name);
	if (!desc) {
		shell_error(shell_ctx, "I3C: Device %s not attached to bus.", tdev->name);
		return -ENODEV;
	}

	mwl.len = strtol(argv[3], NULL, 16);

	ret = i3c_ccc_do_setmwl(desc, &mwl);
	if (ret < 0) {
		shell_error(shell_ctx, "I3C: unable to send CCC SETMWL.");
		return ret;
	}

	return ret;
}

/* i3c ccc setmrl_bc <device> <max read length> [<max ibi length>] */
static int cmd_i3c_ccc_setmrl_bc(const struct shell *shell_ctx, size_t argc, char **argv)
{
	const struct device *dev;
	struct i3c_ccc_mrl mrl;
	int ret;

	dev = device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}

	mrl.len = strtol(argv[2], NULL, 16);
	if (argc > 3) {
		mrl.ibi_len = strtol(argv[3], NULL, 16);
	}

	ret = i3c_ccc_do_setmrl_all(dev, &mrl, argc > 3);
	if (ret < 0) {
		shell_error(shell_ctx, "I3C: unable to send CCC SETMRL BC.");
		return ret;
	}

	return ret;
}

/* i3c ccc setmwl_bc <device> <max write length> */
static int cmd_i3c_ccc_setmwl_bc(const struct shell *shell_ctx, size_t argc, char **argv)
{
	const struct device *dev;
	struct i3c_ccc_mwl mwl;
	int ret;

	dev = device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}

	mwl.len = strtol(argv[3], NULL, 16);

	ret = i3c_ccc_do_setmwl_all(dev, &mwl);
	if (ret < 0) {
		shell_error(shell_ctx, "I3C: unable to send CCC SETMWL BC.");
		return ret;
	}

	return ret;
}

/* i3c ccc rstact_bc <device> <defining byte> */
static int cmd_i3c_ccc_rstact_bc(const struct shell *shell_ctx, size_t argc, char **argv)
{
	const struct device *dev;
	enum i3c_ccc_rstact_defining_byte action;
	int ret;

	dev = device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}

	action = strtol(argv[2], NULL, 16);

	ret = i3c_ccc_do_rstact_all(dev, action);
	if (ret < 0) {
		shell_error(shell_ctx, "I3C: unable to send CCC RSTACT BC.");
		return ret;
	}

	return ret;
}

/* i3c ccc enec_bc <device> <defining byte> */
static int cmd_i3c_ccc_enec_bc(const struct shell *shell_ctx, size_t argc, char **argv)
{
	const struct device *dev;
	struct i3c_ccc_events events;
	int ret;

	dev = device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}

	events.events = strtol(argv[2], NULL, 16);

	ret = i3c_ccc_do_events_all_set(dev, true, &events);
	if (ret < 0) {
		shell_error(shell_ctx, "I3C: unable to send CCC ENEC BC.");
		return ret;
	}

	return ret;
}

/* i3c ccc disec_bc <device> <defining byte> */
static int cmd_i3c_ccc_disec_bc(const struct shell *shell_ctx, size_t argc, char **argv)
{
	const struct device *dev;
	struct i3c_ccc_events events;
	int ret;

	dev = device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}

	events.events = strtol(argv[2], NULL, 16);

	ret = i3c_ccc_do_events_all_set(dev, false, &events);
	if (ret < 0) {
		shell_error(shell_ctx, "I3C: unable to send CCC ENEC BC.");
		return ret;
	}

	return ret;
}

/* i3c ccc enec <device> <target> <defining byte> */
static int cmd_i3c_ccc_enec(const struct shell *shell_ctx, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	struct i3c_ccc_events events;
	int ret;

	dev = device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}
	tdev = device_get_binding(argv[ARGV_TDEV]);
	if (!tdev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_TDEV]);
		return -ENODEV;
	}
	desc = get_i3c_attached_desc_from_dev_name(dev, tdev->name);
	if (!desc) {
		shell_error(shell_ctx, "I3C: Device %s not attached to bus.", tdev->name);
		return -ENODEV;
	}

	events.events = strtol(argv[3], NULL, 16);

	ret = i3c_ccc_do_events_set(desc, true, &events);
	if (ret < 0) {
		shell_error(shell_ctx, "I3C: unable to send CCC ENEC BC.");
		return ret;
	}

	return ret;
}

/* i3c ccc disec <device> <target> <defining byte> */
static int cmd_i3c_ccc_disec(const struct shell *shell_ctx, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	struct i3c_ccc_events events;
	int ret;

	dev = device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}
	tdev = device_get_binding(argv[ARGV_TDEV]);
	if (!tdev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_TDEV]);
		return -ENODEV;
	}
	desc = get_i3c_attached_desc_from_dev_name(dev, tdev->name);
	if (!desc) {
		shell_error(shell_ctx, "I3C: Device %s not attached to bus.", tdev->name);
		return -ENODEV;
	}

	events.events = strtol(argv[3], NULL, 16);

	ret = i3c_ccc_do_events_set(desc, false, &events);
	if (ret < 0) {
		shell_error(shell_ctx, "I3C: unable to send CCC ENEC BC.");
		return ret;
	}

	return ret;
}

/* i3c ccc getstatus <device> <target> [<defining byte>] */
static int cmd_i3c_ccc_getstatus(const struct shell *shell_ctx, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	union i3c_ccc_getstatus status;
	enum i3c_ccc_getstatus_fmt fmt;
	enum i3c_ccc_getstatus_defbyte defbyte = GETSTATUS_FORMAT_2_INVALID;
	int ret;

	dev = device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}
	tdev = device_get_binding(argv[ARGV_TDEV]);
	if (!tdev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_TDEV]);
		return -ENODEV;
	}
	desc = get_i3c_attached_desc_from_dev_name(dev, tdev->name);
	if (!desc) {
		shell_error(shell_ctx, "I3C: Device %s not attached to bus.", tdev->name);
		return -ENODEV;
	}

	/* If there is a defining byte, then it is assumed to be Format 2*/
	if (argc > 3) {
		fmt = GETSTATUS_FORMAT_2;
		defbyte = strtol(argv[3], NULL, 16);
		if (defbyte != GETSTATUS_FORMAT_2_TGTSTAT || defbyte != GETSTATUS_FORMAT_2_PRECR) {
			shell_error(shell_ctx, "Invalid defining byte.");
			return -EINVAL;
		}
	} else {
		fmt = GETSTATUS_FORMAT_1;
	}

	ret = i3c_ccc_do_getstatus(desc, &status, fmt, defbyte);
	if (ret < 0) {
		shell_error(shell_ctx, "I3C: unable to send CCC GETSTATUS.");
		return ret;
	}

	if (fmt == GETSTATUS_FORMAT_2) {
		if (defbyte == GETSTATUS_FORMAT_2_TGTSTAT) {
			shell_print(shell_ctx, "TGTSTAT: 0x%04x", status.fmt2.tgtstat);
		} else if (defbyte == GETSTATUS_FORMAT_2_PRECR) {
			shell_print(shell_ctx, "PRECR: 0x%04x", status.fmt2.precr);
		}
	} else {
		shell_print(shell_ctx, "Status: 0x%04x", status.fmt1.status);
	}

	return ret;
}

/* i3c ccc getcaps <device> <target> [<defining byte>] */
static int cmd_i3c_ccc_getcaps(const struct shell *shell_ctx, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	union i3c_ccc_getcaps caps;
	enum i3c_ccc_getcaps_fmt fmt;
	enum i3c_ccc_getcaps_defbyte defbyte = GETCAPS_FORMAT_2_INVALID;
	int ret;

	dev = device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}
	tdev = device_get_binding(argv[ARGV_TDEV]);
	if (!tdev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_TDEV]);
		return -ENODEV;
	}
	desc = get_i3c_attached_desc_from_dev_name(dev, tdev->name);
	if (!desc) {
		shell_error(shell_ctx, "I3C: Device %s not attached to bus.", tdev->name);
		return -ENODEV;
	}

	if (!(desc->bcr & I3C_BCR_ADV_CAPABILITIES)) {
		shell_error(shell_ctx, "I3C: Device %s does not support advanced capabilities",
			    desc->dev->name);
		return -ENOTSUP;
	}

	/* If there is a defining byte, then it is assumed to be Format 2 */
	if (argc > 3) {
		fmt = GETCAPS_FORMAT_2;
		defbyte = strtol(argv[3], NULL, 16);
		if (defbyte != GETCAPS_FORMAT_2_TGTCAPS || defbyte != GETCAPS_FORMAT_2_TESTPAT ||
		    defbyte != GETCAPS_FORMAT_2_CRCAPS || defbyte != GETCAPS_FORMAT_2_VTCAPS ||
		    defbyte != GETCAPS_FORMAT_2_DBGCAPS) {
			shell_error(shell_ctx, "Invalid defining byte.");
			return -EINVAL;
		}
	} else {
		fmt = GETCAPS_FORMAT_1;
	}

	ret = i3c_ccc_do_getcaps(desc, &caps, fmt, defbyte);
	if (ret < 0) {
		shell_error(shell_ctx, "I3C: unable to send CCC GETCAPS.");
		return ret;
	}

	if (fmt == GETCAPS_FORMAT_2) {
		if (defbyte == GETCAPS_FORMAT_2_TGTCAPS) {
			shell_print(shell_ctx, "TGTCAPS: 0x%02x; 0x%02x; 0x%02x; 0x%02x",
				    caps.fmt2.tgtcaps[0], caps.fmt2.tgtcaps[1],
				    caps.fmt2.tgtcaps[2], caps.fmt2.tgtcaps[3]);
		} else if (defbyte == GETCAPS_FORMAT_2_TESTPAT) {
			shell_print(shell_ctx, "TESTPAT: 0x%08x", caps.fmt2.testpat);
		} else if (defbyte == GETCAPS_FORMAT_2_CRCAPS) {
			shell_print(shell_ctx, "CRCAPS: 0x%02x; 0x%02x", caps.fmt2.crcaps[0],
				    caps.fmt2.crcaps[1]);
		} else if (defbyte == GETCAPS_FORMAT_2_VTCAPS) {
			shell_print(shell_ctx, "VTCAPS: 0x%02x; 0x%02x", caps.fmt2.vtcaps[0],
				    caps.fmt2.vtcaps[1]);
		}
	} else {
		shell_print(shell_ctx, "GETCAPS: 0x%02x; 0x%02x; 0x%02x; 0x%02x",
			    caps.fmt1.getcaps[0], caps.fmt1.getcaps[1], caps.fmt1.getcaps[2],
			    caps.fmt1.getcaps[3]);
	}

	return ret;
}

static int cmd_i3c_attach(const struct shell *shell_ctx, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	int ret;

	dev = device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}
	tdev = device_get_binding(argv[ARGV_TDEV]);
	if (!tdev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_TDEV]);
		return -ENODEV;
	}
	desc = get_i3c_list_desc_from_dev_name(dev, tdev->name);
	if (!desc) {
		shell_error(shell_ctx, "I3C: Device %s not attached to bus.", tdev->name);
		return -ENODEV;
	}

	ret = i3c_attach_i3c_device(desc);
	if (ret < 0) {
		shell_error(shell_ctx, "I3C: unable to attach device %s.", tdev->name);
	}

	return ret;
}

static int cmd_i3c_reattach(const struct shell *shell_ctx, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	uint8_t old_dyn_addr = 0;
	int ret;

	dev = device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}
	tdev = device_get_binding(argv[ARGV_TDEV]);
	if (!tdev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_TDEV]);
		return -ENODEV;
	}
	desc = get_i3c_attached_desc_from_dev_name(dev, tdev->name);
	if (!desc) {
		shell_error(shell_ctx, "I3C: Device %s not attached to bus.", tdev->name);
		return -ENODEV;
	}

	if (argc > 2) {
		old_dyn_addr = strtol(argv[2], NULL, 16);
	}

	ret = i3c_reattach_i3c_device(desc, old_dyn_addr);
	if (ret < 0) {
		shell_error(shell_ctx, "I3C: unable to reattach device %s.", tdev->name);
	}

	return ret;
}

static int cmd_i3c_detach(const struct shell *shell_ctx, size_t argc, char **argv)
{
	const struct device *dev, *tdev;
	struct i3c_device_desc *desc;
	int ret;

	dev = device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}
	tdev = device_get_binding(argv[ARGV_TDEV]);
	if (!tdev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_TDEV]);
		return -ENODEV;
	}
	desc = get_i3c_attached_desc_from_dev_name(dev, tdev->name);
	if (!desc) {
		shell_error(shell_ctx, "I3C: Device %s not attached to bus.", tdev->name);
		return -ENODEV;
	}

	ret = i3c_detach_i3c_device(desc);
	if (ret < 0) {
		shell_error(shell_ctx, "I3C: unable to detach device %s.", tdev->name);
	}

	return ret;
}

static int cmd_i3c_i2c_attach(const struct shell *shell_ctx, size_t argc, char **argv)
{
	const struct device *dev;
	struct i3c_i2c_device_desc *desc;
	uint16_t addr = 0;
	int ret;

	dev = device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}
	addr = strtol(argv[2], NULL, 16);
	desc = get_i3c_i2c_list_desc_from_addr(dev, addr);
	if (!desc) {
		shell_error(shell_ctx, "I3C: I2C addr 0x%02x not listed with the bus.", addr);
		return -ENODEV;
	}

	ret = i3c_attach_i2c_device(desc);
	if (ret < 0) {
		shell_error(shell_ctx, "I3C: unable to attach I2C addr 0x%02x.", addr);
	}

	return ret;
}

static int cmd_i3c_i2c_detach(const struct shell *shell_ctx, size_t argc, char **argv)
{
	const struct device *dev;
	struct i3c_i2c_device_desc *desc;
	uint16_t addr = 0;
	int ret;

	dev = device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}
	addr = strtol(argv[2], NULL, 16);
	desc = get_i3c_i2c_list_desc_from_addr(dev, addr);
	if (!desc) {
		shell_error(shell_ctx, "I3C: I2C addr 0x%02x not listed with the bus.", addr);
		return -ENODEV;
	}

	ret = i3c_detach_i2c_device(desc);
	if (ret < 0) {
		shell_error(shell_ctx, "I3C: unable to detach I2C addr 0x%02x.", addr);
	}

	return ret;
}

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
	SHELL_CMD_ARG(setdasa, &dsub_i3c_device_attached_name,
		      "Send CCC SETDASA\n"
		      "Usage: ccc setdasa <device> <target>",
		      cmd_i3c_ccc_setdasa, 3, 0),
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
	SHELL_CMD_ARG(getstatus, &dsub_i3c_device_attached_name,
		      "Send CCC GETSTATUS\n"
		      "Usage: ccc getstatus <device> <target> [<defining byte>]",
		      cmd_i3c_ccc_getstatus, 3, 1),
	SHELL_CMD_ARG(getcaps, &dsub_i3c_device_attached_name,
		      "Send CCC GETCAPS\n"
		      "Usage: ccc getcaps <device> <target> [<defining byte>]",
		      cmd_i3c_ccc_getcaps, 3, 1),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

/* L1 I3C Shell Commands*/
SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_i3c_cmds,
	SHELL_CMD_ARG(info, &dsub_i3c_device_attached_name,
		      "Get I3C device info\n"
		      "Usage: info <device> [<target>]",
		      cmd_i3c_info, 2, 1),
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
	SHELL_CMD_ARG(ccc, &sub_i3c_ccc_cmds,
		      "Send I3C CCC\n"
		      "Usage: ccc <sub cmd>",
		      NULL, 3, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(i3c, &sub_i3c_cmds, "I3C commands", NULL);
