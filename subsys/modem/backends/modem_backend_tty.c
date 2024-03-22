/*
 * Copyright (c) 2022 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/modem/backend/tty.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modem_backend_tty, CONFIG_MODEM_MODULES_LOG_LEVEL);

#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>

#define MODEM_BACKEND_TTY_THREAD_PRIO          (10)
#define MODEM_BACKEND_TTY_THREAD_RUN_PERIOD_MS (1000)
#define MODEM_BACKEND_TTY_THREAD_POLL_DELAY    (100)

#define MODEM_BACKEND_TTY_STATE_RUN_BIT (1)

static void modem_backend_tty_routine(void *p1, void *p2, void *p3)
{
	struct modem_backend_tty *backend = (struct modem_backend_tty *)p1;
	struct pollfd pd;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	pd.fd = backend->tty_fd;
	pd.events = POLLIN;

	/* Run until run flag is cleared. Check every MODEM_BACKEND_TTY_THREAD_RUN_PERIOD_MS */
	while (atomic_test_bit(&backend->state, MODEM_BACKEND_TTY_STATE_RUN_BIT)) {
		/* Clear events */
		pd.revents = 0;

		if (poll(&pd, 1, MODEM_BACKEND_TTY_THREAD_RUN_PERIOD_MS) < 0) {
			LOG_ERR("Poll operation failed");
			break;
		}

		if (pd.revents & POLLIN) {
			modem_pipe_notify_receive_ready(&backend->pipe);
		}

		k_sleep(K_MSEC(MODEM_BACKEND_TTY_THREAD_POLL_DELAY));
	}
}

static int modem_backend_tty_open(void *data)
{
	struct modem_backend_tty *backend = (struct modem_backend_tty *)data;

	if (atomic_test_and_set_bit(&backend->state, MODEM_BACKEND_TTY_STATE_RUN_BIT)) {
		return -EALREADY;
	}

	backend->tty_fd = open(backend->tty_path, (O_RDWR | O_NONBLOCK), 0644);
	if (backend->tty_fd < 0) {
		return -EPERM;
	}

	k_thread_create(&backend->thread, backend->stack, backend->stack_size,
			modem_backend_tty_routine, backend, NULL, NULL,
			MODEM_BACKEND_TTY_THREAD_PRIO, 0, K_NO_WAIT);

	modem_pipe_notify_opened(&backend->pipe);
	return 0;
}

static int modem_backend_tty_transmit(void *data, const uint8_t *buf, size_t size)
{
	struct modem_backend_tty *backend = (struct modem_backend_tty *)data;
	int ret;

	ret = write(backend->tty_fd, buf, size);
	modem_pipe_notify_transmit_idle(&backend->pipe);
	return ret;
}

static int modem_backend_tty_receive(void *data, uint8_t *buf, size_t size)
{
	int ret;
	struct modem_backend_tty *backend = (struct modem_backend_tty *)data;

	ret = read(backend->tty_fd, buf, size);
	return (ret < 0) ? 0 : ret;
}

static int modem_backend_tty_close(void *data)
{
	struct modem_backend_tty *backend = (struct modem_backend_tty *)data;

	if (!atomic_test_and_clear_bit(&backend->state, MODEM_BACKEND_TTY_STATE_RUN_BIT)) {
		return -EALREADY;
	}

	k_thread_join(&backend->thread, K_MSEC(MODEM_BACKEND_TTY_THREAD_RUN_PERIOD_MS * 2));
	close(backend->tty_fd);
	modem_pipe_notify_closed(&backend->pipe);
	return 0;
}

struct modem_pipe_api modem_backend_tty_api = {
	.open = modem_backend_tty_open,
	.transmit = modem_backend_tty_transmit,
	.receive = modem_backend_tty_receive,
	.close = modem_backend_tty_close,
};

struct modem_pipe *modem_backend_tty_init(struct modem_backend_tty *backend,
					  const struct modem_backend_tty_config *config)
{
	__ASSERT_NO_MSG(backend != NULL);
	__ASSERT_NO_MSG(config != NULL);
	__ASSERT_NO_MSG(config->tty_path != NULL);

	memset(backend, 0x00, sizeof(*backend));
	backend->tty_path = config->tty_path;
	backend->stack = config->stack;
	backend->stack_size = config->stack_size;
	atomic_set(&backend->state, 0);
	modem_pipe_init(&backend->pipe, backend, &modem_backend_tty_api);
	return &backend->pipe;
}
