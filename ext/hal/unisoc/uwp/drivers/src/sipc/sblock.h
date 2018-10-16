/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __HAL_SBLOCK_H
#define __HAL_SBLOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr.h>

/* flag for CMD/DONE msg type */
#define SMSG_CMD_SBLOCK_INIT		0x0001
#define SMSG_DONE_SBLOCK_INIT		0x0002

/* flag for EVENT msg type */
#define SMSG_EVENT_SBLOCK_SEND		0x0001
#define SMSG_EVENT_SBLOCK_RECV		0x0003
#define SMSG_EVENT_SBLOCK_RELEASE	0x0002

#define SBLOCK_STATE_IDLE		0
#define SBLOCK_STATE_READY		1

#define SBLOCK_BLK_STATE_DONE		0
#define SBLOCK_BLK_STATE_PENDING	1

#define SBLOCK_SMEM_ADDR              0x001E1000

#define BLOCK_HEADROOM_SIZE     16
#define CTRLPATH_SBLOCK_SMEM_ADDR   SBLOCK_SMEM_ADDR

#define CTRLPATH_TX_BLOCK_SIZE  (1600+BLOCK_HEADROOM_SIZE)
#define CTRLPATH_TX_BLOCK_NUM   3
#define CTRLPATH_RX_BLOCK_SIZE  (1536+BLOCK_HEADROOM_SIZE)
#define CTRLPATH_RX_BLOCK_NUM   5

#define CTRLPATH_MANAGER_SIZE (sizeof(struct sblock_header) + \
	(CTRLPATH_TX_BLOCK_NUM+CTRLPATH_RX_BLOCK_NUM)* \
	sizeof(struct sblock_blks)*2)
#define DATAPATH_SBLOCK_SMEM_ADDR (CTRLPATH_SBLOCK_SMEM_ADDR + \
	(CTRLPATH_TX_BLOCK_SIZE*CTRLPATH_TX_BLOCK_NUM + \
	CTRLPATH_RX_BLOCK_SIZE* \
	CTRLPATH_RX_BLOCK_NUM) + CTRLPATH_MANAGER_SIZE)

#define DATAPATH_NOR_TX_BLOCK_SIZE  (188+BLOCK_HEADROOM_SIZE)
#define DATAPATH_NOR_TX_BLOCK_NUM   50
#define DATAPATH_NOR_RX_BLOCK_SIZE  (188+BLOCK_HEADROOM_SIZE)
#define DATAPATH_NOR_RX_BLOCK_NUM   50

#define DATAPATH_MANAGER_SIZE (sizeof(struct sblock_header) + \
	(DATAPATH_NOR_TX_BLOCK_NUM+DATAPATH_NOR_RX_BLOCK_NUM)* \
	sizeof(struct sblock_blks)*2)
#define DATAPATH_SPEC_SBLOCK_SMEM_ADDR (DATAPATH_SBLOCK_SMEM_ADDR + \
	(DATAPATH_NOR_TX_BLOCK_SIZE*DATAPATH_NOR_TX_BLOCK_NUM + \
	DATAPATH_NOR_RX_BLOCK_SIZE* \
	DATAPATH_NOR_RX_BLOCK_NUM) + DATAPATH_MANAGER_SIZE)

#define DATAPATH_SPEC_TX_BLOCK_SIZE  (1664+BLOCK_HEADROOM_SIZE)
#define DATAPATH_SPEC_TX_BLOCK_NUM   2
#define DATAPATH_SPEC_RX_BLOCK_SIZE  (1664+BLOCK_HEADROOM_SIZE)
#define DATAPATH_SPEC_RX_BLOCK_NUM   5

#define DATAPATH_SPEC_MANAGER_SIZE (sizeof(struct sblock_header) + \
	(DATAPATH_SPEC_TX_BLOCK_NUM+DATAPATH_SPEC_RX_BLOCK_NUM)* \
	sizeof(struct sblock_blks)*2)
#define BT_SBLOCK_SMEM_ADDR  (DATAPATH_SPEC_SBLOCK_SMEM_ADDR + \
	(DATAPATH_SPEC_TX_BLOCK_SIZE*DATAPATH_SPEC_TX_BLOCK_NUM + \
	DATAPATH_SPEC_RX_BLOCK_SIZE*DATAPATH_SPEC_RX_BLOCK_NUM) \
	+ DATAPATH_SPEC_MANAGER_SIZE)

#define BT_TX_BLOCK_SIZE  (4)
#define BT_TX_BLOCK_NUM   (1)
#define BT_RX_BLOCK_SIZE  (1300)
#define BT_RX_BLOCK_NUM   5

struct sblock_blks {
	u32_t		addr; /*phy address*/
	u32_t		length;
};

/* ring block header */
struct sblock_ring_header {
	/* get|send-block info */
	u32_t		txblk_addr;
	u32_t		txblk_count;
	u32_t		txblk_size;
	u32_t		txblk_blks;
	u32_t		txblk_rdptr;
	u32_t		txblk_wrptr;

	/* release|recv-block info */
	u32_t		rxblk_addr;
	u32_t		rxblk_count;
	u32_t		rxblk_size;
	u32_t		rxblk_blks;
	u32_t		rxblk_rdptr;
	u32_t		rxblk_wrptr;
};

struct sblock_header {
	struct sblock_ring_header ring;
	struct sblock_ring_header pool;
};


#define MAX_BLOCK_COUNT		DATAPATH_NOR_RX_BLOCK_NUM
struct sblock_ring {
	struct sblock_header	*header;
	void			*txblk_virt; /* virt of header->txblk_addr */
	void			*rxblk_virt; /* virt of header->rxblk_addr */

	struct sblock_blks	*r_txblks;
	struct sblock_blks	*r_rxblks;
	struct sblock_blks	*p_txblks;
	struct sblock_blks	*p_rxblks;

	int txrecord[MAX_BLOCK_COUNT]; /* record the state of every txblk */
	int rxrecord[MAX_BLOCK_COUNT]; /* record the state of every rxblk */
	int yell; /* need to notify cp */
	struct k_sem	getwait;
	struct k_sem	recvwait;
};

struct sblock_mgr {
	u8_t			dst;
	u8_t			channel;
	u32_t		state;

	void			*smem_virt;
	u32_t		smem_addr;
	u32_t		smem_size;

	u32_t		txblksz;
	u32_t		rxblksz;
	u32_t		txblknum;
	u32_t		rxblknum;

	struct sblock_ring	ring;

	void			(*handler)(int event, void *data);
	void		    (*callback)(int ch);
	void			*data;
};

#define SBLOCKSZ_ALIGN(blksz, size) (((blksz)+((size)-1))&(~((size)-1)))
#define is_power_of_2(x)	((x) != 0 && (((x) & ((x) - 1)) == 0))
#define CONFIG_SBLOCK_THREAD_DEFPRIO 50
#define CONFIG_SBLOCK_THREAD_STACKSIZE 1024

#define SBLOCK_ALIGN_BYTES (4)

static inline u32_t sblock_get_index(u32_t x, u32_t y)
{
	return (x / y);
}

static inline u32_t sblock_get_ringpos(u32_t x, u32_t y)
{
	return is_power_of_2(y) ? (x & (y - 1)) : (x % y);
}

int sblock_create(u8_t dst, u8_t channel,
		u32_t txblocknum, u32_t txblocksize,
		u32_t rxblocknum, u32_t rxblocksize);
int sblock_release(u8_t dst, u8_t channel, struct sblock *blk);
int sblock_get_free_count(u8_t dst, u8_t channel);
int sblock_get_arrived_count(u8_t dst, u8_t channel);
int sblock_receive(u8_t dst, u8_t channel, struct sblock *blk, int timeout);
int sblock_send_finish(u8_t dst, u8_t channel);
int sblock_send(u8_t dst, u8_t channel, u8_t prio, struct sblock *blk);
int sblock_send_prepare(u8_t dst, u8_t channel, u8_t prio, struct sblock *blk);
int sblock_get(u8_t dst, u8_t channel, struct sblock *blk, int timeout);
void sblock_put(u8_t dst, u8_t channel, struct sblock *blk);
int sblock_register_notifier(u8_t dst, u8_t channel,
		void (*handler)(int event, void *data), void *data);
int sblock_register_callback(u8_t channel,
		void (*callback)(int ch));
int sblock_unregister_callback(u8_t channel);
int sblock_state(u8_t channel);

#endif
