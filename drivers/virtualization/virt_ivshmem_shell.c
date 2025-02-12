/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include <zephyr/drivers/virtualization/ivshmem.h>

static const struct device *ivshmem = DEVICE_DT_GET_ONE(qemu_ivshmem);

#ifdef CONFIG_IVSHMEM_DOORBELL

#define STACK_SIZE 512
static struct k_poll_signal doorbell_sig =
	K_POLL_SIGNAL_INITIALIZER(doorbell_sig);
static struct k_poll_event doorbell_evt =
	K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
				 K_POLL_MODE_NOTIFY_ONLY,
				 &doorbell_sig);
K_THREAD_STACK_DEFINE(doorbell_stack, STACK_SIZE);
static bool doorbell_started;
static struct k_thread doorbell_thread;

static void doorbell_notification_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct shell *sh = p1;

	while (1) {
		unsigned int signaled;
		int vector;

		k_poll(&doorbell_evt, 1, K_FOREVER);

		k_poll_signal_check(&doorbell_sig, &signaled, &vector);
		if (signaled == 0) {
			continue;
		}

		shell_fprintf(sh, SHELL_NORMAL,
			      "Received a notification on vector %u\n",
			      (unsigned int)vector);

		k_poll_signal_init(&doorbell_sig);
	}
}

#endif /* CONFIG_IVSHMEM_DOORBELL */

static bool get_ivshmem(const struct shell *sh)
{
	if (!device_is_ready(ivshmem)) {
		shell_error(sh, "IVshmem device is not ready");
		return false;
	}

	return true;
}

static int cmd_ivshmem_shmem(const struct shell *sh,
			     size_t argc, char **argv)
{
	uintptr_t mem;
	size_t size;
	uint32_t id;
	uint16_t vectors;

	if (!get_ivshmem(sh)) {
		return 0;
	}

	size = ivshmem_get_mem(ivshmem, &mem);
	id = ivshmem_get_id(ivshmem);
	vectors = ivshmem_get_vectors(ivshmem);

	shell_fprintf(sh, SHELL_NORMAL,
		      "IVshmem up and running: \n"
		      "\tShared memory: 0x%lx of size %lu bytes\n"
		      "\tPeer id: %u\n"
		      "\tNotification vectors: %u\n",
		      mem, size, id, vectors);

	return 0;
}

static int cmd_ivshmem_dump(const struct shell *sh,
			    size_t argc, char **argv)
{
	uintptr_t dump_pos;
	size_t dump_size;
	uintptr_t mem;
	size_t size;

	if (!get_ivshmem(sh)) {
		return 0;
	}

	dump_pos = strtol(argv[1], NULL, 10);
	dump_size = strtol(argv[2], NULL, 10);

	size = ivshmem_get_mem(ivshmem, &mem);

	if (dump_size > size) {
		shell_error(sh, "Size is too big");
	} else if (dump_pos > size) {
		shell_error(sh, "Position is out of the shared memory");
	} else if ((mem + dump_pos + dump_size) > (mem + size)) {
		shell_error(sh, "Position and size overflow");
	} else {
		shell_hexdump(sh, (const uint8_t *)mem+dump_pos, dump_size);
	}

	return 0;
}

static int cmd_ivshmem_int(const struct shell *sh,
			   size_t argc, char **argv)
{
	int peer_id;
	int vector;
	int ret;

	if (!IS_ENABLED(CONFIG_IVSHMEM_DOORBELL)) {
		shell_error(sh, "CONFIG_IVSHMEM_DOORBELL is not enabled");
		return 0;
	}

	if (!get_ivshmem(sh)) {
		return 0;
	}

	peer_id = strtol(argv[1], NULL, 10);
	vector = strtol(argv[2], NULL, 10);

	ret = ivshmem_int_peer(ivshmem, (uint16_t)peer_id, (uint16_t)vector);
	if (ret != 0) {
		shell_error(sh,
			    "Could not notify peer %u on %u. status %d",
			    peer_id, vector, ret);
		return -EIO;
	}

	shell_fprintf(sh, SHELL_NORMAL,
		      "Notification sent to peer %u on vector %u\n",
		      peer_id, vector);

	return 0;
}

static int cmd_ivshmem_get_notified(const struct shell *sh,
				    size_t argc, char **argv)
{
#ifdef CONFIG_IVSHMEM_DOORBELL
	int vector;

	if (!get_ivshmem(sh)) {
		return 0;
	}

	vector = strtol(argv[1], NULL, 10);

	if (ivshmem_register_handler(ivshmem, &doorbell_sig,
				     (uint16_t)vector)) {
		shell_error(sh, "Could not get notifications on vector %u",
			    vector);
		return -EIO;
	}

	shell_fprintf(sh, SHELL_NORMAL,
		      "Notifications enabled for vector %u\n", vector);

	if (!doorbell_started) {
		k_tid_t tid;

		tid = k_thread_create(
			&doorbell_thread,
			doorbell_stack, STACK_SIZE,
			doorbell_notification_thread,
			(void *)sh, NULL, NULL,
			K_PRIO_COOP(2), 0, K_NO_WAIT);
		if (!tid) {
			shell_error(sh, "Cannot start notification thread");
			return -ENOEXEC;
		}

		k_thread_name_set(tid, "notification_thread");

		k_thread_start(tid);

		doorbell_started = true;
	}
#else
	shell_error(sh, "CONFIG_IVSHMEM_DOORBELL is not enabled");
#endif
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_ivshmem_cmds,
			       SHELL_CMD(shmem, NULL,
					 "Show shared memory info",
					 cmd_ivshmem_shmem),
			       SHELL_CMD_ARG(dump, NULL,
					     "Dump shared memory content",
					     cmd_ivshmem_dump, 3, 0),
			       SHELL_CMD_ARG(int_peer, NULL,
					     "Notify a vector on a peer",
					     cmd_ivshmem_int, 3, 0),
			       SHELL_CMD_ARG(get_notified, NULL,
					     "Get notification on vector",
					     cmd_ivshmem_get_notified, 2, 0),
			       SHELL_SUBCMD_SET_END
		);

SHELL_CMD_REGISTER(ivshmem, &sub_ivshmem_cmds,
		   "IVshmem information", cmd_ivshmem_shmem);
