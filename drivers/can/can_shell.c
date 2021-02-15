/*
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/printk.h>
#include <shell/shell.h>
#include <drivers/can.h>
#include <zephyr/types.h>
#include <stdlib.h>

static struct zcan_work work;

static inline int read_config_options(const struct shell *shell, int pos,
				      char **argv, bool *silent, bool *loopback)
{
	char *arg = argv[pos];

	if (arg[0] != '-') {
		return pos;
	}

	for (arg = &arg[1]; *arg; arg++) {
		switch (*arg) {
		case 's':
			if (silent == NULL) {
				shell_error(shell, "unknown option %c", *arg);
			} else {
				*silent = true;
			}
			break;
		case 'l':
			if (loopback == NULL) {
				shell_error(shell, "unknown option %c", *arg);
			} else {
				*loopback = true;
			}
			break;
		default:
			shell_error(shell, "unknown option %c", *arg);
			return -EINVAL;
		}
	}

	return ++pos;
}

static inline int read_frame_options(const struct shell *shell, int pos,
				     char **argv, bool *rtr, bool *ext)
{
	char *arg = argv[pos];

	if (arg[0] != '-') {
		return pos;
	}

	for (arg = &arg[1]; *arg; arg++) {
		switch (*arg) {
		case 'r':
			if (rtr == NULL) {
				shell_error(shell, "unknown option %c", *arg);
			} else {
				*rtr = true;
			}
			break;
		case 'e':
			if (ext == NULL) {
				shell_error(shell, "unknown option %c", *arg);
			} else {
				*ext = true;
			}
			break;
		default:
			shell_error(shell, "unknown option %c", *arg);
			return -EINVAL;
		}
	}

	return ++pos;
}

static inline int read_bitrate(const struct shell *shell, int pos, char **argv,
			       uint32_t *bitrate)
{
	char *end_ptr;
	long val;

	val = strtol(argv[pos], &end_ptr, 0);
	if (*end_ptr != '\0') {
		shell_error(shell, "bitrate is not a number");
		return -EINVAL;
	}

	*bitrate = (uint32_t)val;

	return ++pos;
}

static inline int read_id(const struct shell *shell, int pos, char **argv,
			  bool ext, uint32_t *id)
{
	char *end_ptr;
	long val;

	val = strtol(argv[pos], &end_ptr, 0);
	if (*end_ptr != '\0') {
		shell_error(shell, "id is not a number");
		return -EINVAL;
	}

	if (val < 0 || val > CAN_EXT_ID_MASK ||
	   (!ext && val > CAN_MAX_STD_ID)) {
		shell_error(shell, "Id invalid. %sid must not be negative or "
				   "bigger than 0x%x",
				   ext ? "ext " : "",
				   ext ? CAN_EXT_ID_MASK : CAN_MAX_STD_ID);
		return -EINVAL;
	}

	*id = (uint32_t)val;

	return ++pos;
}

static inline int read_mask(const struct shell *shell, int pos, char **argv,
			  bool ext, uint32_t *mask)
{
	char *end_ptr;
	long val;

	val = strtol(argv[pos], &end_ptr, 0);
	if (*end_ptr != '\0') {
		shell_error(shell, "Mask is not a number");
		return -EINVAL;
	}

	if (val < 0 || val > CAN_EXT_ID_MASK ||
	   (!ext && val > CAN_MAX_STD_ID)) {
		shell_error(shell, "Mask invalid. %smask must not be negative "
				   "or bigger than 0x%x",
				   ext ? "ext " : "",
				   ext ? CAN_EXT_ID_MASK : CAN_MAX_STD_ID);
		return -EINVAL;
	}

	*mask = (uint32_t)val;

	return ++pos;
}

static inline int read_data(const struct shell *shell, int pos, char **argv,
			    size_t argc, uint8_t *data, uint8_t *dlc)
{
	int i;
	uint8_t *data_ptr = data;

	if (argc - pos > CAN_MAX_DLC) {
		shell_error(shell, "Too many databytes. Max is %d",
			    CAN_MAX_DLC);
		return -EINVAL;
	}

	for (i = pos; i < argc; i++) {
		char *end_ptr;
		long val;

		val = strtol(argv[i], &end_ptr, 0);
		if (*end_ptr != '\0') {
			shell_error(shell, "Data bytes must be numbers");
			return -EINVAL;
		}

		if (val & ~0xFFL) {
			shell_error(shell, "A data bytes must not be > 0xFF");
			return -EINVAL;
		}

		*data_ptr = val;
		data_ptr++;
	}

	*dlc = i - pos;

	return i;
}

static void print_frame(struct zcan_frame *frame, void *arg)
{
	const struct shell *shell = (const struct shell *)arg;

	shell_fprintf(shell, SHELL_NORMAL, "|0x%-8x|%s|%s|%d|",
		      frame->id,
		      frame->id_type == CAN_STANDARD_IDENTIFIER ? "std" : "ext",
		      frame->rtr ? "RTR" : "   ", frame->dlc);

	for (int i = 0; i < CAN_MAX_DLEN; i++) {
		if (i < frame->dlc) {
			shell_fprintf(shell, SHELL_NORMAL, " 0x%02x",
				      frame->data[i]);
		} else {
			shell_fprintf(shell, SHELL_NORMAL, "     ");
		}
	}

	shell_fprintf(shell, SHELL_NORMAL, "|\n");
}

static int cmd_config(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *can_dev;
	int pos = 1;
	bool silent = false, loopback = false;
	enum can_mode mode;
	uint32_t bitrate;
	int ret;

	can_dev = device_get_binding(argv[pos]);
	if (!can_dev) {
		shell_error(shell, "Can't get binding to device \"%s\"",
			    argv[pos]);
		return -EINVAL;
	}

	pos++;

	pos = read_config_options(shell, pos, argv, &silent, &loopback);
	if (pos < 0) {
		return -EINVAL;
	}

	if (silent && loopback) {
		mode = CAN_SILENT_LOOPBACK_MODE;
	} else if (silent) {
		mode = CAN_SILENT_MODE;
	} else if (loopback) {
		mode = CAN_LOOPBACK_MODE;
	} else {
		mode = CAN_NORMAL_MODE;
	}

	ret = can_set_mode(can_dev, mode);
	if (ret) {
		shell_error(shell, "Failed to set mode [%d]",
			    ret);
		return ret;
	}

	pos = read_bitrate(shell, pos, argv, &bitrate);
	if (pos < 0) {
		return -EINVAL;
	}

	ret = can_set_bitrate(can_dev, bitrate, 0);
	if (ret) {
		shell_error(shell, "Failed to set bitrate [%d]",
			    ret);
		return ret;
	}

	return 0;
}

static int cmd_send(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *can_dev;
	int pos = 1;
	bool rtr = false, ext = false;
	struct zcan_frame frame;
	int ret;
	uint32_t id;

	can_dev = device_get_binding(argv[pos]);
	if (!can_dev) {
		shell_error(shell, "Can't get binding to device \"%s\"",
			    argv[pos]);
		return -EINVAL;
	}

	pos++;

	pos = read_frame_options(shell, pos, argv, &rtr, &ext);
	if (pos < 0) {
		return -EINVAL;
	}

	frame.id_type = ext ? CAN_EXTENDED_IDENTIFIER : CAN_STANDARD_IDENTIFIER;
	frame.rtr = rtr ? CAN_REMOTEREQUEST : CAN_DATAFRAME;

	pos = read_id(shell, pos, argv, ext, &id);
	if (pos < 0) {
		return -EINVAL;
	}

	frame.id = id;

	pos = read_data(shell, pos, argv, argc, frame.data, &frame.dlc);
	if (pos < 0) {
		return -EINVAL;
	}

	shell_print(shell, "Send frame with ID 0x%x (%s id) and %d data bytes",
		    frame.id, ext ? "extended" : "standard", frame.dlc);

	ret = can_send(can_dev, &frame, K_FOREVER, NULL, NULL);
	if (ret) {
		shell_error(shell, "Failed to send frame [%d]", ret);
		return -EIO;
	}

	return 0;
}

static int cmd_attach(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *can_dev;
	int pos = 1;
	bool rtr = false, ext = false, rtr_mask = false;
	struct zcan_filter filter;
	int ret;
	uint32_t id, mask;

	can_dev = device_get_binding(argv[pos]);
	if (!can_dev) {
		shell_error(shell, "Can't get binding to device \"%s\"",
			    argv[pos]);
		return -EINVAL;
	}

	pos++;

	pos = read_frame_options(shell, pos, argv, &rtr, &ext);
	if (pos < 0) {
		return -EINVAL;
	}

	filter.id_type = ext ? CAN_EXTENDED_IDENTIFIER : CAN_STANDARD_IDENTIFIER;
	filter.rtr = rtr ? CAN_REMOTEREQUEST : CAN_DATAFRAME;

	pos = read_id(shell, pos, argv, ext, &id);
	if (pos < 0) {
		return -EINVAL;
	}

	filter.id = id;

	if (pos != argc) {
		pos = read_mask(shell, pos, argv, ext, &mask);
		if (pos < 0) {
			return -EINVAL;
		}
		filter.id_mask = mask;
	} else {
		filter.id_mask = ext ? CAN_EXT_ID_MASK : CAN_STD_ID_MASK;
	}

	if (pos != argc) {
		pos = read_frame_options(shell, pos, argv, &rtr_mask, NULL);
		if (pos < 0) {
			return -EINVAL;
		}
	}

	filter.rtr_mask = rtr_mask;

	shell_print(shell, "Attach filter with ID 0x%x (%s id) and mask 0x%x "
			   " RTR: %d",
		    filter.id, ext ? "extended" : "standard", filter.id_mask,
		    filter.rtr_mask);

	ret = can_attach_workq(can_dev, &k_sys_work_q, &work, print_frame,
			       (void *)shell, &filter);
	if (ret < 0) {
		if (ret == CAN_NO_FREE_FILTER) {
			shell_error(shell, "Can't attach, no free filter left");
		} else {
			shell_error(shell, "Failed to attach filter [%d]", ret);
		}

		return -EIO;
	}

	shell_print(shell, "Filter ID: %d", ret);

	return 0;
}

static int cmd_detach(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *can_dev;
	char *end_ptr;
	long id;

	can_dev = device_get_binding(argv[1]);
	if (!can_dev) {
		shell_error(shell, "Can't get binding to device \"%s\"",
			    argv[1]);
		return -EINVAL;
	}



	id = strtol(argv[2], &end_ptr, 0);
	if (*end_ptr != '\0') {
		shell_error(shell, "filter_id is not a number");
		return -EINVAL;
	}

	if (id < 0) {
		shell_error(shell, "filter_id must not be negative");
	}

	can_detach(can_dev, (int)id);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_can,
	SHELL_CMD_ARG(config, NULL,
		      "Configure CAN controller.\n"
		      " Usage: config device_name [-sl] bitrate\n"
		      " -s Silent mode\n"
		      " -l Listen-only mode",
		      cmd_config, 3, 1),
	SHELL_CMD_ARG(send, NULL,
		      "Send a CAN frame.\n"
		      " Usage: send device_name [-re] id [byte_1 byte_2 ...]\n"
		      " -r Remote transmission request\n"
		      " -e Extended address",
		      cmd_send, 3, 12),
	SHELL_CMD_ARG(attach, NULL,
		      "Attach a message filter and print those messages.\n"
		      " Usage: attach device_name [-re] id [mask [-r]]\n"
		      " -r Remote transmission request\n"
		      " -e Extended address",
		      cmd_attach, 3, 3),
	SHELL_CMD_ARG(detach, NULL,
		      "Detach the filter and stop receiving those messages\n"
		      " Usage: detach device_name filter_id",
		      cmd_detach, 3, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_ARG_REGISTER(canbus, &sub_can, "CAN commands", NULL, 2, 0);
