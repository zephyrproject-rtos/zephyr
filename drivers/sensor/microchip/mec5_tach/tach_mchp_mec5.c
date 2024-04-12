/* Copyright (c) 2024 Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mec5_tach

#include <errno.h>
#include <soc.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/logging/log.h>

#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>

#include <device_mec5.h>
#include <mec_pcr_api.h>
#include <mec_tach_api.h>

LOG_MODULE_REGISTER(tach_mec5, CONFIG_SENSOR_LOG_LEVEL);

struct tach_mec5_dev_cfg {
	struct tach_regs * const regs;
	const struct pinctrl_dev_config *pcfg;
#ifdef CONFIG_TACH_MEC5_INTERRUPT
	void (*irq_config)(void);
#endif
	uint8_t edges_count;
	uint8_t read_mode;
};

#define TACH_MEC5_DATA_FLAG_ENABLED BIT(0)

struct tach_mec5_dev_data {
#ifdef CONFIG_TACH_MEC5_INTERRUPT
	struct k_sem sync;
#endif
	uint16_t count;
	uint8_t tach_sts;
	uint8_t flags;
};

#define TACH_MEC5_FAN_STOPPED		0xFFFFU
#define TACH_MEC5_SEC_PER_MINUTE	60U
#define TACH_MEC5_POLL_LOOP_COUNT	20U

/* If interrupt are used wait timeout on TACH ISR */
#define TACH_MEC5_SYNC_WAIT_MS		20u

/* Fetch a sample from the sensor and store it in an internal driver buffer
 *
 * Read all of a sensor's active channels and, if necessary, perform any
 * additional operations necessary to make the values useful.  The user
 * may then get individual channel values by calling sensor_channel_get.
 *
 * The function blocks until the fetch operation is complete.
 *
 * Since the function communicates with the sensor device, it is unsafe
 * to call it in an ISR if the device is connected via I2C or SPI.
 *
 * dev is a pointer to the sensor device
 *
 * returns 0 if successful, negative errno code if failure.
 *
 * NOTE: If the fan stops for some reason the resulting count value is maximum.
 * Set RPM value to 0 in this case.
 */
int tach_mec5_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct tach_mec5_dev_cfg * const devcfg = dev->config;
	struct tach_mec5_dev_data * const data = dev->data;
	struct tach_regs * const regs = devcfg->regs;
#ifdef CONFIG_TACH_MEC5_INTERRUPT
	int ret = 0;

	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	/* mec_tach_status_clr(regs, MEC5_TACH_STS_OOL | MEC5_TACH_STS_CNT_RDY); */
	mec_tach_intr_enable(regs, BIT(MEC5_TACH_IEN_OOL_POS) | BIT(MEC5_TACH_IEN_CNT_RDY_POS), 1u);

	ret = k_sem_take(&data->sync, K_MSEC(TACH_MEC5_SYNC_WAIT_MS));

	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	if (ret == -EAGAIN) {
		return -ETIMEDOUT;
	}

	if ((data->tach_sts & MEC5_TACH_STS_OOL) || (data->count == TACH_MEC5_FAN_STOPPED)) {
		data->count = 0U;
	}
#else
	uint32_t tach_status = 0;
	uint8_t poll_count = 0;

	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	while (poll_count < TACH_MEC5_POLL_LOOP_COUNT) {
		tach_status = mec_tach_status(regs);
		if (tach_status & MEC5_TACH_STS_CNT_RDY) {
			data->count = (uint16_t)(mec_tach_counter(regs) & 0xffffu);
			break;
		}

		poll_count++;

		k_usleep(USEC_PER_MSEC);
	}

	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	if (poll_count == TACH_MEC5_POLL_LOOP_COUNT) {
		return -EINVAL;
	}

	/* Maximum TACH count is fan stopped for unknown reason. Report 0 */
	if (data->count == TACH_MEC5_FAN_STOPPED) {
		data->count = 0U;
	}
#endif
	return 0;
}

/* Get a reading from a sensor device
 *
 * Return a useful value for a particular channel, from the driver's
 * internal data.  Before calling this function, a sample must be
 * obtained by calling sensor_sample_fetch or sensor_sample_fetch_chan.
 * It is guaranteed that two subsequent calls of this function for the
 * same channels will yield the same value, if sensor_sample_fetch or
 * sensor_sample_fetch_chan has not been called in the meantime.
 *
 * dev is a pointer to the sensor device
 * chan is the channel to read
 * val is where to store the value
 *
 * return 0 if successful, negative errno code if failure.
 *
 * NOTES:
 * We must convert the tachometer count to revolutions per minute (rpm)
 *
 * case 1: Tachometer reading mode is increment counter on rising edge of the
 * tachometer input signal. Not implemented at this time. Return -EIO.
 *
 * case 2: Tachometer reading mode is increment counter on rising edge of the
 * PCR slow clock (default 100 KHz).  The PCR slow clock defaults to 100 KHz but
 * can be changed.
 *
 */
static int tach_mec5_channel_get(const struct device *dev,
				 enum sensor_channel chan,
				 struct sensor_value *sval)
{
	const struct tach_mec5_dev_cfg * const devcfg = dev->config;
	struct tach_mec5_dev_data * const data = dev->data;
	uint32_t slow_clk_freq = mec_pcr_slow_clock_freq_get();

	if (!sval) {
		return -EINVAL;
	}

	if (chan != SENSOR_CHAN_RPM) {
		return -ENOTSUP;
	}

	/* Tach not in clock based count mode or the clock has been disabled! */
	if ((devcfg->read_mode != MEC_TACH_READ_MODE_100K_CLK_REDGE) || (!slow_clk_freq)) {
		return -EIO;
	}

	/* Convert the count per slow_clock_freq cycles to rpm */
	if ((data->count != TACH_MEC5_FAN_STOPPED) && (data->count != 0U)) {
		sval->val1 = (TACH_MEC5_SEC_PER_MINUTE * slow_clk_freq) / data->count;
		sval->val2 = 0U;
	} else {
		sval->val1 =  0U;
	}

	sval->val2 = 0U;

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int tach_mec5_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct tach_mec5_dev_cfg * const devcfg = dev->config;
	struct tach_mec5_dev_data * const data = dev->data;
	struct tach_regs * const regs = devcfg->regs;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		if (data->flags & TACH_MEC5_DATA_FLAG_ENABLED) {
			tach_enable(regs, 1);
		}
	break;
	case PM_DEVICE_ACTION_SUSPEND:
		if (tach_is_enabled(regs)) {
			tach_enable(regs, 0);
			data->flags |= TACH_MEC5_DATA_FLAG_ENABLED;
		} else {
			data->flags &= (uint8_t)~(TACH_MEC5_DATA_FLAG_ENABLED);
		}
	break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

#ifdef CONFIG_TACH_MEC5_INTERRUPT
static void tach_mec5_isr(const struct device *dev)
{
	const struct tach_mec5_dev_cfg * const devcfg = dev->config;
	struct tach_mec5_dev_data * const data = dev->data;
	struct tach_regs * const regs = devcfg->regs;
	uint32_t tach_cnt = mec_tach_counter(regs);
	uint8_t hwsts = (uint8_t)mec_tach_status(regs) & 0xffu;

	mec_tach_intr_enable(regs, BIT(MEC5_TACH_IEN_OOL_POS) | BIT(MEC5_TACH_IEN_CNT_RDY_POS), 0);
	mec_tach_status_clr(regs, MEC5_TACH_STATUS_ALL);
	mec_tach_girq_status_clr(regs);

	data->count = (uint16_t)(tach_cnt & 0xffffu);
	data->tach_sts = hwsts;
	k_sem_give(&data->sync);
}
#endif

static int tach_mec5_dev_init(const struct device *dev)
{
	const struct tach_mec5_dev_cfg * const devcfg = dev->config;
	struct tach_regs * const regs = devcfg->regs;
	uint32_t limits = MEC5_TACH_LIMITS(0u, 0xffffu);
	uint32_t flags = MEC5_TACH_CFG_ENABLE | MEC5_TACH_CFG_FILTER_EN;
	int ret = 0;
#ifdef CONFIG_TACH_MEC5_INTERRUPT
	struct tach_mec5_dev_data * const data = dev->data;

	k_sem_init(&data->sync, 0, 1);
#endif
	ret = pinctrl_apply_state(devcfg->pcfg, PINCTRL_STATE_DEFAULT);

	if (ret != 0) {
		LOG_ERR("MEC5 TACH PINCTRL init failed (%d)", ret);
		return ret;
	}

	flags |= (((uint32_t)devcfg->edges_count << MEC5_TACH_CFG_INTERVAL_EDGES_POS)
		  & MEC5_TACH_CFG_INTERVAL_EDGES_MSK);
	if (devcfg->read_mode == MEC_TACH_READ_MODE_100K_CLK_REDGE) {
		flags |= MEC5_TACH_CFG_CNT_INCR_CLK;
	}

	ret = mec_tach_init(regs, limits, flags);
	if (ret != MEC_RET_OK) {
		return -EIO;
	}

#ifdef CONFIG_TACH_MEC5_INTERRUPT
	if (devcfg->irq_config) {
		devcfg->irq_config();
		mec_tach_girq_enable(regs, 1);
	}
#endif

	return 0;
}

static const struct sensor_driver_api tach_mec5_driver_api = {
	.sample_fetch = tach_mec5_sample_fetch,
	.channel_get = tach_mec5_channel_get,
};

#ifdef CONFIG_TACH_MEC5_INTERRUPT
#define TACH_MEC5_IRQ_CFG(i)						\
	static void tach_mec5_irq_cfg_##i(void)				\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(i),				\
			    DT_INST_IRQ(i, priority),			\
			    tach_mec5_isr,				\
			    DEVICE_DT_INST_GET(i), 0);			\
		irq_enable(DT_INST_IRQN(i));				\
	}
#define TACH_MEC5_DEV_CFG(i)									\
	static const struct tach_mec5_dev_cfg tach_mec5_devcfg_##i = {				\
		.regs = (struct tach_regs *)DT_INST_REG_ADDR(i),				\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(i),					\
		.irq_config = tach_mec5_irq_cfg_##i,						\
		.edges_count = DT_PROP_OR(i, edges_count, MEC_TACH_CNT9_EDGES_4PER),		\
		.read_mode = DT_PROP_OR(i, read_mode, MEC_TACH_READ_MODE_100K_CLK_REDGE),	\
	}
#else
#define TACH_MEC5_IRQ_CFG(i)
#define TACH_MEC5_DEV_CFG(i)									\
	static const struct tach_mec5_dev_cfg tach_mec5_devcfg_##i = {				\
		.regs = (struct tach_regs *)DT_INST_REG_ADDR(i),				\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(i),					\
		.edges_count = DT_PROP_OR(i, edges_count, MEC_TACH_CNT9_EDGES_4PER),		\
		.read_mode = DT_PROP_OR(i, read_mode, MEC_TACH_READ_MODE_100K_CLK_REDGE),	\
	}
#endif

#define TACH_MEC5_DEVICE(id)					\
	static struct tach_mec5_dev_data tach_mec5_data_##id;	\
								\
	PINCTRL_DT_INST_DEFINE(id);				\
								\
	TACH_MEC5_IRQ_CFG(id)					\
								\
	TACH_MEC5_DEV_CFG(id);					\
								\
	PM_DEVICE_DT_INST_DEFINE(id, tach_mec5_pm_action);	\
								\
	SENSOR_DEVICE_DT_INST_DEFINE(id,			\
			    tach_mec5_dev_init,			\
			    PM_DEVICE_DT_INST_GET(id),		\
			    &tach_mec5_data_##id,		\
			    &tach_mec5_devcfg_##id,		\
			    POST_KERNEL,			\
			    CONFIG_SENSOR_INIT_PRIORITY,	\
			    &tach_mec5_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TACH_MEC5_DEVICE)
