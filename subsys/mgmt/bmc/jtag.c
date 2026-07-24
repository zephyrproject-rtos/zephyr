/*
 * SPDX-FileCopyrightText: © 2025-2026 Tenstorrent USA, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bmc_jtag, LOG_LEVEL_INF);

#include <zephyr/posix/fcntl.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/net/socket.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>

#define JTAG_PORT 7777
#define JTAG_DAEMON_STACK_SIZE K_THREAD_STACK_SIZEOF(jtag_stack_area)
#define JTAG_DAEMON_PRIORITY   CONFIG_BMC_JTAG_DAEMON_PRIORITY

K_THREAD_STACK_DEFINE(jtag_stack_area, CONFIG_BMC_JTAG_DAEMON_STACK_SIZE);
static struct k_thread jtag_thread_data;

#define JTAG_TCK_NODE  DT_ALIAS(jtagtck)
#define JTAG_TMS_NODE  DT_ALIAS(jtagtms)
#define JTAG_TDI_NODE  DT_ALIAS(jtagtdi)
#define JTAG_TDO_NODE  DT_ALIAS(jtagtdo)

BUILD_ASSERT(DT_NODE_EXISTS(JTAG_TCK_NODE), "alias jtagtck missing");
BUILD_ASSERT(DT_NODE_EXISTS(JTAG_TMS_NODE), "alias jtagtms missing");
BUILD_ASSERT(DT_NODE_EXISTS(JTAG_TDI_NODE), "alias jtagtdi missing");
BUILD_ASSERT(DT_NODE_EXISTS(JTAG_TDO_NODE), "alias jtagtdo missing");

static const struct gpio_dt_spec tck = GPIO_DT_SPEC_GET(JTAG_TCK_NODE, gpios);
static const struct gpio_dt_spec tms = GPIO_DT_SPEC_GET(JTAG_TMS_NODE, gpios);
static const struct gpio_dt_spec tdi = GPIO_DT_SPEC_GET(JTAG_TDI_NODE, gpios);
static const struct gpio_dt_spec tdo = GPIO_DT_SPEC_GET(JTAG_TDO_NODE, gpios);

static int tck_prev = -1;
static inline void set_tck(int state)
{
	if (tck_prev != state) {
		gpio_pin_set_dt(&tck, state);
		tck_prev = state;
	}
}

static int tms_prev = -1;
static inline void set_tms(int state)
{
	if (tms_prev != state) {
		gpio_pin_set_dt(&tms, state);
		tms_prev = state;
	}
}

static int tdi_prev = -1;
static inline void set_tdi(int state)
{
	if (tdi_prev != state) {
		gpio_pin_set_dt(&tdi, state);
		tdi_prev = state;
	}
}

static inline int get_tdo(void)
{
	return gpio_pin_get_dt(&tdo);
}

static void jtag_pins_disable(void)
{
	/* Make JTAG pins high impedance */
	gpio_pin_configure_dt(&tck, GPIO_INPUT);
	gpio_pin_configure_dt(&tms, GPIO_INPUT);
	gpio_pin_configure_dt(&tdi, GPIO_INPUT);
	gpio_pin_configure_dt(&tdo, GPIO_INPUT);

	LOG_INF("JTAG GPIOs disabled");
}

static void jtag_pins_enable(void)
{
	/* Drive TCK, TDI low and TMS high */
	gpio_pin_configure_dt(&tck, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&tms, GPIO_OUTPUT_ACTIVE);
	gpio_pin_configure_dt(&tdi, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&tdo, GPIO_INPUT);

	/* Reset cached pin states */
	tck_prev = -1;
	tms_prev = -1;
	tdi_prev = -1;

	LOG_INF("JTAG GPIOs enabled");
}

static int jtag_pins_init(void)
{
	if (!device_is_ready(tck.port) || !device_is_ready(tms.port) ||
	    !device_is_ready(tdi.port) || !device_is_ready(tdo.port)) {
		LOG_ERR("JTAG GPIO devices not ready!");
		return -ENODEV;
	}

	/* JTAG pins are high impedance until someone connects to the TCP port */
	jtag_pins_disable();

	return 0;
}

static void handle_client(int client_fd)
{
	/*
	 * Improve performance by enabling TCP_NODELAY (ie disabling Nagle)
	 */
	int opt = 1;
	if (setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0) {
		LOG_WRN("Failed to set TCP_NODELAY: %d", errno);
	}

	LOG_INF("Client connected, TCP_NODELAY enabled.");

	jtag_pins_enable();

	bool finished = false;
	while (!finished) {
		unsigned char cmd_buf;
		int rc;

		rc = recv(client_fd, &cmd_buf, 1, 0);
		if (rc <= 0) {
			if (rc == 0) {
				LOG_INF("Client disconnected gracefully.");
			} else {
				LOG_WRN("Client recv() error: %d", errno);
			}
			finished = true;
			break;
		}

		switch (cmd_buf) {
		/* Blink, ignore */
		case 'b':
		case 'B':
			break;

		/* Write */
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
			/*
			 * We may be able to remove the lock, or perhaps change
			 * our priority to K_PRIO_COOP()
			 */
			unsigned int key = irq_lock();
			set_tms(cmd_buf & 0x02);
			set_tdi(cmd_buf & 0x01);
			/* Write tck last */
			set_tck(cmd_buf & 0x04);
			irq_unlock(key);
			break;

		/* Read */
		case 'R': {
			int tdo_val = get_tdo();

			char resp = tdo_val ? '1' : '0';
			rc = send(client_fd, &resp, 1, 0);
			if (rc <= 0) {
				LOG_WRN("Client send() error: %d", errno);
				finished = true;
			}
			break;
		}

		/* Quit */
		case 'Q':
			LOG_INF("Received 'Q' (Quit) command.");
			finished = true;
			break;

		/* Reset */
		case 'r':
		case 's':
		case 't':
		case 'u':
			LOG_WRN("Reset unsupported");
			break; // Unsupported

		/* SWD */
		case 'O':
		case 'o':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
		case 'g':
			LOG_WRN("SWD unsupported");
			break; // Unsupported

		default:
			LOG_WRN("Unknown command: 0x%02x", cmd_buf);
			break; // Unknown command, don't disconnect
		}
	}

	jtag_pins_disable();
}

static void jtag_daemon_thread(void *a, void *b, void *c)
{
	int server_fd;
	struct sockaddr_in server_addr;

	if (jtag_pins_init() != 0) {
		LOG_ERR("Failed to initialize JTAG pins. Halting daemon.");
		return;
	}

	server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server_fd < 0) {
		LOG_ERR("socket() failed: %d", errno);
		return;
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(JTAG_PORT);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		LOG_ERR("bind() failed: %d", errno);
		close(server_fd);
		return;
	}

	if (listen(server_fd, 1) < 0) {
		LOG_ERR("listen() failed: %d", errno);
		close(server_fd);
		return;
	}

	LOG_INF("JTAG daemon listening on port %d...", JTAG_PORT);

	while (1) {
		struct sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(client_addr);
		int client_fd;

		client_fd = accept(server_fd, (struct sockaddr *)&client_addr,
				   &client_addr_len);
		if (client_fd < 0) {
			LOG_ERR("accept() failed: %d", errno);
			continue;
		}

		handle_client(client_fd);

		close(client_fd);
		LOG_INF("Client disconnected. Waiting for new connection...");
	}

	/* Unreachable */
	close(server_fd);
}

int jtag_init(void)
{
	LOG_INF("Starting OpenOCD JTAG Daemon...");

	/* Start the JTAG daemon thread */
	k_thread_create(&jtag_thread_data, jtag_stack_area,
			JTAG_DAEMON_STACK_SIZE,
			jtag_daemon_thread,
			NULL, NULL, NULL,
			JTAG_DAEMON_PRIORITY, 0, K_NO_WAIT);

	k_thread_name_set(&jtag_thread_data, "jtag_daemon");

	return 0;
}
