/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_ps2

#include <errno.h>
#include <device.h>
#include <drivers/ps2.h>
#include <soc.h>
#include <logging/log.h>

#define LOG_LEVEL CONFIG_PS2_LOG_LEVEL
LOG_MODULE_REGISTER(ps2_mchp_xec);

/* in 50us units */
#define PS2_TIMEOUT 10000

struct ps2_xec_config {
	PS2_Type *base;
	uint8_t girq_id;
	uint8_t girq_bit;
	uint8_t isr_nvic;
};

struct ps2_xec_data {
	ps2_callback_t callback_isr;
	struct k_sem tx_lock;
};

static int ps2_xec_configure(struct device *dev, ps2_callback_t callback_isr)
{
	const struct ps2_xec_config *config = dev->config;
	struct ps2_xec_data *data = dev->data;
	PS2_Type *base = config->base;

	uint8_t  __attribute__((unused)) dummy;

	if (!callback_isr) {
		return -EINVAL;
	}

	data->callback_isr = callback_isr;

	/* In case the self test for a PS2 device already finished and
	 * set the SOURCE bit to 1 we clear it before enabling the
	 * interrupts. Instances must be allocated before the BAT or
	 * the host may time out.
	 */
	MCHP_GIRQ_SRC(config->girq_id) = BIT(config->girq_bit);
	dummy = base->TRX_BUFF;
	base->STATUS = MCHP_PS2_STATUS_RW1C_MASK;

	/* Enable FSM and init instance in rx mode*/
	base->CTRL = MCHP_PS2_CTRL_EN_POS;

	/* We enable the interrupts in the EC aggregator so that the
	 * result  can be forwarded to the ARM NVIC
	 */
	MCHP_GIRQ_ENSET(config->girq_id) = BIT(config->girq_bit);

	k_sem_give(&data->tx_lock);

	return 0;
}


static int ps2_xec_write(struct device *dev, uint8_t value)
{
	const struct ps2_xec_config *config = dev->config;
	struct ps2_xec_data *data = dev->data;
	PS2_Type *base = config->base;
	int i = 0;

	uint8_t  __attribute__((unused)) dummy;

	if (k_sem_take(&data->tx_lock, K_NO_WAIT)) {
		return -EACCES;
	}
	/* Allow the PS2 controller to complete a RX transaction. This
	 * is because the channel may be actively receiving data.
	 * In addition, it is necessary to wait for a previous TX
	 * transaction to complete. The PS2 block has a single
	 * FSM.
	 */
	while (((base->STATUS &
		(MCHP_PS2_STATUS_RX_BUSY | MCHP_PS2_STATUS_TX_IDLE))
		!= MCHP_PS2_STATUS_TX_IDLE) && (i < PS2_TIMEOUT)) {
		k_busy_wait(50);
		i++;
	}

	if (unlikely(i == PS2_TIMEOUT)) {
		LOG_DBG("PS2 write timed out");
		return -ETIMEDOUT;
	}

	/* Inhibit ps2 controller and clear status register */
	base->CTRL = 0x00;

	/* Read to clear data ready bit in the status register*/
	dummy = base->TRX_BUFF;
	k_sleep(K_MSEC(1));
	base->STATUS = MCHP_PS2_STATUS_RW1C_MASK;

	/* Switch the interface to TX mode and enable state machine */
	base->CTRL = MCHP_PS2_CTRL_TR_TX | MCHP_PS2_CTRL_EN;

	/* Write value to TX/RX register */
	base->TRX_BUFF = value;

	k_sem_give(&data->tx_lock);

	return 0;
}

static int ps2_xec_inhibit_interface(struct device *dev)
{
	const struct ps2_xec_config *config = dev->config;
	struct ps2_xec_data *data = dev->data;
	PS2_Type *base = config->base;

	if (k_sem_take(&data->tx_lock, K_MSEC(10)) != 0) {
		return -EACCES;
	}

	base->CTRL = 0x00;
	MCHP_GIRQ_SRC(config->girq_id) = BIT(config->girq_bit);
	NVIC_ClearPendingIRQ(config->isr_nvic);

	k_sem_give(&data->tx_lock);

	return 0;
}

static int ps2_xec_enable_interface(struct device *dev)
{
	const struct ps2_xec_config *config = dev->config;
	struct ps2_xec_data *data = dev->data;
	PS2_Type *base = config->base;

	MCHP_GIRQ_SRC(config->girq_id) = BIT(config->girq_bit);
	base->CTRL = MCHP_PS2_CTRL_EN;

	k_sem_give(&data->tx_lock);

	return 0;
}
static void ps2_xec_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct ps2_xec_config *config = dev->config;
	struct ps2_xec_data *data = dev->data;
	PS2_Type *base = config->base;
	uint32_t status;

	MCHP_GIRQ_SRC(config->girq_id) = BIT(config->girq_bit);

	/* Read and clear status */
	status = base->STATUS;

	if (status & MCHP_PS2_STATUS_RXD_RDY) {
		base->CTRL = 0x00;
		if (data->callback_isr) {
			data->callback_isr(dev, base->TRX_BUFF);
		}
	} else if (status &
		    (MCHP_PS2_STATUS_TX_TMOUT | MCHP_PS2_STATUS_TX_ST_TMOUT)) {
		/* Clear sticky bits and go to read mode */
		base->STATUS = MCHP_PS2_STATUS_RW1C_MASK;
		LOG_ERR("TX time out: %0x", status);
	}

	/* The control register reverts to RX automatically after
	 * transmiting the data
	 */
	base->CTRL = MCHP_PS2_CTRL_EN;
}

static const struct ps2_driver_api ps2_xec_driver_api = {
	.config = ps2_xec_configure,
	.read = NULL,
	.write = ps2_xec_write,
	.disable_callback = ps2_xec_inhibit_interface,
	.enable_callback = ps2_xec_enable_interface,
};

#ifdef CONFIG_PS2_XEC_0
static int ps2_xec_init_0(struct device *dev);

static const struct ps2_xec_config ps2_xec_config_0 = {
	.base = (PS2_Type *) DT_INST_REG_ADDR(0),
	.girq_id = DT_INST_PROP(0, girq),
	.girq_bit = DT_INST_PROP(0, girq_bit),
	.isr_nvic = DT_INST_IRQN(0),
};

static struct ps2_xec_data ps2_xec_port_data_0;

DEVICE_AND_API_INIT(ps2_xec_0, DT_INST_LABEL(0),
		    &ps2_xec_init_0,
		    &ps2_xec_port_data_0, &ps2_xec_config_0,
		    POST_KERNEL, CONFIG_PS2_INIT_PRIORITY,
		    &ps2_xec_driver_api);


static int ps2_xec_init_0(struct device *dev)
{
	ARG_UNUSED(dev);

	struct ps2_xec_data *data = dev->data;

	k_sem_init(&data->tx_lock, 0, 1);

	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    ps2_xec_isr, DEVICE_GET(ps2_xec_0), 0);

	irq_enable(DT_INST_IRQN(0));

	return 0;
}
#endif /* CONFIG_PS2_XEC_0 */

#ifdef CONFIG_PS2_XEC_1
static int ps2_xec_init_1(struct device *dev);

static const struct ps2_xec_config ps2_xec_config_1 = {
	.base = (PS2_Type *) DT_INST_REG_ADDR(1),
	.girq_id = DT_INST_PROP(1, girq),
	.girq_bit = DT_INST_PROP(1, girq_bit),
	.isr_nvic = DT_INST_IRQN(1),

};

static struct ps2_xec_data ps2_xec_port_data_1;

DEVICE_AND_API_INIT(ps2_xec_1, DT_INST_LABEL(1),
		    &ps2_xec_init_1,
		    &ps2_xec_port_data_1, &ps2_xec_config_1,
		    POST_KERNEL, CONFIG_PS2_INIT_PRIORITY,
		    &ps2_xec_driver_api);

static int ps2_xec_init_1(struct device *dev)
{
	ARG_UNUSED(dev);

	struct ps2_xec_data *data = dev->data;

	k_sem_init(&data->tx_lock, 0, 1);

	IRQ_CONNECT(DT_INST_IRQN(1),
		    DT_INST_IRQ(1, priority),
		    ps2_xec_isr, DEVICE_GET(ps2_xec_1), 0);

	irq_enable(DT_INST_IRQN(1));

	return 0;
}
#endif /* CONFIG_PS2_XEC_1 */
