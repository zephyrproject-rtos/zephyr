/*
 * SPDX-FileCopyrightText: Copyright (c) Michele Tavecchio <tavecchiomichele03@gmail.com>
 * SPDX-FileCopyrightText: Copyright (c) Alessandro Sola <alessandrosola03@gmail.com>
 * SPDX-FileCopyrightText: Copyright (c) 2017 Melexis N.V.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT melexis_mlx90632

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_data_types.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include "mlx90632.h"
#include <math.h>

LOG_MODULE_REGISTER(MLX90632, CONFIG_SENSOR_LOG_LEVEL);

static int mlx90632_reg_read(const struct device *dev, uint16_t reg, uint16_t *val)
{
	const struct mlx90632_config *config = dev->config;
	uint8_t reg_addr[2] = {reg >> 8, reg & 0xFF};
	uint8_t data[2];
	int ret;

	ret = i2c_write_read_dt(&config->i2c, reg_addr, sizeof(reg_addr), data, sizeof(data));
	if (ret < 0) {
		LOG_ERR("Failed to read register 0x%04x (rc=%d)", reg, ret);
		return ret;
	}
	*val = sys_get_be16(data);
	return 0;
}

static int mlx90632_reg_read_s16(const struct device *dev, uint16_t reg, int16_t *val)
{
	uint16_t raw;
	int ret = mlx90632_reg_read(dev, reg, &raw);

	if (ret < 0) {
		return ret;
	}

	*val = (int16_t)raw;
	return 0;
}

static uint16_t mlx90632_get_control_bits(void)
{
	uint16_t reg = 0;

#if defined(CONFIG_MLX90632_MODE_SLEEPING_STEP)
	reg |= (0x01 << 1);
#elif defined(CONFIG_MLX90632_MODE_STEP)
	reg |= (0x02 << 1);
#else
	reg |= (0x03 << 1);
#endif
	return reg;
}

static int mlx90632_read_32bit_param(const struct device *dev, uint16_t addr, int32_t *dest)
{
	uint16_t ls, ms;
	int ret;

	ret = mlx90632_reg_read(dev, addr, &ls);
	if (ret < 0) {
		return ret;
	}

	ret = mlx90632_reg_read(dev, addr + 1, &ms);
	if (ret < 0) {
		return ret;
	}

	*dest = (int32_t)((uint32_t)ms << 16 | ls);
	return 0;
}

static int mlx90632_reg_write(const struct device *dev, uint16_t reg, uint16_t val)
{
	const struct mlx90632_config *config = dev->config;
	uint8_t tx_data[4];
	int ret;

	tx_data[0] = reg >> 8;
	tx_data[1] = reg & 0xFF;
	tx_data[2] = (val >> 8) & 0xFF;
	tx_data[3] = val & 0xFF;

	ret = i2c_write_dt(&config->i2c, tx_data, sizeof(tx_data));
	if (ret < 0) {
		LOG_ERR("Failed to write register 0x%04x (rc=%d)", reg, ret);
		return ret;
	}

	return 0;
}

static int mlx90632_load_calibration(const struct device *dev)
{
	struct mlx90632_data *data = dev->data;
	int ret;

	ret = mlx90632_read_32bit_param(dev, MLX90632_EE_P_R, &data->p_r);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_read_32bit_param(dev, MLX90632_EE_P_G, &data->p_g);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_read_32bit_param(dev, MLX90632_EE_P_T, &data->p_t);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_read_32bit_param(dev, MLX90632_EE_P_O, &data->p_o);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_read_32bit_param(dev, MLX90632_EE_Aa, &data->aa);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_read_32bit_param(dev, MLX90632_EE_Ab, &data->ab);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_read_32bit_param(dev, MLX90632_EE_Ba, &data->ba);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_read_32bit_param(dev, MLX90632_EE_Bb, &data->bb);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_read_32bit_param(dev, MLX90632_EE_Ca, &data->ca);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_read_32bit_param(dev, MLX90632_EE_Cb, &data->cb);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_read_32bit_param(dev, MLX90632_EE_Da, &data->da);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_read_32bit_param(dev, MLX90632_EE_Db, &data->db);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_read_32bit_param(dev, MLX90632_EE_Ea, &data->ea);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_read_32bit_param(dev, MLX90632_EE_Eb, &data->eb);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_read_32bit_param(dev, MLX90632_EE_Fa, &data->fa);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_read_32bit_param(dev, MLX90632_EE_Fb, &data->fb);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_read_32bit_param(dev, MLX90632_EE_Ga, &data->ga);
	if (ret < 0) {
		return ret;
	}

	ret = mlx90632_reg_read_s16(dev, MLX90632_EE_Gb, &data->gb);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_reg_read_s16(dev, MLX90632_EE_Ka, &data->ka);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_reg_read_s16(dev, MLX90632_EE_Ha, &data->ha);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_reg_read_s16(dev, MLX90632_EE_Hb, &data->hb);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int mlx90632_set_refresh_rate(const struct device *dev)
{
	uint16_t rate = CONFIG_MLX90632_REFRESH_RATE;
	uint16_t current_ee;
	uint16_t target_ee1, target_ee2, target_ee3;
	int ret;
	int num_regs;

	if (IS_ENABLED(CONFIG_MLX90632_EXTENDED_MODE)) {
		target_ee1 = 0x8000 | (rate << 8);
		target_ee2 = 0x8012 | (rate << 8);
		target_ee3 = 0x800C | (rate << 8);
		num_regs = 3;
	} else {
		target_ee1 = 0x800D | (rate << 8);
		target_ee2 = 0x801D | (rate << 8);
		target_ee3 = 0;
		num_regs = 2;
	}

	ret = mlx90632_reg_read(dev,
				IS_ENABLED(CONFIG_MLX90632_EXTENDED_MODE)
					? MLX90632_EE_EXTENDED_MEAS1
					: MLX90632_EE_MEDICAL_MEAS1,
				&current_ee);
	if (ret == 0 && current_ee == target_ee1) {
		return 0;
	}

	ret = mlx90632_reg_write(dev, MLX90632_REG_CTRL, 0x0000);
	if (ret < 0) {
		return ret;
	}
	uint16_t addrs[3];
	uint16_t vals[3];

	if (IS_ENABLED(CONFIG_MLX90632_EXTENDED_MODE)) {
		addrs[0] = MLX90632_EE_EXTENDED_MEAS1;
		vals[0] = target_ee1;
		addrs[1] = MLX90632_EE_EXTENDED_MEAS2;
		vals[1] = target_ee2;
		addrs[2] = MLX90632_EE_EXTENDED_MEAS3;
		vals[2] = target_ee3;
	} else {
		addrs[0] = MLX90632_EE_MEDICAL_MEAS1;
		vals[0] = target_ee1;
		addrs[1] = MLX90632_EE_MEDICAL_MEAS2;
		vals[1] = target_ee2;
	}

	for (int i = 0; i < num_regs; i++) {
		ret = mlx90632_reg_write(dev, MLX90632_REG_UNLOCK, MLX90632_EE_UNLOCK_KEY);
		if (ret < 0) {
			return ret;
		}

		ret = mlx90632_reg_write(dev, addrs[i], 0x0000);
		if (ret < 0) {
			return ret;
		}

		k_msleep(10);
		ret = mlx90632_reg_write(dev, MLX90632_REG_UNLOCK, MLX90632_EE_UNLOCK_KEY);
		if (ret < 0) {
			return ret;
		}

		ret = mlx90632_reg_write(dev, addrs[i], vals[i]);
		if (ret < 0) {
			return ret;
		}

		k_msleep(10);
	}

	ret = mlx90632_reg_write(dev, MLX90632_REG_UNLOCK, MLX90632_RESET_CMD);
	k_msleep(100);

	return ret;
}

static void mlx90632_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct mlx90632_data *data = CONTAINER_OF(dwork, struct mlx90632_data, data_work);
	const struct device *dev = data->dev;

	uint16_t reg_status;
	int ret;

	ret = mlx90632_reg_read(dev, MLX90632_REG_STATUS, &reg_status);
	if (ret < 0) {
		data->work_ret = ret;
		k_sem_give(&data->data_sem);
		return;
	}

	if (reg_status & MLX90632_STAT_DATA_RDY) {
		data->work_ret = 0;
		k_sem_give(&data->data_sem);
	} else {
		k_work_reschedule(&data->data_work, K_MSEC(10));
	}
}

static int mlx90632_init(const struct device *dev)
{
	const struct mlx90632_config *cfg = dev->config;
	struct mlx90632_data *data = dev->data;
	uint16_t ee_version;
	uint16_t reg_status;
	uint16_t dummy;
	uint16_t reg_ctrl;
	int ret;

	data->dev = dev;
	k_sem_init(&data->data_sem, 0, 1);
	k_work_init_delayable(&data->data_work, mlx90632_work_handler);

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->i2c.bus->name);
		return -ENODEV;
	}

	k_msleep(MLX90632_STARTUP_TIME_MS);

	/* dummy read required for 1V8 I2C communication variant */
	(void)mlx90632_reg_read(dev, MLX90632_REG_STATUS, &dummy);

	ret = mlx90632_reg_read(dev, MLX90632_EE_VERSION, &ee_version);
	if (ret < 0) {
		LOG_ERR("Device not responding at 0x%02x (rc=%d)", cfg->i2c.addr, ret);
		return -ENODEV;
	}

	if ((ee_version & 0x00FF) != MLX90632_DSPv5) {
		LOG_ERR("Unsupported DSP version 0x%02x", ee_version & 0x00FF);
		return -ENOTSUP;
	}

	ret = mlx90632_reg_read(dev, MLX90632_REG_STATUS, &reg_status);
	if (ret < 0) {
		return ret;
	}

	reg_status |= MLX90632_STAT_BROWN_OUT;
	reg_status &= ~MLX90632_STAT_DATA_RDY;

	ret = mlx90632_reg_write(dev, MLX90632_REG_STATUS, reg_status);
	if (ret < 0) {
		return ret;
	}

	ret = mlx90632_load_calibration(dev);
	if (ret < 0) {
		return ret;
	}

	int was_reset = mlx90632_set_refresh_rate(dev);

	if (was_reset < 0) {
		return was_reset;
	}

	ret = mlx90632_reg_read(dev, MLX90632_REG_CTRL, &reg_ctrl);
	if (ret < 0) {
		return ret;
	}

	reg_ctrl &= ~(0x03 << 1);
	reg_ctrl &= ~(0x1F << 4);

#if IS_ENABLED(CONFIG_MLX90632_EXTENDED_MODE)
	reg_ctrl |= (0x11 << 4);
#endif

	reg_ctrl |= mlx90632_get_control_bits();

	ret = mlx90632_reg_write(dev, MLX90632_REG_CTRL, reg_ctrl);
	if (ret < 0) {
		return ret;
	}
	return 0;
}

#ifndef CONFIG_MLX90632_MODE_CONTINUOUS
static int mlx90632_trigger_measurement(const struct device *dev)
{
	uint16_t reg_ctrl;
	int ret;

	ret = mlx90632_reg_read(dev, MLX90632_REG_CTRL, &reg_ctrl);
	if (ret < 0) {
		return ret;
	}

	reg_ctrl |= MLX90632_BIT_SOB;
	return mlx90632_reg_write(dev, MLX90632_REG_CTRL, reg_ctrl);
}
#endif

static int32_t mlx90632_calc_amb(struct mlx90632_data *data)
{
	int64_t am_ta = ((int64_t)data->ram_6 * 1000LL) / 12LL;
	int64_t KGb = ((int64_t)data->gb * 1000LL) >> 10ULL;
	int64_t vr_ta = (int64_t)data->ram_9 * 1000000LL + (KGb * am_ta);
	int64_t tmp = (((am_ta * 1000000000LL) / vr_ta) << 19ULL);

	tmp = tmp / 1000LL;
	int64_t Asub, Bsub, Ablock, Bblock, Cblock;

	Asub = ((int64_t)data->p_t * 10000000000LL) >> 44ULL;
	Bsub = tmp - (((int64_t)data->p_r * 1000LL) >> 8ULL);
	Ablock = Asub * (Bsub * Bsub);
	Bblock = ((Bsub * 10000000LL) / data->p_g) << 20ULL;
	Cblock = ((int64_t)data->p_o * 10000000000LL) >> 8ULL;
	int64_t sum = (Ablock / 1000000LL) + Bblock + Cblock;

	return (int32_t)(sum / 10000000LL);
}

static int64_t mlx90632_preprocess_object(struct mlx90632_data *data)
{
	int64_t kKa = ((int64_t)data->ka * 1000LL) >> 10ULL;

	int64_t vr_ir =
		(int64_t)data->ram_9 * 1000000LL + kKa * (((int64_t)data->ram_6 * 1000LL) / 12LL);

	int64_t tmp;

	if (data->last_cycle == 1) {
		tmp = ((((int64_t)data->ram_4 + data->ram_5) * 500000000000LL) / 12LL) / vr_ir;
	} else {
		tmp = ((((int64_t)data->ram_7 + data->ram_8) * 500000000000LL) / 12LL) / vr_ir;
	}
	return (tmp * 524288LL) / 1000LL;
}

static uint32_t int_sqrt(uint64_t x)
{
	uint64_t res = 0;
	uint64_t bit = 1ULL << 62;

	while (bit > x) {
		bit >>= 2;
	}

	while (bit != 0) {
		if (x >= res + bit) {
			x -= res + bit;
			res = (res >> 1) + bit;
		} else {
			res >>= 1;
		}
		bit >>= 2;
	}
	return (uint32_t)res;
}

static int32_t mlx90632_calc_object(struct mlx90632_data *data)
{
	int64_t v_ir = mlx90632_preprocess_object(data);
	int64_t t_amb = data->ambient_temp;

	int64_t kGa = ((int64_t)data->ga * 100000LL) >> 20ULL;
	int64_t kFb;
	int64_t t_adut;

	int64_t t_amb_k = (t_amb + 273150LL) / 10LL;

	int64_t kEa = ((int64_t)data->ea * 1000LL) >> 16LL;
	int64_t kEb = ((int64_t)data->eb * 1000LL) >> 8LL;

	t_adut = (((t_amb - kEb) * 1000000LL) / kEa) + 25000000LL;

	kFb = (((int64_t)data->fb * (t_adut - 25000000LL)) >> 36ULL) / 10LL;

	int64_t v_ir_comp =
		(v_ir * 100000LL) / (100000LL + (kGa * (t_amb - 25000LL)) / 1000LL + kFb);

	int64_t term = v_ir_comp * (1759218604441600000LL / data->fa);

	int64_t t_obj_k = (int64_t)int_sqrt(int_sqrt(term + (uint64_t)(t_amb_k * t_amb_k) *
								    (t_amb_k * t_amb_k))) *
			  10LL;

	return (int32_t)(t_obj_k - 273150LL);
}

static int mlx90632_read_extended_channels(const struct device *dev)
{
	struct mlx90632_data *data = dev->data;
	int ret;

	ret = mlx90632_reg_read_s16(dev, MLX90632_RAM_52, &data->ram_52);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_reg_read_s16(dev, MLX90632_RAM_53, &data->ram_53);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_reg_read_s16(dev, MLX90632_RAM_54, &data->ram_54);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_reg_read_s16(dev, MLX90632_RAM_55, &data->ram_55);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_reg_read_s16(dev, MLX90632_RAM_56, &data->ram_56);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_reg_read_s16(dev, MLX90632_RAM_57, &data->ram_57);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_reg_read_s16(dev, MLX90632_RAM_58, &data->ram_58);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_reg_read_s16(dev, MLX90632_RAM_59, &data->ram_59);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_reg_read_s16(dev, MLX90632_RAM_60, &data->ram_60);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int32_t mlx90632_calc_ambient_extended(struct mlx90632_data *data)
{
	int64_t am_ta = ((int64_t)data->ram_54 * 1000LL) / 12LL;
	int64_t KGb = ((int64_t)data->gb * 1000LL) >> 10ULL;
	int64_t vr_ta = (int64_t)data->ram_57 * 1000000LL + (KGb * am_ta);
	int64_t tmp_ta = (((am_ta * 1000000000LL) / vr_ta) << 19ULL);

	tmp_ta = tmp_ta / 1000LL;

	int64_t Asub, Bsub, Ablock, Bblock, Cblock;

	Asub = ((int64_t)data->p_t * 10000000000LL) >> 44ULL;
	Bsub = tmp_ta - (((int64_t)data->p_r * 1000LL) >> 8ULL);
	Ablock = Asub * (Bsub * Bsub);
	Bblock = ((Bsub * 10000000LL) / data->p_g) << 20ULL;
	Cblock = ((int64_t)data->p_o * 10000000000LL) >> 8ULL;

	int64_t sum = (Ablock / 1000000LL) + Bblock + Cblock;

	return (int32_t)(sum / 10000000LL);
}

static int32_t mlx90632_calc_object_extended(struct mlx90632_data *data)
{
	int64_t t_amb = data->ambient_temp;

	int64_t kKa = ((int64_t)data->ka * 1000LL) >> 10ULL;
	int64_t am_ta = ((int64_t)data->ram_54 * 1000LL) / 12LL;
	int64_t vr_to = (int64_t)data->ram_57 * 1000000LL + (kKa * am_ta);

	int64_t s =
		(((int64_t)data->ram_52 - data->ram_53 - data->ram_55 + data->ram_56) * 500000LL) +
		((int64_t)data->ram_58 + data->ram_59) * 1000000LL;

	int64_t tmp_s = ((s / 12LL) * 1000000LL) / vr_to;
	int64_t v_ir = (tmp_s * 524288LL) / 1000LL;

	int64_t kFa = data->fa / 2LL;
	int64_t kGa = ((int64_t)data->ga * 100000LL) >> 20ULL;
	int64_t kFb;
	int64_t t_adut;

	int64_t t_amb_k = (t_amb + 273150LL) / 10LL;

	int64_t kEa = ((int64_t)data->ea * 1000LL) >> 16LL;
	int64_t kEb = ((int64_t)data->eb * 1000LL) >> 8LL;

	t_adut = (((t_amb - kEb) * 1000000LL) / kEa) + 25000000LL;

	kFb = (((int64_t)data->fb * (t_adut - 25000000LL)) >> 36ULL) / 10LL;

	int64_t v_ir_comp =
		(v_ir * 100000LL) / (100000LL + (kGa * (t_amb - 25000LL)) / 1000LL + kFb);

	int64_t term = v_ir_comp * (1759218604441600000LL / kFa);

	int64_t t_obj_k = (int64_t)int_sqrt(int_sqrt(term + (uint64_t)(t_amb_k * t_amb_k) *
								    (t_amb_k * t_amb_k))) *
			  10LL;

	return (int32_t)(t_obj_k - 273150LL);
}

static int mlx90632_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct mlx90632_data *data = dev->data;
	uint16_t reg_status;
	int ret;

	ARG_UNUSED(chan);

#ifndef CONFIG_MLX90632_MODE_CONTINUOUS
	ret = mlx90632_trigger_measurement(dev);
	if (ret < 0) {
		return ret;
	}
#endif

	k_sem_reset(&data->data_sem);
	k_work_schedule(&data->data_work, K_NO_WAIT);

	ret = k_sem_take(&data->data_sem, K_SECONDS(2));
	if (ret == -EAGAIN) {
		struct k_work_sync sync;

		(void)k_work_cancel_delayable_sync(&data->data_work, &sync);
		LOG_ERR("Timeout waiting for sensor data");
		return -ETIMEDOUT;
	}

	if (data->work_ret < 0) {
		return data->work_ret;
	}
	ret = mlx90632_reg_read(dev, MLX90632_REG_STATUS, &reg_status);
	if (ret < 0) {
		return ret;
	}
	uint8_t cycle_pos = (reg_status >> 2) & 0x1F;

	data->last_cycle = cycle_pos;

	if (IS_ENABLED(CONFIG_MLX90632_EXTENDED_MODE)) {
		ret = mlx90632_read_extended_channels(dev);
		if (ret < 0) {
			return ret;
		}

		data->ambient_temp = mlx90632_calc_ambient_extended(data);

	} else {
		ret = mlx90632_reg_read_s16(dev, MLX90632_RAM_6, &data->ram_6);
		if (ret < 0) {
			return ret;
		}
		ret = mlx90632_reg_read_s16(dev, MLX90632_RAM_9, &data->ram_9);
		if (ret < 0) {
			return ret;
		}
		if (cycle_pos == 1) {
			ret = mlx90632_reg_read_s16(dev, MLX90632_RAM_4, &data->ram_4);
			if (ret < 0) {
				return ret;
			}
			ret = mlx90632_reg_read_s16(dev, MLX90632_RAM_5, &data->ram_5);
			if (ret < 0) {
				return ret;
			}
		} else {
			ret = mlx90632_reg_read_s16(dev, MLX90632_RAM_7, &data->ram_7);
			if (ret < 0) {
				return ret;
			}
			ret = mlx90632_reg_read_s16(dev, MLX90632_RAM_8, &data->ram_8);
			if (ret < 0) {
				return ret;
			}
		}
		data->ambient_temp = mlx90632_calc_amb(data);
	}

	ret = mlx90632_reg_write(dev, MLX90632_REG_STATUS, reg_status & ~MLX90632_STAT_DATA_RDY);
	if (ret < 0) {
		return ret;
	}

	if (IS_ENABLED(CONFIG_MLX90632_EXTENDED_MODE)) {
		data->object_temp = mlx90632_calc_object_extended(data);
	} else {
		data->object_temp = mlx90632_calc_object(data);
	}

	return 0;
}

static int mlx90632_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct mlx90632_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_DIE_TEMP:
		val->val1 = data->ambient_temp / 1000;
		val->val2 = (data->ambient_temp % 1000) * 1000;
		break;

	case SENSOR_CHAN_AMBIENT_TEMP:
		val->val1 = data->object_temp / 1000;
		val->val2 = (data->object_temp % 1000) * 1000;
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static DEVICE_API(sensor, mlx90632_driver_api) = {
	.sample_fetch = mlx90632_sample_fetch,
	.channel_get = mlx90632_channel_get,
};

#define MLX90632_DEFINE(inst)                                                                      \
	static struct mlx90632_data mlx90632_data_##inst;                                          \
	static const struct mlx90632_config mlx90632_config_##inst = {                             \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, mlx90632_init, NULL, &mlx90632_data_##inst,             \
				     &mlx90632_config_##inst, POST_KERNEL,                         \
				     CONFIG_SENSOR_INIT_PRIORITY, &mlx90632_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MLX90632_DEFINE)
