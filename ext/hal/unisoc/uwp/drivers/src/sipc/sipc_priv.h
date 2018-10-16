/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __HAL_SIPC_PRIV_H
#define __HAL_SIPC_PRIV_H

#ifdef __cplusplus
extern "C" {
#endif

#define SMSG_CACHE_NR		32
#define CONFIG_SMSG_THREAD_DEFPRIO 50
#define CONFIG_SMSG_THREAD_STACKSIZE 1024

	struct smsg_channel {
		/* wait queue for recv-buffer */
		struct k_sem	    rxsem;
		struct k_mutex		rxlock;
		struct k_mutex		txlock;

		u32_t refs;
		u32_t state;

		/* cached msgs for recv */
		u32_t			wrptr;
		u32_t			rdptr;
		struct smsg		caches[SMSG_CACHE_NR];
	};

	struct smsg_queue_buf {
		u32_t			addr;
		u32_t			size;	/* must be 2^n */
		u32_t			rdptr;
		u32_t			wrptr;
	};

	struct smsg_queue {
		/* send-buffer info */
		struct smsg_queue_buf tx_buf;
		/* recv-buffer info */
		struct smsg_queue_buf rx_buf;
	};

		/* smsg ring-buffer between AP/CP ipc */
	struct smsg_ipc {
		char			*name;
		u8_t			dst;
		u8_t			padding[3];

		struct smsg_queue   queue[QUEUE_PRIO_MAX];

#ifdef	CONFIG_SPRD_MAILBOX
		/* target core_id over mailbox */
		int			core_id;
#endif

		/* sipc irq related */
		int			irq;

		u32_t		(*rxirq_status)(void);
		void		(*rxirq_clear)(void);
		void		(*txirq_trigger)(void);

		/* sipc ctrl thread */
		struct tcb_s	*thread;
		k_tid_t			 pid;
		struct k_sem    irq_sem;
		/* lock for send-buffer */
		u32_t            txpinlock;
		/* all fixed channels receivers */
		struct smsg_channel	channels[SMSG_CH_NR];
	};

#define CHAN_STATE_UNUSED	0
#define CHAN_STATE_WAITING	1
#define CHAN_STATE_OPENED	2
#define CHAN_STATE_FREE		3

	/* create/destroy smsg ipc between AP/CP */
	struct smsg_ipc *smsg_ipc_create(u8_t dst);
	int smsg_ipc_destroy(u8_t dst);
	int  smsg_suspend_init(void);

	/* initialize smem pool for AP/CP */
	int smem_init(u32_t addr, u32_t size);

#ifdef __cplusplus
}
#endif

#endif
