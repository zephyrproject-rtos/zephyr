/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_remote_common.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/ipc/ipc_service_backend.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include "shell_utils.h"
#include <stdlib.h>
LOG_MODULE_REGISTER(shremote_cli);

#define TIMEOUT_MS 1000

struct shell_remote_cli {
	struct ipc_ept ept;
	atomic_t msg_len;
#ifdef CONFIG_MULTITHREADING
#ifdef CONFIG_SHELL_REMOTE_CLI_KWORK
	struct k_work work;
#else
	struct k_thread thread;
	struct k_sem cmd_sem;
#endif
	struct k_sem sem;
#else
	volatile bool processed;
#endif
	struct shell_static_entry loc;
	uint8_t buffer[CONFIG_SHELL_REMOTE_CLI_BUF_SIZE] __aligned(8);
};

static struct shell_remote_cli shell_remote;
static void ep_recv(const void *data, size_t len, void *priv);

static const struct ipc_ept_cfg ep_cfg = {
	.name = CONFIG_SHELL_REMOTE_EP_NAME,
	.cb = {
		.bound = NULL,
		.received = ep_recv,
	},
	.priv = (void *)&shell_remote
};

static void cmd_result(struct ipc_ept *ept, int result)
{
	struct shell_remote_msg_result rsp = {.id = SHELL_REMOTE_MSG_RESULT, .result = result};
	int err;

	err = ipc_service_send(ept, (const void *)&rsp, sizeof(rsp));
	if (err < 0) {
		LOG_ERR("Failed to send result");
	}
}

void shell_remote_cli_cbpprintf(const struct shell *sh, enum shell_vt100_color color, void *pkg,
				size_t len)
{
	struct shell_remote_cli *sh_remote = (struct shell_remote_cli *)sh;
	static const uint32_t cpy_flags = CBPRINTF_PACKAGE_CONVERT_RW_STR |
					  (IS_ENABLED(CONFIG_SHELL_REMOTE_CLI_SKIP_RO_STRINGS)
						   ? 0
						   : CBPRINTF_PACKAGE_CONVERT_RO_STR);
	struct shell_remote_msg_print *msg = (struct shell_remote_msg_print *)sh_remote->buffer;
	size_t pkg_len;
	void *fsc_pkg;
	uint32_t total_len;
	uint16_t strl[4];
	int err;

	/* Dry run copy to calculate the total package length. */
	pkg_len = cbprintf_package_copy(pkg, len, NULL, 8, cpy_flags, strl, ARRAY_SIZE(strl));
	total_len = pkg_len + offsetof(struct shell_remote_msg_print, data) + 8;

	if (sh_remote->msg_len != 0) {
		cmd_result(&sh_remote->ept, -EBUSY);
		return;
	}

	msg->id = SHELL_REMOTE_MSG_PRINT;
	msg->color = color;
	fsc_pkg = &msg->data[(8 - ((uintptr_t)&msg->data & 0x7)) & 0x7];
	if (total_len > CONFIG_SHELL_REMOTE_CLI_BUF_SIZE) {
		int len;

		if (IS_ENABLED(CONFIG_SHELL_REMOTE_CLI_SKIP_RO_STRINGS)) {
			/* RO strings are accessible by the host core so static package is used. */
			pkg_len = CONFIG_SHELL_REMOTE_CLI_BUF_SIZE - 8;
			CBPRINTF_STATIC_PACKAGE(fsc_pkg, pkg_len, len, SHELL_REMOTE_CLI_ALIGN,
						0, "Buffer size (%d) too small (%d needed)",
						CONFIG_SHELL_REMOTE_CLI_BUF_SIZE, total_len);
		} else {
			/* Header + 3 addresses + index */
			uint8_t pkg_buf[5 * sizeof(void *)] __aligned(SHELL_REMOTE_CLI_ALIGN);
			uint32_t flags = CBPRINTF_PACKAGE_ADD_STRING_IDXS;

			/* If RO strings need to be added to the package then we need to
			 * do it in two steps: static package and copy to the FSC package.
			 */
			CBPRINTF_STATIC_PACKAGE(pkg_buf, sizeof(pkg_buf), len,
						SHELL_REMOTE_CLI_ALIGN, flags,
						"Buffer size (%d) too small (%d needed)",
						CONFIG_SHELL_REMOTE_CLI_BUF_SIZE, total_len);
			__ASSERT_NO_MSG(len > 0);
			len = cbprintf_package_copy(pkg_buf, len, fsc_pkg, pkg_len, cpy_flags, NULL,
						    0);
			__ASSERT(len >= 0, "Failed to create cprintf package");
		}
		total_len = len + offsetof(struct shell_remote_msg_print, data) + 8;
	} else {
		err = cbprintf_package_copy(pkg, len, fsc_pkg, pkg_len, cpy_flags, strl,
					    ARRAY_SIZE(strl));

		__ASSERT(err >= 0, "Failed to create cprintf package");
	}

#ifndef CONFIG_MULTITHREADING
	sh_remote->processed = false;
#endif
	err = ipc_service_send(&sh_remote->ept, (void *)msg, total_len);
	if (err < 0) {
		LOG_ERR("Failed to send print message");
		return;
	}

	/* Wait for acknowledgment from the remote shell host to indicate that the
	 * message has been processed.
	 */
#ifdef CONFIG_MULTITHREADING
	err = k_sem_take(&sh_remote->sem, K_MSEC(CONFIG_SHELL_REMOTE_TIMEOUT_MS));
#else
	uint64_t start = k_uptime_ticks();
	uint64_t ticks = k_ms_to_ticks_ceil64(CONFIG_SHELL_REMOTE_TIMEOUT_MS);

	while (sh_remote->processed == false) {
		if (k_uptime_ticks() - start > ticks) {
			err = -ETIMEDOUT;
			break;
		}
	}
#endif
	if (err < 0) {
		LOG_ERR("Failed to take semaphore %d", err);
	}
}

static void cmd_get(struct shell_remote_cli *sh_remote, const struct shell_remote_msg_cmd_get *msg,
		    size_t len)
{
	/* A timed-out host may send a new request while this reply is pending. */
	static uint8_t reply_buf[CONFIG_SHELL_REMOTE_CLI_BUF_SIZE] __aligned(8);
	struct shell_remote_msg_cmd *rsp = (void *)reply_buf;
	const struct ipc_service_backend *backend = sh_remote->ept.instance->api;
	const struct shell_static_entry *entry;
	const char *help;
	const char *usage = NULL;
	size_t reply_size = sizeof(reply_buf);
	size_t syntax_len;
	size_t min_msg_len;
	size_t help_size;
	size_t msg_len;
	int err;
	char *strings;

	entry = z_shell_cmd_get(msg->parent, msg->idx, &sh_remote->loc);

	if (entry == NULL) {
		LOG_DBG("Command not found parent:%s, idx:%d",
			msg->parent ? msg->parent->syntax : "NULL", msg->idx);
		cmd_result(&sh_remote->ept, -ENODEV);
		/* Failed to get the command. */
		return;
	} else if (strlen(entry->syntax) == 0) {
		LOG_DBG("Empty syntax, parent:%s, idx:%d",
			msg->parent ? msg->parent->syntax : "NULL", msg->idx);
		cmd_result(&sh_remote->ept, -ENOEXEC);
		/* Command is empty. */
		return;
	}

	if (backend->get_tx_buffer_size != NULL) {
		err = ipc_service_get_tx_buffer_size(&sh_remote->ept);
		if (err >= 0) {
			reply_size = MIN(reply_size, (size_t)err);
		}
	}
	syntax_len = strlen(entry->syntax);
	min_msg_len = offsetof(struct shell_remote_msg_cmd, data) + syntax_len + 2U;
	if (min_msg_len > reply_size) {
		cmd_result(&sh_remote->ept, -ENOEXEC);
		return;
	}

	help = entry->help;
	if (shell_help_is_structured(help)) {
		const struct shell_cmd_help *structured = (const struct shell_cmd_help *)help;

		help = structured->description;
		usage = structured->usage;
	}

	rsp->id = SHELL_REMOTE_MSG_CMD;
	rsp->subcmd = entry->subcmd;
	rsp->entry = entry;
	rsp->handler = entry->handler;
	rsp->args.mandatory = entry->args.mandatory;
	rsp->args.optional = entry->args.optional;
	strings = rsp->data;
	memcpy(strings, entry->syntax, syntax_len);
	strings += syntax_len;
	*strings++ = '\0';
	help_size = reply_size - min_msg_len + 1U;
	if (usage != NULL) {
		err = snprintk(strings, help_size, "%s%sUsage: %s %s", help != NULL ? help : "",
			       help != NULL && help[0] != '\0' ? "\n" : "", entry->syntax, usage);
	} else {
		err = snprintk(strings, help_size, "%s", help != NULL ? help : "");
	}
	if (err < 0) {
		cmd_result(&sh_remote->ept, -ENOEXEC);
		return;
	}
	msg_len = min_msg_len + MIN((size_t)err, help_size - 1U);
	LOG_DBG("Command get syntax:%s len:%zu", entry->syntax, msg_len);

	while (true) {
		err = ipc_service_send(&sh_remote->ept, rsp, msg_len);
		/* Sized backends may block for a TX buffer before returning -ENOMEM. */
		if (err != -EMSGSIZE && (err != -ENOMEM || backend->get_tx_buffer_size != NULL)) {
			break;
		}
		if (msg_len == min_msg_len) {
			cmd_result(&sh_remote->ept, -ENOEXEC);
			return;
		}
		/* Some backends cannot report their capacity. Retry with less help. */
		msg_len = min_msg_len + (msg_len - min_msg_len) / 2U;
		reply_buf[msg_len - 1U] = '\0';
	}
	if (err < 0 || (err > 0 && (size_t)err != msg_len)) {
		LOG_ERR("Failed to send complete command: %d/%zu", err, msg_len);
	}
}

/* Recursive search for the handler. It is using command buffer strings to validate that handler
 * is matching the syntax. Implementation is recursive but it attempts to use minimal stack space.
 */
static int handler_find_in_subtree(const struct shell_static_entry *parent,
				   shell_cmd_handler handler,
				   struct shell_static_entry *dloc,
				   struct shell_static_entry *dloc2,
				   const char *cmd_buf, size_t cmd_len, size_t argc)
{
	const struct shell_static_entry *entry;
	size_t idx = 0;

	while ((entry = z_shell_cmd_get(parent, idx++, dloc)) != NULL) {
		if (entry->handler == handler) {
			return 0;
		}

		if (argc > 1) {
			size_t i = 0;

			/* Compare command syntax with command buffer content. */
			while (entry->syntax[i] != '\0' && i < cmd_len &&
			       entry->syntax[i] == cmd_buf[i]) {
				i++;
			}

			if (cmd_buf[i] == '\0' && entry->syntax[i] == '\0') {
				/* Syntax matched what is in the command buffer. */
				i++;
				return handler_find_in_subtree(entry, handler, dloc2, dloc,
					&cmd_buf[i], cmd_len - i, argc - 1);
			}
		}
	}

	return -ENOENT;
}

/* Function checks if received handler is a valid shell command handler. */
static int handler_is_shell_cmd(shell_cmd_handler handler, const char *data, size_t len,
				size_t argc)
{
	struct shell_static_entry dloc, dloc2;

	return handler_find_in_subtree(NULL, handler, &dloc, &dloc2, data, len, argc);
}

static void cmd_exec(struct shell_remote_cli *sh_remote, struct shell_remote_msg_exec *msg,
		     size_t len)
{
	if (msg->argc > CONFIG_SHELL_ARGC_MAX || msg->argc == 0 || msg->argc < msg->cmd_lvl ||
	    msg->handler == NULL) {
		cmd_result(&sh_remote->ept, -EINVAL);
		return;
	}

	char *argv[msg->argc];
	char *data = msg->data;
	uint32_t cnt = 0;
	uint8_t argc = msg->argc - msg->cmd_lvl;
	char **cmd_argv = &argv[msg->cmd_lvl];
	size_t slen;
	int err;
	uint16_t args_len = (uint16_t)(len - offsetof(struct shell_remote_msg_exec, data));
	char *args_buf = __builtin_alloca_with_align(args_len, 8);

	LOG_DBG("Command execute request: argc:%d, cmd_lvl:%d, handler:%p, data:%s", msg->argc,
		msg->cmd_lvl, msg->handler, data);

	/* Copy arguments to the stack buffer as shell instance buffer may be used
	 * for shell printing.
	 */
	memcpy(args_buf, data, args_len);
	do {
		argv[cnt] = args_buf;
		slen = strlen(args_buf) + 1;
		args_buf += slen;
		cnt++;
		len -= slen;
	} while ((cnt < msg->argc) && (len > 0));

	err = handler_is_shell_cmd(msg->handler, data, args_len, msg->argc);
	if (err < 0) {
		cmd_result(&sh_remote->ept, err);
		return;
	}

	err = msg->handler((const struct shell *)sh_remote, argc, cmd_argv);

	cmd_result(&sh_remote->ept, err);
}

/* Executing command from thread context */
static void cmd_from_thread(struct shell_remote_cli *sh_remote)
{
	union shell_remote_msg msg;
	uint32_t msg_len = (uint32_t)sh_remote->msg_len;

	if (msg_len == 0) {
		return;
	}

	msg.generic = (struct shell_remote_msg_generic *)sh_remote->buffer;
	/* Allow another request to be queued while this command is executing. */
	sh_remote->msg_len = 0;

	switch (msg.generic->id) {
	case SHELL_REMOTE_MSG_CMD_GET:
		cmd_get(sh_remote, msg.cmd_get, msg_len);
		break;
	case SHELL_REMOTE_MSG_EXEC:
		cmd_exec(sh_remote, msg.exec, msg_len);
		break;
	default:
		__ASSERT(0, "Unexpected message:%d", msg.generic->id);
	}
}

#ifdef CONFIG_MULTITHREADING
static void trigger_thread_context(struct shell_remote_cli *sh_remote)
{
#ifdef CONFIG_SHELL_REMOTE_CLI_KWORK
	k_work_submit(&sh_remote->work);
#else
	k_sem_give(&sh_remote->cmd_sem);
#endif
}

#ifdef CONFIG_SHELL_REMOTE_CLI_KWORK
static void cmd_process(struct k_work *work)
{
	struct shell_remote_cli *sh_remote = CONTAINER_OF(work, struct shell_remote_cli, work);

	cmd_from_thread(sh_remote);
}

#else

K_KERNEL_STACK_DEFINE(stack, CONFIG_SHELL_REMOTE_CLI_STACK_SIZE);
static void thread_func(void *arg0, void *arg1, void *arg2)
{
	struct shell_remote_cli *sh_remote = arg0;

	while (true) {
		if (k_sem_take(&sh_remote->cmd_sem, K_FOREVER) == 0) {
			cmd_from_thread(sh_remote);
		}
	}
}

#endif /* CONFIG_SHELL_REMOTE_CLI_KWORK */

static void thread_context_init(struct shell_remote_cli *sh_remote)
{
	k_sem_init(&sh_remote->sem, 0, 1);
#ifdef CONFIG_SHELL_REMOTE_CLI_KWORK
	k_work_init(&sh_remote->work, cmd_process);
#else
	k_sem_init(&sh_remote->cmd_sem, 0, 1);
	k_thread_create(&sh_remote->thread, stack, K_KERNEL_STACK_SIZEOF(stack), thread_func,
			sh_remote, NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);
	k_thread_name_set(&sh_remote->thread, "shell_remote_cli");
#endif
}
#else
void shell_remote_cmd_process(void)
{
	if (!shell_remote.msg_len) {
		return;
	}

	cmd_from_thread(&shell_remote);
}
#endif /* CONFIG_MULTITHREADING */

static void msg_to_thread(struct shell_remote_cli *sh_remote, const void *buf, size_t len)
{
	if (len > CONFIG_SHELL_REMOTE_CLI_BUF_SIZE) {
		LOG_ERR("Too big exec command to hold in the temporary buffer");
		return;
	}

	if (!atomic_cas(&sh_remote->msg_len, 0, len)) {
		LOG_ERR("Previous message not yet processed.");
		return;
	}

	memcpy(sh_remote->buffer, buf, len);
#ifdef CONFIG_MULTITHREADING
	trigger_thread_context(sh_remote);
#endif
}

static void ep_recv(const void *data, size_t len, void *priv)
{
	struct shell_remote_cli *sh_remote = priv;

	/* Shell print operations are acknowledged by sending a result message. */
	if (((struct shell_remote_msg_generic *)data)->id == SHELL_REMOTE_MSG_RESULT) {
#ifdef CONFIG_MULTITHREADING
		k_sem_give(&sh_remote->sem);
#else
		sh_remote->processed = true;
#endif
		return;
	}

	/* Other messages are stored and processed in thread context. */
	msg_to_thread(sh_remote, data, len);
}

static int shell_remote_cli_init(void)
{
	int ret;
	const struct device *ipc_instance = DEVICE_DT_GET(DT_NODELABEL(ipc0));

	ret = ipc_service_open_instance(ipc_instance);
	if ((ret < 0) && (ret != -EALREADY)) {
		LOG_ERR("ipc_service_open_instance() failure");
		return ret;
	}

	ret = ipc_service_register_endpoint(ipc_instance, &shell_remote.ept, &ep_cfg);
	if (ret < 0) {
		LOG_ERR("ipc_service_register_endpoint() failure");
		return ret;
	}

#ifdef CONFIG_MULTITHREADING
	thread_context_init(&shell_remote);
#endif

	return 0;
}

SYS_INIT(shell_remote_cli_init, APPLICATION, 0);
