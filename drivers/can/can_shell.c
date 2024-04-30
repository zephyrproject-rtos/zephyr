/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

LOG_MODULE_REGISTER(can_shell, CONFIG_CAN_LOG_LEVEL);

struct can_shell_tx_event {
	unsigned int frame_no;
	int error;
};

struct can_shell_mode_mapping {
	const char *name;
	can_mode_t mode;
};

#define CAN_SHELL_MODE_MAPPING(_name, _mode) { .name = _name, .mode = _mode }

static const struct can_shell_mode_mapping can_shell_mode_map[] = {
	/* zephyr-keep-sorted-start */
	CAN_SHELL_MODE_MAPPING("fd",              CAN_MODE_FD),
	CAN_SHELL_MODE_MAPPING("listen-only",     CAN_MODE_LISTENONLY),
	CAN_SHELL_MODE_MAPPING("loopback",        CAN_MODE_LOOPBACK),
	CAN_SHELL_MODE_MAPPING("manual-recovery", CAN_MODE_MANUAL_RECOVERY),
	CAN_SHELL_MODE_MAPPING("normal",          CAN_MODE_NORMAL),
	CAN_SHELL_MODE_MAPPING("one-shot",        CAN_MODE_ONE_SHOT),
	CAN_SHELL_MODE_MAPPING("triple-sampling", CAN_MODE_3_SAMPLES),
	/* zephyr-keep-sorted-stop */
};

K_MSGQ_DEFINE(can_shell_tx_msgq, sizeof(struct can_shell_tx_event),
	      CONFIG_CAN_SHELL_TX_QUEUE_SIZE, 4);
const struct shell *can_shell_tx_msgq_sh;
static struct k_work_poll can_shell_tx_msgq_work;
static struct k_poll_event can_shell_tx_msgq_events[] = {
	K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
					K_POLL_MODE_NOTIFY_ONLY,
					&can_shell_tx_msgq, 0)
};

CAN_MSGQ_DEFINE(can_shell_rx_msgq, CONFIG_CAN_SHELL_RX_QUEUE_SIZE);
const struct shell *can_shell_rx_msgq_sh;
static struct k_work_poll can_shell_rx_msgq_work;
static struct k_poll_event can_shell_rx_msgq_events[] = {
	K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
					K_POLL_MODE_NOTIFY_ONLY,
					&can_shell_rx_msgq, 0)
};

/* Forward declarations */
static void can_shell_tx_msgq_triggered_work_handler(struct k_work *work);
static void can_shell_rx_msgq_triggered_work_handler(struct k_work *work);

static void can_shell_print_frame(const struct shell *sh, const struct can_frame *frame)
{
	uint8_t nbytes = can_dlc_to_bytes(frame->dlc);
	int i;

#ifdef CONFIG_CAN_RX_TIMESTAMP
	/* Timestamp */
	shell_fprintf(sh, SHELL_NORMAL, "(%05d)  ", frame->timestamp);
#endif /* CONFIG_CAN_RX_TIMESTAMP */

#ifdef CONFIG_CAN_FD_MODE
	/* Flags */
	shell_fprintf(sh, SHELL_NORMAL, "%c%c  ",
		      (frame->flags & CAN_FRAME_BRS) == 0 ? '-' : 'B',
		      (frame->flags & CAN_FRAME_ESI) == 0 ? '-' : 'P');
#endif /* CONFIG_CAN_FD_MODE */

	/* CAN ID */
	shell_fprintf(sh, SHELL_NORMAL, "%*s%0*x  ",
		(frame->flags & CAN_FRAME_IDE) != 0 ? 0 : 5, "",
		(frame->flags & CAN_FRAME_IDE) != 0 ? 8 : 3,
		(frame->flags & CAN_FRAME_IDE) != 0 ?
		frame->id & CAN_EXT_ID_MASK : frame->id & CAN_STD_ID_MASK);

	/* DLC as number of bytes */
	shell_fprintf(sh, SHELL_NORMAL, "%s[%0*d]  ",
		(frame->flags & CAN_FRAME_FDF) != 0 ? "" : " ",
		(frame->flags & CAN_FRAME_FDF) != 0 ? 2 : 1,
		nbytes);

	/* Data payload */
	if ((frame->flags & CAN_FRAME_RTR) != 0) {
		shell_fprintf(sh, SHELL_NORMAL, "remote transmission request");
	} else {
		for (i = 0; i < nbytes; i++) {
			shell_fprintf(sh, SHELL_NORMAL, "%02x ", frame->data[i]);
		}
	}

	shell_fprintf(sh, SHELL_NORMAL, "\n");
}

static int can_shell_tx_msgq_poll_submit(const struct shell *sh)
{
	int err;

	if (can_shell_tx_msgq_sh == NULL) {
		can_shell_tx_msgq_sh = sh;
		k_work_poll_init(&can_shell_tx_msgq_work, can_shell_tx_msgq_triggered_work_handler);
	}

	err = k_work_poll_submit(&can_shell_tx_msgq_work, can_shell_tx_msgq_events,
				 ARRAY_SIZE(can_shell_tx_msgq_events), K_FOREVER);
	if (err != 0) {
		shell_error(can_shell_tx_msgq_sh, "failed to submit tx msgq polling (err %d)",
			    err);
	}

	return err;
}

static void can_shell_tx_msgq_triggered_work_handler(struct k_work *work)
{
	struct can_shell_tx_event event;

	while (k_msgq_get(&can_shell_tx_msgq, &event, K_NO_WAIT) == 0) {
		if (event.error == 0) {
			shell_print(can_shell_tx_msgq_sh, "CAN frame #%u successfully sent",
				    event.frame_no);
		} else {
			shell_error(can_shell_tx_msgq_sh, "failed to send CAN frame #%u (err %d)",
				    event.frame_no, event.error);
		}
	}

	(void)can_shell_tx_msgq_poll_submit(can_shell_tx_msgq_sh);
}

static void can_shell_tx_callback(const struct device *dev, int error, void *user_data)
{
	struct can_shell_tx_event event;
	int err;

	ARG_UNUSED(dev);

	event.frame_no = POINTER_TO_UINT(user_data);
	event.error = error;

	err = k_msgq_put(&can_shell_tx_msgq, &event, K_NO_WAIT);
	if (err != 0) {
		LOG_ERR("CAN shell tx event queue full");
	}
}

static int can_shell_rx_msgq_poll_submit(const struct shell *sh)
{
	int err;

	if (can_shell_rx_msgq_sh == NULL) {
		can_shell_rx_msgq_sh = sh;
		k_work_poll_init(&can_shell_rx_msgq_work, can_shell_rx_msgq_triggered_work_handler);
	}

	err = k_work_poll_submit(&can_shell_rx_msgq_work, can_shell_rx_msgq_events,
				 ARRAY_SIZE(can_shell_rx_msgq_events), K_FOREVER);
	if (err != 0) {
		shell_error(can_shell_rx_msgq_sh, "failed to submit rx msgq polling (err %d)",
			    err);
	}

	return err;
}

static void can_shell_rx_msgq_triggered_work_handler(struct k_work *work)
{
	struct can_frame frame;

	while (k_msgq_get(&can_shell_rx_msgq, &frame, K_NO_WAIT) == 0) {
		can_shell_print_frame(can_shell_rx_msgq_sh, &frame);
	}

	(void)can_shell_rx_msgq_poll_submit(can_shell_rx_msgq_sh);
}

static const char *can_shell_state_to_string(enum can_state state)
{
	switch (state) {
	case CAN_STATE_ERROR_ACTIVE:
		return "error-active";
	case CAN_STATE_ERROR_WARNING:
		return "error-warning";
	case CAN_STATE_ERROR_PASSIVE:
		return "error-passive";
	case CAN_STATE_BUS_OFF:
		return "bus-off";
	case CAN_STATE_STOPPED:
		return "stopped";
	default:
		return "unknown";
	}
}

static void can_shell_print_extended_modes(const struct shell *sh, can_mode_t cap)
{
	int bit;
	int i;

	for (bit = 0; bit < sizeof(cap) * 8; bit++) {
		/* Skip unset bits */
		if ((cap & BIT(bit)) == 0) {
			continue;
		}

		/* Lookup symbolic mode name */
		for (i = 0; i < ARRAY_SIZE(can_shell_mode_map); i++) {
			if (BIT(bit) == can_shell_mode_map[i].mode) {
				shell_fprintf(sh, SHELL_NORMAL, "%s ", can_shell_mode_map[i].name);
				break;
			}
		}

		if (i == ARRAY_SIZE(can_shell_mode_map)) {
			/* Symbolic name not found, use raw mode */
			shell_fprintf(sh, SHELL_NORMAL, "0x%08x ", (can_mode_t)BIT(bit));
		}
	}
}

static int cmd_can_start(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = device_get_binding(argv[1]);
	int err;

	if (!device_is_ready(dev)) {
		shell_error(sh, "device %s not ready", argv[1]);
		return -ENODEV;
	}

	shell_print(sh, "starting %s", argv[1]);

	err = can_start(dev);
	if (err != 0) {
		shell_error(sh, "failed to start CAN controller (err %d)", err);
		return err;
	}

	return 0;
}

static int cmd_can_stop(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = device_get_binding(argv[1]);
	int err;

	if (!device_is_ready(dev)) {
		shell_error(sh, "device %s not ready", argv[1]);
		return -ENODEV;
	}

	shell_print(sh, "stopping %s", argv[1]);

	err = can_stop(dev);
	if (err != 0) {
		shell_error(sh, "failed to stop CAN controller (err %d)", err);
		return err;
	}

	return 0;
}

static int cmd_can_show(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = device_get_binding(argv[1]);
	const struct device *phy;
	const struct can_timing *timing_min;
	const struct can_timing *timing_max;
	struct can_bus_err_cnt err_cnt;
	enum can_state state;
	uint32_t max_bitrate = 0;
	int max_std_filters = 0;
	int max_ext_filters = 0;
	uint32_t core_clock;
	can_mode_t cap;
	int err;

	if (!device_is_ready(dev)) {
		shell_error(sh, "device %s not ready", argv[1]);
		return -ENODEV;
	}

	err = can_get_core_clock(dev, &core_clock);
	if (err != 0) {
		shell_error(sh, "failed to get CAN core clock (err %d)", err);
		return err;
	}

	err = can_get_max_bitrate(dev, &max_bitrate);
	if (err != 0 && err != -ENOSYS) {
		shell_error(sh, "failed to get maximum bitrate (err %d)", err);
		return err;
	}

	max_std_filters = can_get_max_filters(dev, false);
	if (max_std_filters < 0 && max_std_filters != -ENOSYS) {
		shell_error(sh, "failed to get maximum standard (11-bit) filters (err %d)", err);
		return err;
	}

	max_ext_filters = can_get_max_filters(dev, true);
	if (max_ext_filters < 0 && max_ext_filters != -ENOSYS) {
		shell_error(sh, "failed to get maximum extended (29-bit) filters (err %d)", err);
		return err;
	}

	err = can_get_capabilities(dev, &cap);
	if (err != 0) {
		shell_error(sh, "failed to get CAN controller capabilities (err %d)", err);
		return err;
	}

	err = can_get_state(dev, &state, &err_cnt);
	if (err != 0) {
		shell_error(sh, "failed to get CAN controller state (%d)", err);
		return err;
	}

	shell_print(sh, "core clock:      %d Hz", core_clock);
	shell_print(sh, "max bitrate:     %d bps", max_bitrate);
	shell_print(sh, "max std filters: %d", max_std_filters);
	shell_print(sh, "max ext filters: %d", max_ext_filters);

	shell_fprintf(sh, SHELL_NORMAL, "capabilities:    normal ");
	can_shell_print_extended_modes(sh, cap);
	shell_fprintf(sh, SHELL_NORMAL, "\n");

	shell_fprintf(sh, SHELL_NORMAL, "mode:            normal ");
	can_shell_print_extended_modes(sh, can_get_mode(dev));
	shell_fprintf(sh, SHELL_NORMAL, "\n");

	shell_print(sh, "state:           %s", can_shell_state_to_string(state));
	shell_print(sh, "rx errors:       %d", err_cnt.rx_err_cnt);
	shell_print(sh, "tx errors:       %d", err_cnt.tx_err_cnt);

	timing_min = can_get_timing_min(dev);
	timing_max = can_get_timing_max(dev);

	shell_print(sh, "timing:          sjw %u..%u, prop_seg %u..%u, "
		    "phase_seg1 %u..%u, phase_seg2 %u..%u, prescaler %u..%u",
		    timing_min->sjw, timing_max->sjw,
		    timing_min->prop_seg, timing_max->prop_seg,
		    timing_min->phase_seg1, timing_max->phase_seg1,
		    timing_min->phase_seg2, timing_max->phase_seg2,
		    timing_min->prescaler, timing_max->prescaler);

	if (IS_ENABLED(CONFIG_CAN_FD_MODE) && (cap & CAN_MODE_FD) != 0) {
		timing_min = can_get_timing_data_min(dev);
		timing_max = can_get_timing_data_max(dev);

		shell_print(sh, "timing data:     sjw %u..%u, prop_seg %u..%u, "
			    "phase_seg1 %u..%u, phase_seg2 %u..%u, prescaler %u..%u",
			    timing_min->sjw, timing_max->sjw,
			    timing_min->prop_seg, timing_max->prop_seg,
			    timing_min->phase_seg1, timing_max->phase_seg1,
			    timing_min->phase_seg2, timing_max->phase_seg2,
			    timing_min->prescaler, timing_max->prescaler);
	}

	phy = can_get_transceiver(dev);
	shell_print(sh, "transceiver:     %s", phy != NULL ? phy->name : "passive/none");

#ifdef CONFIG_CAN_STATS
	shell_print(sh, "statistics:");
	shell_print(sh, "  bit errors:    %u", can_stats_get_bit_errors(dev));
	shell_print(sh, "    bit0 errors: %u", can_stats_get_bit0_errors(dev));
	shell_print(sh, "    bit1 errors: %u", can_stats_get_bit1_errors(dev));
	shell_print(sh, "  stuff errors:  %u", can_stats_get_stuff_errors(dev));
	shell_print(sh, "  crc errors:    %u", can_stats_get_crc_errors(dev));
	shell_print(sh, "  form errors:   %u", can_stats_get_form_errors(dev));
	shell_print(sh, "  ack errors:    %u", can_stats_get_ack_errors(dev));
	shell_print(sh, "  rx overruns:   %u", can_stats_get_rx_overruns(dev));
#endif /* CONFIG_CAN_STATS */

	return 0;
}

static int cmd_can_bitrate_set(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = device_get_binding(argv[1]);
	struct can_timing timing = { 0 };
	uint16_t sample_pnt;
	uint32_t bitrate;
	char *endptr;
	int err;

	if (!device_is_ready(dev)) {
		shell_error(sh, "device %s not ready", argv[1]);
		return -ENODEV;
	}

	bitrate = (uint32_t)strtoul(argv[2], &endptr, 10);
	if (*endptr != '\0') {
		shell_error(sh, "failed to parse bitrate");
		return -EINVAL;
	}

	if (argc >= 4) {
		sample_pnt = (uint32_t)strtoul(argv[3], &endptr, 10);
		if (*endptr != '\0') {
			shell_error(sh, "failed to parse sample point");
			return -EINVAL;
		}

		err = can_calc_timing(dev, &timing, bitrate, sample_pnt);
		if (err < 0) {
			shell_error(sh, "failed to calculate timing for "
				    "bitrate %d bps, sample point %d.%d%% (err %d)",
				    bitrate, sample_pnt / 10, sample_pnt % 10, err);
			return err;
		}

		if (argc >= 5) {
			/* Overwrite calculated default SJW with user-provided value */
			timing.sjw = (uint16_t)strtoul(argv[4], &endptr, 10);
			if (*endptr != '\0') {
				shell_error(sh, "failed to parse SJW");
				return -EINVAL;
			}
		}

		shell_print(sh, "setting bitrate to %d bps, sample point %d.%d%% "
			    "(+/- %d.%d%%), sjw %d",
			    bitrate, sample_pnt / 10, sample_pnt % 10, err / 10, err % 10,
			    timing.sjw);

		LOG_DBG("sjw %u, prop_seg %u, phase_seg1 %u, phase_seg2 %u, prescaler %u",
			timing.sjw, timing.prop_seg, timing.phase_seg1, timing.phase_seg2,
			timing.prescaler);

		err = can_set_timing(dev, &timing);
		if (err != 0) {
			shell_error(sh, "failed to set timing (err %d)", err);
			return err;
		}
	} else {
		shell_print(sh, "setting bitrate to %d bps", bitrate);

		err = can_set_bitrate(dev, bitrate);
		if (err != 0) {
			shell_error(sh, "failed to set bitrate (err %d)", err);
			return err;
		}
	}

	return 0;
}

static int cmd_can_dbitrate_set(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = device_get_binding(argv[1]);
	struct can_timing timing = { 0 };
	uint16_t sample_pnt;
	uint32_t bitrate;
	char *endptr;
	int err;

	if (!device_is_ready(dev)) {
		shell_error(sh, "device %s not ready", argv[1]);
		return -ENODEV;
	}

	bitrate = (uint32_t)strtoul(argv[2], &endptr, 10);
	if (*endptr != '\0') {
		shell_error(sh, "failed to parse data bitrate");
		return -EINVAL;
	}

	if (argc >= 4) {
		sample_pnt = (uint32_t)strtoul(argv[3], &endptr, 10);
		if (*endptr != '\0') {
			shell_error(sh, "failed to parse sample point");
			return -EINVAL;
		}

		err = can_calc_timing_data(dev, &timing, bitrate, sample_pnt);
		if (err < 0) {
			shell_error(sh, "failed to calculate timing for "
				    "data bitrate %d bps, sample point %d.%d%% (err %d)",
				    bitrate, sample_pnt / 10, sample_pnt % 10, err);
			return err;
		}

		if (argc >= 5) {
			/* Overwrite calculated default SJW with user-provided value */
			timing.sjw = (uint16_t)strtoul(argv[4], &endptr, 10);
			if (*endptr != '\0') {
				shell_error(sh, "failed to parse SJW");
				return -EINVAL;
			}
		}

		shell_print(sh, "setting data bitrate to %d bps, sample point %d.%d%% "
			    "(+/- %d.%d%%), sjw %d",
			    bitrate, sample_pnt / 10, sample_pnt % 10, err / 10, err % 10,
			    timing.sjw);

		LOG_DBG("sjw %u, prop_seg %u, phase_seg1 %u, phase_seg2 %u, prescaler %u",
			timing.sjw, timing.prop_seg, timing.phase_seg1, timing.phase_seg2,
			timing.prescaler);

		err = can_set_timing_data(dev, &timing);
		if (err != 0) {
			shell_error(sh, "failed to set data timing (err %d)", err);
			return err;
		}
	} else {
		shell_print(sh, "setting data bitrate to %d bps", bitrate);

		err = can_set_bitrate_data(dev, bitrate);
		if (err != 0) {
			shell_error(sh, "failed to set data bitrate (err %d)", err);
			return err;
		}
	}

	return 0;
}

static int can_shell_parse_timing(const struct shell *sh, size_t argc, char **argv,
				  struct can_timing *timing)
{
	char *endptr;

	timing->sjw = (uint32_t)strtoul(argv[2], &endptr, 10);
	if (*endptr != '\0') {
		shell_error(sh, "failed to parse sjw");
		return -EINVAL;
	}

	timing->prop_seg = (uint32_t)strtoul(argv[3], &endptr, 10);
	if (*endptr != '\0') {
		shell_error(sh, "failed to parse prop_seg");
		return -EINVAL;
	}

	timing->phase_seg1 = (uint32_t)strtoul(argv[4], &endptr, 10);
	if (*endptr != '\0') {
		shell_error(sh, "failed to parse phase_seg1");
		return -EINVAL;
	}

	timing->phase_seg2 = (uint32_t)strtoul(argv[5], &endptr, 10);
	if (*endptr != '\0') {
		shell_error(sh, "failed to parse phase_seg2");
		return -EINVAL;
	}

	timing->prescaler = (uint32_t)strtoul(argv[6], &endptr, 10);
	if (*endptr != '\0') {
		shell_error(sh, "failed to parse prescaler");
		return -EINVAL;
	}

	return 0;
}

static int cmd_can_timing_set(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = device_get_binding(argv[1]);
	struct can_timing timing = { 0 };
	int err;

	if (!device_is_ready(dev)) {
		shell_error(sh, "device %s not ready", argv[1]);
		return -ENODEV;
	}

	err = can_shell_parse_timing(sh, argc, argv, &timing);
	if (err < 0) {
		return err;
	}

	shell_print(sh, "setting timing to sjw %u, prop_seg %u, phase_seg1 %u, phase_seg2 %u, "
		    "prescaler %u", timing.sjw, timing.prop_seg, timing.phase_seg1,
		    timing.phase_seg2, timing.prescaler);

	err = can_set_timing(dev, &timing);
	if (err != 0) {
		shell_error(sh, "failed to set timing (err %d)", err);
		return err;
	}

	return 0;
}

static int cmd_can_dtiming_set(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = device_get_binding(argv[1]);
	struct can_timing timing = { 0 };
	int err;

	if (!device_is_ready(dev)) {
		shell_error(sh, "device %s not ready", argv[1]);
		return -ENODEV;
	}

	err = can_shell_parse_timing(sh, argc, argv, &timing);
	if (err < 0) {
		return err;
	}

	shell_print(sh, "setting data phase timing to sjw %u, prop_seg %u, phase_seg1 %u, "
		    "phase_seg2 %u, prescaler %u", timing.sjw, timing.prop_seg, timing.phase_seg1,
		    timing.phase_seg2, timing.prescaler);

	err = can_set_timing_data(dev, &timing);
	if (err != 0) {
		shell_error(sh, "failed to set data phase timing (err %d)", err);
		return err;
	}

	return 0;
}

static int cmd_can_mode_set(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = device_get_binding(argv[1]);
	can_mode_t mode = CAN_MODE_NORMAL;
	can_mode_t raw;
	char *endptr;
	int err;
	int i;
	int j;

	if (!device_is_ready(dev)) {
		shell_error(sh, "device %s not ready", argv[1]);
		return -ENODEV;
	}

	for (i = 2; i < argc; i++) {
		/* Lookup symbolic mode name */
		for (j = 0; j < ARRAY_SIZE(can_shell_mode_map); j++) {
			if (strcmp(argv[i], can_shell_mode_map[j].name) == 0) {
				mode |= can_shell_mode_map[j].mode;
				break;
			}
		}

		if (j == ARRAY_SIZE(can_shell_mode_map)) {
			/* Symbolic name not found, use raw mode if hex number */
			raw = (can_mode_t)strtoul(argv[i], &endptr, 16);
			if (*endptr == '\0') {
				mode |= raw;
				continue;
			}

			shell_error(sh, "failed to parse mode");
			return -EINVAL;
		}
	}

	shell_print(sh, "setting mode 0x%08x", mode);

	err = can_set_mode(dev, mode);
	if (err != 0) {
		shell_error(sh, "failed to set mode 0x%08x (err %d)", mode, err);
		return err;
	}

	return 0;
}

static int cmd_can_send(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = device_get_binding(argv[1]);
	static unsigned int frame_counter;
	unsigned int frame_no;
	struct can_frame frame;
	uint32_t id_mask;
	int argidx = 2;
	uint32_t val;
	char *endptr;
	int nbytes;
	int err;
	int i;

	if (!device_is_ready(dev)) {
		shell_error(sh, "device %s not ready", argv[1]);
		return -ENODEV;
	}

	/* Defaults */
	id_mask = CAN_STD_ID_MASK;
	frame.flags = 0;
	frame.dlc = 0;

	/* Parse options */
	while (argidx < argc && strncmp(argv[argidx], "-", 1) == 0) {
		if (strcmp(argv[argidx], "--") == 0) {
			argidx++;
			break;
		} else if (strcmp(argv[argidx], "-e") == 0) {
			frame.flags |= CAN_FRAME_IDE;
			id_mask = CAN_EXT_ID_MASK;
			argidx++;
		} else if (strcmp(argv[argidx], "-r") == 0) {
			frame.flags |= CAN_FRAME_RTR;
			argidx++;
		} else if (strcmp(argv[argidx], "-f") == 0) {
			frame.flags |= CAN_FRAME_FDF;
			argidx++;
		} else if (strcmp(argv[argidx], "-b") == 0) {
			frame.flags |= CAN_FRAME_BRS;
			argidx++;
		} else {
			shell_error(sh, "unsupported option %s", argv[argidx]);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
	}

	/* Parse CAN ID */
	if (argidx >= argc) {
		shell_error(sh, "missing CAN ID parameter");
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	val = (uint32_t)strtoul(argv[argidx++], &endptr, 16);
	if (*endptr != '\0') {
		shell_error(sh, "failed to parse CAN ID");
		return -EINVAL;
	}

	if (val > id_mask) {
		shell_error(sh, "CAN ID 0x%0*x out of range",
			    (frame.flags & CAN_FRAME_IDE) != 0 ? 8 : 3,
			    val);
		return -EINVAL;
	}

	frame.id = val;

	nbytes = argc - argidx;
	if (nbytes > ARRAY_SIZE(frame.data)) {
		shell_error(sh, "excessive amount of data (%d bytes)", nbytes);
		return -EINVAL;
	}

	frame.dlc = can_bytes_to_dlc(nbytes);

	/* Parse data */
	for (i = 0; i < nbytes; i++) {
		val = (uint32_t)strtoul(argv[argidx++], &endptr, 16);
		if (*endptr != '\0') {
			shell_error(sh, "failed to parse data %s", argv[argidx++]);
			return -EINVAL;
		}

		if (val > 0xff) {
			shell_error(sh, "data 0x%x out of range", val);
			return -EINVAL;
		}

		frame.data[i] = val;
	}

	err = can_shell_tx_msgq_poll_submit(sh);
	if (err != 0) {
		return err;
	}

	frame_no = frame_counter++;

	shell_print(sh, "enqueuing CAN frame #%u with %s (%d-bit) CAN ID 0x%0*x, "
		    "RTR %d, CAN FD %d, BRS %d, DLC %d", frame_no,
		    (frame.flags & CAN_FRAME_IDE) != 0 ? "extended" : "standard",
		    (frame.flags & CAN_FRAME_IDE) != 0 ? 29 : 11,
		    (frame.flags & CAN_FRAME_IDE) != 0 ? 8 : 3, frame.id,
		    (frame.flags & CAN_FRAME_RTR) != 0 ? 1 : 0,
		    (frame.flags & CAN_FRAME_FDF) != 0 ? 1 : 0,
		    (frame.flags & CAN_FRAME_BRS) != 0 ? 1 : 0,
		    can_dlc_to_bytes(frame.dlc));

	err = can_send(dev, &frame, K_NO_WAIT, can_shell_tx_callback, UINT_TO_POINTER(frame_no));
	if (err != 0) {
		shell_error(sh, "failed to enqueue CAN frame #%u (err %d)", frame_no, err);
		return err;
	}

	return 0;
}

static int cmd_can_filter_add(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = device_get_binding(argv[1]);
	struct can_filter filter;
	uint32_t id_mask;
	int argidx = 2;
	uint32_t val;
	char *endptr;
	int err;

	if (!device_is_ready(dev)) {
		shell_error(sh, "device %s not ready", argv[1]);
		return -ENODEV;
	}

	/* Defaults */
	id_mask = CAN_STD_ID_MASK;
	filter.flags = 0U;

	/* Parse options */
	while (argidx < argc && strncmp(argv[argidx], "-", 1) == 0) {
		if (strcmp(argv[argidx], "--") == 0) {
			argidx++;
			break;
		} else if (strcmp(argv[argidx], "-e") == 0) {
			filter.flags |= CAN_FILTER_IDE;
			id_mask = CAN_EXT_ID_MASK;
			argidx++;
		} else {
			shell_error(sh, "unsupported argument %s", argv[argidx]);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
	}

	/* Parse CAN ID */
	if (argidx >= argc) {
		shell_error(sh, "missing CAN ID parameter");
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	val = (uint32_t)strtoul(argv[argidx++], &endptr, 16);
	if (*endptr != '\0') {
		shell_error(sh, "failed to parse CAN ID");
		return -EINVAL;
	}

	if (val > id_mask) {
		shell_error(sh, "CAN ID 0x%0*x out of range",
			    (filter.flags & CAN_FILTER_IDE) != 0 ? 8 : 3,
			    val);
		return -EINVAL;
	}

	filter.id = val;

	if (argidx < argc) {
		/* Parse CAN ID mask */
		val = (uint32_t)strtoul(argv[argidx++], &endptr, 16);
		if (*endptr != '\0') {
			shell_error(sh, "failed to parse CAN ID mask");
			return -EINVAL;
		}

		if (val > id_mask) {
			shell_error(sh, "CAN ID mask 0x%0*x out of range",
				    (filter.flags & CAN_FILTER_IDE) != 0 ? 8 : 3,
				    val);
			return -EINVAL;
		}

	} else {
		val = id_mask;
	}

	filter.mask = val;

	err = can_shell_rx_msgq_poll_submit(sh);
	if (err != 0) {
		return err;
	}

	shell_print(sh, "adding filter with %s (%d-bit) CAN ID 0x%0*x, CAN ID mask 0x%0*x",
		    (filter.flags & CAN_FILTER_IDE) != 0 ? "extended" : "standard",
		    (filter.flags & CAN_FILTER_IDE) != 0 ? 29 : 11,
		    (filter.flags & CAN_FILTER_IDE) != 0 ? 8 : 3, filter.id,
		    (filter.flags & CAN_FILTER_IDE) != 0 ? 8 : 3, filter.mask);

	err = can_add_rx_filter_msgq(dev, &can_shell_rx_msgq, &filter);
	if (err < 0) {
		shell_error(sh, "failed to add filter (err %d)", err);
		return err;
	}

	shell_print(sh, "filter ID: %d", err);

	return 0;
}

static int cmd_can_filter_remove(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = device_get_binding(argv[1]);
	int filter_id;
	char *endptr;

	if (!device_is_ready(dev)) {
		shell_error(sh, "device %s not ready", argv[1]);
		return -ENODEV;
	}

	/* Parse filter ID */
	filter_id = (int)strtol(argv[2], &endptr, 10);
	if (*endptr != '\0') {
		shell_error(sh, "failed to parse filter ID");
		return -EINVAL;
	}

	shell_print(sh, "removing filter with ID %d", filter_id);
	can_remove_rx_filter(dev, filter_id);

	return 0;
}

static int cmd_can_recover(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = device_get_binding(argv[1]);
	k_timeout_t timeout = K_FOREVER;
	int millisec;
	char *endptr;
	int err;

	if (!device_is_ready(dev)) {
		shell_error(sh, "device %s not ready", argv[1]);
		return -ENODEV;
	}

	if (argc >= 3) {
		/* Parse timeout */
		millisec = (int)strtol(argv[2], &endptr, 10);
		if (*endptr != '\0') {
			shell_error(sh, "failed to parse timeout");
			return -EINVAL;
		}

		timeout = K_MSEC(millisec);
		shell_print(sh, "recovering, timeout %d ms", millisec);
	} else {
		shell_print(sh, "recovering, no timeout");
	}

	err = can_recover(dev, timeout);
	if (err != 0) {
		shell_error(sh, "failed to recover CAN controller from bus-off (err %d)", err);
		return err;
	}

	return 0;
}

static void cmd_can_device_name(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, NULL);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_can_device_name, cmd_can_device_name);

static void cmd_can_mode(size_t idx, struct shell_static_entry *entry);

SHELL_DYNAMIC_CMD_CREATE(dsub_can_mode, cmd_can_mode);

static void cmd_can_mode(size_t idx, struct shell_static_entry *entry)
{
	if (idx < ARRAY_SIZE(can_shell_mode_map)) {
		entry->syntax = can_shell_mode_map[idx].name;

	} else {
		entry->syntax = NULL;
	}

	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = &dsub_can_mode;
}

static void cmd_can_device_name_mode(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, NULL);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = &dsub_can_mode;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_can_device_name_mode, cmd_can_device_name_mode);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_can_filter_cmds,
	SHELL_CMD_ARG(add, &dsub_can_device_name,
		"Add rx filter\n"
		"Usage: can filter add <device> [-e] <CAN ID> [CAN ID mask]\n"
		"-e  use extended (29-bit) CAN ID/CAN ID mask\n",
		cmd_can_filter_add, 3, 2),
	SHELL_CMD_ARG(remove, &dsub_can_device_name,
		"Remove rx filter\n"
		"Usage: can filter remove <device> <filter_id>",
		cmd_can_filter_remove, 3, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_can_cmds,
	SHELL_CMD_ARG(start, &dsub_can_device_name,
		"Start CAN controller\n"
		"Usage: can start <device>",
		cmd_can_start, 2, 0),
	SHELL_CMD_ARG(stop, &dsub_can_device_name,
		"Stop CAN controller\n"
		"Usage: can stop <device>",
		cmd_can_stop, 2, 0),
	SHELL_CMD_ARG(show, &dsub_can_device_name,
		"Show CAN controller information\n"
		"Usage: can show <device>",
		cmd_can_show, 2, 0),
	SHELL_CMD_ARG(bitrate, &dsub_can_device_name,
		"Set CAN controller bitrate (sample point and SJW optional)\n"
		"Usage: can bitrate <device> <bitrate> [sample point] [sjw]",
		cmd_can_bitrate_set, 3, 2),
	SHELL_COND_CMD_ARG(CONFIG_CAN_FD_MODE,
		dbitrate, &dsub_can_device_name,
		"Set CAN controller data phase bitrate (sample point and SJW optional)\n"
		"Usage: can dbitrate <device> <data phase bitrate> [sample point] [sjw]",
		cmd_can_dbitrate_set, 3, 2),
	SHELL_CMD_ARG(timing, &dsub_can_device_name,
		"Set CAN controller timing\n"
		"Usage: can timing <device> <sjw> <prop_seg> <phase_seg1> <phase_seg2> <prescaler>",
		cmd_can_timing_set, 7, 0),
	SHELL_COND_CMD_ARG(CONFIG_CAN_FD_MODE,
		dtiming, &dsub_can_device_name,
		"Set CAN controller data phase timing\n"
		"Usage: can dtiming <device> <sjw> <prop_seg> <phase_seg1> <phase_seg2> <prescaler>",
		cmd_can_dtiming_set, 7, 0),
	SHELL_CMD_ARG(mode, &dsub_can_device_name_mode,
		"Set CAN controller mode\n"
		"Usage: can mode <device> <mode> [mode] [mode] [...]",
		cmd_can_mode_set, 3, SHELL_OPT_ARG_CHECK_SKIP),
	SHELL_CMD_ARG(send, &dsub_can_device_name,
		"Enqueue a CAN frame for sending\n"
		"Usage: can send <device> [-e] [-r] [-f] [-b] <CAN ID> [data] [...]\n"
		"-e  use extended (29-bit) CAN ID\n"
		"-r  send Remote Transmission Request (RTR) frame\n"
		"-f  use CAN FD frame format\n"
		"-b  use CAN FD Bit Rate Switching (BRS)",
		cmd_can_send, 3, SHELL_OPT_ARG_CHECK_SKIP),
	SHELL_CMD(filter, &sub_can_filter_cmds,
		"CAN rx filter commands\n"
		"Usage: can filter <add|remove> <device> ...",
		NULL),
	SHELL_COND_CMD_ARG(CONFIG_CAN_MANUAL_RECOVERY_MODE,
		recover, &dsub_can_device_name,
		"Manually recover CAN controller from bus-off state\n"
		"Usage: can recover <device> [timeout ms]",
		cmd_can_recover, 2, 1),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(can, &sub_can_cmds, "CAN controller commands", NULL);
