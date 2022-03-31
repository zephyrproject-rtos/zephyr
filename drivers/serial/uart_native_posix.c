/*
 * Copyright (c) 2018, Oticon A/S
 * Copyright (c) 2022, Bose Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_native_posix_uart

#define _DEFAULT_SOURCE	/* RTS/CTS flow control isn't strict POSIX */

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pty.h>
#include <fcntl.h>
#include <sys/select.h>
#include <unistd.h>
#include <poll.h>
#include <pthread.h>

#include <drivers/uart.h>
#include "cmdline.h" /* native_posix command line options header */
#include "soc.h"
#include "irq_ctrl.h"	/* native_posix IRQ simulation */

/* Consider making these configurable in future? */
#define UART_NATIVE_POSIX_IRQ_FLAGS (0)
#define UART_NATIVE_POSIX_IRQ_PRIORITY (3)

/*
 * UART driver for POSIX ARCH based boards.
 * It can support up to two UARTs.
 *
 * For the first UART:
 *
 * It can either be connected to the process STDIN+STDOUT
 * OR
 * to a dedicated pseudo terminal
 *
 * The 2nd option is the recommended one for interactive use, as the pseudo
 * terminal driver will be configured in "raw" mode, and will therefore behave
 * more like a real UART.
 *
 * When connected to its own pseudo terminal, it may also auto attach a terminal
 * emulator to it, if set so from command line.
 */

IF_ENABLED(CONFIG_NATIVE_UART_0_ON_STDINOUT, (
	static int np_uart_stdin_poll_in(const struct device *dev,
					 unsigned char *p_char);
))

#if !defined(CONFIG_NATIVE_UART_0_ON_STDINOUT) || defined(CONFIG_UART_NATIVE_POSIX_PORT_1_ENABLE)

static int np_uart_poll_in(const struct device *dev,
			   unsigned char *p_char);

#endif /* !CONFIG_NATIVE_UART_0_ON_STDINOUT) || CONFIG_UART_NATIVE_POSIX_PORT_1_ENABLE */

static void np_uart_poll_out(const struct device *dev,
			     unsigned char out_char);

static int np_uart_configure(const struct device *dev,
			     const struct uart_config *cfg);

static int np_uart_config_get(const struct device *dev,
			      struct uart_config *cfg);

static int np_uart_fifo_read(const struct device *dev,
			     uint8_t *rx_data, const int size);

static int np_uart_fifo_fill(const struct device *dev,
			     const uint8_t *tx_data, int len);

IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN, (

	static void np_uart_isr(const void *arg);

	static int np_uart_irq_rx_ready(const struct device *dev);

	static int np_uart_irq_tx_ready(const struct device *dev);

	static void np_uart_irq_tx_enable(const struct device *dev);

	static void np_uart_irq_tx_disable(const struct device *dev);

	static void np_uart_irq_rx_enable(const struct device *dev);

	static void np_uart_irq_rx_disable(const struct device *dev);

	static int np_uart_irq_is_pending(const struct device *dev);

	static int np_uart_irq_update(const struct device *dev);

	static void np_uart_irq_callback_set(const struct device *dev,
					     uart_irq_callback_user_data_t cb,
					     void *user_data);
))

static bool auto_attach;
static bool wait_pts;
static const char default_cmd[] = CONFIG_NATIVE_UART_AUTOATTACH_DEFAULT_CMD;
static char *auto_attach_cmd;

static char *uart0_pathname;	/**< e.g. /dev/ttyS0 */
IF_ENABLED(CONFIG_UART_NATIVE_POSIX_PORT_1_ENABLE, (
	static char *uart1_pathname;
))

struct native_uart_status {
	int out_fd;     /**< File descriptor used for output */
	int in_fd;      /**< File descriptor used for input */

	IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN, (
		int irq;	/**< Simulated IRQ number */
		bool pending;	/**< IRQ pending */
		bool rxen;	/**< true when receive interrupts enabled */
		bool txen;	/**< true when transmit interrupts enabled */
		uart_irq_callback_user_data_t cb;
		void *user_data;
		int event_fd;	/**< signalled when any of the above changes */
	))
};

static struct native_uart_status native_uart_status_0;

static const struct uart_driver_api np_uart_driver_api_0 = {
	.poll_out = np_uart_poll_out,
#if defined(CONFIG_NATIVE_UART_0_ON_STDINOUT)
	.poll_in = np_uart_stdin_poll_in,
#else
	.poll_in = np_uart_poll_in,
#endif

	IF_ENABLED(CONFIG_UART_USE_RUNTIME_CONFIGURE, (
		.configure = np_uart_configure,
		.config_get = np_uart_config_get,
	))

	IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN, (
		.fifo_fill = np_uart_fifo_fill,
		.fifo_read = np_uart_fifo_read,
		.irq_rx_ready = np_uart_irq_rx_ready,
		.irq_tx_ready = np_uart_irq_tx_ready,
		.irq_tx_enable = np_uart_irq_tx_enable,
		.irq_tx_disable = np_uart_irq_tx_disable,
		.irq_rx_enable = np_uart_irq_rx_enable,
		.irq_rx_disable = np_uart_irq_rx_disable,
		.irq_is_pending = np_uart_irq_is_pending,
		.irq_update = np_uart_irq_update,
		.irq_callback_set = np_uart_irq_callback_set,
	))
};

#if defined(CONFIG_UART_NATIVE_POSIX_PORT_1_ENABLE)
static struct native_uart_status native_uart_status_1;

static const struct uart_driver_api np_uart_driver_api_1 = {
	.poll_out = np_uart_poll_out,
	.poll_in = np_uart_poll_in,

	IF_ENABLED(CONFIG_UART_USE_RUNTIME_CONFIGURE, (
		.configure = np_uart_configure,
		.config_get = np_uart_config_get,
	))

	IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN, (
		.fifo_fill = np_uart_fifo_fill,
		.fifo_read = np_uart_fifo_read,
		.irq_rx_ready = np_uart_irq_rx_ready,
		.irq_tx_ready = np_uart_irq_tx_ready,
		.irq_tx_enable = np_uart_irq_tx_enable,
		.irq_tx_disable = np_uart_irq_tx_disable,
		.irq_rx_enable = np_uart_irq_rx_enable,
		.irq_rx_disable = np_uart_irq_rx_disable,
		.irq_is_pending = np_uart_irq_is_pending,
		.irq_update = np_uart_irq_update,
		.irq_callback_set = np_uart_irq_callback_set,
	))
};
#endif /* CONFIG_UART_NATIVE_POSIX_PORT_1_ENABLE */

/**
 * @brief find UART_n index of a np_uart device structure e.g. for log messages
 * @retval 0 for UART_0
 * @retval 1 for UART_1
 */
#define INDEX(device) ((device->data != &native_uart_status_0) ? 1 : 0)

/**
 * @brief enable data logging
 * @retval true enables verbose log
 */
#define DEBUG(device) false /*(INDEX(device) == 1)*/

#define LOG posix_print_trace
#define EXIT posix_print_error_and_exit
#define WARN posix_print_warning

/**
 * @brief Configure an open tty for use by this driver
 * @param fd open file descriptor
 * @return true on success
 */
static bool config_tty(int fd)
{
	struct termios ter;
	int ret;
	int flags;

	/* Set the master PTY as non blocking */
	flags = fcntl(fd, F_GETFL);
	if (flags == -1) {
		WARN("Could not read the master PTY file status flags (%i)\n",
			errno);
		return false;
	}

	ret = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	if (ret == -1) {
		WARN("Could not set the master PTY as non-blocking (%i)\n",
			errno);
		return false;
	}

	/*
	 * Set terminal in "raw" mode:
	 *  Not canonical (no line input)
	 *  No signal generation from Ctr+{C|Z..}
	 *  No echoing, no input or output processing
	 *  No replacing of NL or CR
	 *  No flow control
	 */
	ret = tcgetattr(fd, &ter);
	if (ret == -1) {
		WARN("Could not read terminal driver settings\n");
		return false;
	}
	ter.c_cc[VMIN] = 0;
	ter.c_cc[VTIME] = 0;
	ter.c_lflag &= ~(ICANON | ISIG | IEXTEN | ECHO);
	ter.c_iflag &= ~(BRKINT | ICRNL | IGNBRK | IGNCR | INLCR | INPCK
			 | ISTRIP | IXON | PARMRK);
	ter.c_oflag &= ~OPOST;
	ret = tcsetattr(fd, TCSANOW, &ter);
	if (ret == -1) {
		WARN("Could not change terminal driver settings\n");
		return false;
	}

	return true;
}

/**
 * Attempt to allocate and open a new pseudoterminal
 *
 * Returns the file descriptor of the master side
 * If auto_attach was set, it will also attempt to connect a new terminal
 * emulator to its slave side.
 */
static int open_tty(const char *uart_name, bool do_auto_attach)
{
	int master_pty;
	char *slave_pty_name;
	int ret;

	master_pty = posix_openpt(O_RDWR | O_NOCTTY);
	if (master_pty == -1) {
		EXIT("Could not open a new TTY for the UART\n");
	}
	ret = grantpt(master_pty);
	if (ret == -1) {
		close(master_pty);
		EXIT("Could not grant access to the slave PTY side (%i)\n",
			errno);
	}
	ret = unlockpt(master_pty);
	if (ret == -1) {
		close(master_pty);
		EXIT("Could not unlock the slave PTY side (%i)\n", errno);
	}
	slave_pty_name = ptsname(master_pty);
	if (slave_pty_name == NULL) {
		close(master_pty);
		EXIT("Error getting slave PTY device name (%i)\n", errno);
	}

	if (!config_tty(master_pty)) {
		close(master_pty);
		return -1;
	}

	LOG("%s connected to pseudotty: %s\n", uart_name, slave_pty_name);

	if (wait_pts) {
		/*
		 * This trick sets the HUP flag on the tty master, making it
		 * possible to detect a client connection using poll.
		 * The connection of the client would cause the HUP flag to be
		 * cleared, and in turn set again at disconnect.
		 */
		close(open(slave_pty_name, O_RDWR | O_NOCTTY));
	}
	if (do_auto_attach) {
		if (auto_attach_cmd == NULL) {
			auto_attach_cmd = (char *)default_cmd;
		}

		/* Roll our own %s substitution as sprintf() is too risky */
		char command[strlen(auto_attach_cmd) + strlen(slave_pty_name) + 1];
		const char *insert = strstr(auto_attach_cmd, "%s");

		if (insert) {
			size_t const where = insert-auto_attach_cmd;
			(void)memcpy(command, auto_attach_cmd, where);
			(void)strcpy(command+where, slave_pty_name);
			(void)strcat(command, &auto_attach_cmd[where+2]);
		} else {
			(void)strcpy(command, auto_attach_cmd);
		}

		int ret = system(command);

		if (ret != 0) {
			WARN("Could not attach to the UART with \"%s\"\n", command);
			WARN("The command returned %i\n", WEXITSTATUS(ret));
		}
	}

	return master_pty;
}


/**
 * @brief Initialize the first native_posix serial port
 *
 * @param dev UART_0 device struct
 *
 * @return 0 (if it fails catastrophically, the execution is terminated)
 */
static int np_uart_0_init(const struct device *dev)
{
	struct native_uart_status *d;

	d = (struct native_uart_status *)dev->data;

	if (IS_ENABLED(CONFIG_NATIVE_UART_0_ON_OWN_PTY)) {
		int tty_fn = open_tty(DT_INST_LABEL(0), auto_attach);

		if (tty_fn < 0) {
			EXIT("Could not open ptty for UART_0\n");
		} else {
			d->in_fd = tty_fn;
			d->out_fd = tty_fn;
		}
	} else if (IS_ENABLED(CONFIG_NATIVE_UART_0_ON_DEVICE)) {
		int tty_fn = open(uart0_pathname, O_RDWR | O_NOCTTY);

		if (tty_fn < 0) {
			if (!uart0_pathname) {
				EXIT("--uart_0_device not set\n");
			} else {
				EXIT("Could not open \"%s\" for UART_0\n", uart0_pathname);
			}
		} else if (!config_tty(tty_fn)) {
			close(tty_fn);
			EXIT("Could not configure \"%s\" for UART_0\n", uart0_pathname);
		} else {
			d->in_fd = tty_fn;
			d->out_fd = tty_fn;
		}

		/* Set initial configuration */
		if (DT_INST_PROP(0, current_speed) || DT_INST_PROP(0, hw_flow_control)) {
			struct uart_config cfg;

			if (np_uart_config_get(dev, &cfg) == 0) {
				if (DT_INST_PROP(0, current_speed)) {
					cfg.baudrate = DT_INST_PROP(0, current_speed);
				}
				if (DT_INST_PROP(0, hw_flow_control)) {
					cfg.flow_ctrl = UART_CFG_FLOW_CTRL_RTS_CTS;
				}
				np_uart_configure(dev, &cfg);
			}
		}
	} else { /* NATIVE_UART_0_ON_STDINOUT */
		d->in_fd  = STDIN_FILENO;
		d->out_fd = STDOUT_FILENO;

		if (isatty(STDIN_FILENO)) {
			WARN("The UART driver has been configured to map to the"
			     " process stdin&out (NATIVE_UART_0_ON_STDINOUT), "
			     "but stdin seems to be left attached to the shell."
			     " This will most likely NOT behave as you want it "
			     "to. This option is NOT meant for interactive use "
			     "but for piping/feeding from/to files to the UART"
			);
		}
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

	d->irq = UART0_IRQ;
	IRQ_CONNECT(UART0_IRQ, UART_NATIVE_POSIX_IRQ_PRIORITY,
		    np_uart_isr, dev, UART_NATIVE_POSIX_IRQ_FLAGS);
	irq_enable(UART0_IRQ);

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	return 0;
}

#if defined(CONFIG_UART_NATIVE_POSIX_PORT_1_ENABLE)
/**
 * @brief Initialize a second native_posix serial port
 *
 * @param dev UART_1 device struct
 *
 * @return 0 (if it fails catastrophically, the execution is terminated)
 */
static int np_uart_1_init(const struct device *dev)
{
	struct native_uart_status *d;

	d = (struct native_uart_status *)dev->data;

	if (IS_ENABLED(CONFIG_NATIVE_UART_1_ON_OWN_PTY)) {
		int tty_fn = open_tty(DT_INST_LABEL(1), false);

		if (tty_fn < 0) {
			EXIT("Could not open ptty for UART_0\n");
		} else {
			d->in_fd = tty_fn;
			d->out_fd = tty_fn;
		}
	} else { /* NATIVE_UART_1_ON_DEVICE */
		int tty_fn = open(uart1_pathname, O_RDWR | O_NOCTTY);

		if (tty_fn < 0) {
			if (!uart1_pathname) {
				EXIT("--uart_1_device not set\n");
			} else {
				EXIT("Could not open \"%s\" for UART_1\n", uart1_pathname);
			}
		} else if (!config_tty(tty_fn)) {
			close(tty_fn);
			EXIT("Could not configure \"%s\" for UART_1\n", uart1_pathname);
		} else {
			d->in_fd = tty_fn;
			d->out_fd = tty_fn;
		}

		/* Set initial configuration */
		if (DT_INST_PROP(1, current_speed) || DT_INST_PROP(1, hw_flow_control)) {
			struct uart_config cfg;

			if (np_uart_config_get(dev, &cfg) == 0) {
				if (DT_INST_PROP(1, current_speed)) {
					cfg.baudrate = DT_INST_PROP(1, current_speed);
				}
				if (DT_INST_PROP(1, hw_flow_control)) {
					cfg.flow_ctrl = UART_CFG_FLOW_CTRL_RTS_CTS;
				}
				np_uart_configure(dev, &cfg);
			}
		}
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

	d->irq = UART1_IRQ;
	IRQ_CONNECT(UART1_IRQ, UART_NATIVE_POSIX_IRQ_PRIORITY,
		    np_uart_isr, dev, UART_NATIVE_POSIX_IRQ_FLAGS);
	irq_enable(UART1_IRQ);

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	return 0;
}
#endif /* CONFIG_UART_NATIVE_POSIX_PORT_1_ENABLE */

static int np_uart_configure(const struct device *dev,
			     const struct uart_config *cfg)
{
	struct native_uart_status *d = (struct native_uart_status *)dev->data;
	struct termios ter;

	/* We assume IN is the same as OUT file descriptor */
	int const fd = d->in_fd;

	if (d->in_fd != d->out_fd) {
		/* Probably stdin/stdout */
		WARN("Could not configure stdin/stdout settings\n");
		return -ENOSYS;
	}
	if (tcgetattr(fd, &ter) != 0) {
		WARN("Failed to read tty settings\n");
		return -errno;
	}

	/* We currently only support baud and RTS/CTS flow control */
	if (cfg->parity != UART_CFG_PARITY_NONE) {
		return -ENOTSUP;
	}

	if (cfg->data_bits != UART_CFG_DATA_BITS_8) {
		return -ENOTSUP;
	}

	if (cfg->stop_bits != UART_CFG_STOP_BITS_1) {
		return -ENOTSUP;
	}

	if (cfsetspeed(&ter, cfg->baudrate) != 0) {
		WARN("Could not set %d baud\n", cfg->baudrate);
		return -ENOTSUP;
	}

	switch (cfg->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		ter.c_cflag &= ~CRTSCTS;	/* Not strictly POSIX */
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		ter.c_cflag |= CRTSCTS;		/* Not strictly POSIX */
		break;
	default:
		WARN("Could not set flow control %d\n", cfg->flow_ctrl);
		return -ENOTSUP;
	}

	if (tcsetattr(fd, TCSANOW, &ter) != 0) {
		WARN("Could not write tty settings\n");
		return -errno;
	}

	if (DEBUG(dev)) {
		LOG("UART_%d %u baud %s flow control\n", INDEX(dev),
			cfg->baudrate,
			cfg->flow_ctrl == UART_CFG_FLOW_CTRL_NONE ? "no" : "hw");
	}

	return 0;
};

static int np_uart_config_get(const struct device *dev,
			      struct uart_config *cfg)
{
	struct native_uart_status *d = (struct native_uart_status *)dev->data;
	struct termios ter;

	/* We assume IN is the same as OUT file descriptor */
	int const fd = d->in_fd;

	if (d->in_fd != d->out_fd) {
		/* Probably stdin/stdout */
		WARN("Could not fetch stdin/stdout settings\n");
		return -ENOSYS;
	}
	if (tcgetattr(fd, &ter) != 0) {
		WARN("Failed to fetch tty settings\n");
		return -errno;
	}

	cfg->parity = UART_CFG_PARITY_NONE;
	cfg->stop_bits = UART_CFG_STOP_BITS_1;
	cfg->data_bits = UART_CFG_DATA_BITS_8;
	cfg->baudrate = cfgetispeed(&ter);
	cfg->flow_ctrl = (ter.c_cflag & CRTSCTS)	/* Not strictly POSIX */
			? UART_CFG_FLOW_CTRL_RTS_CTS
			: UART_CFG_FLOW_CTRL_NONE;

	return 0;
}

static int np_uart_fifo_fill(const struct device *dev,
			     const uint8_t *tx_data, int len)
{
	struct native_uart_status *d = (struct native_uart_status *)dev->data;
	int rc = write(d->out_fd, tx_data, len);

	if (DEBUG(dev)) {
		if (rc != len) {
			LOG("UART_%d: TX %d/%d\n", INDEX(dev), rc, len);
		}
		for (int i = 0; i < rc; i++) {
			if (!(i % 16)) {
				if (i) {
					LOG("\n");
				}
				LOG("UART_%d TX: ", INDEX(dev));
			}
			LOG("%02x ", tx_data[i]);
		}
		LOG("\n");
	}
	return rc;
}

static int np_uart_fifo_read(const struct device *dev, uint8_t *rx_data,
			     const int size)
{
	struct native_uart_status *d = (struct native_uart_status *)dev->data;
	int rc = read(d->in_fd, rx_data, size);

	if (DEBUG(dev)) {
		if (rc != size) {
			LOG("UART_%d RX %d/%d\n", INDEX(dev), rc, size);
		}
		for (int i = 0; i < rc; i++) {
			if (!(i % 16)) {
				if (i) {
					LOG("\n");
				}
				LOG("UART_%d RX: ", INDEX(dev));
			}
			LOG("%02x ", rx_data[i]);
		}
		LOG("\n");
	}
	return rc;
}

static int np_uart_rx_ready(const struct device *dev)
{
	struct native_uart_status *d = (struct native_uart_status *)dev->data;
	struct pollfd pfd = {
		.fd = d->in_fd,
		.events = POLLIN,
	};

	poll(&pfd, 1, 0);
	if (!(pfd.revents & POLLIN)) {
		/* Not ready */
		if (DEBUG(dev)) {
			LOG("UART_%d RX not ready\n", INDEX(dev));
		}
		return 0;
	}
	if (DEBUG(dev)) {
		LOG("UART_%d RX ok\n", INDEX(dev));
	}
	return 1;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int np_uart_irq_rx_ready(const struct device *dev)
{
	struct native_uart_status *d = (struct native_uart_status *)dev->data;

	return np_uart_rx_ready(dev) && d->rxen;
}

static int np_uart_irq_tx_ready(const struct device *dev)
{
	struct native_uart_status *d = (struct native_uart_status *)dev->data;
	struct pollfd pfd = {
		.fd = d->out_fd,
		.events = POLLOUT,
	};

	poll(&pfd, 1, 0);
	if (!(pfd.revents & POLLOUT)) {
		/* Not ready */
		if (DEBUG(dev)) {
			LOG("UART_%d TX not ready\n", INDEX(dev));
		}
		return 0;
	}
	if (DEBUG(dev)) {
		LOG("UART_%d TX ok\n", INDEX(dev));
	}
	return d->txen;
}

static void *np_uart_irq_worker(void *context)
{
	const struct device *dev = (const struct device *)context;
	struct native_uart_status *d = (struct native_uart_status *)dev->data;

	/* Setup wake source */
	int pipefd[2];

	if (pipe(pipefd) != 0) {
		EXIT("Failed to instantiate worker signal\n");
		return NULL;
	}
	d->event_fd = pipefd[1];

	/* Keep going forever */
	for (;;) {
		/* Fake an interrupt request mechanism using poll() */
		bool bIrq = false;

		/* Decide what we're waiting for */
		struct pollfd pfd[3] = {
			[0] = {
				.fd = d->in_fd,
				.events = d->rxen ? POLLIN : 0,
			},
			[1] = {
				.fd = d->out_fd,
				.events = d->txen ? POLLOUT : 0,
			},
			[2] = {
				.fd = pipefd[0],
				.events = POLLIN,
			}
		};

		/* Wait forever on one of these things */
		if (DEBUG(dev)) {
			LOG("UART_%d WFI\n", INDEX(dev));
		}
		(void)poll(pfd, 3, -1);
		if (DEBUG(dev)) {
			LOG("UART_%d IRQ ", INDEX(dev));
		}

		/* Handle events */
		if (pfd[0].revents != 0) {
			if (pfd[0].revents == POLLIN) {
				/* Callback should service */
				bIrq = true;
				if (DEBUG(dev)) {
					LOG("RX ");
				}
			} else {
				/* We can't handle POLLERR, POLLHUP, POLLNVAL */
				EXIT("%s in_fd revent 0x%x\n", __func__, pfd[0].revents);
			}
		}
		if (pfd[1].revents != 0) {
			if (pfd[1].revents == POLLOUT) {
				/* Callback should service */
				bIrq = true;
				if (DEBUG(dev)) {
					LOG("TX ");
				}
			} else {
				/* We can't handle POLLERR, POLLHUP, POLLNVAL */
				EXIT("%s out_fd revent 0x%x\n", __func__, pfd[1].revents);
			}
		}
		if (pfd[2].revents != 0) {
			if (pfd[2].revents == POLLIN) {
				/* This just wakes us up to re-read d->rxen, d->txen etc */
				int dummy;

				dummy = read(pipefd[0], &dummy, 1);
				if (DEBUG(dev)) {
					LOG("WAKE ");
				}
			} else {
				/* We can't handle POLLERR, POLLHUP, POLLNVAL */
				EXIT("%s event_fd revent 0x%x\n", __func__, pfd[2].revents);
			}
		}
		if (DEBUG(dev)) {
			LOG("\n");
		}

		/* Wake simulated CPU? */
		if (bIrq) {
			d->pending = true;
			hw_irq_ctrl_set_irq(d->irq);

			/* Wait for np_uart_isr() to complete */
			while (d->pending) {
				struct pollfd patience = {
					.fd = pipefd[0],
					.events = POLLIN,
				};

				(void)poll(&patience, 1, -1);
			}
		}
	}
}

static void np_uart_irq_wake(const struct device *dev)
{
	struct native_uart_status *d = (struct native_uart_status *)dev->data;
	/* Wake worker by writing arbitrary byte to the pipe */
	int dummy = 0;

	dummy = write(d->event_fd, &dummy, 1);
}

/**
 * @brief Simulate a UART interrupt
 * @param arg device structure
 */
static void np_uart_isr(const void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct native_uart_status *d = (struct native_uart_status *)dev->data;

	if (!d->cb) {
		EXIT("%s callback NULL", __func__);
	}
	if (DEBUG(dev)) {
		LOG("UART_%d ISR\n", INDEX(dev));
	}
	d->cb(dev, d->user_data);
	d->pending = false;
	np_uart_irq_wake(dev);
}

static void np_uart_irq_tx_enable(const struct device *dev)
{
	struct native_uart_status *d = (struct native_uart_status *)dev->data;

	if (d->txen) {
		return;
	}
	d->txen = 1;
	if (DEBUG(dev)) {
		LOG("UART_%d TX enable\n", INDEX(dev));
	}
	np_uart_irq_wake(dev);
}

static void np_uart_irq_tx_disable(const struct device *dev)
{
	struct native_uart_status *d = (struct native_uart_status *)dev->data;

	if (!d->txen) {
		return;
	}
	d->txen = 0;
	if (DEBUG(dev)) {
		LOG("UART_%d TX disable\n", INDEX(dev));
	}
	np_uart_irq_wake(dev);
}

static void np_uart_irq_rx_enable(const struct device *dev)
{
	struct native_uart_status *d = (struct native_uart_status *)dev->data;

	if (d->rxen) {
		return;
	}
	d->rxen = 1;
	if (DEBUG(dev)) {
		LOG("UART_%d RX enable\n", INDEX(dev));
	}
	np_uart_irq_wake(dev);
}

static void np_uart_irq_rx_disable(const struct device *dev)
{
	struct native_uart_status *d = (struct native_uart_status *)dev->data;

	if (!d->rxen) {
		return;
	}
	d->rxen = 0;
	if (DEBUG(dev)) {
		LOG("UART_%d RX disable\n", INDEX(dev));
	}
	np_uart_irq_wake(dev);
}

static int np_uart_irq_is_pending(const struct device *dev)
{
	return np_uart_irq_rx_ready(dev) || np_uart_irq_tx_ready(dev);
}

static int np_uart_irq_update(const struct device *dev)
{
	/* No-op on this platform */
	return 1;
}

static void np_uart_irq_callback_set(const struct device *dev,
				     uart_irq_callback_user_data_t cb,
				     void *user_data)
{
	struct native_uart_status *d = (struct native_uart_status *)dev->data;

	d->cb = cb;
	d->user_data = user_data;

	if (!d->event_fd) {
		/* On first callback set we start the worker thread which runs forever */
		pthread_t pt;

		if (DEBUG(dev)) {
			LOG("UART_%d callback set\n", INDEX(dev));
		}
		if (pthread_create(&pt, NULL, np_uart_irq_worker, (void *)dev) != 0) {
			EXIT("%s to instantiate worker thread\n", __func__);
		}
	} else {
		/* Use the event_fd to wake the running worker thread */
		if (DEBUG(dev)) {
			LOG("UART_%d callback change\n", INDEX(dev));
		}
		np_uart_irq_wake(dev);
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

/*
 * @brief Output a character towards the serial port
 *
 * @param dev UART device struct
 * @param out_char Character to send.
 */
static void np_uart_poll_out(const struct device *dev,
			     unsigned char out_char)
{
	struct native_uart_status *d = (struct native_uart_status *)dev->data;

	if (wait_pts) {
		struct pollfd pfd = { .fd = d->out_fd, .events = POLLHUP };

		while (1) {
			poll(&pfd, 1, 0);
			if (!(pfd.revents & POLLHUP)) {
				/* There is now a reader on the slave side */
				break;
			}
			k_sleep(K_MSEC(100));
		}
	}

	(void)np_uart_fifo_fill(dev, &out_char, 1);
}

#ifdef CONFIG_NATIVE_UART_0_ON_STDINOUT

/**
 * @brief Poll the device for input.
 *
 * @param dev UART device structure.
 * @param p_char Pointer to character.
 *
 * @retval 0 If a character arrived and was stored in p_char
 * @retval -1 If no character was available to read
 */
static int np_uart_stdin_poll_in(const struct device *dev,
				 unsigned char *p_char)
{
	static bool disconnected;

	if (disconnected || feof(stdin)) {
		/*
		 * The stdinput is fed from a file which finished or the user
		 * pressed Ctrl+D
		 */
		disconnected = true;
		return -1;
	}

	if (!np_uart_rx_ready(dev)) {
		return -1;
	}
	if (np_uart_fifo_read(dev, p_char, 1) < 1) {
		return -1;
	}
	return 0;
}

#endif /* CONFIG_NATIVE_UART_0_ON_STDINOUT */

#if !defined(CONFIG_NATIVE_UART_0_ON_STDINOUT) || defined(CONFIG_UART_NATIVE_POSIX_PORT_1_ENABLE)

/**
 * @brief Poll the device for input.
 *
 * @param dev UART device structure.
 * @param p_char Pointer to character.
 *
 * @retval 0 If a character arrived and was stored in p_char
 * @retval -1 If no character was available to read
 */
static int np_uart_poll_in(const struct device *dev,
				   unsigned char *p_char)
{
	if (!np_uart_rx_ready(dev)) {
		return -1;
	}
	if (np_uart_fifo_read(dev, p_char, 1) < 1) {
		return -1;
	}
	return 0;
}

#endif /* !CONFIG_NATIVE_UART_0_ON_STDINOUT) || CONFIG_UART_NATIVE_POSIX_PORT_1_ENABLE */

DEVICE_DT_INST_DEFINE(0,
		&np_uart_0_init, NULL,
		(void *)&native_uart_status_0, NULL,
		PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,
		&np_uart_driver_api_0);

#if defined(CONFIG_UART_NATIVE_POSIX_PORT_1_ENABLE)
DEVICE_DT_INST_DEFINE(1,
		&np_uart_1_init, NULL,
		(void *)&native_uart_status_1, NULL,
		PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,
		&np_uart_driver_api_1);
#endif /* CONFIG_UART_NATIVE_POSIX_PORT_1_ENABLE */

#if defined(CONFIG_NATIVE_UART_0_ON_OWN_PTY)
static void auto_attach_cmd_cb(char *argv, int offset)
{
	ARG_UNUSED(argv);
	ARG_UNUSED(offset);

	auto_attach = true;
}
#endif /* CONFIG_NATIVE_UART_0_ON_OWN_PTY */

static void np_add_uart_options(void)
{
	static struct args_struct_t uart_options[] = {
		/*
		 * Fields:
		 * manual, mandatory, switch,
		 * option_name, var_name ,type,
		 * destination, callback,
		 * description
		 */
		IF_ENABLED(CONFIG_NATIVE_UART_0_ON_OWN_PTY, (
			{false, false, true,
			"attach_uart", "", 'b',
			(void *)&auto_attach, NULL,
			"Automatically attach to the UART terminal"},
			{false, false, false,
			"attach_uart_cmd", "\"cmd\"", 's',
			(void *)&auto_attach_cmd, auto_attach_cmd_cb,
			"Command used to automatically attach to the terminal"
			"(implies auto_attach), by "
			"default: '" CONFIG_NATIVE_UART_AUTOATTACH_DEFAULT_CMD "'"},)
		)
		IF_ENABLED(CONFIG_NATIVE_UART_0_ON_DEVICE, (
			{false, true, false,
			"uart_0_device", "pathname", 's',
			(void *)&uart0_pathname, NULL,
			"Host device to attach to UART_0 e.g. /dev/ttyS0"},)
		)
		IF_ENABLED(CONFIG_NATIVE_UART_1_ON_DEVICE, (
			{false, true, false,
			"uart_1_device", "pathname", 's',
			(void *)&uart1_pathname, NULL,
			"Host device to attach to UART_1 e.g. /dev/ttyS1"},)
		)
		IF_ENABLED(CONFIG_UART_NATIVE_WAIT_PTS_READY_ENABLE, (
			{false, false, true,
			"wait_uart", "", 'b',
			(void *)&wait_pts, NULL,
			"Hold writes to the uart/pts until a client is "
			"connected/ready"},)
		)
		ARG_TABLE_ENDMARKER
	};

	native_add_command_line_opts(uart_options);
}

static void np_cleanup_uart(void)
{
	if (IS_ENABLED(CONFIG_NATIVE_UART_0_ON_OWN_PTY)) {
		if (native_uart_status_0.in_fd != 0) {
			close(native_uart_status_0.in_fd);
		}
	}

#if defined(CONFIG_UART_NATIVE_POSIX_PORT_1_ENABLE)
	if (native_uart_status_1.in_fd != 0) {
		close(native_uart_status_1.in_fd);
	}
#endif
}

NATIVE_TASK(np_add_uart_options, PRE_BOOT_1, 11);
NATIVE_TASK(np_cleanup_uart, ON_EXIT, 99);
