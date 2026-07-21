/* Copyright 2025 Mediatek
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/devicetree.h>
#include <soc.h>

#define DT_DRV_COMPAT mediatek_ipi

#define CPU2DSP_IRQ		BIT(0)
#define DSP2CPU_IRQ		BIT(1)
#define DSP2SPM_IRQ_B		BIT(2)

struct mtk_ipi {
	uint32_t int2cirq;
	uint32_t int_pol;
	uint32_t int_en;
	uint32_t int_status;
};

struct ipi_cfg {
	volatile struct mtk_ipi *ipi;
};

struct ipi_data {
	mtk_adsp_ipi_handler_t handler;
	void *handler_arg;
};

void mtk_adsp_ipi_set_handler(const struct device *ipi, uint32_t chan,
			       mtk_adsp_ipi_handler_t handler, void *arg)
{
	struct ipi_data *data = ((struct device *)ipi)->data;

	data->handler = handler;
	data->handler_arg = arg;
}

void mtk_adsp_ipi_signal(const struct device *ipi, uint32_t op)
{
	const struct ipi_cfg *cfg = ((struct device *)ipi)->config;

	/* wakeup CPU */
	cfg->ipi->int2cirq &= ~(DSP2SPM_IRQ_B);
	/* trigger IPI IRQ to CPU */
	cfg->ipi->int2cirq |= DSP2CPU_IRQ;
}

#define DEF_DEVPTR(N) DEVICE_DT_INST_GET(N),
const struct device * const ipi_dev = {
	DEF_DEVPTR(0)
};

static void ipi_handle(const void *arg)
{
	const struct ipi_cfg *cfg = ((struct device *)arg)->config;
	struct ipi_data *data = ((struct device *)arg)->data;

	if (data->handler != NULL) {
		data->handler(arg, data->handler_arg);
	}
	/* clear IPI IRQ from CPU */
	cfg->ipi->int2cirq &= ~(CPU2DSP_IRQ);
}

static void ipi_isr(const void *arg)
{
	ipi_handle(ipi_dev);
}

#define DEF_IRQ(N)							\
	{ IRQ_CONNECT(DT_INST_IRQN(N), 0, ipi_isr, DEVICE_DT_INST_GET(N), 0); \
	  irq_enable(DT_INST_IRQN(N)); }

static int ipi_init(void)
{
	DEF_IRQ(0);
	return 0;
}

SYS_INIT(ipi_init, POST_KERNEL, 0);

#define DEF_DEV(N)							\
	static struct ipi_data dev_data##N;				\
	static const struct ipi_cfg dev_cfg##N =			\
		{ .ipi = (void *)DT_INST_REG_ADDR(N), };		\
	DEVICE_DT_INST_DEFINE(N, NULL, NULL, &dev_data##N, &dev_cfg##N,	\
			      POST_KERNEL, 0, NULL);

DEF_DEV(0);
