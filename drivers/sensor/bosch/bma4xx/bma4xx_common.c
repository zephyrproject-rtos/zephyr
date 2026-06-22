/*
 * Copyright (c) 2024 Cienet
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bosch_bma4xx

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor_clock.h>
#include "bma4xx.h"
#include "bma4xx_interrupt.h"
#include "bma4xx_defs.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bma4xx, CONFIG_SENSOR_LOG_LEVEL);

#ifdef CONFIG_BMA4XX_STREAM

static uint16_t bma4xx_compute_fifo_wm(const struct bma4xx_runtime_config *new_cfg)
{
	int64_t odr;
	int rc;
	const bool accel_enabled = new_cfg->accel_pwr_mode != 0;
	const bool aux_enabled = new_cfg->aux_pwr_mode != 0;

	if (new_cfg->batch_ticks == 0 || !accel_enabled) {
		return 0;
	}

	const int pkt_size = (accel_enabled && aux_enabled)
				     ? BMA4XX_FIFO_MA_LENGTH + BMA4XX_FIFO_HEADER_LENGTH
				     : BMA4XX_FIFO_A_LENGTH + BMA4XX_FIFO_HEADER_LENGTH;

	struct sensor_value val = {0};

	rc = bma4xx_accel_reg_to_hz(new_cfg->accel_odr, &val);

	if (rc) {
		LOG_ERR("Invalid ord value");
		return -EINVAL;
	}

	/* Measured in Hz */
	odr = sensor_value_to_micro(&val) / INT64_C(1000000);

	/* Convert 'odr' to bytes * batch_ticks / sec */
	odr *= (int64_t)new_cfg->batch_ticks * pkt_size;

	/* 'odr' = byte_ticks_per_sec * sensor_ns_per_tick / NSEC_PER_SEC = bytes */
	odr = DIV_ROUND_UP(sensor_clock_cycles_to_ns(odr), NSEC_PER_SEC);

	return (uint16_t)MIN(odr, 0x3ff);
}

#endif /* CONFIG_BMA4XX_STREAM */

int bma4xx_configure(const struct device *dev, struct bma4xx_runtime_config *cfg)
{
	struct bma4xx_data *dev_data = dev->data;
	int res;
	uint8_t value;

	/* Disable interrupts, reconfigured at end */
	res = dev_data->hw_ops->write_reg(dev, BMA4XX_REG_INT_MAP_DATA, 0);
	__ASSERT(res == 0, "%s could not disable interrupts", __func__);

	/* If FIFO is enabled now, disable and flush */
	if (dev_data->cfg.fifo_en) {
		res |= dev_data->hw_ops->write_reg(dev, BMA4XX_REG_FIFO_CONFIG_1,
						   FIELD_PREP(BMA4XX_FIFO_ACC_EN, 0));
		__ASSERT(res == 0, "%s could not disable fifo acceleration", __func__);

		res |= dev_data->hw_ops->write_reg(dev, BMA4XX_REG_CMD,
						   FIELD_PREP(BMA4XX_CMD_FIFO_FLUSH, 1));
		__ASSERT(res == 0, "%s could not flush fifo", __func__);
	}

	/* Switch to performance power mode */
	res |= dev_data->hw_ops->write_reg(dev, BMA4XX_REG_ACCEL_CONFIG,
					   FIELD_PREP(BMA4XX_BIT_ACC_PERF_MODE, 1));
	__ASSERT(res == 0, "%s could not enable performance power mode", __func__);

	/* Enable non-latch mode
	 * Regarding the discussion in the Bosch Community, enabling latch mode on bma4xx
	 * might result in multiple FIFO interrupts. Therefore, it is recommended to use
	 * non-latch mode instead.
	 * Reference:
	 * https://community.bosch-sensortec.com/mems-sensors-forum-jrmujtaw/post/bma456-sends-multiple-fifo-interrupt-bma4-fifo-wm-int-vWCT2Uz7Alv6flK
	 */
	res |= dev_data->hw_ops->write_reg(dev, BMA4XX_REG_INT_LATCH, 0);
	__ASSERT(res == 0, "%s could not enable non-latch mode", __func__);

	if (dev_data->cfg.accel_odr > 0) {
		/* Enable accelerometer */
		res |= dev_data->hw_ops->write_reg(dev, BMA4XX_REG_POWER_CTRL,
						   FIELD_PREP(BMA4XX_BIT_POWER_CTRL_ACC_EN, 1));
		__ASSERT(res == 0, "%s could not enable accelerometer", __func__);
	} else {
		LOG_DBG("Sample rate is 0, accelerometer not enabled");
	}

	/* Disable advance power save mode */
	res |= dev_data->hw_ops->write_reg(dev, BMA4XX_REG_POWER_CONF,
					   FIELD_PREP(BMA4XX_BIT_POWER_CONF_ADV_PWR_SAVE, 0));
	__ASSERT(res == 0, "%s could not disable advance power save mode", __func__);

	/* Write acceleration range */
	res |= dev_data->hw_ops->write_reg(dev, BMA4XX_REG_ACCEL_RANGE,
					   FIELD_PREP(BMA4XX_MASK_ACC_RANGE, cfg->accel_fs_range));
	__ASSERT(res == 0, "%s could not write acceleration range", __func__);

	/* Write data rate and bandwidth */
	uint8_t odr_bw_value = FIELD_PREP(BMA4XX_MASK_ACC_CONF_ODR, cfg->accel_odr) |
			       FIELD_PREP(BMA4XX_MASK_ACC_CONF_BWP, cfg->accel_bwp);

	res |= dev_data->hw_ops->write_reg(dev, BMA4XX_REG_ACCEL_CONFIG, odr_bw_value);
	__ASSERT(res == 0, "%s could not write data rate and bandwidth", __func__);

	res |= dev_data->hw_ops->read_reg(dev, BMA4XX_REG_INT_STAT_1, &value);
	__ASSERT(res == 0, "%s could not clear interrupt status", __func__);

#ifdef CONFIG_BMA4XX_STREAM
	if (cfg->fifo_en) {
		LOG_INF("FIFO ENABLED");

		/* Enable FIFO header mode and FIFO acceleration data and
		 * disable FIFO auxiliary data.
		 */
		uint8_t fifo_config_1_value = FIELD_PREP(BMA4XX_FIFO_HEADER_EN, 1) |
					      FIELD_PREP(BMA4XX_FIFO_ACC_EN, 1) |
					      FIELD_PREP(BMA4XX_FIFO_AUX_EN, 0);

		LOG_DBG("FIFO_CONFIG1 (0x%x) 0x%x", BMA4XX_REG_FIFO_CONFIG_1, fifo_config_1_value);
		res |= dev_data->hw_ops->write_reg(dev, BMA4XX_REG_FIFO_CONFIG_1,
						   fifo_config_1_value);
		__ASSERT(res == 0, "%s could not write fifo config 1", __func__);

		uint8_t pwr_ctrl_value = 0;

		res |= dev_data->hw_ops->read_reg(dev, BMA4XX_REG_POWER_CTRL, &pwr_ctrl_value);
		__ASSERT(res == 0, "%s could not read power ctrl value", __func__);

		cfg->accel_pwr_mode = FIELD_GET(BMA4XX_BIT_POWER_CTRL_ACC_EN, pwr_ctrl_value);
		cfg->aux_pwr_mode = FIELD_GET(BMA4XX_BIT_POWER_CTRL_AUX_EN, pwr_ctrl_value);

		/* Set watermark and interrupt handling first */
		uint16_t fifo_wm = bma4xx_compute_fifo_wm(cfg);

		__ASSERT(fifo_wm >= 0, "%s could not get valid watermark value", __func__);

		uint8_t fifo_wml = fifo_wm & 0xFF;

		LOG_DBG("FIFO_WTM_0( (0x%x)) (WM Low) 0x%x", BMA4XX_REG_FIFO_WTM_0, fifo_wml);
		res |= dev_data->hw_ops->write_reg(dev, BMA4XX_REG_FIFO_WTM_0, fifo_wml);
		__ASSERT(res == 0, "%s could not write watermark level LSB", __func__);

		uint8_t fifo_wmh = (fifo_wm >> 8) & 0x0F;

		LOG_DBG("FIFO_WTM_1 (0x%x) (WM High) 0x%x", BMA4XX_REG_FIFO_WTM_1, fifo_wmh);
		res |= dev_data->hw_ops->write_reg(dev, BMA4XX_REG_FIFO_WTM_1, fifo_wmh);
		__ASSERT(res == 0, "%s could not write watermark level MSB", __func__);

		res |= bma4xx_enable_interrupt1(dev, cfg);
		__ASSERT(res == 0, "%s could not enable interrupts", __func__);

	} else {
#endif /* CONFIG_BMA4XX_STREAM */
		LOG_INF("FIFO DISABLED");

		/* No fifo mode so set data ready as interrupt source */
		uint8_t int_map_data = FIELD_PREP(BMA4XX_BIT_INT_MAP_DATA_INT1_DRDY, 1);

		LOG_DBG("MAP_DATA (0x%x) 0x%x", BMA4XX_REG_INT_MAP_DATA, int_map_data);
		res |= dev_data->hw_ops->write_reg(dev, BMA4XX_REG_INT_MAP_DATA, int_map_data);
		__ASSERT(res == 0, "%s could not enable FIFO data ready interrupt", __func__);

#ifdef CONFIG_BMA4XX_STREAM
	}
#endif /* CONFIG_BMA4XX_STREAM */

	return res;
}

int bma4xx_safely_configure(const struct device *dev, struct bma4xx_runtime_config *cfg)
{
	struct bma4xx_data *drv_data = dev->data;
	int ret = bma4xx_configure(dev, cfg);

	if (ret == 0) {
		memcpy(&drv_data->cfg, cfg, sizeof(struct bma4xx_runtime_config));
	} else {
		ret = bma4xx_configure(dev, &drv_data->cfg);
	}

	return ret;
}
