/*
 * @file
 * @brief Command processor related functions
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_WIFIMGR_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_DECLARE(wifimgr);

#include "wifimgr.h"

K_THREAD_STACK_ARRAY_DEFINE(cmd_stacks, 1, WIFIMGR_CMD_PROCESSOR_STACKSIZE);

int cmd_processor_add_sender(struct cmd_processor *handle, unsigned int cmd_id,
			     char type, cmd_func_t fn, void *arg)
{
	struct cmd_processor *prcs = (struct cmd_processor *)handle;
	struct cmd_sender *sndr;

	if (!prcs || !type || !fn)
		return -EINVAL;

	if (cmd_id < WIFIMGR_CMD_MAX)
		sndr = &prcs->cmd_pool[cmd_id];
	else
		return -EINVAL;

	sndr->type = type;
	sndr->fn = fn;

	if (arg)
		sndr->arg = arg;

	return 0;
}

int cmd_processor_remove_sender(struct cmd_processor *handle,
				unsigned int cmd_id)
{
	struct cmd_processor *prcs = (struct cmd_processor *)handle;
	struct cmd_sender *sndr;

	if (!prcs)
		return -EINVAL;

	if (cmd_id < WIFIMGR_CMD_MAX)
		sndr = &prcs->cmd_pool[cmd_id];
	else
		return -EINVAL;

	sndr->type = 0;
	sndr->fn = NULL;
	sndr->arg = NULL;

	return 0;
}

static void cmd_processor_post_process(void *handle,
				       struct cmd_message *msg, int reply)
{
	struct cmd_processor *prcs = (struct cmd_processor *)handle;
	int ret;

	/* Reply commands */
	msg->reply = reply;
	ret =
	    mq_send(prcs->mq, (const char *)msg, sizeof(struct cmd_message), 0);
	if (ret == -1) {
		wifimgr_err("failed to send [%s] reply! errno %d\n",
			    wifimgr_cmd2str(msg->cmd_id), errno);
	} else {
		wifimgr_dbg("send [%s] reply: %d\n",
			    wifimgr_cmd2str(msg->cmd_id), msg->reply);
	}
}

static void *cmd_processor(void *handle)
{
	struct cmd_processor *prcs = (struct cmd_processor *)handle;
	struct cmd_sender *sndr;
	struct wifi_manager *mgr =
	    container_of(prcs, struct wifi_manager, prcs);
	struct cmd_message msg;
	int prio;
	int ret;

	wifimgr_dbg("starting %s, pid=%p\n", __func__, pthread_self());

	if (!prcs) {
		pthread_exit(handle);
		return NULL;
	}

	while (prcs->is_started) {
		/* Wait for commands */
		ret = mq_receive(prcs->mq, (char *)&msg, sizeof(msg), &prio);
		if (ret == -1) {
			wifimgr_err("failed to get command! ret %d, errno %d\n",
				    ret, errno);
			continue;
		} else if (msg.reply) {
			/* Drop command reply when receiving it */
			wifimgr_err("recv [%s] reply: %d? drop it!\n",
				    wifimgr_cmd2str(msg.cmd_id), msg.reply);
			continue;
		}

		wifimgr_dbg("recv [%s], buf: 0x%08x\n",
			    wifimgr_cmd2str(msg.cmd_id), *(int *)msg.buf);

		/* Ask state machine whether the command could be executed */
		ret = wifimgr_sm_query_cmd(mgr, msg.cmd_id);
		if (ret) {
			cmd_processor_post_process(prcs, &msg, ret);

			if (ret == -EBUSY)
				wifimgr_err("Busy(%s)! try again later\n",
					    wifimgr_sts2str_cmd(mgr,
								msg.cmd_id));
			continue;
		}

		sem_wait(&prcs->exclsem);
		sndr = &prcs->cmd_pool[msg.cmd_id];
		if (sndr->fn) {
			/* Set params through message buffer */
			if (sndr->arg && (sndr->type != WIFIMGR_CMD_TYPE_GET)) {
				wifimgr_hexdump(msg.buf, msg.buf_len);
				memcpy(sndr->arg, msg.buf, msg.buf_len);
			}
			/* Call command function */
			ret = sndr->fn(sndr->arg);
			/* Trigger state machine */
			wifimgr_sm_cmd_step(mgr, msg.cmd_id, ret);
			/* Get results through message buffer */
			if (msg.buf && (sndr->type == WIFIMGR_CMD_TYPE_GET)) {
				memcpy(msg.buf, sndr->arg, msg.buf_len);
				wifimgr_hexdump(msg.buf, msg.buf_len);
			}
		} else {
			wifimgr_err("[%s] not allowed under %s!\n",
				    wifimgr_cmd2str(msg.cmd_id),
				    wifimgr_sts2str_cmd(mgr, msg.cmd_id));
			ret = -EPERM;
		}

		sem_post(&prcs->exclsem);
		cmd_processor_post_process(prcs, &msg, ret);
	}

	pthread_exit(handle);
	return NULL;
}

int wifimgr_cmd_processor_init(struct cmd_processor *handle)
{
	struct cmd_processor *prcs = (struct cmd_processor *)handle;
	struct mq_attr attr;
	pthread_attr_t tattr;
	struct sched_param sparam;
	int ret;

	if (!prcs)
		return -EINVAL;

	/* Fill in attributes for message queue */
	attr.mq_maxmsg = WIFIMGR_CMD_MQUEUE_NR;
	attr.mq_msgsize = sizeof(struct cmd_message);
	attr.mq_flags = 0;

	/* Open message queue of command sender */
	prcs->mq = mq_open(WIFIMGR_CMD_MQUEUE, O_RDWR | O_CREAT, 0666, &attr);
	if (prcs->mq == (mqd_t)-1) {
		wifimgr_err("failed to open command queue %s! errno: %d\n",
			    WIFIMGR_CMD_MQUEUE, errno);
		return -errno;
	}

	sem_init(&prcs->exclsem, 0, 1);
	prcs->is_started = true;

	/* Starts internal threads to process commands */
	pthread_attr_init(&tattr);
	sparam.sched_priority = WIFIMGR_CMD_PROCESSOR_PRIORITY;
	pthread_attr_setschedparam(&tattr, &sparam);
	pthread_attr_setstack(&tattr, &cmd_stacks[0][0],
			      WIFIMGR_CMD_PROCESSOR_STACKSIZE);
	pthread_attr_setschedpolicy(&tattr, SCHED_FIFO);

	ret = pthread_create(&prcs->pid, &tattr, cmd_processor, prcs);
	if (ret) {
		wifimgr_err("failed to start %s!\n", WIFIMGR_CMD_PROCESSOR);
		mq_close(prcs->mq);
		return ret;
	}
	wifimgr_dbg("started %s, pid=%p\n", WIFIMGR_CMD_PROCESSOR, prcs->pid);

	return 0;
}

void wifimgr_cmd_processor_exit(struct cmd_processor *handle)
{
	struct cmd_processor *prcs = (struct cmd_processor *)handle;

	/* Close message queue */
	if (prcs->mq && (prcs->mq != (mqd_t)-1)) {
		mq_close(prcs->mq);
		mq_unlink(WIFIMGR_CMD_MQUEUE);
	}

	sem_destroy(&prcs->exclsem);
	prcs->is_started = false;
}
