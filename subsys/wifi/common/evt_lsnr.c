/*
 * @file
 * @brief Event listener related functions
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

K_THREAD_STACK_ARRAY_DEFINE(evt_stacks, 1, WIFIMGR_EVT_LISTENER_STACKSIZE);

int wifimgr_notify_event(unsigned int evt_id, void *buf, int buf_len)
{
	struct mq_des *mq;
	struct mq_attr attr;
	struct evt_message msg;
	int ret;

	attr.mq_maxmsg = 16;
	attr.mq_msgsize = sizeof(msg);
	attr.mq_flags = 0;
	mq = mq_open(WIFIMGR_EVT_MQUEUE, O_WRONLY | O_CREAT, 0666, &attr);
	if (mq == (mqd_t)-1) {
		wifimgr_err("failed to open event queue %s! errno %d\n",
			    WIFIMGR_EVT_MQUEUE, errno);
		return -errno;
	}

	msg.evt_id = evt_id;
	msg.buf_len = buf_len;
	msg.buf = NULL;
	if (buf_len) {
		msg.buf = malloc(buf_len);
		if (!msg.buf)
			return -ENOMEM;
		memcpy(msg.buf, buf, buf_len);
	}

	/* Notify events */
	ret = mq_send(mq, (const char *)&msg, sizeof(msg), 0);
	if (ret == -1) {
		free(msg.buf);
		wifimgr_err("failed to send [%s]! errno %d\n",
			    wifimgr_evt2str(msg.evt_id), errno);
	} else {
		wifimgr_dbg("send [%s], buf: 0x%08x\n",
			    wifimgr_evt2str(msg.evt_id), *(int *)msg.buf);
	}

	mq_close(mq);

	return ret;
}

static struct evt_receiver *search_event(struct evt_listener *lsnr,
					 unsigned int evt_id)
{
	struct evt_receiver *rcvr;

	/* Loop through list to find the corresponding event */
	wifimgr_list_for_each_entry(rcvr, &lsnr->evt_list, struct evt_receiver,
				    node) {
		if (rcvr->expected_evt == evt_id)
			return rcvr;
	}

	return NULL;
}

static void free_event(struct evt_listener *lsnr, struct evt_receiver *rcvr)
{
	/* Unlink the receiver from the list */
	wifimgr_list_remove(&lsnr->evt_list, &rcvr->node);

	/* Link the receiver back into the free list */
	wifimgr_list_append(&lsnr->free_evt_list, &rcvr->node);
}

int evt_listener_add_receiver(struct evt_listener *handle, unsigned int evt_id,
			      bool oneshot, evt_cb_t cb, void *arg)
{
	struct evt_listener *lsnr = (struct evt_listener *)handle;
	struct evt_receiver *rcvr;
	int ret;

	if (!lsnr || !cb)
		return -EINVAL;

	sem_wait(&lsnr->exclsem);

	/* Check whether the event receiver already exist */
	rcvr = search_event(lsnr, evt_id);
	if (rcvr) {
		wifimgr_warn("[%s] receiver already exist!\n",
			     wifimgr_evt2str(evt_id));
		sem_post(&lsnr->exclsem);
		return 0;
	}

	/* Allocate a receiver struct from the free pool */
	rcvr = (struct evt_receiver *)
	    wifimgr_list_remove_first(&lsnr->free_evt_list);
	if (!rcvr) {
		ret = -ENOMEM;
		wifimgr_err("no free event receiver! %d\n", ret);
		sem_post(&lsnr->exclsem);
		return ret;
	}

	rcvr->expected_evt = evt_id;
	rcvr->oneshot = oneshot;
	rcvr->cb = cb;
	rcvr->arg = arg;

	/* Link the evt_listener into the list */
	wifimgr_list_append(&lsnr->evt_list, &rcvr->node);
	sem_post(&lsnr->exclsem);

	return 0;
}

int evt_listener_remove_receiver(struct evt_listener *handle,
				 unsigned int evt_id)
{
	struct evt_listener *lsnr = (struct evt_listener *)handle;
	struct evt_receiver *rcvr;

	if (!lsnr)
		return -EINVAL;

	/* Get exclusive access to the struct */
	sem_wait(&lsnr->exclsem);

	rcvr = search_event(lsnr, evt_id);
	if (!rcvr) {
		wifimgr_warn("no [%s] receiver to remove!\n",
			     wifimgr_evt2str(evt_id));
		sem_post(&lsnr->exclsem);
		return -ENOENT;
	}

	free_event(lsnr, rcvr);
	sem_post(&lsnr->exclsem);
	memset(rcvr, 0, sizeof(struct evt_receiver));

	return 0;
}

static void *evt_listener(void *handle)
{
	struct evt_listener *lsnr = (struct evt_listener *)handle;
	struct evt_receiver *rcvr;
	struct wifi_manager *mgr =
	    container_of(lsnr, struct wifi_manager, lsnr);
	struct evt_message msg;
	int prio;
	int ret;

	wifimgr_dbg("starting %s, pid=%p\n", __func__, pthread_self());

	if (!lsnr) {
		pthread_exit(handle);
		return NULL;
	}

	while (lsnr->is_started) {
		bool match = false;

		/* Wait for events */
		ret = mq_receive(lsnr->mq, (char *)&msg, sizeof(msg), &prio);
		if (ret == -1) {
			wifimgr_err("failed to receive event! errno %d\n",
				    errno);
			continue;
		}

		wifimgr_dbg("recv [%s], buf: 0x%08x\n",
			    wifimgr_evt2str(msg.evt_id), *(int *)msg.buf);

		sem_wait(&lsnr->exclsem);

		rcvr = search_event(lsnr, msg.evt_id);
		if (rcvr) {
			if (rcvr->oneshot)
				free_event(lsnr, rcvr);

			match = true;
			wifimgr_dbg("receiver 0x%p matches\n", rcvr);
		}

		sem_post(&lsnr->exclsem);

		if (match == true) {
			if (msg.buf_len) {
				wifimgr_hexdump(msg.buf, msg.buf_len);
				memcpy(rcvr->arg, msg.buf, msg.buf_len);
			}

			/* Call event callback */
			ret = rcvr->cb(rcvr->arg);
			/* Trigger state machine */
			wifimgr_sm_evt_step(mgr, msg.evt_id, ret);
		} else {
			wifimgr_warn("unexpected [%s] under %s!\n",
				     wifimgr_evt2str(msg.evt_id),
				     wifimgr_sts2str_evt(mgr, msg.evt_id));
		}

		free(msg.buf);
	}

	pthread_exit(handle);
	return NULL;
}

int wifimgr_evt_listener_init(struct evt_listener *handle)
{
	struct evt_listener *lsnr = (struct evt_listener *)handle;
	struct mq_attr attr;
	pthread_attr_t tattr;
	struct sched_param sparam;
	int ret;
	int i;

	if (!lsnr)
		return -EINVAL;

	/* Fill in attributes for message queue */
	attr.mq_maxmsg = 16;
	attr.mq_msgsize = sizeof(struct evt_message);
	attr.mq_flags = 0;

	/* Open message queue of event receiver */
	lsnr->mq = mq_open(WIFIMGR_EVT_MQUEUE, O_RDWR | O_CREAT, 0666, &attr);
	if (lsnr->mq == (mqd_t)-1) {
		wifimgr_err("failed to open event queue %s! errno %d\n",
			    WIFIMGR_EVT_MQUEUE, errno);
		return -errno;
	}

	/* Initialize the event receiver allocation pool */
	wifimgr_list_init(&lsnr->evt_list);
	wifimgr_list_init(&lsnr->free_evt_list);
	for (i = 0; i < WIFIMGR_EVT_RECEIVER_NR; i++)
		wifimgr_list_append(&lsnr->free_evt_list,
				     (wifimgr_snode_t *) &lsnr->evt_pool[i]);

	sem_init(&lsnr->exclsem, 0, 1);
	lsnr->is_started = true;

	/* Starts internal threads to listen for frames and events */
	pthread_attr_init(&tattr);
	sparam.sched_priority = WIFIMGR_EVT_LISTENER_PRIORITY;
	pthread_attr_setschedparam(&tattr, &sparam);
	pthread_attr_setstack(&tattr, &evt_stacks[0][0],
			      WIFIMGR_EVT_LISTENER_STACKSIZE);
	pthread_attr_setschedpolicy(&tattr, SCHED_FIFO);

	ret = pthread_create(&lsnr->evt_pid, &tattr, evt_listener, lsnr);
	if (ret) {
		wifimgr_err("failed to start %s!\n", WIFIMGR_EVT_LISTENER);
		mq_close(lsnr->mq);
		return ret;
	}
	wifimgr_dbg("started %s, pid=%p\n", WIFIMGR_EVT_LISTENER,
		    lsnr->evt_pid);

	return 0;
}

void wifimgr_evt_listener_exit(struct evt_listener *handle)
{
	struct evt_listener *lsnr = (struct evt_listener *)handle;

	/* Close message queue */
	if (lsnr->mq && (lsnr->mq != (mqd_t)-1)) {
		mq_close(lsnr->mq);
		mq_unlink(WIFIMGR_CMD_MQUEUE);
	}

	/* Deinitialize the event receiver allocation pool */
	wifimgr_list_free(&lsnr->evt_list);
	wifimgr_list_free(&lsnr->free_evt_list);

	sem_destroy(&lsnr->exclsem);
	lsnr->is_started = false;
}
