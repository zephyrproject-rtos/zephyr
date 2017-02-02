/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <stdio.h>
#include <kernel.h>
#include <board.h>
#include <device.h>
#include <init.h>
#include <aio_comparator.h>

#include "qm_comparator.h"

#define INT_COMPARATORS_MASK 0x7FFFF
#define AIO_QMSI_CMP_COUNT		(19)
#if (QM_LAKEMONT)
#define CMP_INTR_ROUTER QM_INTERRUPT_ROUTER->comparator_0_host_int_mask
#else
#define CMP_INTR_ROUTER QM_INTERRUPT_ROUTER->comparator_0_ss_int_mask
#endif

struct aio_qmsi_cmp_cb {
	aio_cmp_cb cb;
	void *param;
};

struct aio_qmsi_cmp_dev_data_t {
	/** Number of total comparators */
	uint8_t num_cmp;
	/** Callback for each comparator */
	struct aio_qmsi_cmp_cb cb[AIO_QMSI_CMP_COUNT];
};

/* Shadow configuration to keep track of changes */
static qm_ac_config_t config;

static int aio_cmp_config(struct device *dev);

static int aio_qmsi_cmp_disable(struct device *dev, uint8_t index)
{
	if (index >= AIO_QMSI_CMP_COUNT) {
		return -EINVAL;
	}

	/* Disable interrupt to current core */
	CMP_INTR_ROUTER |= (1 << index);

	/* Disable comparator according to index */
	config.cmp_en &= ~(1 << index);
	config.power &= ~(1 << index);
	config.reference &= ~(1 << index);
	config.polarity &= ~(1 << index);

	if (qm_ac_set_config(&config) != 0) {
		return -EINVAL;
	}

	return 0;
}

static int aio_qmsi_cmp_configure(struct device *dev, uint8_t index,
			 enum aio_cmp_polarity polarity,
			 enum aio_cmp_ref refsel,
			 aio_cmp_cb cb, void *param)
{
	struct aio_qmsi_cmp_dev_data_t *dev_data =
		(struct aio_qmsi_cmp_dev_data_t *)dev->driver_data;

	if (index >= AIO_QMSI_CMP_COUNT) {
		return -EINVAL;
	}

	aio_qmsi_cmp_disable(dev, index);

	dev_data->cb[index].cb = cb;
	dev_data->cb[index].param = param;

	if (refsel == AIO_CMP_REF_A) {
		config.reference &= ~(1 << index);
	} else {
		config.reference |= (1 << index);
	}

	if (polarity == AIO_CMP_POL_RISE) {
		config.polarity &= ~(1 << index);
	} else {
		config.polarity |= (1 << index);
	}
	/* The driver will not use QMSI callback mechanism */
	config.callback = NULL;
	/* Enable comparator */
	config.cmp_en |= (1 << index);
	config.power |= (1 << index);

	if (qm_ac_set_config(&config) != 0) {
		return -EINVAL;
	}

	/* Enable Interrupts to current core for an specific comparator */
	CMP_INTR_ROUTER &= ~(1 << index);

	return 0;
}

static uint32_t aio_cmp_qmsi_get_pending_int(struct device *dev)
{
	return QM_SCSS_CMP->cmp_stat_clr;
}

static const struct aio_cmp_driver_api aio_cmp_funcs = {
	.disable = aio_qmsi_cmp_disable,
	.configure = aio_qmsi_cmp_configure,
	.get_pending_int = aio_cmp_qmsi_get_pending_int,
};

static int aio_qmsi_cmp_init(struct device *dev)
{
	uint8_t i;
	struct aio_qmsi_cmp_dev_data_t *dev_data =
		(struct aio_qmsi_cmp_dev_data_t *)dev->driver_data;

	aio_cmp_config(dev);

	/* Disable all comparator interrupts */
	CMP_INTR_ROUTER |= INT_COMPARATORS_MASK;

	/* Clear status and dissble all comparators */
	QM_SCSS_CMP->cmp_stat_clr |= INT_COMPARATORS_MASK;
	QM_SCSS_CMP->cmp_pwr &= ~INT_COMPARATORS_MASK;
	QM_SCSS_CMP->cmp_en &= ~INT_COMPARATORS_MASK;

	/* Don't use the QMSI callback */
	config.callback = NULL;
	/* Get Initial configuration from HW */
	config.reference = QM_SCSS_CMP->cmp_ref_sel;
	config.polarity = QM_SCSS_CMP->cmp_ref_pol;
	config.power = QM_SCSS_CMP->cmp_pwr;
	config.cmp_en = QM_SCSS_CMP->cmp_en;

	/* Clear callback pointers */
	for (i = 0; i < dev_data->num_cmp; i++) {
		dev_data->cb[i].cb = NULL;
		dev_data->cb[i].param = NULL;
	}

	irq_enable(IRQ_GET_NUMBER(QM_IRQ_COMPARATOR_0_INT));

	return 0;
}

static void aio_qmsi_cmp_isr(void *data)
{
	uint8_t i;
	struct device *dev = data;
	struct aio_qmsi_cmp_dev_data_t *dev_data =
		(struct aio_qmsi_cmp_dev_data_t *)dev->driver_data;

	uint32_t int_status = QM_SCSS_CMP->cmp_stat_clr;

	for (i = 0; i < dev_data->num_cmp; i++) {
		if (int_status & (1 << i)) {
			if (dev_data->cb[i].cb != NULL) {
				dev_data->cb[i].cb(dev_data->cb[i].param);
			}
		}
	}

	/* Clear all pending interrupts */
	QM_SCSS_CMP->cmp_stat_clr = int_status;
}

static struct aio_qmsi_cmp_dev_data_t aio_qmsi_cmp_dev_data = {
		.num_cmp = AIO_QMSI_CMP_COUNT,
};

DEVICE_AND_API_INIT(aio_qmsi_cmp, CONFIG_AIO_COMPARATOR_0_NAME,
		    &aio_qmsi_cmp_init, &aio_qmsi_cmp_dev_data, NULL,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    (void *)&aio_cmp_funcs);

static int aio_cmp_config(struct device *dev)
{
	ARG_UNUSED(dev);

	IRQ_CONNECT(IRQ_GET_NUMBER(QM_IRQ_COMPARATOR_0_INT),
		    CONFIG_AIO_COMPARATOR_0_IRQ_PRI, aio_qmsi_cmp_isr,
		    DEVICE_GET(aio_qmsi_cmp), 0);

	return 0;
}
