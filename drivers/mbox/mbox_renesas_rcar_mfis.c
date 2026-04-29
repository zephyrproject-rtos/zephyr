/*
 * Copyright 2026 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/mbox.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>

#define LOG_LEVEL CONFIG_MBOX_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(renesas_rcar_mfis_mbox);

#define DT_DRV_COMPAT renesas_rcar_mfis_mbox

/* Register domain offsets */
#define RCAR_MFIS_REG_OFFS	      0x000000
#define RCAR_MFIS_SCP_REG_OFFS	      0x040000
#define RCAR_MFIS_COMMON_REG_OFFS     0x1e0000
#define RCAR_MFIS_SCP_COMMON_REG_OFFS 0x1e1000

/* Write access control registers */
#define RCAR_MFIS_WACNTR_AP_RT (RCAR_MFIS_COMMON_REG_OFFS + 0x904)
#define RCAR_MFIS_WACNTR_SCP   (RCAR_MFIS_SCP_COMMON_REG_OFFS + 0x904)

#define RCAR_MFIS_WACNTR_AP_RT_CODEVAL 0xacce0000
#define RCAR_MFIS_WACNTR_SCP_CODEVAL   0xacc00000

#define RCAR_MFIS_WACNTR_AP_RT_REGADDR_MASK GENMASK(15, 0)
#define RCAR_MFIS_WACNTR_SCP_REGADDR_MASK   GENMASK(19, 0)

/*
 * From the current AP/RT core's perspective, two channel types may be used.
 * A channel can be capable of sending (receiving) signal/data to (from)
 * another AP/RT core domain XOR to (from) SCP core domain.
 * - AP/RT channel IDs = 0 to 63
 * - SCP channel IDs = 64 to 95 for AP cores (cores 0 to 31)
 *   SCP channel IDs = 64 to 75 for RT cores (cores 0 to 11)
 */
#define RCAR_MFIS_AP_RT_CH_CNT 64
/* AP to RT communication control registers */
#define RCAR_MFIS_COMIICR(ch) (RCAR_MFIS_REG_OFFS + ((ch) << 12))

#ifdef CONFIG_CPU_CORTEX_R52
#define RCAR_MFIS_SCP_CH_CNT 12
/* SCP to RT communication control registers */
#define RCAR_MFIS_XSIICR(cpu_id) (RCAR_MFIS_SCP_REG_OFFS + ((cpu_id) << 12) + 0x20000)
#else
#define RCAR_MFIS_SCP_CH_CNT 32
/* SCP to AP communication control registers */
#define RCAR_MFIS_XSIICR(cpu_id) (RCAR_MFIS_SCP_REG_OFFS + ((cpu_id) << 12))
#endif /* CONFIG_CPU_CORTEX_R52 */

#define RCAR_MFIS_MAX_CHANNELS (RCAR_MFIS_AP_RT_CH_CNT + RCAR_MFIS_SCP_CH_CNT)

#define RCAR_MFIS_IS_SCP_CH(ch)	    ((ch) >= RCAR_MFIS_AP_RT_CH_CNT)
#define RCAR_MFIS_IS_INVALID_CH(ch) ((ch) >= RCAR_MFIS_MAX_CHANNELS)

#define RCAR_MFIS_SCP_CH_TO_CPU_ID(ch) ((ch) - RCAR_MFIS_AP_RT_CH_CNT)

#define RCAR_MFIS_IRQ_BIT BIT(0)

/* MTU */
#define RCAR_MFIS_MBOX_SIZE sizeof(uint32_t)

/* Helpers to select write access control info */
#define RCAR_MFIS_WACNTR(ch) ((ch) < RCAR_MFIS_AP_RT_CH_CNT ?	\
			      RCAR_MFIS_WACNTR_AP_RT :		\
			      RCAR_MFIS_WACNTR_SCP)

#define RCAR_MFIS_WACNTR_CODEVAL(ch) ((ch) < RCAR_MFIS_AP_RT_CH_CNT ?	\
				      RCAR_MFIS_WACNTR_AP_RT_CODEVAL :	\
				      RCAR_MFIS_WACNTR_SCP_CODEVAL)

#define RCAR_MFIS_WACNTR_REGADDR_MASK(ch) ((ch) < RCAR_MFIS_AP_RT_CH_CNT ?		\
					   RCAR_MFIS_WACNTR_AP_RT_REGADDR_MASK :	\
					   RCAR_MFIS_WACNTR_SCP_REGADDR_MASK)

/* Helper to select inbound communication control register */
#define RCAR_MFIS_IICR(ch) ((ch) < RCAR_MFIS_AP_RT_CH_CNT ?			\
			    RCAR_MFIS_COMIICR(ch) :				\
			    RCAR_MFIS_XSIICR(RCAR_MFIS_SCP_CH_TO_CPU_ID(ch)))
/* Outbound communication control register */
#define RCAR_MFIS_EICR(ch) (RCAR_MFIS_IICR(ch) + 0x4)
/* Inbound message buffer register */
#define RCAR_MFIS_IMBR(ch) (RCAR_MFIS_IICR(ch) + 0x40)
/* Outbound message buffer register */
#define RCAR_MFIS_EMBR(ch) (RCAR_MFIS_IMBR(ch) + 0x4)

struct rcar_mfis_ch_data {
	bool enabled;
	uint32_t received_data;
	mbox_callback_t cb;
	void *user_data;
};

struct rcar_mfis_data {
	DEVICE_MMIO_RAM; /* Must be first */
	struct rcar_mfis_ch_data ch_data[RCAR_MFIS_MAX_CHANNELS];
	struct k_spinlock data_lock;
	struct k_spinlock wr_lock;
};

typedef void (*rcar_mfis_config_irq_t)(const struct device *dev);

struct rcar_mfis_config {
	DEVICE_MMIO_ROM; /* Must be first */
	unsigned int irqs[RCAR_MFIS_MAX_CHANNELS];
	rcar_mfis_config_irq_t config_func;
};

static inline uint32_t rcar_mfis_read(const struct device *dev, uint32_t offs)
{
	uint32_t value = sys_read32(DEVICE_MMIO_GET(dev) + offs);

	LOG_DBG("reg = 0x%08lx ; val = 0x%08x", DEVICE_MMIO_GET(dev) + offs, value);

	return value;
}

static inline void rcar_mfis_write(const struct device *dev, uint32_t offs,
				   uint32_t value)
{
	LOG_DBG("reg = 0x%08lx ; val = 0x%08x", DEVICE_MMIO_GET(dev) + offs, value);

	sys_write32(value, DEVICE_MMIO_GET(dev) + offs);
}

static void rcar_mfis_write_prot(const struct device *dev, uint32_t channel,
				 uint32_t offs, uint32_t value)
{
	uint32_t wacntr_val;

	/*
	 * Write the register address (the one that must be written to)
	 * to the write access control register (WACNTR)
	 */
	wacntr_val = RCAR_MFIS_WACNTR_CODEVAL(channel) |
		     (offs & RCAR_MFIS_WACNTR_REGADDR_MASK(channel));
	rcar_mfis_write(dev, RCAR_MFIS_WACNTR(channel), wacntr_val);

	/* Write register */
	rcar_mfis_write(dev, offs, value);
}

static void rcar_mfis_isr(const struct device *dev, uint32_t channel)
{
	struct rcar_mfis_data *data = dev->data;
	struct mbox_msg msg;
	k_spinlock_key_t data_key, wr_key;
	uint32_t iicr, imbr, iicr_val;

	if (IS_ENABLED(CONFIG_CPU_CORTEX_R52) || RCAR_MFIS_IS_SCP_CH(channel)) {
		/*
		 * For [(AP to RT) || (SCP to AP || SCP to RT)], use the "S->R" direction.
		 *
		 * For the communication between AP and RT, the documentation uses the
		 * "sender to receiver" and "receiver to sender" terminology. By convention,
		 * in this driver, RT = receiver and AP = sender. As a result, the RT core
		 * domain must use the "sender to receiver" registers (COMIICR and COMIMBR)
		 * for receiving a message from the AP core domain.
		 *
		 * For receiving a message from the SCP core domain, the AP core domain must
		 * use ASIICR and ASIMBR. Similarly, the RT core domain must use RSIICR and
		 * RSIMBR.
		 */
		iicr = RCAR_MFIS_IICR(channel);
		imbr = RCAR_MFIS_IMBR(channel);
	} else {
		/*
		 * For (RT to AP), use the other direction ("R->S").
		 *
		 * Here the AP core domain must use the "receiver to sender" registers
		 * (COMEICR and COMEMBR) for receiving a message from the RT core domain.
		 */
		iicr = RCAR_MFIS_EICR(channel);
		imbr = RCAR_MFIS_EMBR(channel);
	}

	LOG_DBG("ch_%u : iicr = 0x%08x ; imbr = 0x%08x", channel, iicr, imbr);

	data_key = k_spin_lock(&data->data_lock);

	/* Get message */
	if (data->ch_data[channel].cb) {
		data->ch_data[channel].received_data = rcar_mfis_read(dev, imbr);
		msg.size = RCAR_MFIS_MBOX_SIZE;
		msg.data = (const void *)&data->ch_data[channel].received_data;
		data->ch_data[channel].cb(dev, channel, data->ch_data[channel].user_data, &msg);
	}

	k_spin_unlock(&data->data_lock, data_key);

	/* Clear interrupt */
	wr_key = k_spin_lock(&data->wr_lock);
	iicr_val = rcar_mfis_read(dev, iicr);
	iicr_val &= ~RCAR_MFIS_IRQ_BIT;
	rcar_mfis_write_prot(dev, channel, iicr, iicr_val);
	k_spin_unlock(&data->wr_lock, wr_key);
}

static int rcar_mfis_send(const struct device *dev, uint32_t channel,
			  const struct mbox_msg *msg)
{
	struct rcar_mfis_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t eicr, embr;
	uint32_t __aligned(4) data32;

	if (RCAR_MFIS_IS_INVALID_CH(channel)) {
		LOG_ERR("Invalid channel (val = %u, max = %u)",
			channel, RCAR_MFIS_MAX_CHANNELS - 1);
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_CPU_CORTEX_R52) || RCAR_MFIS_IS_SCP_CH(channel)) {
		/*
		 * For [(RT to AP) || (AP to SCP || RT to SCP)], use the "R->S" direction.
		 *
		 * For the communication between AP and RT, the documentation uses the
		 * "sender to receiver" and "receiver to sender" terminology. By convention,
		 * in this driver, RT = receiver and AP = sender. As a result, the RT core
		 * domain must use the "receiver to sender" registers (COMEICR and COMEMBR)
		 * for sending a message to the AP core domain.
		 *
		 * For sending a message to the SCP core domain, the AP core domain must
		 * use ASEICR and ASEMBR. Similarly, the RT core domain must use RSEICR and
		 * RSEMBR.
		 */
		eicr = RCAR_MFIS_EICR(channel);
		embr = RCAR_MFIS_EMBR(channel);
	} else {
		/*
		 * For (AP to RT), use the other direction ("S->R").
		 *
		 * Here the AP core domain must use the "sender to receiver" registers
		 * (COMIICR and COMIMBR) for sending a message to the RT core domain.
		 */
		eicr = RCAR_MFIS_IICR(channel);
		embr = RCAR_MFIS_IMBR(channel);
	}

	LOG_DBG("ch_%u : eicr = 0x%08x ; embr = 0x%08x", channel, eicr, embr);

	/* Signalling mode */
	if (msg == NULL) {
		/* Issue interrupt */
		key = k_spin_lock(&data->wr_lock);
		rcar_mfis_write_prot(dev, channel, eicr, RCAR_MFIS_IRQ_BIT);
		k_spin_unlock(&data->wr_lock, key);

		return 0;
	}

	/* Data transfer mode */
	if (msg->size > RCAR_MFIS_MBOX_SIZE) {
		LOG_ERR("Invalid data size (val = %u, max = %u)",
			msg->size, RCAR_MFIS_MBOX_SIZE);
		return -EMSGSIZE;
	}

	/*
	 * Write to buffer (handle the case where msg->data is not word-aligned)
	 * before issuing interrupt.
	 */
	memcpy(&data32, msg->data, msg->size);
	key = k_spin_lock(&data->wr_lock);
	rcar_mfis_write_prot(dev, channel, embr, data32);
	rcar_mfis_write_prot(dev, channel, eicr, RCAR_MFIS_IRQ_BIT);
	k_spin_unlock(&data->wr_lock, key);

	return 0;
}

static int rcar_mfis_register_callback(const struct device *dev, uint32_t channel,
				       mbox_callback_t cb, void *user_data)
{
	struct rcar_mfis_data *data = dev->data;
	k_spinlock_key_t key;

	if (RCAR_MFIS_IS_INVALID_CH(channel)) {
		LOG_ERR("Invalid channel (val = %u, max = %u)",
			channel, RCAR_MFIS_MAX_CHANNELS - 1);
		return -EINVAL;
	}

	key = k_spin_lock(&data->data_lock);
	data->ch_data[channel].cb = cb;
	data->ch_data[channel].user_data = user_data;
	k_spin_unlock(&data->data_lock, key);

	return 0;
}

static int rcar_mfis_mtu_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return RCAR_MFIS_MBOX_SIZE;
}

static uint32_t rcar_mfis_max_channels_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return RCAR_MFIS_MAX_CHANNELS;
}

static int rcar_mfis_set_enabled(const struct device *dev, uint32_t channel,
				 bool enable)
{
	const struct rcar_mfis_config *config = dev->config;
	struct rcar_mfis_data *data = dev->data;
	k_spinlock_key_t key;
	int ret = 0;

	if (RCAR_MFIS_IS_INVALID_CH(channel)) {
		LOG_ERR("Invalid channel (val = %u, max = %u)",
			channel, RCAR_MFIS_MAX_CHANNELS - 1);
		return -EINVAL;
	}

	key = k_spin_lock(&data->data_lock);

	if ((enable && data->ch_data[channel].enabled) ||
	    (!enable && !data->ch_data[channel].enabled)) {
		ret = -EALREADY;
		goto unlock;
	}

	if (enable && !data->ch_data[channel].cb) {
		LOG_WRN("Enabling channel without a registered callback\n");
	}

	if (enable) {
		irq_enable(config->irqs[channel]);
	} else {
		irq_disable(config->irqs[channel]);
	}

	data->ch_data[channel].enabled = enable;

unlock:
	k_spin_unlock(&data->data_lock, key);

	return ret;
}

static DEVICE_API(mbox, rcar_mfis_driver_api) = {
	.send = rcar_mfis_send,
	.register_callback = rcar_mfis_register_callback,
	.mtu_get = rcar_mfis_mtu_get,
	.max_channels_get = rcar_mfis_max_channels_get,
	.set_enabled = rcar_mfis_set_enabled,
};

static int rcar_mfis_init(const struct device *dev)
{
	const struct rcar_mfis_config *config = dev->config;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	config->config_func(dev);

	return 0;
}

#define RCAR_MFIS_ISR_DEFINE(idx, n)					\
	static void rcar_mfis_isr_##n##_##idx(const struct device *dev)	\
	{								\
		rcar_mfis_isr(dev, idx);				\
	}								\

#define RCAR_MFIS_IRQ_INIT(idx, n)				\
	IRQ_CONNECT(DT_INST_IRQN_BY_IDX(n, idx),		\
		    DT_INST_IRQ_BY_IDX(n, idx, priority),	\
		    rcar_mfis_isr_##n##_##idx,			\
		    DEVICE_DT_INST_GET(n),			\
		    DT_INST_IRQ_BY_IDX(n, idx, flags));		\

#define RCAR_MFIS_IRQN(idx, n)				\
	.irqs[idx] = DT_INST_IRQN_BY_IDX(n, idx),	\

#define RCAR_MFIS_INSTANCE_DEFINE(n)						\
	LISTIFY(DT_NUM_IRQS(DT_DRV_INST(n)), RCAR_MFIS_ISR_DEFINE, (), n)	\
										\
	static void rcar_mfis_irq_config_##n(const struct device *dev)		\
	{									\
		LISTIFY(DT_NUM_IRQS(DT_DRV_INST(n)), RCAR_MFIS_IRQ_INIT, (), n)	\
	}									\
										\
	static const struct rcar_mfis_config rcar_mfis_##n##_config = {		\
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),				\
		LISTIFY(DT_NUM_IRQS(DT_DRV_INST(n)), RCAR_MFIS_IRQN, (), n)	\
		.config_func = rcar_mfis_irq_config_##n,			\
	};									\
										\
	static struct rcar_mfis_data rcar_mfis_##n##_data;			\
										\
	DEVICE_DT_INST_DEFINE(n,						\
			      rcar_mfis_init,					\
			      NULL,						\
			      &rcar_mfis_##n##_data,				\
			      &rcar_mfis_##n##_config,				\
			      PRE_KERNEL_1,					\
			      CONFIG_MBOX_INIT_PRIORITY,			\
			      &rcar_mfis_driver_api)

DT_INST_FOREACH_STATUS_OKAY(RCAR_MFIS_INSTANCE_DEFINE)
