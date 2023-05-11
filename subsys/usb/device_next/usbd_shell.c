/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
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

const struct shell *ctx_shell;

USBD_CONFIGURATION_DEFINE(config_baz, USB_SCD_REMOTE_WAKEUP, 200);
USBD_CONFIGURATION_DEFINE(config_foo, USB_SCD_SELF_POWERED, 200);

USBD_DESC_LANG_DEFINE(lang);
USBD_DESC_STRING_DEFINE(mfr, "ZEPHYR", 1);
USBD_DESC_STRING_DEFINE(product, "Zephyr USBD foobaz", 2);
USBD_DESC_STRING_DEFINE(sn, "0123456789ABCDEF", 3);

USBD_DEVICE_DEFINE(uds_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
		   0x2fe3, 0xffff);

int cmd_wakeup_request(const struct shell *sh,
		       size_t argc, char **argv)
{
	int err;

	err = usbd_wakeup_request(&uds_ctx);
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

	cfg = strtol(argv[2], NULL, 10);
	ret = usbd_register_class(&uds_ctx, argv[1], cfg);

	if (ret) {
		shell_error(sh,
			    "dev: failed to add USB class %s to configuration %u",
			    argv[1], cfg);
	} else {
		shell_print(sh,
			    "dev: added USB class %s to configuration %u",
			    argv[1], cfg);
	}

	return ret;
}

static int cmd_unregister(const struct shell *sh,
			  size_t argc, char **argv)
{
	uint8_t cfg;
	int ret;

	cfg = strtol(argv[2], NULL, 10);
	ret = usbd_unregister_class(&uds_ctx, argv[1], cfg);
	if (ret) {
		shell_error(sh,
			    "dev: failed to remove USB class %s from configuration %u",
			    argv[1], cfg);
	} else {
		shell_print(sh,
			    "dev: removed USB class %s from configuration %u",
			    argv[1], cfg);
	}

	return ret;
}

static int cmd_usbd_magic(const struct shell *sh,
			  size_t argc, char **argv)
{
	int err;

	err = usbd_add_descriptor(&uds_ctx, &lang);
	err |= usbd_add_descriptor(&uds_ctx, &mfr);
	err |= usbd_add_descriptor(&uds_ctx, &product);
	err |= usbd_add_descriptor(&uds_ctx, &sn);
	if (err) {
		shell_error(sh, "dev: Failed to initialize descriptors, %d", err);
	}

	err = usbd_add_configuration(&uds_ctx, &config_foo);
	if (err) {
		shell_error(sh, "dev: Failed to add configuration");
	}

	if (IS_ENABLED(CONFIG_USBD_LOOPBACK_CLASS)) {
		err = usbd_register_class(&uds_ctx, "loopback_0", 1);
		if (err) {
			shell_error(sh, "dev: Failed to add loopback_0 class");
		}
	}

	ctx_shell = sh;
	err = usbd_init(&uds_ctx);
	if (err) {
		shell_error(sh, "dev: Failed to initialize device support");
	}

	err = usbd_enable(&uds_ctx);
	if (err) {
		shell_error(sh, "dev: Failed to enable device support");
	}

	return err;
}

static int cmd_usbd_defaults(const struct shell *sh,
			     size_t argc, char **argv)
{
	int err;

	err = usbd_add_descriptor(&uds_ctx, &lang);
	err |= usbd_add_descriptor(&uds_ctx, &mfr);
	err |= usbd_add_descriptor(&uds_ctx, &product);
	err |= usbd_add_descriptor(&uds_ctx, &sn);

	if (err) {
		shell_error(sh, "dev: Failed to initialize descriptors, %d", err);
	} else {
		shell_print(sh, "dev: USB descriptors initialized");
	}

	return err;
}

static int cmd_usbd_init(const struct shell *sh,
			 size_t argc, char **argv)
{
	int err;

	ctx_shell = sh;
	err = usbd_init(&uds_ctx);

	if (err == -EALREADY) {
		shell_error(sh, "dev: USB already initialized");
	} else if (err) {
		shell_error(sh, "dev: Failed to initialize %d", err);
	} else {
		shell_print(sh, "dev: USB initialized");
	}

	return err;
}

static int cmd_usbd_enable(const struct shell *sh,
			   size_t argc, char **argv)
{
	int err;

	err = usbd_enable(&uds_ctx);

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

	err = usbd_disable(&uds_ctx);

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

	err = usbd_shutdown(&uds_ctx);

	if (err) {
		shell_error(sh, "dev: Failed to shutdown USB");
		return err;
	}

	shell_print(sh, "dev: USB completely disabled");

	return 0;
}

static int cmd_device_bcd(const struct shell *sh, size_t argc,
			  char *argv[])
{
	uint16_t bcd;
	int ret;

	bcd = strtol(argv[1], NULL, 16);
	ret = usbd_device_set_bcd(&uds_ctx, bcd);
	if (ret) {
		shell_error(sh, "dev: failed to set device bcdUSB to %x", bcd);
	}

	return ret;
}

static int cmd_device_pid(const struct shell *sh, size_t argc,
			  char *argv[])
{
	uint16_t pid;
	int ret;

	pid = strtol(argv[1], NULL, 16);
	ret = usbd_device_set_pid(&uds_ctx, pid);
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
	ret = usbd_device_set_vid(&uds_ctx, vid);
	if (ret) {
		shell_error(sh, "dev: failed to set device idVendor to %x", vid);
	}

	return ret;
}

static int cmd_device_class(const struct shell *sh, size_t argc,
			    char *argv[])
{
	uint8_t value;
	int ret;

	value = strtol(argv[1], NULL, 16);
	ret = usbd_device_set_class(&uds_ctx, value);
	if (ret) {
		shell_error(sh, "dev: failed to set device class to %x", value);
	}

	return ret;
}

static int cmd_device_subclass(const struct shell *sh, size_t argc,
			       char *argv[])
{
	uint8_t value;
	int ret;

	value = strtol(argv[1], NULL, 16);
	ret = usbd_device_set_subclass(&uds_ctx, value);
	if (ret) {
		shell_error(sh, "dev: failed to set device subclass to %x", value);
	}

	return ret;
}

static int cmd_device_proto(const struct shell *sh, size_t argc,
			    char *argv[])
{
	uint8_t value;
	int ret;

	value = strtol(argv[1], NULL, 16);
	ret = usbd_device_set_proto(&uds_ctx, value);
	if (ret) {
		shell_error(sh, "dev: failed to set device proto to %x", value);
	}

	return ret;
}

static int cmd_config_add(const struct shell *sh, size_t argc,
			  char *argv[])
{
	uint8_t cfg;
	int ret;

	cfg = strtol(argv[1], NULL, 10);

	if (cfg == 1) {
		ret = usbd_add_configuration(&uds_ctx, &config_foo);
	} else if (cfg == 2) {
		ret = usbd_add_configuration(&uds_ctx, &config_baz);
	} else {
		shell_error(sh, "dev: Configuration %u not available", cfg);
		return -EINVAL;
	}

	if (ret) {
		shell_error(sh, "dev: failed to add configuration %u", cfg);
	}

	return ret;
}

static int cmd_config_self(const struct shell *sh, size_t argc,
			   char *argv[])
{
	bool self;
	uint8_t cfg;
	int ret;

	cfg = strtol(argv[1], NULL, 10);
	if (!strcmp(argv[2], "yes")) {
		self = true;
	} else {
		self = false;
	}

	ret = usbd_config_attrib_self(&uds_ctx, cfg, self);
	if (ret) {
		shell_error(sh,
			    "dev: failed to set attribute self powered to %u",
			    cfg);
	}

	return ret;
}

static int cmd_config_rwup(const struct shell *sh, size_t argc,
			   char *argv[])
{
	bool rwup;
	uint8_t cfg;
	int ret;

	cfg = strtol(argv[1], NULL, 10);
	if (!strcmp(argv[2], "yes")) {
		rwup = true;
	} else {
		rwup = false;
	}

	ret = usbd_config_attrib_rwup(&uds_ctx, cfg, rwup);
	if (ret) {
		shell_error(sh,
			    "dev: failed to set attribute remote wakeup to %x",
			    cfg);
	}

	return ret;
}

static int cmd_config_power(const struct shell *sh, size_t argc,
			    char *argv[])
{
	uint8_t cfg;
	uint8_t power;
	int ret;

	cfg = strtol(argv[1], NULL, 10);
	power = strtol(argv[1], NULL, 10);

	ret = usbd_config_maxpower(&uds_ctx, cfg, power);
	if (ret) {
		shell_error(sh, "dev: failed to set bMaxPower value to %u", cfg);
	}

	return ret;
}

static void class_node_name_lookup(size_t idx, struct shell_static_entry *entry)
{
	size_t match_idx = 0;

	entry->syntax = NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;

	STRUCT_SECTION_FOREACH(usbd_class_node, node) {
		if ((node->name != NULL) && (strlen(node->name) != 0)) {
			if (match_idx == idx) {
				entry->syntax = node->name;
				break;
			}

			++match_idx;
		}
	}
}

SHELL_DYNAMIC_CMD_CREATE(dsub_node_name, class_node_name_lookup);

SHELL_STATIC_SUBCMD_SET_CREATE(device_cmds,
	SHELL_CMD_ARG(bcd, NULL, "<bcdUSB>",
		      cmd_device_bcd, 2, 0),
	SHELL_CMD_ARG(pid, NULL, "<idProduct>",
		      cmd_device_pid, 2, 0),
	SHELL_CMD_ARG(vid, NULL, "<idVendor>",
		      cmd_device_vid, 2, 0),
	SHELL_CMD_ARG(class, NULL, "<bDeviceClass>",
		      cmd_device_class, 2, 0),
	SHELL_CMD_ARG(subclass, NULL, "<bDeviceSubClass>",
		      cmd_device_subclass, 2, 0),
	SHELL_CMD_ARG(proto, NULL, "<bDeviceProtocol>",
		      cmd_device_proto, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(config_cmds,
	SHELL_CMD_ARG(add, NULL, "<configuration>",
		      cmd_config_add, 2, 0),
	SHELL_CMD_ARG(power, NULL, "<configuration> <bMaxPower>",
		      cmd_config_power, 3, 0),
	SHELL_CMD_ARG(rwup, NULL, "<configuration> <yes, no>",
		      cmd_config_rwup, 3, 0),
	SHELL_CMD_ARG(self, NULL, "<configuration> <yes, no>",
		      cmd_config_self, 3, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(class_cmds,
	SHELL_CMD_ARG(add, &dsub_node_name, "<name> <configuration>",
		      cmd_register, 3, 0),
	SHELL_CMD_ARG(add, &dsub_node_name, "<name> <configuration>",
		      cmd_unregister, 3, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_usbd_cmds,
	SHELL_CMD_ARG(wakeup, NULL, "[none]",
		      cmd_wakeup_request, 1, 0),
	SHELL_CMD_ARG(magic, NULL, "[none]",
		      cmd_usbd_magic, 1, 0),
	SHELL_CMD_ARG(defaults, NULL, "[none]",
		      cmd_usbd_defaults, 1, 0),
	SHELL_CMD_ARG(init, NULL, "[none]",
		      cmd_usbd_init, 1, 0),
	SHELL_CMD_ARG(enable, NULL, "[none]",
		      cmd_usbd_enable, 1, 0),
	SHELL_CMD_ARG(disable, NULL, "[none]",
		      cmd_usbd_disable, 1, 0),
	SHELL_CMD_ARG(shutdown, NULL, "[none]",
		      cmd_usbd_shutdown, 1, 0),
	SHELL_CMD_ARG(device, &device_cmds, "device commands",
		      NULL, 1, 0),
	SHELL_CMD_ARG(config, &config_cmds, "configuration commands",
		      NULL, 1, 0),
	SHELL_CMD_ARG(class, &class_cmds, "class commands",
		      NULL, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(usbd, &sub_usbd_cmds, "USB device support commands", NULL);
