/**
 * @brief UART Driver for interacting with host serial ports
 *
 * @note  Driver can open and send characters to the host serial ports (such as /dev/ttyUSB0 or
 * /dev/ttyACM0). Only polling Uart API is implemented. Driver can be configured via devicetree,
 * command line options or at runtime.
 *
 * To learn more see Native TTY section at:
 * https://docs.zephyrproject.org/latest/boards/posix/native_sim/doc/index.html
 * or
 * ${ZEPHYR_BASE}/boards/posix/native_sim/doc/index.rst
 *
 * Copyright (c) 2023 Marko Sagadin
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

#include <nsi_tracing.h>

#include "cmdline.h"
#include "posix_native_task.h"
#include "uart_native_tty_bottom.h"
#include "nsi_host_trampolines.h"

#define WARN(...)  nsi_print_warning(__VA_ARGS__)
#define ERROR(...) nsi_print_error_and_exit(__VA_ARGS__)

#define DT_DRV_COMPAT zephyr_native_tty_uart

struct native_tty_data {
	/* File descriptor used for the tty device. */
	int fd;
	/* Absolute path to the tty device. */
	char *serial_port;
	/* Baudrate set from the command line. If UINT32_MAX, it was not set. */
	int cmd_baudrate;
	/* Serial port set from the command line. If NULL, it was not set. */
	char *cmd_serial_port;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	/* Emulated tx irq is enabled. */
	bool tx_irq_enabled;
	/* Emulated rx irq is enabled. */
	bool rx_irq_enabled;
	/* IRQ callback */
	uart_irq_callback_user_data_t callback;
	/* IRQ callback data */
	void *cb_data;
#endif
};

struct native_tty_config {
	struct uart_config uart_config;
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static struct k_thread rx_thread;
static K_KERNEL_STACK_DEFINE(rx_stack, CONFIG_ARCH_POSIX_RECOMMENDED_STACK_SIZE);
#define NATIVE_TTY_INIT_LEVEL POST_KERNEL
#else
#define NATIVE_TTY_INIT_LEVEL PRE_KERNEL_1
#endif

/**
 * @brief Convert from uart_config to native_tty_bottom_cfg eqvivalent struct
 *
 * @param bottom_cfg
 * @param cfg
 *
 * @return 0 on success, negative errno otherwise.
 */
static int native_tty_conv_to_bottom_cfg(struct native_tty_bottom_cfg *bottom_cfg,
					 const struct uart_config *cfg)
{
	bottom_cfg->baudrate = cfg->baudrate;

	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		bottom_cfg->parity = NTB_PARITY_NONE;
		break;
	case UART_CFG_PARITY_ODD:
		bottom_cfg->parity = NTB_PARITY_ODD;
		break;
	case UART_CFG_PARITY_EVEN:
		bottom_cfg->parity = NTB_PARITY_EVEN;
		break;
	default:
		return -ENOTSUP;
	}

	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		bottom_cfg->stop_bits = NTB_STOP_BITS_1;
		break;
	case UART_CFG_STOP_BITS_2:
		bottom_cfg->stop_bits = NTB_STOP_BITS_2;
		break;
	default:
		return -ENOTSUP;
	}

	switch (cfg->data_bits) {
	case UART_CFG_DATA_BITS_5:
		bottom_cfg->data_bits = NTB_DATA_BITS_5;
		break;
	case UART_CFG_DATA_BITS_6:
		bottom_cfg->data_bits = NTB_DATA_BITS_6;
		break;
	case UART_CFG_DATA_BITS_7:
		bottom_cfg->data_bits = NTB_DATA_BITS_7;
		break;
	case UART_CFG_DATA_BITS_8:
		bottom_cfg->data_bits = NTB_DATA_BITS_8;
		break;
	default:
		return -ENOTSUP;
	}

	if (cfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE) {
		WARN("Could not set flow control, any kind of hw flow control is not supported.\n");
		return -ENOTSUP;
	}

	bottom_cfg->flow_ctrl = NTB_FLOW_CTRL_NONE;

	return 0;
}

/*
 * @brief Output a character towards the serial port
 *
 * @param dev		UART device structure.
 * @param out_char	Character to send.
 */
static void native_tty_uart_poll_out(const struct device *dev, unsigned char out_char)
{
	struct native_tty_data *data = dev->data;

	int ret = nsi_host_write(data->fd, &out_char, 1);

	if (ret == -1) {
		ERROR("Could not write to %s\n", data->serial_port);
	}
}

/**
 * @brief Poll the device for input.
 *
 * @param dev		UART device structure.
 * @param p_char	Pointer to a character.
 *
 * @retval 0	If a character arrived.
 * @retval -1	If no character was available to read.
 */
static int native_tty_uart_poll_in(const struct device *dev, unsigned char *p_char)
{
	struct native_tty_data *data = dev->data;

	return nsi_host_read(data->fd, p_char, 1) > 0 ? 0 : -1;
}

static int native_tty_configure(const struct device *dev, const struct uart_config *cfg)
{
	int fd = ((struct native_tty_data *)dev->data)->fd;
	struct native_tty_bottom_cfg bottom_cfg;

	int rc = native_tty_conv_to_bottom_cfg(&bottom_cfg, cfg);
	if (rc) {
		WARN("Could not convert uart config to native tty bottom cfg\n");
		return rc;
	}

	return native_tty_configure_bottom(fd, &bottom_cfg);
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int native_tty_uart_fifo_fill(const struct device *dev,
				     const uint8_t *tx_data,
				     int size)
{
	struct native_tty_data *data = dev->data;

	return nsi_host_write(data->fd, (void *)tx_data, size);
}

static int native_tty_uart_fifo_read(const struct device *dev,
				     uint8_t *rx_data,
				     const int size)
{
	struct native_tty_data *data = dev->data;

	return nsi_host_read(data->fd, rx_data, size);
}

static int native_tty_uart_irq_tx_ready(const struct device *dev)
{
	struct native_tty_data *data = dev->data;

	return data->tx_irq_enabled ? 1 : 0;
}

static int native_tty_uart_irq_tx_complete(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 1;
}

static void native_tty_uart_irq_tx_enable(const struct device *dev)
{
	struct native_tty_data *data = dev->data;

	data->tx_irq_enabled = true;
}

static void native_tty_uart_irq_tx_disable(const struct device *dev)
{
	struct native_tty_data *data = dev->data;

	data->tx_irq_enabled = false;
}

static void native_tty_uart_irq_rx_enable(const struct device *dev)
{
	struct native_tty_data *data = dev->data;

	data->rx_irq_enabled = true;
}

static void native_tty_uart_irq_rx_disable(const struct device *dev)
{
	struct native_tty_data *data = dev->data;

	data->rx_irq_enabled = false;
}

static int native_tty_uart_irq_rx_ready(const struct device *dev)
{
	struct native_tty_data *data = dev->data;

	if (data->rx_irq_enabled && native_tty_poll_bottom(data->fd) == 1) {
		return 1;
	}
	return 0;
}

static int native_tty_uart_irq_is_pending(const struct device *dev)
{
	return native_tty_uart_irq_rx_ready(dev) ||
		native_tty_uart_irq_tx_ready(dev);
}

static int native_tty_uart_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 1;
}

static void native_tty_uart_irq_handler(const struct device *dev)
{
	struct native_tty_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, data->cb_data);
	} else {
		WARN("No callback!\n");
	}
}

/*
 * Emulate uart interrupts using a polling thread
 */
void native_tty_uart_irq_function(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);
	struct device *dev = (struct device *)arg1;
	struct native_tty_data *data = dev->data;

	while (1) {
		if (data->rx_irq_enabled) {
			int ret = native_tty_poll_bottom(data->fd);

			if (ret == 1) {
				native_tty_uart_irq_handler(dev);
			} else if (ret < 0) {
				WARN("Poll returned error %d\n", ret);
			} else {
				k_sleep(K_MSEC(1));
			}
		}
		if (data->tx_irq_enabled) {
			native_tty_uart_irq_handler(dev);
		}
		if (data->tx_irq_enabled == false && data->rx_irq_enabled == false) {
			k_sleep(K_MSEC(10));
		}
	}
}

static void native_tty_uart_irq_callback_set(const struct device *dev,
					     uart_irq_callback_user_data_t cb,
					     void *cb_data)
{
	struct native_tty_data *data = dev->data;

	data->callback = cb;
	data->cb_data = cb_data;
}

static void native_tty_irq_init(const struct device *dev)
{
	/* Create a thread which will wait for data - replacement for IRQ */
	k_thread_create(&rx_thread, rx_stack, K_KERNEL_STACK_SIZEOF(rx_stack),
			native_tty_uart_irq_function,
			(void *)dev, NULL, NULL,
			K_HIGHEST_THREAD_PRIO, 0, K_NO_WAIT);
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int native_tty_serial_init(const struct device *dev)
{
	struct native_tty_data *data = dev->data;
	struct uart_config uart_config = ((struct native_tty_config *)dev->config)->uart_config;

	/* Default value for cmd_serial_port is NULL, this is due to the set 's' type in
	 * command line opts. If it is anything else then it was configured via command
	 * line.
	 */
	if (data->cmd_serial_port) {
		data->serial_port = data->cmd_serial_port;
	}

	/* Default value for cmd_baudrate is UINT32_MAX, this is due to the set 'u' type in
	 * command line opts. If it is anything else then it was configured via command
	 * line.
	 */
	if (data->cmd_baudrate != UINT32_MAX) {
		uart_config.baudrate = data->cmd_baudrate;
	}

	/* Serial port needs to be set either in the devicetree or provided via command line
	 * opts, if that is not the case, then abort.
	 */
	if (!data->serial_port) {
		ERROR("%s: path to the serial port was not set.\n", dev->name);
	}

	/* Try to open a serial port as with read/write access, also prevent serial port
	 * from becoming the controlling terminal.
	 */

	data->fd = native_tty_open_tty_bottom(data->serial_port);

	if (native_tty_configure(dev, &uart_config)) {
		ERROR("%s: could not configure serial port %s\n", dev->name, data->serial_port);
	}

	posix_print_trace("%s connected to the serial port: %s\n", dev->name, data->serial_port);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	/* Start irq emulation thread */
	native_tty_irq_init(dev);
#endif
	return 0;
}

static struct uart_driver_api native_tty_uart_driver_api = {
	.poll_out = native_tty_uart_poll_out,
	.poll_in = native_tty_uart_poll_in,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = native_tty_configure,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill        = native_tty_uart_fifo_fill,
	.fifo_read        = native_tty_uart_fifo_read,
	.irq_tx_enable    = native_tty_uart_irq_tx_enable,
	.irq_tx_disable	  = native_tty_uart_irq_tx_disable,
	.irq_tx_ready     = native_tty_uart_irq_tx_ready,
	.irq_tx_complete  = native_tty_uart_irq_tx_complete,
	.irq_rx_enable    = native_tty_uart_irq_rx_enable,
	.irq_rx_disable   = native_tty_uart_irq_rx_disable,
	.irq_rx_ready     = native_tty_uart_irq_rx_ready,
	.irq_is_pending   = native_tty_uart_irq_is_pending,
	.irq_update       = native_tty_uart_irq_update,
	.irq_callback_set = native_tty_uart_irq_callback_set,
#endif
};

#define NATIVE_TTY_INSTANCE(inst)                                                                  \
	static const struct native_tty_config native_tty_##inst##_cfg = {                          \
		.uart_config =                                                                     \
			{                                                                          \
				.data_bits = UART_CFG_DATA_BITS_8,                                 \
				.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,                              \
				.parity = UART_CFG_PARITY_NONE,                                    \
				.stop_bits = UART_CFG_STOP_BITS_1,                                 \
				.baudrate = DT_INST_PROP(inst, current_speed),                     \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	static struct native_tty_data native_tty_##inst##_data = {                                 \
		.serial_port = DT_INST_PROP_OR(inst, serial_port, NULL),                           \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, native_tty_serial_init, NULL, &native_tty_##inst##_data,       \
			      &native_tty_##inst##_cfg, NATIVE_TTY_INIT_LEVEL, 55,                 \
			      &native_tty_uart_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NATIVE_TTY_INSTANCE);

#define INST_NAME(inst) DEVICE_DT_NAME(DT_DRV_INST(inst))

#define NATIVE_TTY_COMMAND_LINE_OPTS(inst)                                                         \
	{                                                                                          \
		.option = INST_NAME(inst) "_port",						   \
		.name = "\"serial_port\"",                                                         \
		.type = 's',                                                                       \
		.dest = &native_tty_##inst##_data.cmd_serial_port,                                 \
		.descript = "Set a serial port for " INST_NAME(inst) " uart device, "		   \
		"overriding the one in devicetree.",						   \
	},                                                                                         \
	{											   \
		.option = INST_NAME(inst) "_baud",						   \
		.name = "baudrate",								   \
		.type = 'u',									   \
		.dest = &native_tty_##inst##_data.cmd_baudrate,					   \
		.descript = "Set a baudrate for " INST_NAME(inst) " device, overriding the "	   \
		"baudrate of " STRINGIFY(DT_INST_PROP(inst, current_speed))			   \
		"set in the devicetree.",							   \
	},

/**
 * @brief Adds command line options for setting serial port and baud rate for each uart
 * device.
 */
static void native_tty_add_serial_options(void)
{
	static struct args_struct_t opts[] = {
		DT_INST_FOREACH_STATUS_OKAY(NATIVE_TTY_COMMAND_LINE_OPTS) ARG_TABLE_ENDMARKER};

	native_add_command_line_opts(opts);
}

#define NATIVE_TTY_CLEANUP(inst)                                                                   \
	if (native_tty_##inst##_data.fd != 0) {                                                    \
		nsi_host_close(native_tty_##inst##_data.fd);                                       \
	}

/**
 * @brief Cleans up any open serial ports on the exit.
 */
static void native_tty_cleanup_uart(void)
{
	DT_INST_FOREACH_STATUS_OKAY(NATIVE_TTY_CLEANUP);
}

NATIVE_TASK(native_tty_add_serial_options, PRE_BOOT_1, 11);
NATIVE_TASK(native_tty_cleanup_uart, ON_EXIT, 99);
