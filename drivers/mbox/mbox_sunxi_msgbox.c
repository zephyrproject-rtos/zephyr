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
 * RD_IRQ_STATUS is write-one-to-clear, and the hardware only honours the clear
 * once the channel FIFO is empty. This makes the drain-then-clear sequence in
 * the ISR race free: a message that arrives after the drain loop keeps the
 * pending bit asserted.
 *
 * Clock gate: CCU MSGBOX_BGR_REG (CCU + 0x071C): bit1 = MSGBOX1 gating (riscv),
 * bit17 = MSGBOX1 reset de-assert. Both must be 1 before use. The A7 only
 * enables MSGBOX0, so the E907 enables its own gate here.
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

#define CHANS        4
#define FIFO_DEPTH   8
/* Upper bound for a blocking send; the FIFO is 8 entries deep and the peer
 * drains it from an interrupt, so a millisecond is already very generous.
 */
#define SEND_TIMEOUT K_MSEC(1)

struct mbox_sunxi_config {
	mm_reg_t rx_base;  /* this processor's MSGBOX block  */
	mm_reg_t tx_base;  /* the peer (CPUX) block          */
	uint32_t ccu_base; /* CCU base, for MSGBOX_BGR       */
	uint32_t ccu_bgr_off;
	uint32_t ccu_riscv_bits; /* gating|reset bits for MSGBOX1 */
	void (*irq_config)(void);
};

struct mbox_sunxi_data {
	mbox_callback_t cb[CHANS];
	void *user_data[CHANS];
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
	k_timepoint_t end;
	uint32_t word;

	if (chan >= CHANS) {
		return -EINVAL;
	}
	if (msg == NULL || msg->data == NULL || msg->size != sizeof(word)) {
		return -EINVAL; /* this controller carries one 32-bit word */
	}
	/* msg->data is not guaranteed to be 32-bit aligned. */
	memcpy(&word, msg->data, sizeof(word));

	end = sys_timepoint_calc(SEND_TIMEOUT);
	do {
		if ((sys_read32(cfg->tx_base + MSG_MSG_STATUS(0, chan)) & MSG_NUM_MASK) <
		    FIFO_DEPTH) {
			sys_write32(word, cfg->tx_base + MSG_FIFO(0, chan));
			return 0;
		}
		k_busy_wait(1);
	} while (!sys_timepoint_expired(end));

	return -EBUSY;
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
	uint32_t bit;

	if (chan >= CHANS) {
		return -EINVAL;
	}
	bit = MSG_RD_IRQ_BIT(chan);
	sunxi_rmw(cfg->rx_base + MSG_RD_IRQ_EN(0), bit, enable ? bit : 0);
	return 0;
}

static void sunxi_msgbox_isr(const struct device *dev)
{
	const struct mbox_sunxi_config *cfg = dev->config;
	struct mbox_sunxi_data *data = dev->data;
	uint32_t status = sys_read32(cfg->rx_base + MSG_RD_IRQ_STATUS(0));

	for (int p = 0; p < CHANS; p++) {
		if (!(status & MSG_RD_IRQ_BIT(p))) {
			continue;
		}

		while ((sys_read32(cfg->rx_base + MSG_MSG_STATUS(0, p)) & MSG_NUM_MASK) != 0) {
			uint32_t word = sys_read32(cfg->rx_base + MSG_FIFO(0, p));
			struct mbox_msg m = {.data = &word, .size = sizeof(word)};

			if (data->cb[p] != NULL) {
				data->cb[p](dev, p, data->user_data[p], &m);
			}
		}

		/* Write-one-to-clear. Do not read-modify-write the status
		 * register: that could clear a bit raised by another channel
		 * between the read and the write.
		 */
		sys_write32(MSG_RD_IRQ_BIT(p), cfg->rx_base + MSG_RD_IRQ_STATUS(0));
	}
}

static int sunxi_msgbox_init(const struct device *dev)
{
	const struct mbox_sunxi_config *cfg = dev->config;

	/* Enable MSGBOX1 clock gate + de-assert its reset (CCU + 0x071C). */
	if (cfg->ccu_base != 0U) {
		sunxi_rmw(cfg->ccu_base + cfg->ccu_bgr_off, cfg->ccu_riscv_bits,
			  cfg->ccu_riscv_bits);
	}

	/* Start from a clean state: drop stale FIFO entries and pending IRQs. */
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
		.ccu_base = DT_INST_PROP_OR(inst, allwinner_ccu_base, 0),                          \
		.ccu_bgr_off = DT_INST_PROP_OR(inst, allwinner_ccu_bgr_off, 0x71c),                \
		.ccu_riscv_bits = DT_INST_PROP_OR(inst, allwinner_ccu_riscv_bits, 0x20002),        \
		.irq_config = sunxi_msgbox_irq_config_##inst,                                      \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, sunxi_msgbox_init, NULL, &mbox_sunxi_data_##inst,              \
			      &mbox_sunxi_config_##inst, PRE_KERNEL_1,                             \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &sunxi_msgbox_api);

DT_INST_FOREACH_STATUS_OKAY(SUNXI_MSGBOX_INIT)
