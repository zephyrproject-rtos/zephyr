/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hal_log.h>
LOG_MODULE_DECLARE(LOG_MODULE_NAME);

#include <zephyr.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <uwp_hal.h>
#include <string.h>

#include "sipc.h"
#include "sipc_priv.h"

#define SMSG_STACK_SIZE		(2048)
struct k_thread smsg_thread;
K_THREAD_STACK_MEMBER(smsg_stack, SMSG_STACK_SIZE);
static struct smsg_ipc smsg_ipcs[SIPC_ID_NR];

#define SMSG_IRQ_TXBUF_ADDR  (0)
#define SMSG_IRQ_TXBUF_SIZE	 (0x200)
#define SMSG_IRQ_RXBUF_ADDR	 (SMSG_IRQ_TXBUF_ADDR + SMSG_IRQ_TXBUF_SIZE)
#define SMSG_IRQ_RXBUF_SIZE	 (0x100)

#define SMSG_PRIO_TXBUF_ADDR  (SMSG_IRQ_RXBUF_ADDR + SMSG_IRQ_RXBUF_SIZE)
#define SMSG_PRIO_TXBUF_SIZE  (0x200)
#define SMSG_PRIO_RXBUF_ADDR  (SMSG_PRIO_TXBUF_ADDR + SMSG_PRIO_TXBUF_SIZE)
#define SMSG_PRIO_RXBUF_SIZE  (0x200)

#define SMSG_TXBUF_ADDR	 (SMSG_PRIO_RXBUF_ADDR + SMSG_PRIO_RXBUF_SIZE)
#define SMSG_TXBUF_SIZE	 (SZ_1K)
#define SMSG_RXBUF_ADDR	 (SMSG_TXBUF_ADDR + SMSG_TXBUF_SIZE)
#define SMSG_RXBUF_SIZE	 (SZ_1K)

#define SMSG_RINGHDR  (SMSG_TXBUF_ADDR + SMSG_TXBUF_SIZE + SMSG_RXBUF_SIZE)

#define SMSG_IRQ_TXBUF_RDPTR	(SMSG_RINGHDR + 0)
#define SMSG_IRQ_TXBUF_WRPTR	(SMSG_RINGHDR + 4)
#define SMSG_IRQ_RXBUF_RDPTR	(SMSG_RINGHDR + 8)
#define SMSG_IRQ_RXBUF_WRPTR	(SMSG_RINGHDR + 12)

#define SMSG_PRIO_TXBUF_RDPTR	(SMSG_RINGHDR + 16)
#define SMSG_PRIO_TXBUF_WRPTR	(SMSG_RINGHDR + 20)
#define SMSG_PRIO_RXBUF_RDPTR	(SMSG_RINGHDR + 24)
#define SMSG_PRIO_RXBUF_WRPTR	(SMSG_RINGHDR + 28)

#define SMSG_TXBUF_RDPTR	(SMSG_RINGHDR + 32)
#define SMSG_TXBUF_WRPTR	(SMSG_RINGHDR + 36)
#define SMSG_RXBUF_RDPTR	(SMSG_RINGHDR + 40)
#define SMSG_RXBUF_WRPTR	(SMSG_RINGHDR + 44)

void sipc_init_smsg_queue_buf(struct smsg_queue_buf *buf,
		u32_t size, u32_t addr, u32_t rdptr, u32_t wrptr)
{
	buf->size = size / sizeof(struct smsg);
	buf->addr = addr;
	buf->rdptr = rdptr;
	buf->wrptr = wrptr;
}

static struct smsg_ipc *smsg_set_addr(struct smsg_ipc *ipc, u32_t base)
{
	sipc_init_smsg_queue_buf(&ipc->queue[QUEUE_PRIO_IRQ].tx_buf,
			SMSG_IRQ_TXBUF_SIZE,
			base + SMSG_IRQ_TXBUF_ADDR,
			base + SMSG_IRQ_TXBUF_RDPTR,
			base + SMSG_IRQ_TXBUF_WRPTR);

	sipc_init_smsg_queue_buf(&ipc->queue[QUEUE_PRIO_IRQ].rx_buf,
			SMSG_IRQ_RXBUF_SIZE,
			base + SMSG_IRQ_RXBUF_ADDR,
			base + SMSG_IRQ_RXBUF_RDPTR,
			base + SMSG_IRQ_RXBUF_WRPTR);

/*prio msg deq init*/
	sipc_init_smsg_queue_buf(&ipc->queue[QUEUE_PRIO_HIGH].tx_buf,
			SMSG_PRIO_TXBUF_SIZE,
			base + SMSG_PRIO_TXBUF_ADDR,
			base + SMSG_PRIO_TXBUF_RDPTR,
			base + SMSG_PRIO_TXBUF_WRPTR);

	sipc_init_smsg_queue_buf(&ipc->queue[QUEUE_PRIO_HIGH].rx_buf,
			SMSG_PRIO_RXBUF_SIZE,
			base + SMSG_PRIO_RXBUF_ADDR,
			base + SMSG_PRIO_RXBUF_RDPTR,
			base + SMSG_PRIO_RXBUF_WRPTR);

/*normal msg deq init*/
	sipc_init_smsg_queue_buf(&ipc->queue[QUEUE_PRIO_NORMAL].tx_buf,
			SMSG_TXBUF_SIZE,
			base + SMSG_TXBUF_ADDR,
			base + SMSG_TXBUF_RDPTR,
			base + SMSG_TXBUF_WRPTR);

	sipc_init_smsg_queue_buf(&ipc->queue[QUEUE_PRIO_NORMAL].rx_buf,
			SMSG_RXBUF_SIZE,
			base + SMSG_RXBUF_ADDR,
			base + SMSG_RXBUF_RDPTR,
			base + SMSG_RXBUF_WRPTR);

return ipc;
}

void smsg_clear_queue_buf(struct smsg_queue *queue)
{
	sci_write32(queue->tx_buf.rdptr, 0);
	sci_write32(queue->tx_buf.wrptr, 0);
	sci_write32(queue->rx_buf.rdptr, 0);
	sci_write32(queue->rx_buf.wrptr, 0);
}

void smsg_clear_queue(struct smsg_ipc *ipc, int prio)
{
	struct smsg_queue *queue;

	if (prio >= QUEUE_PRIO_MAX) {
		LOG_ERR("Invalid queue priority %d.\n", prio);
		return;
	}

	queue = &ipc->queue[prio];

	smsg_clear_queue_buf(queue);
}

extern void sblock_process(struct smsg *msg);
int smsg_msg_dispatch_thread(int argc, char *argv[])
{
	int prio;
	struct smsg_ipc *ipc = &smsg_ipcs[0];
	struct smsg *msg;
	struct smsg recv_smsg;
	struct smsg_channel *ch;
	uintptr_t rxpos;
	struct smsg_queue_buf *rx_buf;

	while (1) {
		k_sem_take(&ipc->irq_sem, K_FOREVER);

		for (prio = QUEUE_PRIO_IRQ; prio < QUEUE_PRIO_MAX; prio++) {
			rx_buf = &(ipc->queue[prio].rx_buf);
			if (sys_read32(rx_buf->wrptr) !=
					sys_read32(rx_buf->rdptr))
				break;
		}

		while (sys_read32(rx_buf->wrptr) != sys_read32(rx_buf->rdptr)) {
			rxpos = (sys_read32(rx_buf->rdptr) & (rx_buf->size - 1))
				* sizeof(struct smsg) + rx_buf->addr;

			msg = (struct smsg *)rxpos;

			memcpy(&recv_smsg, msg, sizeof(struct smsg));
			sys_write32(sys_read32(rx_buf->rdptr) + 1,
					rx_buf->rdptr);
			LOG_DBG("read smsg: channel=%d, type=%d, flag=0x%04x, value=0x%08x %d %d\n",
					msg->channel, msg->type, msg->flag,
					msg->value, sys_read32(rx_buf->wrptr),
					sys_read32(rx_buf->rdptr));
			if (recv_smsg.channel >= SMSG_CH_NR
					|| recv_smsg.type >= SMSG_TYPE_NR
					|| SMSG_TYPE_DIE == recv_smsg.type) {
				LOG_ERR("invalid smsg: channel=%d, type=%d",
					recv_smsg.channel, recv_smsg.type);
				continue;
			}
			if (recv_smsg.type == SMSG_TYPE_WIFI_IRQ) {
				if (recv_smsg.flag == SMSG_WIFI_IRQ_OPEN) {
					sprd_wifi_irq_enable_num(
						recv_smsg.value);
					LOG_DBG("wifi irq %d open\n",
					recv_smsg.value);
				} else if (recv_smsg.flag ==
						SMSG_WIFI_IRQ_CLOSE) {
					sprd_wifi_irq_disable_num(
						recv_smsg.value);
					LOG_DBG("wifi irq %d close\n",
					recv_smsg.value);
				}
				continue;
			}
			ch = &ipc->channels[recv_smsg.channel];

			sblock_process(&recv_smsg);
		}
	}

}

int smsg_ipc_destroy(u8_t dst)
{
	struct smsg_ipc *ipc = &smsg_ipcs[dst];

	k_thread_abort(ipc->pid);

	return 0;
}

int smsg_ch_open(u8_t dst, u8_t channel, int prio, int timeout)
{
	struct smsg_ipc *ipc = &smsg_ipcs[dst];
	struct smsg_channel *ch;
	struct smsg mopen;

	int ret = 0;

	LOG_DBG("open dst %d channel %d", dst, channel);
	if (!ipc) {
		LOG_ERR("get ipc %d failed.\n", dst);
		return -ENODEV;
	}

	ch = &ipc->channels[channel];
	if (ch->state != CHAN_STATE_UNUSED) {
		LOG_ERR("ipc channel %d had been opened.\n", channel);
		return -ENODEV;
	}

	k_sem_init(&ch->rxsem, 0, 1);
	k_mutex_init(&ch->rxlock);
	k_mutex_init(&ch->txlock);

	smsg_set(&mopen, channel, SMSG_TYPE_OPEN, SMSG_OPEN_MAGIC, 0);
	ret = smsg_send(dst, prio, &mopen, timeout);
	if (ret != 0) {
		LOG_WRN("smsg send error, errno %d!\n", ret);
		ch->state = CHAN_STATE_UNUSED;

		return ret;
	}
	LOG_DBG("send open success");

	ch->state = CHAN_STATE_OPENED;
	LOG_DBG("open channel success");

	return 0;
}

int smsg_ch_close(u8_t dst, u8_t channel, int prio, int timeout)
{
	struct smsg_ipc *ipc = &smsg_ipcs[dst];
	struct smsg_channel *ch = &ipc->channels[channel];
	struct smsg mclose;

	smsg_set(&mclose, channel, SMSG_TYPE_CLOSE, SMSG_CLOSE_MAGIC, 0);
	smsg_send(dst, prio, &mclose, timeout);

	ch->state = CHAN_STATE_FREE;

	/* finally, update the channel state*/
	ch->state = CHAN_STATE_UNUSED;

	return 0;
}

int smsg_send_irq(u8_t dst, struct smsg *msg)
{
	struct smsg_ipc *ipc = &smsg_ipcs[dst];
	struct smsg_queue_buf *tx_buf;
	uintptr_t txpos;
	int ret = 0;

	if (!ipc) {
		return -ENODEV;
	}

	tx_buf = &ipc->queue[QUEUE_PRIO_IRQ].tx_buf;

	if (sys_read32(tx_buf->wrptr) - sys_read32(tx_buf->rdptr)
		>= tx_buf->size) {
		LOG_DBG("smsg irq txbuf is full! %d %d %d\n",
			sys_read32(tx_buf->wrptr),
			sys_read32(tx_buf->rdptr), msg->value);
		ret = -EBUSY;
		goto send_failed;
	}
	/* calc txpos and write smsg */
	txpos = (sys_read32(tx_buf->wrptr) & (tx_buf->size - 1)) *
		sizeof(struct smsg) + tx_buf->addr;
	memcpy((void *)txpos, msg, sizeof(struct smsg));
	/*
	 *  ipc_info("write smsg: wrptr=%u, rdptr=%u, txpos=0x%lx %d\n",
	 *	sys_read32(ipc->queue_irq.tx_buf.wrptr),
	 *	sys_read32(ipc->queue_irq.tx_buf.rdptr), txpos,msg->value);
	 */

	/* update wrptr */
	sys_write32(sys_read32(tx_buf->wrptr) + 1, tx_buf->wrptr);
	uwp_ipi_trigger(IPI_CORE_BTWF, IPI_TYPE_IRQ0);

send_failed:
	return ret;
}

int smsg_send(u8_t dst, u8_t prio, struct smsg *msg, int timeout)
{
	struct smsg_channel *ch;
	struct smsg_queue_buf *tx_buf;
	struct smsg_ipc *ipc = &smsg_ipcs[dst];
	u32_t txpos;
	int ret = 0;

	ch = &ipc->channels[msg->channel];

	if (ch->state != CHAN_STATE_OPENED &&
			msg->type != SMSG_TYPE_OPEN &&
			msg->type != SMSG_TYPE_CLOSE &&
			msg->type != SMSG_TYPE_DONE &&
			msg->channel != SMSG_CH_IRQ_DIS) {
		LOG_WRN("channel %d not opened!\n", msg->channel);
		return -EINVAL;
	}

	if (prio >= QUEUE_PRIO_MAX) {
		LOG_ERR("Invalid queue priority %d.\n", prio);
		return -EINVAL;
	}

	tx_buf = &(ipc->queue[prio].tx_buf);

	LOG_DBG("%d smsg txbuf wr %d rd %d!\n", prio,
			sys_read32(tx_buf->wrptr),
			sys_read32(tx_buf->rdptr));

	if ((int)(sys_read32(tx_buf->wrptr)	- sys_read32(tx_buf->rdptr))
			>= tx_buf->size) {
		LOG_WRN("smsg txbuf is full! %d %d\n",
				sys_read32(tx_buf->wrptr),
				sys_read32(tx_buf->rdptr));

		return -EBUSY;
	}

	k_mutex_lock(&ch->txlock, K_FOREVER);
	/* calc txpos and write smsg */
	txpos = (sys_read32(tx_buf->wrptr) & (tx_buf->size - 1)) *
		sizeof(struct smsg) + tx_buf->addr;
	memcpy((void *)txpos, msg, sizeof(struct smsg));

	/*
	 *  LOG_DBG("write smsg: wrptr=%u, rdptr=%u, txpos=0x%x\n",
	 *		sys_read32(tx_buf->wrptr),
	 *		sys_read32(tx_buf->rdptr), txpos);
	 */

	/* update wrptr */
	sys_write32(sys_read32(tx_buf->wrptr) + 1, tx_buf->wrptr);
	k_mutex_unlock(&ch->txlock);

	uwp_ipi_trigger(IPI_CORE_BTWF, IPI_TYPE_IRQ0);

	return ret;
}

static void smsg_irq_handler(void *arg)
{
	struct smsg_ipc *ipc = &smsg_ipcs[0];

	irq_disable(NVIC_INT_GNSS2BTWF_IPI);

	uwp_ipi_clear_remote(IPI_CORE_BTWF, IPI_TYPE_IRQ0);

	k_sem_give(&ipc->irq_sem);

	irq_enable(NVIC_INT_GNSS2BTWF_IPI);
}

int smsg_init(u32_t dst, u32_t smsg_base)
{
	struct smsg_ipc *ipc = &smsg_ipcs[dst];

	LOG_INF("smsg init dst %d addr 0x%x.", dst, smsg_base);

	uwp_sys_enable(BIT(APB_EB_IPI));
	uwp_sys_reset(BIT(APB_EB_IPI));
	IRQ_CONNECT(NVIC_INT_GNSS2BTWF_IPI, 5,
				smsg_irq_handler,
				NVIC_INT_GNSS2BTWF_IPI, 0);
	irq_enable(NVIC_INT_GNSS2BTWF_IPI);

	smsg_set_addr(ipc, smsg_base);
	smsg_clear_queue(ipc, QUEUE_PRIO_NORMAL);
	smsg_clear_queue(ipc, QUEUE_PRIO_HIGH);
	smsg_clear_queue(ipc, QUEUE_PRIO_IRQ);

	k_sem_init(&ipc->irq_sem, 0, 1);

	ipc->pid = k_thread_create(&smsg_thread,
			smsg_stack, SMSG_STACK_SIZE,
			(k_thread_entry_t)smsg_msg_dispatch_thread,
			NULL, NULL, NULL,
			 K_PRIO_COOP(7), 0, 0);

	return 0;
}
