/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>
#include <zephyr/drivers/mspi.h>

/* Maximum bytes we can read and write at once */
#define MAX_MSPI_BYTES 32
/* Argument count for MSPI write_addr command */
#define MSPI_WRITE_ADDR_ARGC 6
/* Argument count for MSPI write_reg command */
#define MSPI_WRITE_REG_ARGC 4


static int cmd_config_dev(const struct shell *sh, size_t argc, char **argv)
{
	static const struct option long_options[] = {
		{"freq", required_argument, NULL, 'f'},
		{"io-mode", required_argument, NULL, 'i'},
		{"data-rate", required_argument, NULL, 'd'},
		{"cpp-mode", required_argument, NULL, 'P'},
		{"endian", required_argument, NULL, 'e'},
		{"ce-pol", required_argument, NULL, 'p'},
		{"dqs", required_argument, NULL, 'q'},
		{"rx-dummy", required_argument, NULL, 'R'},
		{"tx-dummy", required_argument, NULL, 'T'},
		{"read-cmd", required_argument, NULL, 'r'},
		{"write-cmd", required_argument, NULL, 'w'},
		{"cmd-length", required_argument, NULL, 'c'},
		{"addr-length", required_argument, NULL, 'a'},
		{"wr-mode-bits", required_argument, NULL, 'M'},
		{"wr-mode-len", required_argument, NULL, 'O'},
		{"rd-mode-bits", required_argument, NULL, 'm'},
		{"rd-mode-len", required_argument, NULL, 'o'},
		{"caddr-len", required_argument, NULL, 'C'},
		{"addr-shift", required_argument, NULL, 's'},
		{"mem-boundary", required_argument, NULL, 'b'},
		{"time-to-break", required_argument, NULL, 't'},
		{0, 0, 0, 0},
	};
	static const char *short_options = "f:i:d:P:e:p:q:R:T:r:w:c:a:M:O:m:o:C:s:b:t:";
	char *arg;
	struct getopt_state *state;
	const struct device *controller = device_get_binding(argv[1]);
	enum mspi_dev_cfg_mask cfg_mask = MSPI_DEVICE_CONFIG_NONE;
	struct mspi_dev_cfg cfg = {0};
	struct mspi_dev_id dev_id = {0};
	int ret;

	if (!device_is_ready(controller)) {
		shell_error(sh, "Device %s not found", argv[1]);
		return -ENODEV;
	}

	dev_id.dev_idx = strtol(argv[2], NULL, 0);

	while ((ret = getopt_long(argc, argv, short_options,
				  long_options, NULL)) != -1) {
		state = getopt_state_get();
		switch (ret) {
		case 'f':
			arg = state->optarg;
			cfg_mask |= MSPI_DEVICE_CONFIG_FREQUENCY;
			cfg.freq = strtol(arg, NULL, 0);
			break;
		case 'i':
			arg = state->optarg;
			cfg_mask |= MSPI_DEVICE_CONFIG_IO_MODE;
			if (strcmp(arg, "1-1-1") == 0) {
				cfg.io_mode = MSPI_IO_MODE_SINGLE;
			} else if (strcmp(arg, "2-2-2") == 0) {
				cfg.io_mode = MSPI_IO_MODE_DUAL;
			} else if (strcmp(arg, "1-1-2") == 0) {
				cfg.io_mode = MSPI_IO_MODE_DUAL_1_1_2;
			} else if (strcmp(arg, "1-2-2") == 0) {
				cfg.io_mode = MSPI_IO_MODE_DUAL_1_2_2;
			} else if (strcmp(arg, "4-4-4") == 0) {
				cfg.io_mode = MSPI_IO_MODE_QUAD;
			} else if (strcmp(arg, "1-1-4") == 0) {
				cfg.io_mode = MSPI_IO_MODE_QUAD_1_1_4;
			} else if (strcmp(arg, "1-4-4") == 0) {
				cfg.io_mode = MSPI_IO_MODE_QUAD_1_4_4;
			} else if (strcmp(arg, "8-8-8") == 0) {
				cfg.io_mode = MSPI_IO_MODE_OCTAL;
			} else if (strcmp(arg, "1-1-8") == 0) {
				cfg.io_mode = MSPI_IO_MODE_OCTAL_1_1_8;
			} else if (strcmp(arg, "1-8-8") == 0) {
				cfg.io_mode = MSPI_IO_MODE_OCTAL_1_8_8;
			} else if (strcmp(arg, "16-16-16") == 0) {
				cfg.io_mode = MSPI_IO_MODE_HEX;
			} else if (strcmp(arg, "8-8-16") == 0) {
				cfg.io_mode = MSPI_IO_MODE_HEX_8_8_16;
			} else if (strcmp(arg, "8-16-16") == 0) {
				cfg.io_mode = MSPI_IO_MODE_HEX_8_16_16;
			} else {
				shell_error(sh, "Unsupported io mode: %s", arg);
				return -ENOTSUP;
			}
			break;
		case 'd':
			arg = state->optarg;
			cfg_mask |= MSPI_DEVICE_CONFIG_DATA_RATE;
			if (strcmp(arg, "s-s-s") == 0) {
				cfg.data_rate = MSPI_DATA_RATE_SINGLE;
			} else if (strcmp(arg, "s-s-d") == 0) {
				cfg.data_rate = MSPI_DATA_RATE_S_S_D;
			} else if (strcmp(arg, "s-d-d") == 0) {
				cfg.data_rate = MSPI_DATA_RATE_S_D_D;
			} else if (strcmp(arg, "d-d-d") == 0) {
				cfg.data_rate = MSPI_DATA_RATE_DUAL;
			} else {
				shell_error(sh, "Unsupported data rate: %s", arg);
				return -ENOTSUP;
			}
			break;
		case 'P':
			arg = state->optarg;
			cfg_mask |= MSPI_DEVICE_CONFIG_CPP;
			switch (strtol(arg, NULL, 0)) {
			case 0:
				cfg.cpp = MSPI_CPP_MODE_0;
				break;
			case 1:
				cfg.cpp = MSPI_CPP_MODE_1;
				break;
			case 2:
				cfg.cpp = MSPI_CPP_MODE_2;
				break;
			case 3:
				cfg.cpp = MSPI_CPP_MODE_3;
				break;
			default:
				shell_error(sh, "Unsupported polarity mode: %s", arg);
				return -ENOTSUP;
			}
			break;
		case 'e':
			arg = state->optarg;
			cfg_mask |= MSPI_DEVICE_CONFIG_ENDIAN;
			if (strcmp(arg, "big") == 0) {
				cfg.endian = MSPI_XFER_BIG_ENDIAN;
			} else if (strcmp(arg, "little") == 0) {
				cfg.endian = MSPI_XFER_LITTLE_ENDIAN;
			} else {
				shell_error(sh, "Unsupported endian mode: %s", arg);
				return -ENOTSUP;
			}
			break;
		case 'p':
			arg = state->optarg;
			cfg_mask |= MSPI_DEVICE_CONFIG_CE_POL;
			if (strcmp(arg, "high") == 0) {
				cfg.ce_polarity = MSPI_CE_ACTIVE_HIGH;
			} else if (strcmp(arg, "low") == 0) {
				cfg.ce_polarity = MSPI_CE_ACTIVE_LOW;
			} else {
				shell_error(sh, "Unsupported ce polarity: %s", arg);
				return -ENOTSUP;
			}
		case 'q':
			arg = state->optarg;
			cfg_mask |= MSPI_DEVICE_CONFIG_DQS;
			if (strcmp(arg, "on") == 0) {
				cfg.dqs_enable = true;
			} else if (strcmp(arg, "off") == 0) {
				cfg.dqs_enable = false;
			} else {
				shell_error(sh, "Unsupported dqs setting: %s", arg);
				return -ENOTSUP;
			}
		case 'R':
			cfg_mask |= MSPI_DEVICE_CONFIG_RX_DUMMY;
			cfg.rx_dummy = strtol(state->optarg, NULL, 0);
			break;
		case 'T':
			cfg_mask |= MSPI_DEVICE_CONFIG_TX_DUMMY;
			cfg.tx_dummy = strtol(state->optarg, NULL, 0);
			break;
		case 'r':
			cfg_mask |= MSPI_DEVICE_CONFIG_READ_CMD;
			cfg.read_cmd = strtol(state->optarg, NULL, 0);
			break;
		case 'w':
			cfg_mask |= MSPI_DEVICE_CONFIG_WRITE_CMD;
			cfg.write_cmd = strtol(state->optarg, NULL, 0);
			break;
		case 'c':
			cfg_mask |= MSPI_DEVICE_CONFIG_CMD_LEN;
			cfg.cmd_length = strtol(state->optarg, NULL, 0);
			break;
		case 'a':
			cfg_mask |= MSPI_DEVICE_CONFIG_ADDR_LEN;
			cfg.addr_length = strtol(state->optarg, NULL, 0);
			break;
		case 'M':
			cfg_mask |= MSPI_DEVICE_CONFIG_WR_MODE_BITS;
			cfg.write_mode_bits = strtol(state->optarg, NULL, 0);
			break;
		case 'O':
			cfg_mask |= MSPI_DEVICE_CONFIG_WR_MODE_LEN;
			cfg.write_mode_length = strtol(state->optarg, NULL, 0);
			break;
		case 'm':
			cfg_mask |= MSPI_DEVICE_CONFIG_RD_MODE_BITS;
			cfg.read_mode_bits = strtol(state->optarg, NULL, 0);
			break;
		case 'o':
			cfg_mask |= MSPI_DEVICE_CONFIG_RD_MODE_LEN;
			cfg.read_mode_length = strtol(state->optarg, NULL, 0);
			break;
		case 'C':
			cfg_mask |= MSPI_DEVICE_CONFIG_CADDR_LEN;
			cfg.column_addr_length = strtol(state->optarg, NULL, 0);
			break;
		case 's':
			cfg_mask |= MSPI_DEVICE_CONFIG_ADDR_SHIFT;
			cfg.addr_shift = strtol(state->optarg, NULL, 0);
			break;
		case 'b':
			cfg_mask |= MSPI_DEVICE_CONFIG_MEM_BOUND;
			cfg.mem_boundary = strtol(state->optarg, NULL, 0);
			break;
		case 't':
			cfg_mask |= MSPI_DEVICE_CONFIG_BREAK_TIME;
			cfg.time_to_break = strtol(state->optarg, NULL, 0);
			break;
		default:
			shell_error(sh, "Invalid option %c", state->optopt);
			return -EINVAL;
		}
	}

	ret = mspi_dev_config(controller, &dev_id, cfg_mask, &cfg);
	if (ret < 0) {
		shell_error(sh, "MSPI device configuration failed (%d)", ret);
	}
	return ret;
}

static int cmd_send_cmd(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *controller = device_get_binding(argv[1]);
	struct mspi_xfer_packet pkt = {
		.dir = MSPI_TX
	};
	struct mspi_xfer xfer = {
		.packets = &pkt,
		.num_packet = 1,
	};
	struct mspi_dev_id dev_id = {0};
	int ret;

	if (!device_is_ready(controller)) {
		shell_error(sh, "Device %s not found", argv[1]);
		return -ENODEV;
	}

	dev_id.dev_idx = strtol(argv[2], NULL, 0);
	pkt.cmd = strtol(argv[3], NULL, 0);
	xfer.cmd_length = DIV_ROUND_UP(find_msb_set(pkt.cmd), BITS_PER_BYTE);
	ret = mspi_transceive(controller, &dev_id, &xfer);
	if (ret < 0) {
		shell_error(sh, "MSPI command failed (%d)", ret);
	}
	return ret;
}

static int cmd_read_reg(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *controller = device_get_binding(argv[1]);
	uint8_t rx_buf[MAX_MSPI_BYTES];
	struct mspi_xfer_packet pkt = {
		.dir = MSPI_RX,
		.data_buf = rx_buf,
	};
	struct mspi_xfer xfer = {
		.packets = &pkt,
		.num_packet = 1,
		.addr_length = 0,
	};
	struct mspi_dev_id dev_id = {0};
	int ret;

	if (!device_is_ready(controller)) {
		shell_error(sh, "Device %s not found", argv[1]);
		return -ENODEV;
	}

	dev_id.dev_idx = strtol(argv[2], NULL, 0);
	pkt.cmd = strtol(argv[3], NULL, 0);
	pkt.num_bytes = strtol(argv[4], NULL, 0);
	xfer.cmd_length = DIV_ROUND_UP(find_msb_set(pkt.cmd), BITS_PER_BYTE);
	if (pkt.num_bytes > sizeof(rx_buf)) {
		shell_error(sh, "Cannot read this many bytes into RX buffer");
		return -ENOTSUP;
	}
	ret = mspi_transceive(controller, &dev_id, &xfer);
	if (ret < 0) {
		shell_error(sh, "MSPI command failed (%d)", ret);
		return ret;
	}
	shell_hexdump(sh, pkt.data_buf, pkt.num_bytes);
	return ret;
}

static int cmd_read_addr(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *controller = device_get_binding(argv[1]);
	uint8_t rx_buf[MAX_MSPI_BYTES];
	struct mspi_xfer_packet pkt = {
		.dir = MSPI_RX,
		.data_buf = rx_buf,
	};
	struct mspi_xfer xfer = {
		.packets = &pkt,
		.num_packet = 1,
	};
	struct mspi_dev_id dev_id = {0};
	int ret;

	if (!device_is_ready(controller)) {
		shell_error(sh, "Device %s not found", argv[1]);
		return -ENODEV;
	}

	dev_id.dev_idx = strtol(argv[2], NULL, 0);
	xfer.addr_length = strtol(argv[3], NULL, 0);
	pkt.cmd = strtol(argv[4], NULL, 0);
	pkt.address = strtol(argv[5], NULL, 0);
	pkt.num_bytes = strtol(argv[6], NULL, 0);
	xfer.cmd_length = DIV_ROUND_UP(find_msb_set(pkt.cmd), BITS_PER_BYTE);
	if (pkt.num_bytes > sizeof(rx_buf)) {
		shell_error(sh, "Cannot read this many bytes into RX buffer");
		return -ENOTSUP;
	}
	ret = mspi_transceive(controller, &dev_id, &xfer);
	if (ret < 0) {
		shell_error(sh, "MSPI command failed (%d)", ret);
		return ret;
	}
	shell_hexdump(sh, pkt.data_buf, pkt.num_bytes);
	return ret;
}

static int cmd_write_reg(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *controller = device_get_binding(argv[1]);
	uint8_t tx_buf[MAX_MSPI_BYTES];
	struct mspi_xfer_packet pkt = {
		.dir = MSPI_TX,
		.data_buf = tx_buf,
	};
	struct mspi_xfer xfer = {
		.packets = &pkt,
		.num_packet = 1,
		.addr_length = 0,
	};
	struct mspi_dev_id dev_id = {0};
	int ret;

	if (!device_is_ready(controller)) {
		shell_error(sh, "Device %s not found", argv[1]);
		return -ENODEV;
	}

	dev_id.dev_idx = strtol(argv[2], NULL, 0);
	pkt.cmd = strtol(argv[3], NULL, 0);
	xfer.cmd_length = DIV_ROUND_UP(find_msb_set(pkt.cmd), BITS_PER_BYTE);

	/*
	 * Read remaining arguments as bytes. We limit the number of
	 * arguments in the shell command definition, no need to check here.
	 */
	for (int i = 0; i < (argc - MSPI_WRITE_REG_ARGC); i++) {
		tx_buf[i] = strtol(argv[i + MSPI_WRITE_REG_ARGC], NULL, 0);
	}
	pkt.num_bytes = argc - MSPI_WRITE_REG_ARGC;

	ret = mspi_transceive(controller, &dev_id, &xfer);
	if (ret < 0) {
		shell_error(sh, "MSPI command failed (%d)", ret);
		return ret;
	}
	return ret;
}

static int cmd_write_addr(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *controller = device_get_binding(argv[1]);
	uint8_t tx_buf[MAX_MSPI_BYTES];
	struct mspi_xfer_packet pkt = {
		.dir = MSPI_TX,
		.data_buf = tx_buf,
	};
	struct mspi_xfer xfer = {
		.packets = &pkt,
		.num_packet = 1,
	};
	struct mspi_dev_id dev_id = {0};
	int ret;

	if (!device_is_ready(controller)) {
		shell_error(sh, "Device %s not found", argv[1]);
		return -ENODEV;
	}

	dev_id.dev_idx = strtol(argv[2], NULL, 0);
	xfer.addr_length = strtol(argv[3], NULL, 0);
	pkt.cmd = strtol(argv[4], NULL, 0);
	pkt.address = strtol(argv[5], NULL, 0);
	xfer.cmd_length = DIV_ROUND_UP(find_msb_set(pkt.cmd), BITS_PER_BYTE);

	/*
	 * Read remaining arguments as bytes. We limit the number of
	 * arguments in the shell command definition, no need to check here.
	 */
	for (int i = 0; i < (argc - MSPI_WRITE_ADDR_ARGC); i++) {
		tx_buf[i] = strtol(argv[i + MSPI_WRITE_ADDR_ARGC], NULL, 0);
	}
	pkt.num_bytes = argc - MSPI_WRITE_ADDR_ARGC;

	ret = mspi_transceive(controller, &dev_id, &xfer);
	if (ret < 0) {
		shell_error(sh, "MSPI command failed (%d)", ret);
		return ret;
	}
	return ret;
}

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, NULL);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

SHELL_STATIC_SUBCMD_SET_CREATE(mspi_cmds,
	SHELL_CMD_ARG(config_dev, &dsub_device_name,
		      "Configure device. Syntax:\n"
		      "mspi config_dev <device> <idx> <optional parameters>\n"
		      "Additional optional arguments:\n"
		      "-f/--freq=<sclk_freq>\n"
		      "-i/--io-mode=[1-1-1, 1-1-2, 1-2-2, 2-2-2, "
		      "1-1-4, 1-4-4, 4-4-4, 1-1-8, 1-8-8, 8-8-8 "
		      "8-8-16, 8-16-16, 16-16-16]\n"
		      "-d/--data-rate=[s-s-s, s-s-d, s-d-d, d-d-d]\n"
		      "-P/--cpp-mode=[0, 1, 2, 3]\n"
		      "-e/--endian=[big, little]\n"
		      "-p/--ce-pol=[high, low]\n"
		      "-q/--dqs=[on, off]\n"
		      "-R/--rx-dummy=<cycle_count>\n"
		      "-T/--tx-dummy=<cycle_count>\n"
		      "-r/--read-cmd=<cmd>\n"
		      "-w/--write-cmd=<cmd>\n"
		      "-c/--cmd-length=<len>\n"
		      "-a/--addr-length=<len>\n"
		      "-M/--wr-mode-bits=<val>\n"
		      "-O/--wr-mode-len=<len>\n"
		      "-m/--rd-mode-bits=<val>\n"
		      "-o/--rd-mode-len=<len>\n"
		      "-C/--caddr-len=<len>\n"
		      "-s/--addr-shift=<shift>\n"
		      "-b/--mem-boundary=<val>\n"
		      "-t/--time-to-break=<val>",
		      cmd_config_dev, 3, 21),
	SHELL_CMD_ARG(send_cmd, &dsub_device_name,
		      "Send MSPI command. Syntax:\n"
		      "mspi send_cmd <device> <idx> <cmd>",
		      cmd_send_cmd, 4, 0),
	SHELL_CMD_ARG(read_reg, &dsub_device_name,
		      "Send command, and read respond from MSPI device. Syntax:\n"
		      "mspi read_reg <device> <idx> <cmd> <len>",
		      cmd_read_reg, 5, 0),
	SHELL_CMD_ARG(write_reg, &dsub_device_name,
		      "Send command, and write data to MSPI device. Syntax:\n"
		      "mspi write_reg <device> <idx> <cmd> <bytes>",
		      cmd_write_reg, MSPI_WRITE_REG_ARGC, MAX_MSPI_BYTES),
	SHELL_CMD_ARG(read_addr, &dsub_device_name,
		      "Read from address on MSPI device. Syntax:\n"
		      "mspi read_addr <device> <idx> <addr_len> <cmd> <addr> <len>",
		      cmd_read_addr, 7, 0),
	SHELL_CMD_ARG(write_addr, &dsub_device_name,
		      "Read from address on MSPI device. Syntax:\n"
		      "mspi write_addr <device> <idx> <addr_len> <cmd> <addr> <bytes>",
		      cmd_write_addr, MSPI_WRITE_ADDR_ARGC, MAX_MSPI_BYTES),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(mspi, &mspi_cmds, "MSPI commands", NULL);
