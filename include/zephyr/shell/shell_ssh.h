/*
 * Copyright (c) 2024 Grant Ramsay <grant.ramsay@hotmail.com>
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SSH shell backend
 * @ingroup shell_api
 */

#ifndef ZEPHYR_INCLUDE_SHELL_SSH_H_
#define ZEPHYR_INCLUDE_SHELL_SSH_H_

#include <zephyr/net/socket.h>
#include <zephyr/net/ssh/server.h>
#include <zephyr/shell/shell.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum password length supported by the shell SSH transport. */
#define SHELL_SSH_MAX_PASSWORD_LEN 32

/** @cond INTERNAL_HIDDEN */

struct ssh_channel;
struct shell_ssh;

struct shell_ssh_passwd {
	char buf[SHELL_SSH_MAX_PASSWORD_LEN + 1];
	size_t len;
};

struct shell_ssh_userdata {
	const struct shell *sh;
	const struct shell *ssh;
	struct shell_ssh *shell_ssh;
	struct ssh_channel *channel;
	struct ssh_transport *transport;
	struct shell_ssh_passwd password;
};

/** @cond INTERNAL_HIDDEN */
extern const struct shell_transport_api shell_ssh_transport_api;
/** @endcond */

#define SHELL_SSH_GET_NAME(_name) _name##_shell_ssh

/** @endcond */

/**
 * @brief Define a shell SSH transport instance.
 *
 * This macro defines a shell transport backed by SSH. Use it together
 * with SHELL_DEFINE() to create a shell instance that is accessible
 * over SSH.
 *
 * @param _name  Name of the transport instance. This token is used to
 *               construct the internal symbol names.
 */
#define SHELL_SSH_DEFINE(_name)					\
	static struct shell_ssh SHELL_SSH_GET_NAME(_name);	\
	static struct shell_transport _name = {			\
		.api = &shell_ssh_transport_api,		\
		.ctx = &_name##_shell_ssh,			\
	}

/** @cond INTERNAL_HIDDEN */

struct shell_ssh {
	shell_transport_handler_t handler;
	struct ssh_channel *ssh_channel;
	void *context;
};


int shell_ssh_setup(const struct shell *sh, struct ssh_channel *channel);

/** @cond INTERNAL_HIDDEN */

#if defined(CONFIG_SSH_SERVER)
int shell_sshd_event_callback(struct ssh_server *ssh_server,
			      const struct ssh_server_event *event,
			      void *user_data);
int shell_sshd_transport_event_callback(struct ssh_transport *transport,
					const struct ssh_transport_event *event,
					void *user_data);
#endif /* CONFIG_SSH_SERVER */

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SHELL_SSH_H_ */
