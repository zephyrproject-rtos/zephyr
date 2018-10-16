/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hal_log.h>
LOG_MODULE_DECLARE(LOG_MODULE_NAME);

#include <zephyr.h>
#include <uwp_hal.h>
#include <string.h>

#include "sipc.h"
#include "sipc_priv.h"
#include "sblock.h"

static struct sblock_mgr sblocks[SIPC_ID_NR][SMSG_CH_NR-SMSG_CH_OFFSET];

int sblock_get(u8_t dst, u8_t channel, struct sblock *blk, int timeout);
int sblock_send(u8_t dst, u8_t channel, u8_t prio, struct sblock *blk);
extern int sprd_bt_irq_init(void);

static int sblock_recover(u8_t dst, u8_t channel)
{
	int i, j;
	struct sblock_mgr *sblock = &sblocks[dst][channel-SMSG_CH_OFFSET];
	struct sblock_ring *ring = NULL;

	volatile struct sblock_ring_header *ringhd = NULL;
	volatile struct sblock_ring_header *poolhd = NULL;

	if (!sblock) {
		return -ENODEV;
	}

	ring = &sblock->ring;
	ringhd = (volatile struct sblock_ring_header *)(&ring->header->ring);
	poolhd = (volatile struct sblock_ring_header *)(&ring->header->pool);

	sblock->state = SBLOCK_STATE_IDLE;

	/* clean txblks ring */
	ringhd->txblk_wrptr = ringhd->txblk_rdptr;

    /* recover txblks pool */
	poolhd->txblk_rdptr = poolhd->txblk_wrptr;
	for (i = 0, j = 0; i < poolhd->txblk_count; i++) {
		if (ring->txrecord[i] == SBLOCK_BLK_STATE_DONE) {
			ring->p_txblks[j].addr = i * sblock->txblksz +
			poolhd->txblk_addr;
			ring->p_txblks[j].length = sblock->txblksz;
			poolhd->txblk_wrptr = poolhd->txblk_wrptr + 1;
			j++;
		}
	}

	/* clean rxblks ring */
	ringhd->rxblk_rdptr = ringhd->rxblk_wrptr;

	/* recover rxblks pool */
	poolhd->rxblk_wrptr = poolhd->rxblk_rdptr;
	for (i = 0, j = 0; i < poolhd->rxblk_count; i++) {
		if (ring->rxrecord[i] == SBLOCK_BLK_STATE_DONE) {
			ring->p_rxblks[j].addr = i * sblock->rxblksz +
			poolhd->rxblk_addr;
			ring->p_rxblks[j].length = sblock->rxblksz;
			poolhd->rxblk_wrptr = poolhd->rxblk_wrptr + 1;
			j++;
		}
	}

	return 0;
}

static int get_channel_prio(int channel)
{
	int prio;

	switch (channel) {
	case SMSG_CH_BT:
	case SMSG_CH_WIFI_CTRL:
		prio = QUEUE_PRIO_HIGH;
		break;
	default:
		prio = QUEUE_PRIO_NORMAL;
		break;
	}

	return prio;
}

void sblock_process(struct smsg *msg)
{
	int channel = msg->channel;
	struct sblock_mgr *sblock = &sblocks[0][channel-SMSG_CH_OFFSET];
	struct smsg mcmd;
	int ret = 0;
	int recovery = 0;
	int prio = get_channel_prio(channel);

	switch (msg->type) {
	case SMSG_TYPE_OPEN:
		/* handle channel recovery */
		if (recovery) {
			if (sblock->handler) {
				sblock->handler(SBLOCK_NOTIFY_CLOSE,
				sblock->data);
			}
			sblock_recover(sblock->dst, sblock->channel);
		}
		smsg_open_ack(sblock->dst, sblock->channel);
		break;
	case SMSG_TYPE_CLOSE:
		/* handle channel recovery */
		smsg_close_ack(sblock->dst, sblock->channel);
		if (sblock->handler) {
			sblock->handler(SBLOCK_NOTIFY_CLOSE, sblock->data);
		}
		sblock->state = SBLOCK_STATE_IDLE;
		break;
	case SMSG_TYPE_CMD:
		/* respond cmd done for sblock init */
		smsg_set(&mcmd, sblock->channel, SMSG_TYPE_DONE,
				SMSG_DONE_SBLOCK_INIT, sblock->smem_addr);
		smsg_send(sblock->dst, prio, &mcmd, -1);
		sblock->state = SBLOCK_STATE_READY;
		recovery = 1;
		LOG_DBG("ap cp create %d channel success!", sblock->channel);
		if (sblock->channel == SMSG_CH_BT) {
			sprd_bt_irq_init();
		}
		break;
	case SMSG_TYPE_EVENT:
		switch (msg->flag) {
		case SMSG_EVENT_SBLOCK_SEND:
			if (sblock->callback) {
				sblock->callback(sblock->channel);
			}
			break;
		case SMSG_EVENT_SBLOCK_RECV:
			break;
		case SMSG_EVENT_SBLOCK_RELEASE:
			break;
		default:
			ret = 1;
			break;
		}
		break;
	default:
		ret = 1;
		break;
	};
	if (ret) {
		LOG_WRN("non-handled sblock msg: %d-%d, %d, %d, %d",
			sblock->dst, sblock->channel,
			msg->type, msg->flag, msg->value);
		ret = 0;
	}

}

int sblock_create(u8_t dst, u8_t channel,
		u32_t txblocknum, u32_t txblocksize,
		u32_t rxblocknum, u32_t rxblocksize)
{
	struct sblock_mgr *sblock;
	volatile struct sblock_ring_header *ringhd = NULL;
	volatile struct sblock_ring_header *poolhd = NULL;
	struct sblock_ring *ring = NULL;
	u32_t hsize;
	u32_t block_addr;
	int prio;
	int i;
	int ret;

	sblock = &sblocks[dst][channel-SMSG_CH_OFFSET];

	sblock->state = SBLOCK_STATE_IDLE;
	sblock->dst = dst;
	sblock->channel = channel;
	txblocksize = SBLOCKSZ_ALIGN(txblocksize, SBLOCK_ALIGN_BYTES);
	rxblocksize = SBLOCKSZ_ALIGN(rxblocksize, SBLOCK_ALIGN_BYTES);
	sblock->txblksz = txblocksize;
	sblock->rxblksz = rxblocksize;
	sblock->txblknum = txblocknum;
	sblock->rxblknum = rxblocknum;
	sblock->handler = NULL;
	sblock->callback = NULL;
	/* allocate smem */
	hsize = sizeof(struct sblock_header);
	/* for header*/
	sblock->smem_size = hsize +
	    /* for blks */
		txblocknum * txblocksize + rxblocknum * rxblocksize +
		/* for ring*/
		(txblocknum + rxblocknum) * sizeof(struct sblock_blks) +
		/* for pool*/
		(txblocknum + rxblocknum) * sizeof(struct sblock_blks);

	switch (channel) {
	case SMSG_CH_WIFI_CTRL:
		block_addr = CTRLPATH_SBLOCK_SMEM_ADDR;
		break;
	case SMSG_CH_WIFI_DATA_NOR:
		block_addr = DATAPATH_SBLOCK_SMEM_ADDR;
		break;
	case SMSG_CH_WIFI_DATA_SPEC:
		block_addr = DATAPATH_SPEC_SBLOCK_SMEM_ADDR;
		break;
	case SMSG_CH_BT:
		block_addr = BT_SBLOCK_SMEM_ADDR;
		break;
	default:
		block_addr = CTRLPATH_SBLOCK_SMEM_ADDR;
		break;
	}

	sblock->smem_addr = block_addr;
	LOG_DBG("smem_addr 0x%x record_share_addr 0x%x channel %d",
		sblock->smem_addr, block_addr, channel);

	ring = &sblock->ring;
	ring->header = (struct sblock_header *)sblock->smem_addr;
	ringhd = (volatile struct sblock_ring_header *)(&ring->header->ring);
	poolhd = (volatile struct sblock_ring_header *)(&ring->header->pool);

	ringhd->txblk_addr = sblock->smem_addr + hsize;
	ringhd->txblk_count = txblocknum;
	ringhd->txblk_size = txblocksize;
	ringhd->txblk_rdptr = 0;
	ringhd->txblk_wrptr = 0;
	ringhd->txblk_blks = sblock->smem_addr + hsize +
		txblocknum * txblocksize + rxblocknum * rxblocksize;
	ringhd->rxblk_addr = ringhd->txblk_addr + txblocknum * txblocksize;
	ringhd->rxblk_count = rxblocknum;
	ringhd->rxblk_size = rxblocksize;
	ringhd->rxblk_rdptr = 0;
	ringhd->rxblk_wrptr = 0;
	ringhd->rxblk_blks = ringhd->txblk_blks +
	txblocknum * sizeof(struct sblock_blks);

	poolhd->txblk_addr = sblock->smem_addr + hsize;
	poolhd->txblk_count = txblocknum;
	poolhd->txblk_size = txblocksize;
	poolhd->txblk_rdptr = 0;
	poolhd->txblk_wrptr = 0;
	poolhd->txblk_blks = ringhd->rxblk_blks +
	rxblocknum * sizeof(struct sblock_blks);
	poolhd->rxblk_addr = ringhd->txblk_addr + txblocknum * txblocksize;
	poolhd->rxblk_count = rxblocknum;
	poolhd->rxblk_size = rxblocksize;
	poolhd->rxblk_rdptr = 0;
	poolhd->rxblk_wrptr = 0;
	poolhd->rxblk_blks = poolhd->txblk_blks +
	txblocknum * sizeof(struct sblock_blks);
		LOG_DBG("0x%p ringhd %p poolhd %p", sblock, ringhd, poolhd);

	ring->r_txblks = (struct sblock_blks *)ringhd->txblk_blks;
	ring->r_rxblks = (struct sblock_blks *)ringhd->rxblk_blks;
	ring->p_txblks = (struct sblock_blks *)poolhd->txblk_blks;
	ring->p_rxblks = (struct sblock_blks *)poolhd->rxblk_blks;
	ring->txblk_virt = (void *)ringhd->txblk_addr;
	ring->rxblk_virt = (void *)ringhd->rxblk_addr;

	for (i = 0; i < txblocknum; i++) {
		ring->p_txblks[i].addr = poolhd->txblk_addr + i * txblocksize;
		ring->p_txblks[i].length = txblocksize;
		ring->txrecord[i] = SBLOCK_BLK_STATE_DONE;
		poolhd->txblk_wrptr++;
	}
	for (i = 0; i < rxblocknum; i++) {
		ring->p_rxblks[i].addr = poolhd->rxblk_addr + i * rxblocksize;
		ring->p_rxblks[i].length = rxblocksize;
		ring->rxrecord[i] = SBLOCK_BLK_STATE_DONE;
		poolhd->rxblk_wrptr++;
	}

	ring->yell = 1;
	k_sem_init(&ring->getwait, 0, 1);
	k_sem_init(&ring->recvwait, 0, 1);

	sblock->state = SBLOCK_STATE_READY;

	LOG_DBG("r_txblks %p %p %p %p",
	ring->r_txblks, ring->r_rxblks, ring->p_txblks, ring->p_rxblks);

	prio = get_channel_prio(channel);

	ret = smsg_ch_open(sblock->dst, sblock->channel, prio, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to open channel %d", sblock->channel);
		return ret;
	}

	return 0;
}

void sblock_destroy(u8_t dst, u8_t channel)
{
	struct sblock_mgr *sblock = &sblocks[dst][channel-SMSG_CH_OFFSET];
	int prio;

	switch (sblock->channel) {
	case SMSG_CH_WIFI_CTRL:
		prio = QUEUE_PRIO_HIGH;
		break;
	case SMSG_CH_WIFI_DATA_NOR:
		prio = QUEUE_PRIO_NORMAL;
		break;
	case SMSG_CH_WIFI_DATA_SPEC:
		prio = QUEUE_PRIO_NORMAL;
		break;
	default:
		prio = QUEUE_PRIO_NORMAL;
		break;
	}
	sblock->state = SBLOCK_STATE_IDLE;
	smsg_ch_close(dst, channel, prio, -1);
}

int sblock_unregister_callback(u8_t channel)
{
	int dst = 0;
	struct sblock_mgr *sblock = &sblocks[dst][channel-SMSG_CH_OFFSET];

	LOG_DBG("%d channel=%d ", dst, channel);

	sblock->callback = NULL;

	return 0;
}

int sblock_register_callback(u8_t channel,
		void (*callback)(int ch))
{
	int dst = 0;
	struct sblock_mgr *sblock = &sblocks[dst][channel-SMSG_CH_OFFSET];

	LOG_DBG("%d channel=%d %p", dst, channel, callback);

	sblock->callback = callback;

	return 0;
}

int sblock_state(u8_t channel)
{
	int dst = 0;
	struct sblock_mgr *sblock = &sblocks[dst][channel-SMSG_CH_OFFSET];

	return sblock->state;
}

int sblock_register_notifier(u8_t dst, u8_t channel,
		void (*handler)(int event, void *data), void *data)
{
	struct sblock_mgr *sblock = &sblocks[dst][channel-SMSG_CH_OFFSET];

#ifndef CONFIG_SIPC_WCN
	if (sblock->handler) {
		LOG_WRN("sblock handler already registered");
		return -EBUSY;
	}
#endif
	sblock->handler = handler;
	sblock->data = data;

	return 0;
}


/*这个地方是发送失败了 再重新放入pool*/
void sblock_put(u8_t dst, u8_t channel, struct sblock *blk)
{
	struct sblock_mgr *sblock = &sblocks[dst][channel-SMSG_CH_OFFSET];
	struct sblock_ring *ring = NULL;
	volatile struct sblock_ring_header *poolhd = NULL;
	int txpos;
	int index;

	ring = &sblock->ring;
	poolhd = (volatile struct sblock_ring_header *)(&ring->header->pool);

	txpos = sblock_get_ringpos(poolhd->txblk_rdptr-1, poolhd->txblk_count);
	ring->r_txblks[txpos].addr = (u32_t)blk->addr;
	ring->r_txblks[txpos].length = poolhd->txblk_size;
	poolhd->txblk_rdptr = poolhd->txblk_rdptr - 1;
	LOG_DBG("%d %d %d ", poolhd->txblk_rdptr, poolhd->txblk_wrptr, txpos);

	if ((int)(poolhd->txblk_wrptr - poolhd->txblk_rdptr) == 1) {
		wakeup_smsg_task_all(&(ring->getwait));
	}

	index = sblock_get_index(((u32_t)blk->addr - (u32_t)ring->txblk_virt),
	sblock->txblksz);
	ring->txrecord[index] = SBLOCK_BLK_STATE_DONE;
}

int sblock_get(u8_t dst, u8_t channel, struct sblock *blk, int timeout)
{
	struct sblock_mgr *sblock = &sblocks[dst][channel-SMSG_CH_OFFSET];
	struct sblock_ring *ring = NULL;
	volatile struct sblock_ring_header *poolhd = NULL;
	int txpos, index;
	int ret = 0;

	ring = &sblock->ring;
	poolhd = (volatile struct sblock_ring_header *)(&ring->header->pool);
	LOG_DBG("%d %d ch=%d", poolhd->txblk_rdptr,
		poolhd->txblk_wrptr, channel);

	if (poolhd->txblk_rdptr == poolhd->txblk_wrptr) {
		ret = k_sem_take(&ring->getwait, timeout);
		if (ret) {
			LOG_WRN("wait timeout!");
			ret = ret;
		}

		if (sblock->state == SBLOCK_STATE_IDLE) {
			LOG_WRN("sblock state is idle!");
			ret = -EIO;
		}
	}
	/* multi-gotter may cause got failure */
	if (poolhd->txblk_rdptr != poolhd->txblk_wrptr &&
			sblock->state == SBLOCK_STATE_READY) {
		txpos = sblock_get_ringpos(poolhd->txblk_rdptr,
			poolhd->txblk_count);

		LOG_DBG("txpos %d poolhd->txblk_rdptr %d",
			txpos, poolhd->txblk_rdptr);
		blk->addr = (void *)ring->p_txblks[txpos].addr;
		blk->length = poolhd->txblk_size;
		poolhd->txblk_rdptr++;
		index = sblock_get_index(((u32_t)blk->addr -
			(u32_t)ring->txblk_virt), sblock->txblksz);
		ring->txrecord[index] = SBLOCK_BLK_STATE_PENDING;
	} else {
		ret = sblock->state == SBLOCK_STATE_READY ? -EAGAIN : -EIO;
	}

	return ret;
}

static int sblock_send_ex(u8_t dst, u8_t channel,
	u8_t prio, struct sblock *blk, bool yell)
{
	struct sblock_mgr *sblock = &sblocks[dst][channel-SMSG_CH_OFFSET];
	struct sblock_ring *ring;
	volatile struct sblock_ring_header *ringhd;
	struct smsg mevt;
	int txpos, index;
	int ret = 0;

	if (sblock->state != SBLOCK_STATE_READY) {
		LOG_WRN("sblock-%d-%d not ready!", dst, channel);
		return sblock ? -EIO : -ENODEV;
	}

	ring = &sblock->ring;
	ringhd = (volatile struct sblock_ring_header *)(&ring->header->ring);

	txpos = sblock_get_ringpos(ringhd->txblk_wrptr, ringhd->txblk_count);
	ring->r_txblks[txpos].addr = (u32_t)blk->addr;
	ring->r_txblks[txpos].length = blk->length;
	ringhd->txblk_wrptr++;

	if (sblock->state == SBLOCK_STATE_READY) {
		if (yell) {
			smsg_set(&mevt, channel, SMSG_TYPE_EVENT,
				SMSG_EVENT_SBLOCK_SEND, 0);
			ret = smsg_send(dst, prio, &mevt, 0);
			} else if (!ring->yell &&
			((int)(ringhd->txblk_wrptr -
				ringhd->txblk_rdptr) == 1)) {
				ring->yell = 1;
			}
	}

	index = sblock_get_index(((u32_t)blk->addr - (u32_t)ring->txblk_virt),
		sblock->txblksz);
	ring->txrecord[index] = SBLOCK_BLK_STATE_DONE;

	return ret;
}

int sblock_send(u8_t dst, u8_t channel, u8_t prio, struct sblock *blk)
{
	return sblock_send_ex(dst, channel, prio, blk, true);
}

int sblock_send_prepare(u8_t dst, u8_t channel, u8_t prio, struct sblock *blk)
{
	return sblock_send_ex(dst, channel, prio, blk, false);
}

int sblock_send_finish(u8_t dst, u8_t channel)
{
	struct sblock_mgr *sblock = &sblocks[dst][channel-SMSG_CH_OFFSET];
	struct sblock_ring *ring;
	struct smsg mevt;
	int ret = 0;

	if (sblock->state != SBLOCK_STATE_READY) {
		LOG_WRN("sblock-%d-%d not ready!", dst, channel);
		return sblock ? -EIO : -ENODEV;
	}

	ring = &sblock->ring;

	if (ring->yell) {
		ring->yell = 0;
		smsg_set(&mevt, channel, SMSG_TYPE_EVENT,
			SMSG_EVENT_SBLOCK_SEND, 0);
		ret = smsg_send(dst, QUEUE_PRIO_NORMAL, &mevt, 0);
	}

	return ret;
}

int sblock_receive(u8_t dst, u8_t channel, struct sblock *blk, int timeout)
{
	struct sblock_mgr *sblock = &sblocks[dst][channel-SMSG_CH_OFFSET];
	struct sblock_ring *ring;
	volatile struct sblock_ring_header *ringhd;
	int rxpos, index, ret = 0;

	if (sblock->state != SBLOCK_STATE_READY) {
		LOG_WRN("sblock-%d-%d not ready!", dst, channel);
		return sblock ? -EIO : -ENODEV;
	}
	ring = &sblock->ring;
	ringhd = (volatile struct sblock_ring_header *)(&ring->header->ring);

	LOG_DBG("channel=%d,%d, wrptr=%d, rdptr=%d",
		channel, timeout, ringhd->rxblk_wrptr, ringhd->rxblk_rdptr);

	if (ringhd->rxblk_wrptr == ringhd->rxblk_rdptr) {
		if (timeout == 0) {
			/* no wait */
			LOG_DBG("ch %d is empty,please wait!", channel);
			ret = -ENODATA;
		} else if (timeout < 0) {
			/* wait forever */
			ret = k_sem_take(&ring->recvwait, K_FOREVER);
			if (ret < 0) {
				LOG_WRN(" wait interrupted!");
			}

			if (sblock->state == SBLOCK_STATE_IDLE) {
				LOG_WRN(" sblock state is idle!");
				ret = -EIO;
			}

		} else {
			/* wait timeout */
			ret = k_sem_take(&ring->recvwait, timeout);
			if (ret < 0) {
				LOG_WRN(" wait interrupted!");
			} else if (ret == 0) {
				LOG_WRN(" wait timeout!");
				ret = -ETIME;
			}

			if (sblock->state == SBLOCK_STATE_IDLE) {
				LOG_WRN(" sblock state is idle!");
				ret = -EIO;
			}
		}
	}

	if (ret < 0) {
		return ret;
	}

	if (ringhd->rxblk_wrptr != ringhd->rxblk_rdptr &&
			sblock->state == SBLOCK_STATE_READY) {
		rxpos = sblock_get_ringpos(ringhd->rxblk_rdptr,
			ringhd->rxblk_count);
		blk->addr = (void *)ring->r_rxblks[rxpos].addr;
		blk->length = ring->r_rxblks[rxpos].length;
		ringhd->rxblk_rdptr = ringhd->rxblk_rdptr + 1;
		LOG_DBG("channel=%d, rxpos=%d, addr=%p, len=%d",
			channel, rxpos, blk->addr, blk->length);
		index = sblock_get_index(((u32_t)blk->addr -
			(u32_t)ring->rxblk_virt), sblock->rxblksz);
		ring->rxrecord[index] = SBLOCK_BLK_STATE_PENDING;
	} else {
		ret = sblock->state == SBLOCK_STATE_READY ? -EAGAIN : -EIO;
	}
	return ret;
}

int sblock_get_arrived_count(u8_t dst, u8_t channel)
{
	struct sblock_mgr *sblock = &sblocks[dst][channel-SMSG_CH_OFFSET];
	struct sblock_ring *ring = NULL;
	volatile struct sblock_ring_header *ringhd = NULL;
	int blk_count = 0;

	if (sblock->state != SBLOCK_STATE_READY) {
		LOG_WRN("sblock-%d-%d not ready!", dst, channel);
		return -ENODEV;
	}

	ring = &sblock->ring;
	ringhd = (volatile struct sblock_ring_header *)(&ring->header->ring);

	blk_count = (int)(ringhd->rxblk_wrptr - ringhd->rxblk_rdptr);

	return blk_count;

}

int sblock_get_free_count(u8_t dst, u8_t channel)
{
	struct sblock_mgr *sblock = &sblocks[dst][channel-SMSG_CH_OFFSET];
	struct sblock_ring *ring = NULL;
	volatile struct sblock_ring_header *poolhd = NULL;
	int blk_count = 0;


	if (sblock->state != SBLOCK_STATE_READY) {
		LOG_WRN("sblock-%d-%d not ready!", dst, channel);
		return -ENODEV;
	}

	ring = &sblock->ring;
	poolhd = (volatile struct sblock_ring_header *)(&ring->header->pool);

	blk_count = (int)(poolhd->txblk_wrptr - poolhd->txblk_rdptr);

	return blk_count;
}


int sblock_release(u8_t dst, u8_t channel, struct sblock *blk)
{
	struct sblock_mgr *sblock = &sblocks[dst][channel-SMSG_CH_OFFSET];
	struct sblock_ring *ring = NULL;
	volatile struct sblock_ring_header *poolhd = NULL;
	struct smsg mevt;
	int rxpos;
	int index;
	int prio;
	int rx_num;

	if (sblock->state != SBLOCK_STATE_READY) {
		LOG_WRN("sblock-%d-%d not ready!", dst, channel);
		return -ENODEV;
	}

	switch (sblock->channel) {
	case SMSG_CH_BT:
		rx_num = BT_RX_BLOCK_NUM;
		prio = QUEUE_PRIO_HIGH;
		break;
	case SMSG_CH_WIFI_CTRL:
		rx_num = CTRLPATH_RX_BLOCK_NUM;
		prio = QUEUE_PRIO_HIGH;
		break;
	case SMSG_CH_WIFI_DATA_NOR:
		rx_num = DATAPATH_NOR_RX_BLOCK_NUM;
		prio = QUEUE_PRIO_NORMAL;
		break;
	case SMSG_CH_WIFI_DATA_SPEC:
		rx_num = DATAPATH_SPEC_RX_BLOCK_NUM;
		prio = QUEUE_PRIO_NORMAL;
		break;
	default:
		rx_num = CTRLPATH_RX_BLOCK_NUM;
		prio = QUEUE_PRIO_NORMAL;
		break;
	}

	ring = &sblock->ring;
	poolhd = (volatile struct sblock_ring_header *)(&ring->header->pool);

	rxpos = sblock_get_ringpos(poolhd->rxblk_wrptr, poolhd->rxblk_count);
	ring->p_rxblks[rxpos].addr = (u32_t)blk->addr;
	ring->p_rxblks[rxpos].length = poolhd->rxblk_size;
	poolhd->rxblk_wrptr = poolhd->rxblk_wrptr + 1;
	LOG_DBG(":ch=%d addr=%x %d %d", channel, ring->p_rxblks[rxpos].addr,
		poolhd->rxblk_wrptr, poolhd->rxblk_rdptr);

	if ((int)(poolhd->rxblk_wrptr - poolhd->rxblk_rdptr) >= (rx_num-2) &&
			sblock->state == SBLOCK_STATE_READY) {
		/* send smsg to notify the peer side */
		LOG_DBG("send release smsg");
		smsg_set(&mevt, channel, SMSG_TYPE_EVENT,
			SMSG_EVENT_SBLOCK_RELEASE, 0);
		smsg_send(dst, prio, &mevt, -1);
	}

	index = sblock_get_index(((u32_t)blk->addr - (u32_t)ring->rxblk_virt),
		sblock->rxblksz);
	ring->rxrecord[index] = SBLOCK_BLK_STATE_DONE;

	return 0;
}
