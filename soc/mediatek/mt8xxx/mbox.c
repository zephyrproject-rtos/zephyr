/* Copyright 2023 The ChromiumOS Authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/devicetree.h>
#include <soc.h>

#define DT_DRV_COMPAT mediatek_mbox

/* Mailbox: a simple interrupt source.  Each direction has a 5-bit
 * command register and will latch an interrupt if any of the bits are
 * non-zero.  The interrupt bits get cleared/acknowledged by writing
 * ones to the corresponding bits of "cmd_clear".  There are five
 * scratch registers for use as message data in each direction.
 *
 * The same device is mapped at the same address by the host and DSP,
 * and the naming is from the perspective of the DSP: the "in"
 * registers control interrupts on the DSP, the "out" registers are
 * for transmitting data to the host.
 *
 * There is an array of the devices.  Linux's device-tree defines two.
 * SOF uses those for IPC, but also implements platform_trace_point()
 * using the third (no linux driver though?).  The upstream headers
 * list interrupts for FIVE, and indeed those all seem to be present
 * and working.
 *
 * In practice: The first device (mbox0) is for IPC commands in both
 * directions.  The cmd register is written with a 1 ("IPI_OP_REQ")
 * and the command is placed in shared DRAM.
 *
 * Note that the 8195 has ten "msg" registers.  These only exist on
 * this version of the hardware, and act as uninspected r/w scratch
 * registers to both the host and DSP, with no other behavior.  They
 * aren't used by the Linux kernel at all (which has a single driver
 * for all these DSPs), so are described here for completeness but
 * otherwise ignored.  In practice they don't do anything that simple
 * shared memory can't.
 */

struct mtk_mbox {
#ifdef SOC_SERIES_MT8195
	uint32_t in_cmd;
	uint32_t in_cmd_clr;
	uint32_t in_msg[5];
	uint32_t out_cmd;
	uint32_t out_cmd_clr;
	uint32_t out_msg[5];
#else
	uint32_t in_cmd;
	uint32_t out_cmd;
	uint32_t in_cmd_clr;
	uint32_t out_cmd_clr;
#endif
};

struct mbox_cfg {
	volatile struct mtk_mbox *mbox;
};

struct mbox_data {
	mtk_adsp_mbox_handler_t handlers[MTK_ADSP_MBOX_CHANNELS];
	void *handler_arg[MTK_ADSP_MBOX_CHANNELS];
};

void mtk_adsp_mbox_set_handler(const struct device *mbox, uint32_t chan,
			       mtk_adsp_mbox_handler_t handler, void *arg)
{
	struct mbox_data *data = ((struct device *)mbox)->data;

	if (chan < MTK_ADSP_MBOX_CHANNELS) {
		data->handlers[chan] = handler;
		data->handler_arg[chan] = arg;
	}
}

void mtk_adsp_mbox_signal(const struct device *mbox, uint32_t chan)
{
	const struct mbox_cfg *cfg = ((struct device *)mbox)->config;

	if (chan < MTK_ADSP_MBOX_CHANNELS) {
		cfg->mbox->out_cmd |= BIT(chan);
	}
}

#define DEF_DEVPTR(N) DEVICE_DT_INST_GET(N),
const struct device * const mbox_devs[] = {
	DT_INST_FOREACH_STATUS_OKAY(DEF_DEVPTR)
};

static void mbox_handle(const void *arg)
{
	const struct mbox_cfg *cfg = ((struct device *)arg)->config;
	struct mbox_data *data = ((struct device *)arg)->data;

	for (int i = 0; i < MTK_ADSP_MBOX_CHANNELS; i++) {
		if (cfg->mbox->in_cmd & BIT(i)) {
			if (data->handlers[i] != NULL) {
				data->handlers[i](arg, data->handler_arg[i]);
			}
		}
	}

	cfg->mbox->in_cmd_clr = cfg->mbox->in_cmd; /* ACK */
}

static void mbox_isr(const void *arg)
{
	for (int i = 0; i < ARRAY_SIZE(mbox_devs); i++) {
		mbox_handle(mbox_devs[i]);
	}
}

#define DEF_IRQ(N)							\
	{ IRQ_CONNECT(DT_INST_IRQN(N), 0, mbox_isr, DEVICE_DT_INST_GET(N), 0); \
	  irq_enable(DT_INST_IRQN(N)); }

static int mbox_init(void)
{
	DT_INST_FOREACH_STATUS_OKAY(DEF_IRQ);
	return 0;
}

SYS_INIT(mbox_init, POST_KERNEL, 0);

#define DEF_DEV(N)							\
	static struct mbox_data dev_data##N;				\
	static const struct mbox_cfg dev_cfg##N =			\
		{ .mbox = (void *)DT_INST_REG_ADDR(N), };		\
	DEVICE_DT_INST_DEFINE(N, NULL, NULL, &dev_data##N, &dev_cfg##N,	\
			      POST_KERNEL, 0, NULL);

DT_INST_FOREACH_STATUS_OKAY(DEF_DEV)
