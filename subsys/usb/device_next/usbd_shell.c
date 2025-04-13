/*
 * Copyright (c) 2022,2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/util.h>
#include <zephyr/shell/shell.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/iterable_sections.h>

/* Default configurations used in the shell context. */
USBD_CONFIGURATION_DEFINE(config_1_fs, USB_SCD_REMOTE_WAKEUP, 200, NULL);
USBD_CONFIGURATION_DEFINE(config_1_hs, USB_SCD_REMOTE_WAKEUP, 200, NULL);
USBD_CONFIGURATION_DEFINE(config_2_fs, USB_SCD_SELF_POWERED, 200, NULL);
USBD_CONFIGURATION_DEFINE(config_2_hs, USB_SCD_SELF_POWERED, 200, NULL);

static struct usbd_shell_config {
	struct usbd_config_node *cfg_nd;
	enum usbd_speed speed;
	const char *name;
} sh_configs[] = {
	{.cfg_nd = &config_1_fs, .speed = USBD_SPEED_FS, .name = "FS1",},
	{.cfg_nd = &config_1_hs, .speed = USBD_SPEED_HS, .name = "HS1",},
	{.cfg_nd = &config_2_fs, .speed = USBD_SPEED_FS, .name = "FS2",},
	{.cfg_nd = &config_2_hs, .speed = USBD_SPEED_HS, .name = "HS2",},
};

static struct usbd_shell_speed {
	enum usbd_speed speed;
	const char *name;
} sh_speed[] = {
	{.speed = USBD_SPEED_FS, .name = "fs",},
	{.speed = USBD_SPEED_HS, .name = "hs",},
};

/* Default string descriptors used in the shell context. */
USBD_DESC_LANG_DEFINE(lang);
USBD_DESC_MANUFACTURER_DEFINE(mfr, "ZEPHYR");
USBD_DESC_PRODUCT_DEFINE(product, "Zephyr USBD foobaz");
IF_ENABLED(CONFIG_HWINFO, (USBD_DESC_SERIAL_NUMBER_DEFINE(sn)));

/* Default device descriptors and context used in the shell. */
USBD_DEVICE_DEFINE(sh_uds_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
		   0x2fe3, 0xffff);

static struct usbd_context *my_uds_ctx = &sh_uds_ctx;
static enum usbd_speed current_cmd_speed = USBD_SPEED_FS;

static int cmd_wakeup_request(const struct shell *sh,
			      size_t argc, char **argv)
{
	int err;

	err = usbd_wakeup_request(my_uds_ctx);
	if (err) {
		shell_error(sh, "dev: Failed to wakeup remote %d", err);
	} else {
		shell_print(sh, "dev: Requested remote wakeup");
	}

	return err;
}

static int cmd_register(const struct shell *sh,
			size_t argc, char **argv)
{
	uint8_t cfg;
	int ret;

	cfg = strtol(argv[3], NULL, 10);
	ret = usbd_register_class(my_uds_ctx, argv[1], current_cmd_speed, cfg);
	if (ret) {
		shell_error(sh,
			    "dev: failed to register USB class %s to configuration %s %u",
			    argv[1], argv[2], cfg);
	} else {
		shell_print(sh,
			    "dev: register USB class %s to configuration %s %u",
			    argv[1], argv[2], cfg);
	}

	return ret;
}

static int cmd_unregister(const struct shell *sh,
			  size_t argc, char **argv)
{
	uint8_t cfg;
	int ret;

	cfg = strtol(argv[3], NULL, 10);
	ret = usbd_unregister_class(my_uds_ctx, argv[1], current_cmd_speed, cfg);
	if (ret) {
		shell_error(sh,
			    "dev: failed to remove USB class %s from configuration %s %u",
			    argv[1], argv[2], cfg);
	} else {
		shell_print(sh,
			    "dev: removed USB class %s from configuration %s %u",
			    argv[1], argv[2], cfg);
	}

	return ret;
}

static int cmd_usbd_default_strings(const struct shell *sh,
				    size_t argc, char **argv)
{
	int err;

	err = usbd_add_descriptor(my_uds_ctx, &lang);
	err |= usbd_add_descriptor(my_uds_ctx, &mfr);
	err |= usbd_add_descriptor(my_uds_ctx, &product);
	IF_ENABLED(CONFIG_HWINFO, (
		err |= usbd_add_descriptor(my_uds_ctx, &sn);
	))

	if (err) {
		shell_error(sh, "dev: Failed to add default string descriptors, %d", err);
	} else {
		shell_print(sh, "dev: added default string descriptors");
	}

	return err;
}

static int register_classes(const struct shell *sh)
{
	int err;

	STRUCT_SECTION_FOREACH_ALTERNATE(usbd_class_fs, usbd_class_node, c_nd) {
		err = usbd_register_class(my_uds_ctx, c_nd->c_data->name,
					  USBD_SPEED_FS, 1);
		if (err) {
			shell_error(sh,
				    "dev: failed to register FS %s (%d)",
				    c_nd->c_data->name, err);
			return err;
		}

		shell_print(sh, "dev: register FS %s", c_nd->c_data->name);
	}

	if (!USBD_SUPPORTS_HIGH_SPEED ||
	    usbd_caps_speed(my_uds_ctx) != USBD_SPEED_HS) {
		return 0;
	}

	STRUCT_SECTION_FOREACH_ALTERNATE(usbd_class_hs, usbd_class_node, c_nd) {
		err = usbd_register_class(my_uds_ctx, c_nd->c_data->name,
					  USBD_SPEED_HS, 1);
		if (err) {
			shell_error(sh,
				    "dev: failed to register HS %s (%d)",
				    c_nd->c_data->name, err);
			return err;
		}

		shell_print(sh, "dev: register HS %s", c_nd->c_data->name);
	}

	return 0;
}

static int cmd_usbd_init(const struct shell *sh,
			 size_t argc, char **argv)
{
	int err;

	err = usbd_init(my_uds_ctx);

	if (err == -EALREADY) {
		shell_error(sh, "dev: USB already initialized");
	} else if (err) {
		shell_error(sh, "dev: Failed to initialize device support (%d)", err);
	} else {
		shell_print(sh, "dev: USB initialized");
	}

	return err;
}

static int cmd_usbd_default_config(const struct shell *sh,
				   size_t argc, char **argv)
{
	int err;

	err = cmd_usbd_default_strings(sh, 0, NULL);
	if (err) {
		return err;
	}

	if (usbd_caps_speed(my_uds_ctx) == USBD_SPEED_HS) {
		err = usbd_add_configuration(my_uds_ctx, USBD_SPEED_HS, &config_1_hs);
		if (err) {
			shell_error(sh, "dev: Failed to add HS configuration");
			return err;
		}
	}

	err = usbd_add_configuration(my_uds_ctx, USBD_SPEED_FS, &config_1_fs);
	if (err) {
		shell_error(sh, "dev: Failed to add FS configuration");
		return err;
	}

	err = register_classes(sh);
	if (err) {
		return err;
	}

	return cmd_usbd_init(sh, 0, NULL);
}

static int cmd_usbd_enable(const struct shell *sh,
			   size_t argc, char **argv)
{
	int err;

	err = usbd_enable(my_uds_ctx);

	if (err == -EALREADY) {
		shell_error(sh, "dev: USB already enabled");
	} else if (err) {
		shell_error(sh, "dev: Failed to enable USB, error %d", err);
	} else {
		shell_print(sh, "dev: USB enabled");
	}

	return err;
}

static int cmd_usbd_disable(const struct shell *sh,
			    size_t argc, char **argv)
{
	int err;

	err = usbd_disable(my_uds_ctx);

	if (err) {
		shell_error(sh, "dev: Failed to disable USB");
		return err;
	}

	shell_print(sh, "dev: USB disabled");

	return 0;
}

static int cmd_usbd_shutdown(const struct shell *sh,
			     size_t argc, char **argv)
{
	int err;

	err = usbd_shutdown(my_uds_ctx);

	if (err) {
		shell_error(sh, "dev: Failed to shutdown USB");
		return err;
	}

	shell_print(sh, "dev: USB completely disabled");

	return 0;
}

static int cmd_select(const struct shell *sh, size_t argc, char **argv)
{
	STRUCT_SECTION_FOREACH(usbd_context, ctx) {
		if (strcmp(argv[1], ctx->name) == 0) {
			my_uds_ctx = ctx;
			shell_print(sh,
				    "dev: select %s as my USB device context",
				    argv[1]);

			return 0;
		}
	}

	shell_error(sh, "dev: failed to select %s", argv[1]);

	return -ENODEV;
}

static int cmd_device_bcd_usb(const struct shell *sh, size_t argc,
			      char *argv[])
{
	uint16_t bcd;
	int ret;

	bcd = strtol(argv[2], NULL, 16);
	ret = usbd_device_set_bcd_usb(my_uds_ctx, current_cmd_speed, bcd);
	if (ret) {
		shell_error(sh, "dev: failed to set device bcdUSB to %x", bcd);
	} else {
		shell_error(sh, "dev: set device bcdUSB to %x", bcd);
	}

	return ret;
}

static int cmd_device_pid(const struct shell *sh, size_t argc,
			  char *argv[])
{
	uint16_t pid;
	int ret;

	pid = strtol(argv[1], NULL, 16);
	ret = usbd_device_set_pid(my_uds_ctx, pid);
	if (ret) {
		shell_error(sh, "dev: failed to set device idProduct to %x", pid);
	}

	return ret;
}

static int cmd_device_vid(const struct shell *sh, size_t argc,
			  char *argv[])
{
	uint16_t vid;
	int ret;

	vid = strtol(argv[1], NULL, 16);
	ret = usbd_device_set_vid(my_uds_ctx, vid);
	if (ret) {
		shell_error(sh, "dev: failed to set device idVendor to %x", vid);
	}

	return ret;
}

static int cmd_device_code_triple(const struct shell *sh, size_t argc,
				  char *argv[])
{
	uint8_t class, subclass, protocol;
	int ret;

	class = strtol(argv[2], NULL, 16);
	subclass = strtol(argv[3], NULL, 16);
	protocol = strtol(argv[4], NULL, 16);
	ret = usbd_device_set_code_triple(my_uds_ctx, current_cmd_speed,
					  class, subclass, protocol);
	if (ret) {
		shell_error(sh, "dev: failed to set device code triple to %x %x %x",
			    class, subclass, protocol);
	} else {
		shell_error(sh, "dev: set device code triple to %x %x %x",
			    class, subclass, protocol);
	}

	return ret;
}

static int cmd_config_add(const struct shell *sh, size_t argc,
			  char *argv[])
{
	int ret = -EINVAL;

	for (unsigned int i = 0; i < ARRAY_SIZE(sh_configs); i++) {
		if (!strcmp(argv[1], sh_configs[i].name)) {
			ret = usbd_add_configuration(my_uds_ctx,
						     sh_configs[i].speed,
						     sh_configs[i].cfg_nd);
			break;
		}
	}

	if (ret) {
		shell_error(sh, "dev: failed to add configuration %s", argv[1]);
	}

	return ret;
}

static int cmd_config_set_selfpowered(const struct shell *sh, const bool self,
				      size_t argc, char *argv[])
{
	uint8_t cfg;
	int ret;

	cfg = strtol(argv[2], NULL, 10);

	ret = usbd_config_attrib_self(my_uds_ctx, current_cmd_speed, cfg, self);
	if (ret) {
		shell_error(sh,
			    "dev: failed to set attribute Self-powered to %u",
			    cfg);
	} else {
		shell_print(sh,
			    "dev: set configuration %u attribute Self-powered to %u",
			    cfg, self);
	}

	return ret;
}

static int cmd_config_selfpowered(const struct shell *sh,
				  size_t argc, char *argv[])
{
	return cmd_config_set_selfpowered(sh, true, argc, argv);
}

static int cmd_config_buspowered(const struct shell *sh,
				 size_t argc, char *argv[])
{
	return cmd_config_set_selfpowered(sh, false, argc, argv);
}

static int cmd_config_rwup(const struct shell *sh, const bool rwup,
			   size_t argc, char *argv[])
{
	uint8_t cfg;
	int ret;

	cfg = strtol(argv[2], NULL, 10);

	ret = usbd_config_attrib_rwup(my_uds_ctx, current_cmd_speed, cfg, rwup);
	if (ret) {
		shell_error(sh,
			    "dev: failed set configuration %u Remote Wakeup to %u",
			    cfg, rwup);
	} else {
		shell_print(sh,
			    "dev: set configuration %u Remote Wakeup to %u",
			    cfg, rwup);
	}

	return ret;
}

static int cmd_config_set_rwup(const struct shell *sh,
			       size_t argc, char *argv[])
{
	return cmd_config_rwup(sh, true, argc, argv);
}

static int cmd_config_clear_rwup(const struct shell *sh,
				 size_t argc, char *argv[])
{
	return cmd_config_rwup(sh, false, argc, argv);
}

static int cmd_config_power(const struct shell *sh, size_t argc,
			    char *argv[])
{
	uint16_t power;
	uint8_t cfg;
	int ret;

	cfg = strtol(argv[2], NULL, 10);
	power = strtol(argv[3], NULL, 10);

	if (power > UINT8_MAX) {
		power = UINT8_MAX;
		shell_print(sh, "dev: limit bMaxPower value to %u", power);
	}

	ret = usbd_config_maxpower(my_uds_ctx, current_cmd_speed, cfg, power);
	if (ret) {
		shell_error(sh,
			    "dev: failed to set configuration %u bMaxPower value to %u",
			    cfg, power);
	} else {
		shell_print(sh,
			    "dev: set configuration %u bMaxPower value to %u",
			    cfg, power);
	}

	return ret;
}

static void configuration_speed(size_t idx, struct shell_static_entry *entry)
{
	size_t match_idx = 0;

	entry->syntax = NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;

	for (unsigned int i = 0; i < ARRAY_SIZE(sh_speed); i++) {
		if (match_idx == idx) {
			entry->syntax = sh_speed[i].name;
			current_cmd_speed = sh_speed[i].speed;
			break;
		}

		++match_idx;
	}
}

SHELL_DYNAMIC_CMD_CREATE(dsub_config_speed, configuration_speed);

static void configuration_lookup(size_t idx, struct shell_static_entry *entry)
{
	size_t match_idx = 0;

	entry->syntax = NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;

	for (unsigned int i = 0; i < ARRAY_SIZE(sh_configs); i++) {
		if (match_idx == idx) {
			entry->syntax = sh_configs[i].name;
			break;
		}

		++match_idx;
	}
}

SHELL_DYNAMIC_CMD_CREATE(dsub_config_name, configuration_lookup);

static void class_node_name_lookup(size_t idx, struct shell_static_entry *entry)
{
	size_t match_idx = 0;

	entry->syntax = NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = &dsub_config_speed;

	STRUCT_SECTION_FOREACH_ALTERNATE(usbd_class_fs, usbd_class_node, c_nd) {
		if ((c_nd->c_data->name != NULL) &&
		    (strlen(c_nd->c_data->name) != 0)) {
			if (match_idx == idx) {
				entry->syntax = c_nd->c_data->name;
				break;
			}

			++match_idx;
		}
	}
}

static void device_context_lookup(size_t idx, struct shell_static_entry *entry)
{
	size_t match_idx = 0;

	entry->syntax = NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;

	STRUCT_SECTION_FOREACH(usbd_context, ctx) {
		if ((ctx->name != NULL) && (strlen(ctx->name) != 0)) {
			if (match_idx == idx) {
				entry->syntax = ctx->name;
				break;
			}

			++match_idx;
		}
	}
}

SHELL_DYNAMIC_CMD_CREATE(dsub_node_name, class_node_name_lookup);
SHELL_DYNAMIC_CMD_CREATE(dsub_context_name, device_context_lookup);

SHELL_STATIC_SUBCMD_SET_CREATE(device_cmds,
	SHELL_CMD_ARG(pid, NULL,
		      "<idProduct> sets device Product ID",
		      cmd_device_pid, 2, 0),
	SHELL_CMD_ARG(vid, NULL,
		      "<idVendor> sets device Vendor ID",
		      cmd_device_vid, 2, 0),
	SHELL_CMD_ARG(bcd_usb, &dsub_config_speed,
		      "<speed> <bcdUSB> sets device USB specification version",
		      cmd_device_bcd_usb, 3, 0),
	SHELL_CMD_ARG(triple, &dsub_config_speed,
		      "<speed> <Base Class> <SubClass> <Protocol> sets device code triple",
		      cmd_device_code_triple, 5, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(config_cmds,
	SHELL_CMD_ARG(add, &dsub_config_name,
		      "<configuration name> adds one of the pre-defined configurations",
		      cmd_config_add, 2, 0),
	SHELL_CMD_ARG(power, &dsub_config_speed,
		      "<speed> <configuration value> <bMaxPower> sets the bMaxPower",
		      cmd_config_power, 4, 0),
	SHELL_CMD_ARG(set-rwup, &dsub_config_speed,
		      "<speed> <configuration value> sets Remote Wakeup bit",
		      cmd_config_set_rwup, 3, 0),
	SHELL_CMD_ARG(clear-rwup, &dsub_config_speed,
		      "<speed> <configuration value> clears Remote Wakeup bit",
		      cmd_config_clear_rwup, 3, 0),
	SHELL_CMD_ARG(selfpowered, &dsub_config_speed,
		      "<speed> <configuration value> sets Self-power bit",
		      cmd_config_selfpowered, 3, 0),
	SHELL_CMD_ARG(buspowered, &dsub_config_speed,
		      "<speed> <configuration value> clears Self-power bit",
		      cmd_config_buspowered, 3, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(class_cmds,
	SHELL_CMD_ARG(register, &dsub_node_name,
		      "<name> <speed> <configuration value> registers class instance",
		      cmd_register, 4, 0),
	SHELL_CMD_ARG(unregister, &dsub_node_name,
		      "<name> <speed> <configuration value> unregisters class instance",
		      cmd_unregister, 4, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_usbd_cmds,
	SHELL_CMD_ARG(defstr, NULL,
		      "[none] adds default string descriptors",
		      cmd_usbd_default_strings, 1, 0),
	SHELL_CMD_ARG(defcfg, NULL,
		      "[none] initializes default configuration with all available classes",
		      cmd_usbd_default_config, 1, 0),
	SHELL_CMD_ARG(init, NULL,
		      "[none] initializes USB device support",
		      cmd_usbd_init, 1, 0),
	SHELL_CMD_ARG(enable, NULL,
		      "[none] enables USB device support]",
		      cmd_usbd_enable, 1, 0),
	SHELL_CMD_ARG(disable, NULL,
		      "[none] disables USB device support",
		      cmd_usbd_disable, 1, 0),
	SHELL_CMD_ARG(shutdown, NULL,
		      "[none] shutdown USB device support",
		      cmd_usbd_shutdown, 1, 0),
	SHELL_CMD_ARG(select, &dsub_context_name,
		      "<USB device context name> selects context used by the shell",
		      cmd_select, 2, 0),
	SHELL_CMD_ARG(device, &device_cmds,
		      "device commands",
		      NULL, 1, 0),
	SHELL_CMD_ARG(config, &config_cmds,
		      "configuration commands",
		      NULL, 1, 0),
	SHELL_CMD_ARG(class, &class_cmds,
		      "class commands",
		      NULL, 1, 0),
	SHELL_CMD_ARG(wakeup, NULL,
		      "[none] signals remote wakeup",
		      cmd_wakeup_request, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(usbd, &sub_usbd_cmds, "USB device support commands", NULL);
