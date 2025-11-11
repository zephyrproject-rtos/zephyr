/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_pca9422

#include <errno.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/mfd/pca9422.h>
#include "fsl_power.h"
#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_pca9422, CONFIG_MFD_LOG_LEVEL);

#define PCA9422_REG_DEV_INFO   0x00U
#define PCA9422_BIT_DEV_ID     GENMASK(7, 3)
#define PCA9422_BIT_DEV_REV    GENMASK(2, 0)
#define PCA9422_DEV_ID_VAL     0x00U
#define PCA9422_DEV_REV_VAL    0x02U /* B1 */
#define PCA9422_DEV_REV_VAL_B0 0x01U /* B0 */

#define PCA9422_REG_TOP_INT   0x01U
#define PCA9422_BIT_SYS_INT   BIT(4)
#define PCA9422_BIT_CHG_INT   BIT(3)
#define PCA9422_BIT_SW_BB_INT BIT(2)
#define PCA9422_BIT_SW_INT    BIT(1)
#define PCA9422_BIT_LDO_INT   BIT(0)

#define PCA9422_REG_SUB_INT0         0x02U
#define PCA9422_REG_SUB_INT0_MASK    0x03U
#define PCA9422_BIT_ON_SHORT_PUSH    BIT(7)
#define PCA9422_BIT_THERMAL_WARNING  BIT(6)
#define PCA9422_BIT_THSD             BIT(5)
#define PCA9422_BIT_THSD_EXIT        BIT(4)
#define PCA9422_BIT_VSYS_MIN_WARNING BIT(3)
#define PCA9422_BIT_WD_TIMER         BIT(2)
#define PCA9422_BIT_VSYS_OVP         BIT(1)
#define PCA9422_BIT_VSYS_OVP_EXIT    BIT(0)

#define PCA9422_REG_SUB_INT1      0x04U
#define PCA9422_REG_SUB_INT1_MASK 0x05U
#define PCA9422_BIT_MODE_CMPL     BIT(1)
#define PCA9422_BIT_ON_LONG_PUSH  BIT(0)

#define PCA9422_REG_SUB_INT2      0x06U
#define PCA9422_REG_SUB_INT2_MASK 0x07U
#define PCA9422_BIT_VOUTSW1       BIT(7)
#define PCA9422_BIT_VOUTSW2       BIT(6)
#define PCA9422_BIT_VOUTSW3       BIT(5)
#define PCA9422_BIT_VOUTSW4       BIT(4)
#define PCA9422_BIT_VOUTLDO1      BIT(3)
#define PCA9422_BIT_VOUTLDO2      BIT(2)
#define PCA9422_BIT_VOUTLDO3      BIT(1)
#define PCA9422_BIT_VOUTLDO4      BIT(0)

#define PCA9422_REG_INT1        0x0EU
#define PCA9422_REG_INT1_MASK   0x0FU
#define PCA9422_BIT_VR_FLT1     BIT(3)
#define PCA9422_BIT_BB_FAULT_OC BIT(0)

#define PCA9422_REG_INT_DEVICE_0 0x5CU

struct mfd_pca9422_child {
	const struct device *dev;
	child_isr_t child_isr;
};

struct mfd_pca9422_config {
	struct i2c_dt_spec bus;
	void (*irq_config_func)(const struct device *dev);
};

struct mfd_pca9422_data {
	struct k_work work;
	const struct device *dev;
	struct mfd_pca9422_child children[PCA9422_DEV_MAX];
};

static void mfd_pca9422_isr(const struct device *dev)
{
	struct mfd_pca9422_data *data = dev->data;

	/*
	 * Disable pca9422 interrupt
	 *
	 * If MCU like RT595 or RT798 has the dedicated pin interrupt for PMIC
	 * instead of GPIO interrupt, should disable PMIC pin interrupt here
	 * before clearing PMIC_IRQ flag
	 */
	imxrt_disable_pmic_interrupt();

	k_work_submit(&data->work);
}

static void mfd_pca9422_work_handler(struct k_work *work)
{
	struct mfd_pca9422_data *data = CONTAINER_OF(work, struct mfd_pca9422_data, work);
	struct mfd_pca9422_child *child;
	uint8_t reg_top_int;
	uint8_t reg_val[6]; /* Sub level interrupt 0, 1, 2 value and mask */
	int ret = 0;

	/* Read Top level interrupt */
	ret = mfd_pca9422_reg_read_byte(data->dev, PCA9422_REG_TOP_INT, &reg_top_int);
	if (ret < 0) {
		LOG_ERR("%s: REG_TOP_INT read error(%d)\n", __func__, ret);
		goto error;
	}

	/* Check interrupt group */
	if (reg_top_int & PCA9422_BIT_SYS_INT) {
		/* System level interrupt event trigger */
		/* Read sub level interrupt 0, 1 and mask */
		ret = mfd_pca9422_reg_burst_read(data->dev, PCA9422_REG_SUB_INT0, reg_val, 4);
		if (ret < 0) {
			LOG_ERR("%s: REG_SUB_INT0 read error(%d)\n", __func__, ret);
			goto error;
		}
		/*
		 * Check interrupt event and add some notification or behavior for
		 * it.
		 */
		LOG_INF("%s: sub_int[0]=0x%x,  [1]=0x%x\n", __func__, reg_val[0], reg_val[2]);
		LOG_INF("%s: sub_mask[0]=0x%x, [1]=0x%x\n", __func__, reg_val[1], reg_val[3]);
	}

	if (reg_top_int & PCA9422_BIT_CHG_INT) {
		/* Battery charger block interrupt event trigger */
		/* Set child device to charger */
		child = &data->children[PCA9422_DEV_CHG];
		/* Generate charger interrupt handler */
		LOG_INF("%s: charger interrupt\n", __func__);
		if (child->child_isr != NULL) {
			child->child_isr(child->dev);
		} else {
			/* There is no charger isr */
			/* Read charger interrupt registers to clear them */
			/* Read int_device interrupt 0, 1, int_charger 0, 1, 2, and 3 */
			ret = mfd_pca9422_reg_burst_read(data->dev, PCA9422_REG_INT_DEVICE_0,
							 reg_val, 6);
			if (ret < 0) {
				LOG_ERR("%s: REG_INT_DEVICE_0 read error(%d)\n", __func__, ret);
			} else {
				/* Check interrupt event */
				LOG_INF("%s: int_device[0]=0x%x,  [1]=0x%x\n", __func__, reg_val[0],
					reg_val[1]);
				LOG_INF("%s: int_charger[0]=0x%x, [1]=0x%x, [2]=0x%x, [3]=0x%x\n",
					__func__, reg_val[2], reg_val[3], reg_val[4], reg_val[5]);
			}
		}
	}

	if (reg_top_int & (PCA9422_BIT_SW_BB_INT | PCA9422_BIT_SW_INT | PCA9422_BIT_LDO_INT)) {
		/* Regulator block interrupt event trigger */
		/* Read sub level interrupt 2 and mask to clear flags */
		ret = mfd_pca9422_reg_burst_read(data->dev, PCA9422_REG_SUB_INT2, reg_val, 2);
		if (ret < 0) {
			LOG_ERR("%s: REG_SUB_INT2 read error(%d)\n", __func__, ret);
			goto error;
		}
		/* Check interrupt event */
		LOG_INF("%s: sub_int[2]=0x%x, mask[2]=0x%x\n", __func__, reg_val[0], reg_val[1]);
		if (reg_top_int & PCA9422_BIT_SW_BB_INT) {
			/* Read int1 and int1_mask registers to clear flags */
			ret = mfd_pca9422_reg_burst_read(data->dev, PCA9422_REG_INT1, reg_val, 2);
			if (ret < 0) {
				LOG_ERR("%s: REG_INT1 read error(%d)\n", __func__, ret);
				goto error;
			}
			/*
			 * Check interrupt event and add some notification
			 * or event for it
			 */
			LOG_INF("%s: int1=0x%x, mask=0x%x\n", __func__, reg_val[0], reg_val[1]);
		}
	}

error:
	/*
	 * Clear interrupt flag
	 *
	 * If MCU like RT595 or RT798 has the dedicated pin interrupt for PMIC
	 * instead of GPIO interrupt, should clear PMIC interrupt flag here
	 * before enabling PMIC_IRQ
	 */
	imxrt_clear_pmic_interrupt();

	/* Enable interrupt */
	imxrt_enable_pmic_interrupt();
}

void mfd_pca9422_set_irqhandler(const struct device *dev, const struct device *child_dev,
				enum child_dev child_idx, child_isr_t handler)
{
	struct mfd_pca9422_data *data = dev->data;
	struct mfd_pca9422_child *child;

	/* Set child device piont */
	child = &data->children[child_idx];

	/* Store the interrupt handler and device instance for child device */
	child->dev = child_dev;
	child->child_isr = handler;
}

static int mfd_pca9422_init(const struct device *dev)
{
	const struct mfd_pca9422_config *config = dev->config;
	struct mfd_pca9422_data *data = dev->data;
	uint8_t val;
	int ret;

	if (!i2c_is_ready_dt(&config->bus)) {
		return -ENODEV;
	}

	ret = i2c_reg_read_byte_dt(&config->bus, PCA9422_REG_DEV_INFO, &val);
	if (ret < 0) {
		return ret;
	}

	if ((val != PCA9422_DEV_REV_VAL) && (val != PCA9422_DEV_REV_VAL_B0)) {
		return -ENODEV;
	}

	k_work_init(&data->work, mfd_pca9422_work_handler);

	config->irq_config_func(dev);

	/*
	 * Clear interrupt flag
	 *
	 * If MCU like RT595 or RT798 has the dedicated pin interrupt for PMIC
	 * instead of GPIO interrupt, should clear PMIC interrupt flag here
	 * before enabling PMIC_IRQ
	 */
	imxrt_clear_pmic_interrupt();

	/*
	 * Enable PMIC pin interrupt
	 *
	 * If MCU like RT595 or RT798 has the dedicated pin interrupt for PMIC
	 * instead of GPIO interrupt, should set to enable the dedicated pin interrupt
	 */
	imxrt_enable_pmic_interrupt();

	/* Clear PCA9422 interrupt registers */
	/* Read all interrupt registers */
	ret = i2c_reg_read_byte_dt(&config->bus, PCA9422_REG_TOP_INT, &val);
	if (ret < 0) {
		return ret;
	}
	ret = i2c_reg_read_byte_dt(&config->bus, PCA9422_REG_SUB_INT0, &val);
	if (ret < 0) {
		return ret;
	}
	ret = i2c_reg_read_byte_dt(&config->bus, PCA9422_REG_SUB_INT1, &val);
	if (ret < 0) {
		return ret;
	}
	ret = i2c_reg_read_byte_dt(&config->bus, PCA9422_REG_SUB_INT2, &val);
	if (ret < 0) {
		return ret;
	}
	ret = i2c_reg_read_byte_dt(&config->bus, PCA9422_REG_INT1, &val);
	if (ret < 0) {
		return ret;
	}

	/* Set sub level mask register */
	/* All interrupts are masked in default */
	val = 0xFFU;
	/* Enable ON key short-press interrupt */
	val = val & ~PCA9422_BIT_ON_SHORT_PUSH;
	ret = i2c_reg_write_byte_dt(&config->bus, PCA9422_REG_SUB_INT0_MASK, val);
	if (ret < 0) {
		return ret;
	}

	val = 0xFFU;
	ret = i2c_reg_write_byte_dt(&config->bus, PCA9422_REG_SUB_INT1_MASK, val);
	if (ret < 0) {
		return ret;
	}

	val = 0xFFU;
	ret = i2c_reg_write_byte_dt(&config->bus, PCA9422_REG_SUB_INT2_MASK, val);
	if (ret < 0) {
		return ret;
	}

	/* Set INT1 mask register */
	val = 0xFFU;
	return i2c_reg_write_byte_dt(&config->bus, PCA9422_REG_INT1_MASK, val);
}

int mfd_pca9422_reg_burst_read(const struct device *dev, uint8_t reg, uint8_t *value, size_t len)
{
	const struct mfd_pca9422_config *config = dev->config;

	return i2c_burst_read_dt(&config->bus, reg, value, len);
}

int mfd_pca9422_reg_read_byte(const struct device *dev, uint8_t reg, uint8_t *value)
{
	const struct mfd_pca9422_config *config = dev->config;

	return i2c_reg_read_byte_dt(&config->bus, reg, value);
}

int mfd_pca9422_reg_burst_write(const struct device *dev, uint8_t reg, uint8_t *value, size_t len)
{
	const struct mfd_pca9422_config *config = dev->config;

	return i2c_burst_write_dt(&config->bus, reg, value, len);
}

int mfd_pca9422_reg_write_byte(const struct device *dev, uint8_t reg, uint8_t value)
{
	const struct mfd_pca9422_config *config = dev->config;

	return i2c_reg_write_byte_dt(&config->bus, reg, value);
}

int mfd_pca9422_reg_update_byte(const struct device *dev, uint8_t reg, uint8_t mask, uint8_t value)
{
	const struct mfd_pca9422_config *config = dev->config;

	return i2c_reg_update_byte_dt(&config->bus, reg, mask, value);
}

#define MFD_PCA9422_INIT(inst)                                                                     \
                                                                                                   \
	static void mfd_pca9422_config_func_##inst(const struct device *dev);                      \
                                                                                                   \
	static const struct mfd_pca9422_config mfd_pca9422_config_##inst = {                       \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.irq_config_func = mfd_pca9422_config_func_##inst,                                 \
	};                                                                                         \
                                                                                                   \
	static struct mfd_pca9422_data mfd_pca9422_data_##inst = {                                 \
		.dev = DEVICE_DT_INST_GET(inst),                                                   \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, mfd_pca9422_init, NULL, &mfd_pca9422_data_##inst,              \
			      &mfd_pca9422_config_##inst, POST_KERNEL, CONFIG_MFD_INIT_PRIORITY,   \
			      NULL);                                                               \
                                                                                                   \
	static void mfd_pca9422_config_func_##inst(const struct device *dev)                       \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), mfd_pca9422_isr,      \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}

DT_INST_FOREACH_STATUS_OKAY(MFD_PCA9422_INIT)
