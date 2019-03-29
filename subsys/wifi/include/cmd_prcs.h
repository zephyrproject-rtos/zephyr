/*
 * @file
 * @brief Command processor header
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _WIFIMGR_CMD_PRCS_H_
#define _WIFIMGR_CMD_PRCS_H_

#include "ctrl_iface.h"

#define WIFIMGR_CMD_PROCESSOR		"wifimgr_cmd_processor"
#define WIFIMGR_CMD_PROCESSOR_PRIORITY	(1)
#define WIFIMGR_CMD_PROCESSOR_STACKSIZE	(4096)

#define WIFIMGR_CMD_MQUEUE	"wifimgr_cmd_mq"
#define WIFIMGR_CMD_MQUEUE_NR	WIFIMGR_CMD_MAX
#define WIFIMGR_CMD_SENDER_NR	WIFIMGR_CMD_MAX

#if CONFIG_MSG_COUNT_MAX < WIFIMGR_CMD_MAX
#error "Please increase CONFIG_MSG_COUNT_MAX!"
#endif

#define WIFIMGR_CMD_TIMEOUT	5

/* Function pointer prototype for commands */
typedef int (*cmd_func_t) (void *arg);
typedef void (*cmd_cb_t) (void *cb_arg, void *arg);

struct cmd_sender {
#define WIFIMGR_CMD_TYPE_ERROR		0
#define WIFIMGR_CMD_TYPE_SET		1
#define WIFIMGR_CMD_TYPE_GET		2
#define WIFIMGR_CMD_TYPE_EXCHANGE	3
	char type;
	cmd_func_t fn;
	void *arg;
};

struct cmd_processor {
	sem_t exclsem;		/* exclusive access to the struct */
	mqd_t mq;

	bool is_started:1;
	pthread_t pid;

	struct cmd_sender cmd_pool[WIFIMGR_CMD_SENDER_NR];
};

/* Structure defining the messages passed to a processor thread */
struct cmd_message {
	wifimgr_snode_t cmd_node;
	unsigned int cmd_id;	/* Command ID */
	int reply;		/* Command reply */
	int buf_len;		/* Command message length in bytes */
	void *buf;		/* Command message pointer */
};

int cmd_processor_add_sender(struct cmd_processor *handle, unsigned int cmd_id,
			     char type, cmd_func_t fn, void *arg);
int cmd_processor_remove_sender(struct cmd_processor *handle,
				unsigned int cmd_id);
int wifimgr_cmd_processor_init(struct cmd_processor *handle);
void wifimgr_cmd_processor_exit(struct cmd_processor *handle);

#endif
