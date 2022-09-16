/*
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/printk.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/can.h>
#include <zephyr/types.h>
#include <stdlib.h>

CAN_MSGQ_DEFINE(msgq, 4);
const struct shell *msgq_shell;
static struct k_work_poll msgq_work;
static struct k_poll_event msgq_events[1] = {
	K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
					K_POLL_MODE_NOTIFY_ONLY,
					&msgq, 0)
};

static inline int read_config_options(const struct shell *sh, int pos,
				      char **argv, bool *listenonly, bool *loopback)
{
	char *arg = argv[pos];

	if (arg[0] != '-') {
		return pos;
	}

	for (arg = &arg[1]; *arg; arg++) {
		switch (*arg) {
		case 's':
			if (listenonly == NULL) {
				shell_error(sh, "Unknown option %c", *arg);
			} else {
				*listenonly = true;
			}
			break;
		case 'l':
			if (loopback == NULL) {
				shell_error(sh, "Unknown option %c", *arg);
			} else {
				*loopback = true;
			}
			break;
		default:
			shell_error(sh, "Unknown option %c", *arg);
			return -EINVAL;
		}
	}

	return ++pos;
}

static inline int read_frame_options(const struct shell *sh, int pos,
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
				shell_error(sh, "Unknown option %c", *arg);
			} else {
				*rtr = true;
			}
			break;
		case 'e':
			if (ext == NULL) {
				shell_error(sh, "Unknown option %c", *arg);
			} else {
				*ext = true;
			}
			break;
		default:
			shell_error(sh, "Unknown option %c", *arg);
			return -EINVAL;
		}
	}

	return ++pos;
}

static inline int read_bitrate(const struct shell *sh, int pos, char **argv,
			       uint32_t *bitrate)
{
	char *end_ptr;
	long val;

	val = strtol(argv[pos], &end_ptr, 0);
	if (*end_ptr != '\0') {
		shell_error(sh, "Bitrate is not a number");
		return -EINVAL;
	}

	*bitrate = (uint32_t)val;

	return ++pos;
}

static inline int read_id(const struct shell *sh, int pos, char **argv,
			  bool ext, uint32_t *id)
{
	char *end_ptr;
	long val;

	val = strtol(argv[pos], &end_ptr, 0);
	if (*end_ptr != '\0') {
		shell_error(sh, "ID is not a number");
		return -EINVAL;
	}

	if (val < 0 || val > CAN_EXT_ID_MASK ||
	   (!ext && val > CAN_MAX_STD_ID)) {
		shell_error(sh, "ID invalid. %sid must not be negative or "
			    "bigger than 0x%x",
			    ext ? "ext " : "",
			    ext ? CAN_EXT_ID_MASK : CAN_MAX_STD_ID);
		return -EINVAL;
	}

	*id = (uint32_t)val;

	return ++pos;
}

static inline int read_mask(const struct shell *sh, int pos, char **argv,
			  bool ext, uint32_t *mask)
{
	char *end_ptr;
	long val;

	val = strtol(argv[pos], &end_ptr, 0);
	if (*end_ptr != '\0') {
		shell_error(sh, "Mask is not a number");
		return -EINVAL;
	}

	if (val < 0 || val > CAN_EXT_ID_MASK ||
	   (!ext && val > CAN_MAX_STD_ID)) {
		shell_error(sh, "Mask invalid. %smask must not be negative "
				"or bigger than 0x%x",
				ext ? "ext " : "",
				ext ? CAN_EXT_ID_MASK : CAN_MAX_STD_ID);
		return -EINVAL;
	}

	*mask = (uint32_t)val;

	return ++pos;
}

static inline int read_data(const struct shell *sh, int pos, char **argv,
			    size_t argc, uint8_t *data, uint8_t *dlc)
{
	int i;
	uint8_t *data_ptr = data;

	if (argc - pos > CAN_MAX_DLC) {
		shell_error(sh, "Too many databytes. Max is %d",
			    CAN_MAX_DLC);
		return -EINVAL;
	}

	for (i = pos; i < argc; i++) {
		char *end_ptr;
		long val;

		val = strtol(argv[i], &end_ptr, 0);
		if (*end_ptr != '\0') {
			shell_error(sh, "Data bytes must be numbers");
			return -EINVAL;
		}

		if (val & ~0xFFL) {
			shell_error(sh, "A data bytes must not be > 0xFF");
			return -EINVAL;
		}

		*data_ptr = val;
		data_ptr++;
	}

	*dlc = i - pos;

	return i;
}

static void print_frame(struct zcan_frame *frame, const struct shell *sh)
{
	shell_fprintf(sh, SHELL_NORMAL, "|0x%-8x|%s|%s|%d|",
		      frame->id,
		      frame->id_type == CAN_STANDARD_IDENTIFIER ? "std" : "ext",
		      frame->rtr ? "RTR" : "   ", frame->dlc);

	for (int i = 0; i < CAN_MAX_DLEN; i++) {
		if (i < frame->dlc) {
			shell_fprintf(sh, SHELL_NORMAL, " 0x%02x",
				      frame->data[i]);
		} else {
			shell_fprintf(sh, SHELL_NORMAL, "     ");
		}
	}

	shell_fprintf(sh, SHELL_NORMAL, "|\n");
}

static void msgq_triggered_work_handler(struct k_work *work)
{
	struct zcan_frame frame;
	int ret;

	while (k_msgq_get(&msgq, &frame, K_NO_WAIT) == 0) {
		print_frame(&frame, msgq_shell);
	}

	ret = k_work_poll_submit(&msgq_work, msgq_events,
				 ARRAY_SIZE(msgq_events), K_FOREVER);
	if (ret != 0) {
		shell_error(msgq_shell, "Failed to resubmit msgq polling [%d]", ret);
	}
}

static int cmd_config(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *can_dev;
	int pos = 1;
	bool listenonly = false, loopback = false;
	can_mode_t mode = CAN_MODE_NORMAL;
	uint32_t bitrate;
	int ret;

	can_dev = device_get_binding(argv[pos]);
	if (!can_dev) {
		shell_error(sh, "Can't get binding to device \"%s\"",
			    argv[pos]);
		return -EINVAL;
	}

	pos++;

	pos = read_config_options(sh, pos, argv, &listenonly, &loopback);
	if (pos < 0) {
		return -EINVAL;
	}

	if (listenonly) {
		mode |= CAN_MODE_LISTENONLY;
	}

	if (loopback) {
		mode |= CAN_MODE_LOOPBACK;
	}

	ret = can_set_mode(can_dev, mode);
	if (ret) {
		shell_error(sh, "Failed to set mode [%d]",
			    ret);
		return ret;
	}

	pos = read_bitrate(sh, pos, argv, &bitrate);
	if (pos < 0) {
		return -EINVAL;
	}

	ret = can_set_bitrate(can_dev, bitrate);
	if (ret) {
		shell_error(sh, "Failed to set bitrate [%d]",
			    ret);
		return ret;
	}

	return 0;
}

static int cmd_send(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *can_dev;
	int pos = 1;
	bool rtr = false, ext = false;
	struct zcan_frame frame;
	int ret;
	uint32_t id;

	can_dev = device_get_binding(argv[pos]);
	if (!can_dev) {
		shell_error(sh, "Can't get binding to device \"%s\"",
			    argv[pos]);
		return -EINVAL;
	}

	pos++;

	pos = read_frame_options(sh, pos, argv, &rtr, &ext);
	if (pos < 0) {
		return -EINVAL;
	}

	frame.id_type = ext ? CAN_EXTENDED_IDENTIFIER : CAN_STANDARD_IDENTIFIER;
	frame.rtr = rtr ? CAN_REMOTEREQUEST : CAN_DATAFRAME;

	pos = read_id(sh, pos, argv, ext, &id);
	if (pos < 0) {
		return -EINVAL;
	}

	frame.id = id;

	pos = read_data(sh, pos, argv, argc, frame.data, &frame.dlc);
	if (pos < 0) {
		return -EINVAL;
	}

	shell_print(sh, "Send frame with ID 0x%x (%s ID) and %d data bytes",
		    frame.id, ext ? "extended" : "standard", frame.dlc);

	ret = can_send(can_dev, &frame, K_FOREVER, NULL, NULL);
	if (ret) {
		shell_error(sh, "Failed to send frame [%d]", ret);
		return -EIO;
	}

	return 0;
}

static int cmd_add_rx_filter(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *can_dev;
	int pos = 1;
	bool rtr = false, ext = false, rtr_mask = false;
	struct zcan_filter filter;
	int ret;
	uint32_t id, mask;

	can_dev = device_get_binding(argv[pos]);
	if (!can_dev) {
		shell_error(sh, "Can't get binding to device \"%s\"",
			    argv[pos]);
		return -EINVAL;
	}

	pos++;

	pos = read_frame_options(sh, pos, argv, &rtr, &ext);
	if (pos < 0) {
		return -EINVAL;
	}

	filter.id_type = ext ? CAN_EXTENDED_IDENTIFIER : CAN_STANDARD_IDENTIFIER;
	filter.rtr = rtr ? CAN_REMOTEREQUEST : CAN_DATAFRAME;

	pos = read_id(sh, pos, argv, ext, &id);
	if (pos < 0) {
		return -EINVAL;
	}

	filter.id = id;

	if (pos != argc) {
		pos = read_mask(sh, pos, argv, ext, &mask);
		if (pos < 0) {
			return -EINVAL;
		}
		filter.id_mask = mask;
	} else {
		filter.id_mask = ext ? CAN_EXT_ID_MASK : CAN_STD_ID_MASK;
	}

	if (pos != argc) {
		pos = read_frame_options(sh, pos, argv, &rtr_mask, NULL);
		if (pos < 0) {
			return -EINVAL;
		}
	}

	filter.rtr_mask = rtr_mask;

	shell_print(sh, "Add RX filter with ID 0x%x (%s ID), mask 0x%x, RTR %d",
		    filter.id, ext ? "extended" : "standard", filter.id_mask,
		    filter.rtr_mask);

	ret = can_add_rx_filter_msgq(can_dev, &msgq, &filter);
	if (ret < 0) {
		if (ret == -ENOSPC) {
			shell_error(sh, "Failed to add RX filter, no free filter left");
		} else {
			shell_error(sh, "Failed to add RX filter [%d]", ret);
		}

		return -EIO;
	}

	shell_print(sh, "Filter ID: %d", ret);

	if (msgq_shell == NULL) {
		msgq_shell = sh;
		k_work_poll_init(&msgq_work, msgq_triggered_work_handler);
	}

	ret = k_work_poll_submit(&msgq_work, msgq_events,
				 ARRAY_SIZE(msgq_events), K_FOREVER);
	if (ret != 0) {
		shell_error(sh, "Failed to submit msgq polling [%d]", ret);
	}

	return 0;
}

static int cmd_remove_rx_filter(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *can_dev;
	char *end_ptr;
	long id;

	can_dev = device_get_binding(argv[1]);
	if (!can_dev) {
		shell_error(sh, "Can't get binding to device \"%s\"",
			    argv[1]);
		return -EINVAL;
	}

	id = strtol(argv[2], &end_ptr, 0);
	if (*end_ptr != '\0') {
		shell_error(sh, "filter_id is not a number");
		return -EINVAL;
	}

	if (id < 0) {
		shell_error(sh, "filter_id must not be negative");
	}

	can_remove_rx_filter(can_dev, (int)id);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_can,
	SHELL_CMD_ARG(config, NULL,
		      "Configure CAN controller.\n"
		      " Usage: config device_name [-sl] bitrate\n"
		      " -s Listen-only mode\n"
		      " -l Loopback mode",
		      cmd_config, 3, 1),
	SHELL_CMD_ARG(send, NULL,
		      "Send a CAN frame.\n"
		      " Usage: send device_name [-re] id [byte_1 byte_2 ...]\n"
		      " -r Remote transmission request\n"
		      " -e Extended address",
		      cmd_send, 3, 12),
	SHELL_CMD_ARG(add_rx_filter, NULL,
		      "Add a RX filter and print matching frames.\n"
		      " Usage: add_rx_filter device_name [-re] id [mask [-r]]\n"
		      " -r Remote transmission request\n"
		      " -e Extended address",
		      cmd_add_rx_filter, 3, 3),
	SHELL_CMD_ARG(remove_rx_filter, NULL,
		      "Remove a RX filter and stop printing matching frames\n"
		      " Usage: remove_rx_filter device_name filter_id",
		      cmd_remove_rx_filter, 3, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_ARG_REGISTER(canbus, &sub_can, "CAN commands", NULL, 2, 0);
