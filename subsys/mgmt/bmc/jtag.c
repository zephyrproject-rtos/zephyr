/*
 * SPDX-FileCopyrightText: Copyright (c) 2025-2026 Tenstorrent USA, Inc.
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Bit-banged JTAG adapter speaking the OpenOCD remote_bitbang protocol over
 * TCP, so that a host can debug the managed system through the BMC.
 */

#include <string.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/bmc.h>
#include <zephyr/net/socket.h>

LOG_MODULE_DECLARE(bmc, CONFIG_BMC_LOG_LEVEL);

#define JTAG_PORT CONFIG_BMC_JTAG_PORT

#define JTAG_TCK_NODE DT_ALIAS(jtagtck)
#define JTAG_TMS_NODE DT_ALIAS(jtagtms)
#define JTAG_TDI_NODE DT_ALIAS(jtagtdi)
#define JTAG_TDO_NODE DT_ALIAS(jtagtdo)

BUILD_ASSERT(DT_NODE_EXISTS(JTAG_TCK_NODE), "alias jtagtck missing");
BUILD_ASSERT(DT_NODE_EXISTS(JTAG_TMS_NODE), "alias jtagtms missing");
BUILD_ASSERT(DT_NODE_EXISTS(JTAG_TDI_NODE), "alias jtagtdi missing");
BUILD_ASSERT(DT_NODE_EXISTS(JTAG_TDO_NODE), "alias jtagtdo missing");

static const struct gpio_dt_spec tck = GPIO_DT_SPEC_GET(JTAG_TCK_NODE, gpios);
static const struct gpio_dt_spec tms = GPIO_DT_SPEC_GET(JTAG_TMS_NODE, gpios);
static const struct gpio_dt_spec tdi = GPIO_DT_SPEC_GET(JTAG_TDI_NODE, gpios);
static const struct gpio_dt_spec tdo = GPIO_DT_SPEC_GET(JTAG_TDO_NODE, gpios);

static K_THREAD_STACK_DEFINE(jtag_stack, CONFIG_BMC_JTAG_STACK_SIZE);
static struct k_thread jtag_thread_data;

/* Cached pin levels, so that unchanged lines are not driven again. */
static int tck_prev = -1;
static int tms_prev = -1;
static int tdi_prev = -1;

static inline void set_tck(int state)
{
	if (tck_prev != state) {
		gpio_pin_set_dt(&tck, state);
		tck_prev = state;
	}
}

static inline void set_tms(int state)
{
	if (tms_prev != state) {
		gpio_pin_set_dt(&tms, state);
		tms_prev = state;
	}
}

static inline void set_tdi(int state)
{
	if (tdi_prev != state) {
		gpio_pin_set_dt(&tdi, state);
		tdi_prev = state;
	}
}

static void jtag_pins_disable(void)
{
	/* Make the JTAG pins high impedance. */
	gpio_pin_configure_dt(&tck, GPIO_INPUT);
	gpio_pin_configure_dt(&tms, GPIO_INPUT);
	gpio_pin_configure_dt(&tdi, GPIO_INPUT);
	gpio_pin_configure_dt(&tdo, GPIO_INPUT);

	LOG_INF("JTAG GPIOs disabled");
}

static void jtag_pins_enable(void)
{
	/* Drive TCK and TDI low, TMS high. */
	gpio_pin_configure_dt(&tck, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&tms, GPIO_OUTPUT_ACTIVE);
	gpio_pin_configure_dt(&tdi, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&tdo, GPIO_INPUT);

	tck_prev = -1;
	tms_prev = -1;
	tdi_prev = -1;

	LOG_INF("JTAG GPIOs enabled");
}

static int jtag_pins_init(void)
{
	if (!gpio_is_ready_dt(&tck) || !gpio_is_ready_dt(&tms) || !gpio_is_ready_dt(&tdi) ||
	    !gpio_is_ready_dt(&tdo)) {
		LOG_ERR("JTAG GPIO devices not ready");
		return -ENODEV;
	}

	/* The pins stay high impedance until a client connects. */
	jtag_pins_disable();

	return 0;
}

static void jtag_write(unsigned char cmd)
{
	unsigned int key;

	/*
	 * The three lines have to move as one, otherwise the target can sample
	 * a half-updated bus.
	 */
	key = irq_lock();
	set_tms(cmd & 0x02);
	set_tdi(cmd & 0x01);
	/* TCK last, it is the edge the target latches on. */
	set_tck(cmd & 0x04);
	irq_unlock(key);
}

static void handle_client(int client_fd)
{
	bool finished = false;
	int opt = 1;

	/* Disable Nagle, the protocol is a stream of single byte commands. */
	if (zsock_setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0) {
		LOG_WRN("Could not set TCP_NODELAY (err=%d)", errno);
	}

	LOG_INF("JTAG client connected");

	jtag_pins_enable();

	while (!finished) {
		unsigned char cmd;
		int ret;

		ret = zsock_recv(client_fd, &cmd, 1, 0);
		if (ret <= 0) {
			if (ret == 0) {
				LOG_INF("JTAG client disconnected");
			} else {
				LOG_WRN("JTAG client recv error (err=%d)", errno);
			}

			break;
		}

		switch (cmd) {
		/* Blink, ignored. */
		case 'b':
		case 'B':
			break;

		/* Write TCK, TMS and TDI. */
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
			jtag_write(cmd);
			break;

		/* Read TDO. */
		case 'R': {
			char resp = gpio_pin_get_dt(&tdo) ? '1' : '0';

			ret = zsock_send(client_fd, &resp, 1, 0);
			if (ret <= 0) {
				LOG_WRN("JTAG client send error (err=%d)", errno);
				finished = true;
			}

			break;
		}

		case 'Q':
			LOG_INF("JTAG client requested quit");
			finished = true;
			break;

		/* Reset. */
		case 'r':
		case 's':
		case 't':
		case 'u':
			LOG_WRN("JTAG reset is unsupported");
			break;

		/* SWD. */
		case 'O':
		case 'o':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
		case 'g':
			LOG_WRN("SWD is unsupported");
			break;

		default:
			/* Unknown command, but no reason to drop the client. */
			LOG_WRN("Unknown JTAG command 0x%02x", cmd);
			break;
		}
	}

	jtag_pins_disable();
}

static void jtag_thread(void *a, void *b, void *c)
{
	struct sockaddr_in server_addr;
	int server_fd;

	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	if (jtag_pins_init() < 0) {
		LOG_ERR("Could not initialise the JTAG pins, stopping the daemon");
		return;
	}

	server_fd = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server_fd < 0) {
		LOG_ERR("JTAG socket() failed (err=%d)", errno);
		return;
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(JTAG_PORT);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	if (zsock_bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		LOG_ERR("JTAG bind() failed (err=%d)", errno);
		zsock_close(server_fd);
		return;
	}

	if (zsock_listen(server_fd, 1) < 0) {
		LOG_ERR("JTAG listen() failed (err=%d)", errno);
		zsock_close(server_fd);
		return;
	}

	LOG_INF("JTAG daemon listening on port %d", JTAG_PORT);

	while (true) {
		struct sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(client_addr);
		int client_fd;

		client_fd = zsock_accept(server_fd, (struct sockaddr *)&client_addr,
					 &client_addr_len);
		if (client_fd < 0) {
			LOG_ERR("JTAG accept() failed (err=%d)", errno);
			continue;
		}

		handle_client(client_fd);
		zsock_close(client_fd);
	}
}

static int jtag_init(void)
{
	k_thread_create(&jtag_thread_data, jtag_stack, K_THREAD_STACK_SIZEOF(jtag_stack),
			jtag_thread, NULL, NULL, NULL, CONFIG_BMC_JTAG_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&jtag_thread_data, "bmc_jtag");

	return 0;
}

BMC_COMPONENT_DEFINE(bmc_jtag, BMC_INIT_PHASE_SERVICE, jtag_init, true);
