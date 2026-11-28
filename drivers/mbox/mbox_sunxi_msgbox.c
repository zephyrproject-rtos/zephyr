/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Allwinner V851S MSGBOX driver, RISC-V (E907) side.
 *
 * Hardware (V851S&V851SE Datasheet 1.0, sec 3.15):
 *   MBOX(CPUX)  @ 0x03003000   base for the Cortex-A7
 *   MBOX(RISC)  @ 0x06020000   base for the E907 (this driver)
 *   N (remote)  = 0            (only one peer)
 *   P (channel) = 0..3         (4 bidirectional channels, 8-deep FIFO each)
 *
 * Per-block registers (offset = N*0x100, plus P*0x4 for the per-channel ones):
 *   RD_IRQ_EN    0x20   RD_IRQ_STATUS 0x24
 *   WR_IRQ_EN    0x30   WR_IRQ_STATUS 0x34
 *   FIFO_STATUS  0x50   MSG_STATUS    0x60   MSG (FIFO data) 0x70
 *   WR_THRESHOLD 0x80
 *   Bit per channel: RX pending/enable = 2P, TX = 2P+1.
 *
 * Signalling model (same one the A7 sunxi-msgbox driver uses):
 *   - To RX: read this processor's block MSG(P); an RD_IRQ fires on new data.
 *   - To TX: write the PEER's block MSG(P) (0x03003000 here); that raises the
 *     peer's RD_IRQ. Check the peer's MSG_STATUS before writing so a full FIFO
 *     does not silently drop the message.
 *
 * The controller has no doorbell register separate from the FIFO, so a
 * signal-only message (NULL payload, as sent by ipm_send() and rpmsg_service)
 * is transmitted as a single zero word. The peer treats such a word the same
 * way as any other notification: it only ever uses the interrupt. The FIFO
 * transfer unit is one 32-bit word, so a data message must be exactly four
 * bytes long.
 *
 * RD_IRQ_STATUS is write-one-to-clear, and the hardware only honours the clear
 * once the channel FIFO is empty. This makes the drain-then-clear sequence in
 * the ISR race free: a message that arrives after the drain loop keeps the
 * pending bit asserted.
 *
 * Clock gate: the A7 owns the CCU. Its remoteproc driver enables the MSGBOX1
 * gate and de-asserts the block reset (CCU MSGBOX_BGR_REG bits 1 and 17)
 * before releasing the E907, so this driver does not touch the CCU.
 */

#define DT_DRV_COMPAT allwinner_sunxi_msgbox_riscv

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(mbox_sunxi_msgbox, CONFIG_MBOX_LOG_LEVEL);

/* Register offsets inside one MSGBOX block */
#define MSG_RD_IRQ_EN(n)     (0x20 + (n) * 0x100)
#define MSG_RD_IRQ_STATUS(n) (0x24 + (n) * 0x100)
#define MSG_MSG_STATUS(n, p) (0x60 + (n) * 0x100 + (p) * 4)
#define MSG_FIFO(n, p)       (0x70 + (n) * 0x100 + (p) * 4)

#define MSG_NUM_MASK      0xf
#define MSG_RD_IRQ_BIT(p) BIT((p) * 2)

#define CHANS      4
#define FIFO_DEPTH 8

struct mbox_sunxi_config {
	mm_reg_t rx_base; /* this processor's MSGBOX block  */
	mm_reg_t tx_base; /* the peer (CPUX) block          */
	void (*irq_config)(void);
};

struct mbox_sunxi_data {
	mbox_callback_t cb[CHANS];
	void *user_data[CHANS];
	bool enabled[CHANS];
};

static inline uint32_t sunxi_rmw(mm_reg_t addr, uint32_t mask, uint32_t val)
{
	uint32_t v = sys_read32(addr);

	v = (v & ~mask) | (val & mask);
	sys_write32(v, addr);
	return v;
}

static int sunxi_msgbox_send(const struct device *dev, mbox_channel_id_t chan,
			     const struct mbox_msg *msg)
{
	const struct mbox_sunxi_config *cfg = dev->config;
	uint32_t word = 0;

	if (chan >= CHANS) {
		return -EINVAL;
	}

	/* Signal-only messages (no payload) have no doorbell of their own on
	 * this controller, so they are sent as a zero word. Otherwise the
	 * message must be exactly one FIFO word.
	 */
	if (msg != NULL && msg->size != 0) {
		if (msg->data == NULL || msg->size != sizeof(word)) {
			return -EMSGSIZE;
		}
		/* msg->data is not guaranteed to be 32-bit aligned. */
		memcpy(&word, msg->data, sizeof(word));
	}

	/* Non-blocking, as the mailbox API expects: a full FIFO means the peer
	 * is not draining it, and the caller (which may be an ISR) decides
	 * whether to retry.
	 */
	if ((sys_read32(cfg->tx_base + MSG_MSG_STATUS(0, chan)) & MSG_NUM_MASK) >= FIFO_DEPTH) {
		return -EBUSY;
	}

	sys_write32(word, cfg->tx_base + MSG_FIFO(0, chan));
	return 0;
}

static int sunxi_msgbox_register_cb(const struct device *dev, mbox_channel_id_t chan,
				    mbox_callback_t cb, void *user_data)
{
	struct mbox_sunxi_data *data = dev->data;

	if (chan >= CHANS) {
		return -EINVAL;
	}
	data->cb[chan] = cb;
	data->user_data[chan] = user_data;
	return 0;
}

static int sunxi_msgbox_mtu_get(const struct device *dev)
{
	ARG_UNUSED(dev);
	return sizeof(uint32_t);
}

static uint32_t sunxi_msgbox_max_channels_get(const struct device *dev)
{
	ARG_UNUSED(dev);
	return CHANS;
}

static int sunxi_msgbox_set_enabled(const struct device *dev, mbox_channel_id_t chan, bool enable)
{
	const struct mbox_sunxi_config *cfg = dev->config;
	struct mbox_sunxi_data *data = dev->data;
	uint32_t bit;

	if (chan >= CHANS) {
		return -EINVAL;
	}
	if (data->enabled[chan] == enable) {
		return -EALREADY;
	}
	bit = MSG_RD_IRQ_BIT(chan);
	sunxi_rmw(cfg->rx_base + MSG_RD_IRQ_EN(0), bit, enable ? bit : 0);
	data->enabled[chan] = enable;
	return 0;
}

static void sunxi_msgbox_isr(const struct device *dev)
{
	const struct mbox_sunxi_config *cfg = dev->config;
	struct mbox_sunxi_data *data = dev->data;
	uint32_t status;

	/* Drain and clear, then look again: a message can arrive between the
	 * last FIFO read and the status write. The bound keeps a peer that
	 * floods the FIFO from holding the CPU in the ISR forever.
	 */
	for (int round = 0; round < FIFO_DEPTH; round++) {
		status = sys_read32(cfg->rx_base + MSG_RD_IRQ_STATUS(0));
		if (status == 0U) {
			break;
		}

		for (int p = 0; p < CHANS; p++) {
			if (!(status & MSG_RD_IRQ_BIT(p))) {
				continue;
			}

			while ((sys_read32(cfg->rx_base + MSG_MSG_STATUS(0, p)) & MSG_NUM_MASK) !=
			       0) {
				uint32_t word = sys_read32(cfg->rx_base + MSG_FIFO(0, p));
				struct mbox_msg m = {.data = &word, .size = sizeof(word)};

				if (data->cb[p] != NULL) {
					data->cb[p](dev, p, data->user_data[p], &m);
				}
			}

			/* Write-one-to-clear. Do not read-modify-write the status
			 * register: that could clear a bit raised by another
			 * channel between the read and the write.
			 */
			sys_write32(MSG_RD_IRQ_BIT(p), cfg->rx_base + MSG_RD_IRQ_STATUS(0));
		}
	}
}

static int sunxi_msgbox_init(const struct device *dev)
{
	const struct mbox_sunxi_config *cfg = dev->config;

	/* Start from a clean state: no RX interrupts, drop stale FIFO entries
	 * and pending IRQs. The A7 has already ungated the block.
	 */
	sys_write32(0, cfg->rx_base + MSG_RD_IRQ_EN(0));
	for (int p = 0; p < CHANS; p++) {
		while ((sys_read32(cfg->rx_base + MSG_MSG_STATUS(0, p)) & MSG_NUM_MASK) != 0) {
			(void)sys_read32(cfg->rx_base + MSG_FIFO(0, p));
		}
		sys_write32(MSG_RD_IRQ_BIT(p), cfg->rx_base + MSG_RD_IRQ_STATUS(0));
	}

	cfg->irq_config();
	return 0;
}

static DEVICE_API(mbox, sunxi_msgbox_api) = {
	.send = sunxi_msgbox_send,
	.register_callback = sunxi_msgbox_register_cb,
	.mtu_get = sunxi_msgbox_mtu_get,
	.max_channels_get = sunxi_msgbox_max_channels_get,
	.set_enabled = sunxi_msgbox_set_enabled,
};

/* PRE_KERNEL_2, not PRE_KERNEL_1: the CLIC driver runs at PRE_KERNEL_1 with the
 * same priority (CONFIG_INTC_INIT_PRIORITY == CONFIG_MBOX_INIT_PRIORITY == 40),
 * and this init enables its interrupt.
 */
#define SUNXI_MSGBOX_INIT(inst)                                                                    \
	static struct mbox_sunxi_data mbox_sunxi_data_##inst;                                      \
	static void sunxi_msgbox_irq_config_##inst(void)                                           \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), sunxi_msgbox_isr,     \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}                                                                                          \
	static const struct mbox_sunxi_config mbox_sunxi_config_##inst = {                         \
		.rx_base = DT_INST_REG_ADDR_BY_NAME(inst, rx),                                     \
		.tx_base = DT_INST_REG_ADDR_BY_NAME(inst, tx),                                     \
		.irq_config = sunxi_msgbox_irq_config_##inst,                                      \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, sunxi_msgbox_init, NULL, &mbox_sunxi_data_##inst,              \
			      &mbox_sunxi_config_##inst, PRE_KERNEL_2, CONFIG_MBOX_INIT_PRIORITY,  \
			      &sunxi_msgbox_api);

DT_INST_FOREACH_STATUS_OKAY(SUNXI_MSGBOX_INIT)
