/*
 * Copyright (c) 2018, Oticon A/S
 * Copyright (c) 2025, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_native_posix_uart)
#define DT_DRV_COMPAT zephyr_native_posix_uart
#warning "zephyr,native-posix-uart is deprecated in favor of zephyr,native-pty-uart"
#else
#define DT_DRV_COMPAT zephyr_native_pty_uart
#endif

#include <stdbool.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <cmdline.h> /* native_sim command line options header */
#include <posix_native_task.h>
#include <nsi_host_trampolines.h>
#include <nsi_tracing.h>
#include "uart_native_pty_bottom.h"

#define ERROR posix_print_error_and_exit
#define WARN posix_print_warning

/*
 * UART driver for native simulator based boards.
 * It can support a configurable number of UARTs.
 *
 * One (and only one) of this can be connected to the process STDIN+STDOUT otherwise, they are
 * connected to a dedicated pseudo terminal.
 *
 * Connecting to a dedicated PTY is the recommended option for interactive use, as the pseudo
 * terminal driver will be configured in "raw" mode and will therefore behave more like a real UART.
 *
 * When connected to its own pseudo terminal, it may also auto attach a terminal emulator to it,
 * if set so from command line.
 */

struct native_pty_status {
	int out_fd;       /* File descriptor used for output */
	int in_fd;        /* File descriptor used for input */
	bool on_stdinout; /* This UART is connected to a PTY and not STDIN/OUT */

	bool auto_attach;      /* For PTY, attach a terminal emulator automatically */
	char *auto_attach_cmd; /* If auto_attach, which command to launch the terminal emulator */
	bool wait_pts;         /* Hold writes to the uart/pts until a client is connected/ready */
	bool cmd_request_stdinout; /* User requested to connect this UART to the stdin/out */
#ifdef CONFIG_UART_ASYNC_API
	struct  {
		const struct device *dev;
		struct k_work_delayable tx_done;
		uart_callback_t user_callback;
		void *user_data;
		const uint8_t *tx_buf;
		size_t tx_len;
		uint8_t *rx_buf;
		size_t rx_len;
		/* Instance-specific RX thread. */
		struct k_thread rx_thread;
		/* Stack for RX thread */
		K_KERNEL_STACK_MEMBER(rx_stack, CONFIG_ARCH_POSIX_RECOMMENDED_STACK_SIZE);
	} async;
#endif /* CONFIG_UART_ASYNC_API */
};

static void np_uart_poll_out(const struct device *dev, unsigned char out_char);
static int np_uart_poll_in(const struct device *dev, unsigned char *p_char);
static int np_uart_init(const struct device *dev);

#ifdef CONFIG_UART_ASYNC_API
static void np_uart_tx_done_work(struct k_work *work);
static int np_uart_callback_set(const struct device *dev, uart_callback_t callback,
				void *user_data);
static int np_uart_tx(const struct device *dev, const uint8_t *buf, size_t len, int32_t timeout);
static int np_uart_tx_abort(const struct device *dev);
static int np_uart_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len);
static int np_uart_rx_enable(const struct device *dev, uint8_t *buf, size_t len, int32_t timeout);
static int np_uart_rx_disable(const struct device *dev);
#endif /* CONFIG_UART_ASYNC_API */

static DEVICE_API(uart, np_uart_driver_api) = {
	.poll_out = np_uart_poll_out,
	.poll_in = np_uart_poll_in,
#ifdef CONFIG_UART_ASYNC_API
	.callback_set = np_uart_callback_set,
	.tx = np_uart_tx,
	.tx_abort = np_uart_tx_abort,
	.rx_buf_rsp = np_uart_rx_buf_rsp,
	.rx_enable = np_uart_rx_enable,
	.rx_disable = np_uart_rx_disable,
#endif /* CONFIG_UART_ASYNC_API */
};

#define NATIVE_PTY_INSTANCE(inst)                                        \
	static struct native_pty_status native_pty_status_##inst;        \
								         \
	DEVICE_DT_INST_DEFINE(inst, np_uart_init, NULL,                  \
			      (void *)&native_pty_status_##inst, NULL,   \
			      PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, \
			      &np_uart_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NATIVE_PTY_INSTANCE);

/**
 * @brief Initialize a native_pty serial port
 *
 * @param dev uart device struct
 *
 * @return 0 (if it fails catastrophically, the execution is terminated)
 */
static int np_uart_init(const struct device *dev)
{

	static bool stdinout_used;
	struct native_pty_status *d;

	d = (struct native_pty_status *)dev->data;

	if (IS_ENABLED(CONFIG_UART_NATIVE_PTY_0_ON_STDINOUT)) {
		static bool first_node = true;

		if (first_node) {
			d->on_stdinout = true;
		}
		first_node = false;
	}

	if (d->cmd_request_stdinout) {
		if (stdinout_used) {
			nsi_print_warning("%s requested to connect to STDIN/OUT, but another UART"
					  " is already connected to it => ignoring request.\n",
					  dev->name);
		} else {
			d->on_stdinout = true;
		}
	}

	if (d->on_stdinout == true) {
		d->in_fd  = np_uart_pty_get_stdin_fileno();
		d->out_fd = np_uart_pty_get_stdout_fileno();
		stdinout_used = true;
	} else {
		if (d->auto_attach_cmd == NULL) {
			d->auto_attach_cmd = CONFIG_UART_NATIVE_PTY_AUTOATTACH_DEFAULT_CMD;
		} else { /* Running with --attach_uart_cmd, implies --attach_uart */
			d->auto_attach = true;
		}
		int tty_fn = np_uart_open_pty(dev->name, d->auto_attach_cmd, d->auto_attach,
					      d->wait_pts);
		d->in_fd = tty_fn;
		d->out_fd = tty_fn;
	}

#ifdef CONFIG_UART_ASYNC_API
	k_work_init_delayable(&d->async.tx_done, np_uart_tx_done_work);
	d->async.dev = dev;
#endif

	return 0;
}

/*
 * @brief Output a character towards the serial port
 *
 * @param dev UART device struct
 * @param out_char Character to send.
 */
static void np_uart_poll_out(const struct device *dev, unsigned char out_char)
{
	int ret;
	struct native_pty_status *d = (struct native_pty_status *)dev->data;

	if (d->wait_pts) {
		while (1) {
			int rc = np_uart_slave_connected(d->out_fd);

			if (rc == 1) {
				break;
			}
			k_sleep(K_MSEC(100));
		}
	}

	/* The return value of write() cannot be ignored (there is a warning)
	 * but we do not need the return value for anything.
	 */
	ret = nsi_host_write(d->out_fd, &out_char, 1);
	(void) ret;
}

/**
 * @brief Poll the device for input.
 *
 * @param dev UART device structure.
 * @param p_char Pointer to character.
 *
 * @retval 0 If a character arrived and was stored in p_char
 * @retval -1 If no character was available to read
 */
static int np_uart_stdin_poll_in(const struct device *dev, unsigned char *p_char)
{
	int in_f = ((struct native_pty_status *)dev->data)->in_fd;
	static bool disconnected;
	int rc;

	if (disconnected == true) {
		return -1;
	}

	rc = np_uart_stdin_poll_in_bottom(in_f, p_char, 1);
	if (rc == -2) {
		disconnected = true;
	}
	return rc == 1 ? 0 : -1;
}

/**
 * @brief Poll the device for input.
 *
 * @param dev UART device structure.
 * @param p_char Pointer to character.
 *
 * @retval 0 If a character arrived and was stored in p_char
 * @retval -1 If no character was available to read
 */
static int np_uart_pty_poll_in(const struct device *dev, unsigned char *p_char)
{
	int n = -1;
	int in_f = ((struct native_pty_status *)dev->data)->in_fd;

	n = nsi_host_read(in_f, p_char, 1);
	if (n == -1) {
		return -1;
	}
	return 0;
}

static int np_uart_poll_in(const struct device *dev, unsigned char *p_char)
{
	if (((struct native_pty_status *)dev->data)->on_stdinout) {
		return np_uart_stdin_poll_in(dev, p_char);
	} else {
		return np_uart_pty_poll_in(dev, p_char);
	}
}

#ifdef CONFIG_UART_ASYNC_API

static int np_uart_callback_set(const struct device *dev, uart_callback_t callback, void *user_data)
{
	struct native_pty_status *data = dev->data;

	data->async.user_callback = callback;
	data->async.user_data = user_data;

	return 0;
}

static void np_uart_tx_done_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct native_pty_status *data =
		CONTAINER_OF(dwork, struct native_pty_status, async.tx_done);
	struct uart_event evt;
	unsigned int key = irq_lock();

	evt.type = UART_TX_DONE;
	evt.data.tx.buf = data->async.tx_buf;
	evt.data.tx.len = data->async.tx_len;

	(void)nsi_host_write(data->out_fd, evt.data.tx.buf, evt.data.tx.len);

	data->async.tx_buf = NULL;

	if (data->async.user_callback) {
		data->async.user_callback(data->async.dev, &evt, data->async.user_data);
	}
	irq_unlock(key);
}

static int np_uart_tx(const struct device *dev, const uint8_t *buf, size_t len, int32_t timeout)
{
	struct native_pty_status *data = dev->data;

	if (data->async.tx_buf) {
		/* Port is busy */
		return -EBUSY;
	}
	data->async.tx_buf = buf;
	data->async.tx_len = len;

	/* Run the callback on the next tick to give the caller time to use the return value */
	k_work_reschedule(&data->async.tx_done, K_TICKS(1));
	return 0;
}

static int np_uart_tx_abort(const struct device *dev)
{
	struct native_pty_status *data = dev->data;
	struct k_work_sync sync;
	struct uart_event evt;
	bool not_idle;

	/* Cancel the callback */
	not_idle = k_work_cancel_delayable_sync(&data->async.tx_done, &sync);
	if (!not_idle) {
		return -EFAULT;
	}

	/* Generate TX_DONE event with number of bytes transmitted */
	evt.type = UART_TX_DONE;
	evt.data.tx.buf = data->async.tx_buf;
	evt.data.tx.len = 0;
	if (data->async.user_callback) {
		data->async.user_callback(data->async.dev, &evt, data->async.user_data);
	}

	/* Reset state */
	data->async.tx_buf = NULL;
	return 0;
}

/*
 * Emulate async interrupts using a polling thread
 */
static void native_pty_uart_async_poll_function(void *arg1, void *arg2, void *arg3)
{
	const struct device *dev = arg1;
	struct native_pty_status *data = dev->data;
	struct uart_event evt;
	int rc;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (data->async.rx_len) {
		rc = np_uart_stdin_poll_in_bottom(data->in_fd, data->async.rx_buf,
						  data->async.rx_len);
		if (rc > 0) {
			/* Data received */
			evt.type = UART_RX_RDY;
			evt.data.rx.buf = data->async.rx_buf;
			evt.data.rx.offset = 0;
			evt.data.rx.len = rc;
			/* User callback */
			if (data->async.user_callback) {
				data->async.user_callback(data->async.dev, &evt,
							  data->async.user_data);
			}
		}
		if ((data->async.rx_len != 0) && (rc < 0)) {
			/* Sleep if RX not disabled and last read didn't result in any data */
			k_sleep(K_MSEC(10));
		}
	}
}

static int np_uart_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	/* Driver never requests additional buffers */
	return -ENOTSUP;
}

static int np_uart_rx_enable(const struct device *dev, uint8_t *buf, size_t len, int32_t timeout)
{
	struct native_pty_status *data = dev->data;

	ARG_UNUSED(timeout);

	if (data->async.rx_buf != NULL) {
		return -EBUSY;
	}

	data->async.rx_buf = buf;
	data->async.rx_len = len;

	/* Create a thread which will wait for data - replacement for IRQ */
	k_thread_create(&data->async.rx_thread, data->async.rx_stack,
			K_KERNEL_STACK_SIZEOF(data->async.rx_stack),
			native_pty_uart_async_poll_function,
			(void *)dev, NULL, NULL,
			K_HIGHEST_THREAD_PRIO, 0, K_NO_WAIT);
	return 0;
}

static int np_uart_rx_disable(const struct device *dev)
{
	struct native_pty_status *data = dev->data;

	if (data->async.rx_buf == NULL) {
		return -EFAULT;
	}

	data->async.rx_len = 0;
	data->async.rx_buf = NULL;

	/* Wait for RX thread to terminate */
	return k_thread_join(&data->async.rx_thread, K_FOREVER);
}

#endif /* CONFIG_UART_ASYNC_API */


#define NATIVE_PTY_SET_AUTO_ATTACH_CMD(inst, cmd)      \
	native_pty_status_##inst.auto_attach_cmd = cmd;
#define NATIVE_PTY_SET_AUTO_ATTACH(inst, value)        \
	native_pty_status_##inst.auto_attach = value;
#define NATIVE_PTY_SET_WAIT_PTS(inst, value)           \
	native_pty_status_##inst.wait_pts = value;

static void auto_attach_cmd_cb(char *argv, int offset)
{
	DT_INST_FOREACH_STATUS_OKAY_VARGS(NATIVE_PTY_SET_AUTO_ATTACH_CMD, &argv[offset]);
	DT_INST_FOREACH_STATUS_OKAY_VARGS(NATIVE_PTY_SET_AUTO_ATTACH, true);
}

static void auto_attach_cb(char *argv, int offset)
{
	DT_INST_FOREACH_STATUS_OKAY_VARGS(NATIVE_PTY_SET_AUTO_ATTACH, true);
}

static void wait_pts_cb(char *argv, int offset)
{
	DT_INST_FOREACH_STATUS_OKAY_VARGS(NATIVE_PTY_SET_WAIT_PTS, true);
}

#define INST_NAME(inst) DEVICE_DT_NAME(DT_DRV_INST(inst))

#define NATIVE_PTY_COMMAND_LINE_OPTS(inst)                                                         \
	{                                                                                          \
		.is_switch = true,                                                                 \
		.option = INST_NAME(inst) "_stdinout",                                             \
		.type = 'b',                                                                       \
		.dest = &native_pty_status_##inst.cmd_request_stdinout,                            \
		.descript = "Connect "INST_NAME(inst)" to STDIN/OUT instead of a PTY"              \
			    " (can only be done for one UART)"                                     \
	},                                                                                         \
	{                                                                                          \
		.is_switch = true,                                                                 \
		.option = INST_NAME(inst) "_attach_uart",                                          \
		.type = 'b',                                                                       \
		.dest = &native_pty_status_##inst.auto_attach,                                     \
		.descript = "Automatically attach "INST_NAME(inst)" to a terminal emulator."       \
			    " (only applicable when connected to PTYs)"                            \
	},                                                                                         \
	{                                                                                          \
		.option = INST_NAME(inst) "_attach_uart_cmd",                                      \
		.name = "\"cmd\"",                                                                 \
		.type = 's',                                                                       \
		.dest = &native_pty_status_##inst.auto_attach_cmd,                                 \
		.descript = "Command used to automatically attach to the terminal "INST_NAME(inst) \
			    " (implies "INST_NAME(inst)"_auto_attach), by default: "               \
			    "'" CONFIG_UART_NATIVE_PTY_AUTOATTACH_DEFAULT_CMD "'"                  \
			    " (only applicable when connected to PTYs)"                            \
	},                                                                                         \
	{                                                                                          \
		.is_switch = true,                                                                 \
		.option = INST_NAME(inst) "_wait_uart",                                            \
		.type = 'b',                                                                       \
		.dest = &native_pty_status_##inst.wait_pts,                                        \
		.descript = "Hold writes to "INST_NAME(inst)" until a client is connected/ready"   \
			   " (only applicable when connected to PTYs)"                             \
	},

static void np_add_uart_options(void)
{
	static struct args_struct_t uart_options[] = {
	/* Set of parameters that apply to all PTY UARTs: */
	{
		.is_switch = true,
		.option = "attach_uart",
		.type = 'b',
		.call_when_found = auto_attach_cb,
		.descript = "Automatically attach all PTY UARTs to a terminal emulator."
			    " (only applicable when connected to PTYs)"
	},
	{
		.option = "attach_uart_cmd",
		.name = "\"cmd\"",
		.type = 's',
		.call_when_found = auto_attach_cmd_cb,
		.descript = "Command used to automatically attach all PTY UARTs to a terminal "
			    "emulator (implies auto_attach), by default: "
			    "'" CONFIG_UART_NATIVE_PTY_AUTOATTACH_DEFAULT_CMD "'"
			    " (only applicable when connected to PTYs)"
	},
	{
		.is_switch = true,
		.option = "wait_uart",
		.type = 'b',
		.call_when_found = wait_pts_cb,
		.descript = "Hold writes to all PTY UARTs until a client is connected/ready"
			    " (only applicable when connected to PTYs)"
	},
	/* Set of parameters that apply to each individual PTY UART: */
	DT_INST_FOREACH_STATUS_OKAY(NATIVE_PTY_COMMAND_LINE_OPTS)
	ARG_TABLE_ENDMARKER
	};

	native_add_command_line_opts(uart_options);
}

#define NATIVE_PTY_CLEANUP(inst)                                                                \
	if ((!native_pty_status_##inst.on_stdinout) && (native_pty_status_##inst.in_fd != 0)) { \
		nsi_host_close(native_pty_status_##inst.in_fd);                                 \
		native_pty_status_##inst.in_fd = 0;                                             \
	}

static void np_cleanup_uart(void)
{
	DT_INST_FOREACH_STATUS_OKAY(NATIVE_PTY_CLEANUP);
}

NATIVE_TASK(np_add_uart_options, PRE_BOOT_1, 11);
NATIVE_TASK(np_cleanup_uart, ON_EXIT, 99);
