/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <math.h>
#include <string.h>

#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/drivers/sensor/emul_sensor_regmap.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(emul_sensor_regmap, CONFIG_SENSOR_LOG_LEVEL);

#define Q31_SCALE ((int64_t)INT32_MAX + 1)

static const struct emul_sensor_reg *find_reg(const struct emul_sensor_regmap *desc, uint8_t addr)
{
	for (size_t i = 0; i < desc->num_regs; i++) {
		if (desc->regs[i].addr == addr) {
			return &desc->regs[i];
		}
	}

	return NULL;
}

static uint8_t reg_bytes(const struct emul_sensor_regmap *desc, const struct emul_sensor_reg *reg)
{
	if (reg != NULL && reg->bytes != 0U) {
		return reg->bytes;
	}

	return MAX(desc->reg_bytes, 1U);
}

static int transfer(const struct emul *target, struct i2c_msg *msgs, int num_msgs, int addr)
{
	const struct emul_sensor_regmap *cfg = target->cfg;
	struct emul_sensor_regmap_data *data = target->data;
	bool writing = false;
	uint16_t address;
	uint8_t offset = 0;
	uint32_t pending_write = 0;
	int write_reg = -1;
	int ret = 0;

	ARG_UNUSED(addr);
	if (msgs == NULL || num_msgs < 1) {
		return -EINVAL;
	}
	k_mutex_lock(&data->lock, K_FOREVER);
	address = data->ptr;
	offset = data->pos;
	for (int m = 0; m < num_msgs; m++) {
		bool read = (msgs[m].flags & I2C_MSG_READ) != 0U;
		size_t start = 0;

		if (msgs[m].buf == NULL || msgs[m].len == 0U ||
		    (msgs[m].flags & I2C_MSG_ADDR_10_BITS) != 0U) {
			ret = -EINVAL;
			break;
		}
		if (!read && (!writing || (msgs[m].flags & I2C_MSG_RESTART) != 0U)) {
			data->ptr = msgs[m].buf[0] & ~cfg->addr_ignore;
			address = data->ptr;
			offset = 0;
			start = 1;
		}
		for (size_t b = start; b < msgs[m].len; b++) {
			uint8_t byte_offset = offset;
			int reg = address <= UINT8_MAX ? address : -EIO;
			const struct emul_sensor_reg *desc;
			uint32_t old;
			uint32_t mask;
			uint8_t shift;

			if (reg < 0) {
				ret = reg;
				goto out;
			}
			desc = find_reg(cfg, reg);
			if (desc == NULL) {
				ret = -EIO;
				goto out;
			}
			if (byte_offset >= reg_bytes(cfg, desc)) {
				ret = -EIO;
				goto out;
			}
			shift = 8U * (!cfg->big_endian ? byte_offset
						       : reg_bytes(cfg, desc) - byte_offset - 1U);
			old = data->regs[reg];
			LOG_DBG("%s %s (0x%02x) byte %u", read ? "read" : "write", desc->name, reg,
				byte_offset);
			if (byte_offset == 0U || reg != write_reg) {
				pending_write = old;
				write_reg = reg;
			}
			mask = 0xffU << shift;
			if (read) {
				msgs[m].buf[b] = old >> shift;
				if (byte_offset + 1U == reg_bytes(cfg, desc)) {
					data->regs[reg] &= ~desc->clear_on_read;
				}
			} else {
				if ((desc->flags & EMUL_SENSOR_REG_RO) != 0U) {
					mask = 0;
				} else if (desc->write_mask != 0U) {
					mask &= desc->write_mask;
				}
				pending_write = (pending_write & ~mask) |
						(((uint32_t)msgs[m].buf[b] << shift) & mask);
				if (byte_offset + 1U == reg_bytes(cfg, desc)) {
					data->regs[reg] = pending_write & ~desc->self_clear;
				}
			}
			offset++;
			if (offset == reg_bytes(cfg, desc)) {
				offset = 0;
				address++;
			}
		}
		writing = !read && (msgs[m].flags & I2C_MSG_STOP) == 0U;
	}
out:
	data->ptr = address;
	data->pos = offset;
	k_mutex_unlock(&data->lock);
	return ret;
}

static const struct emul_sensor_channel *find_channel(const struct emul_sensor_regmap *desc,
						      struct sensor_chan_spec ch)
{
	if (ch.chan_idx != 0U) {
		return NULL;
	}

	for (size_t i = 0; i < desc->num_channels; i++) {
		if (desc->channels[i].chan == ch.chan_type) {
			return &desc->channels[i];
		}
	}

	return NULL;
}

static struct emul_sensor_field active_field(const struct emul *target,
					     const struct emul_sensor_channel *c)
{
	struct emul_sensor_regmap_data *data = target->data;
	struct emul_sensor_field f = {
		.bits = c->bits, .pos = c->pos, .lsb = c->lsb, .min = c->min, .max = c->max};

	if (c->select.mask != 0U) {
		uint32_t sel = FIELD_GET(c->select.mask, data->regs[c->select.reg]);
		const struct emul_sensor_field *v =
			&c->variants[MIN(sel, ARRAY_SIZE(c->variants) - 1)];

		if (v->bits != 0U) {
			f.bits = v->bits;
		}
		if (v->pos != 0U) {
			f.pos = v->pos;
		}
		if (v->lsb != 0.0) {
			f.lsb = v->lsb;
		}
		if (v->min != 0.0 || v->max != 0.0) {
			f.min = v->min;
			f.max = v->max;
		}
	}

	return f;
}

/* Data word made of the registers holding the field, in the byte order of the device. */
static uint64_t word_get(const struct emul *target, const struct emul_sensor_channel *c,
			 uint8_t nregs, uint8_t bytes)
{
	const struct emul_sensor_regmap *desc = target->cfg;
	struct emul_sensor_regmap_data *data = target->data;
	uint64_t word = 0;

	for (uint8_t k = 0; k < nregs; k++) {
		uint64_t v = data->regs[(uint8_t)(c->reg + k)];

		if (desc->big_endian) {
			word = (word << (8U * bytes)) | v;
		} else {
			word |= v << (8U * bytes * k);
		}
	}

	return word;
}

static void word_set(const struct emul *target, const struct emul_sensor_channel *c, uint8_t nregs,
		     uint8_t bytes, uint64_t word)
{
	const struct emul_sensor_regmap *desc = target->cfg;
	struct emul_sensor_regmap_data *data = target->data;
	uint64_t mask = BIT64_MASK(8U * bytes);

	for (uint8_t k = 0; k < nregs; k++) {
		unsigned int shift = 8U * bytes * (desc->big_endian ? (nregs - 1U - k) : k);

		data->regs[(uint8_t)(c->reg + k)] = (word >> shift) & mask;
	}
}

static double pow2(int e)
{
	double r = 1.0;

	for (; e > 0; e--) {
		r *= 2.0;
	}
	for (; e < 0; e++) {
		r /= 2.0;
	}

	return r;
}

static q31_t to_q31(double v, int8_t shift)
{
	double scaled = v * Q31_SCALE / pow2(shift);

	return (q31_t)CLAMP(scaled, (double)INT32_MIN, (double)INT32_MAX);
}

static void convert_channel(const struct emul *target, const struct emul_sensor_channel *c,
			    double input)
{
	const struct emul_sensor_regmap *desc = target->cfg;
	struct emul_sensor_regmap_data *data = target->data;
	struct emul_sensor_field f;
	uint8_t bytes, nregs;
	double raw_d;
	int64_t raw, lo, hi;
	uint64_t word, mask;

	f = active_field(target, c);
	if (f.bits == 0U || f.bits > 32U || f.pos + f.bits > 64U || f.lsb <= 0.0 ||
	    !isfinite(f.lsb) || !isfinite(c->offset) || !isfinite(f.min) || !isfinite(f.max)) {
		return;
	}
	raw_d = (input - c->offset) / f.lsb;
	if (c->is_signed) {
		lo = -(1LL << (f.bits - 1U));
		hi = (1LL << (f.bits - 1U)) - 1;
	} else {
		lo = 0;
		hi = (1LL << f.bits) - 1;
	}
	raw_d = CLAMP(raw_d, (double)lo, (double)hi);
	raw = (int64_t)(raw_d + (raw_d >= 0.0 ? 0.5 : -0.5));

	bytes = reg_bytes(desc, find_reg(desc, c->reg));
	nregs = DIV_ROUND_UP(f.pos + f.bits, 8U * bytes);
	mask = BIT64_MASK(f.bits) << f.pos;
	word = word_get(target, c, nregs, bytes);
	word = (word & ~mask) | (((uint64_t)raw << f.pos) & mask);
	word_set(target, c, nregs, bytes, word);

	if (c->ready.mask != 0U) {
		data->regs[c->ready.reg] |= c->ready.mask;
	}
}

static int regmap_set_channel(const struct emul *target, struct sensor_chan_spec ch,
			      const q31_t *value, int8_t shift)
{
	const struct emul_sensor_regmap *desc = target->cfg;
	struct emul_sensor_regmap_data *data = target->data;
	const struct emul_sensor_channel *c = find_channel(desc, ch);

	if (value == NULL || shift < -31 || shift > 31) {
		return -EINVAL;
	}
	if (c == NULL) {
		return -ENOTSUP;
	}
	k_mutex_lock(&data->lock, K_FOREVER);
	double input = (double)*value * pow2(shift) / Q31_SCALE;
	struct emul_sensor_field f = active_field(target, c);

	if (f.bits == 0U || f.bits > 32U || f.pos + f.bits > 64U || f.lsb <= 0.0 ||
	    !isfinite(f.lsb) || !isfinite(c->offset) || !isfinite(f.min) || !isfinite(f.max)) {
		k_mutex_unlock(&data->lock);
		return -ENOTSUP;
	}
	if ((f.min != 0.0 || f.max != 0.0) && (input < f.min - f.lsb || input > f.max + f.lsb)) {
		k_mutex_unlock(&data->lock);
		return -ERANGE;
	}
	convert_channel(target, c, input);
	k_mutex_unlock(&data->lock);
	return 0;
}

static int regmap_get_sample_range(const struct emul *target, struct sensor_chan_spec ch,
				   q31_t *lower, q31_t *upper, q31_t *epsilon, int8_t *shift)
{
	const struct emul_sensor_regmap *desc = target->cfg;
	const struct emul_sensor_channel *c = find_channel(desc, ch);
	struct emul_sensor_field f;
	double lo, hi, absmax;
	int8_t s = 0;

	if (lower == NULL || upper == NULL || epsilon == NULL || shift == NULL) {
		return -EINVAL;
	}
	if (c == NULL) {
		return -ENOTSUP;
	}

	struct emul_sensor_regmap_data *data = target->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	f = active_field(target, c);
	if (f.bits == 0U || f.bits > 32U || f.pos + f.bits > 64U || f.lsb <= 0.0 ||
	    !isfinite(f.lsb) || !isfinite(c->offset) || !isfinite(f.min) || !isfinite(f.max)) {
		k_mutex_unlock(&data->lock);
		return -ENOTSUP;
	}
	lo = f.min;
	hi = f.max;
	if (lo == 0.0 && hi == 0.0) {
		if (c->is_signed) {
			lo = c->offset - pow2(f.bits - 1) * f.lsb;
			hi = c->offset + (pow2(f.bits - 1) - 1.0) * f.lsb;
		} else {
			lo = c->offset;
			hi = c->offset + (pow2(f.bits) - 1.0) * f.lsb;
		}
	}

	absmax = MAX(lo < 0.0 ? -lo : lo, hi < 0.0 ? -hi : hi);
	while (absmax >= pow2(s) && s < 31) {
		s++;
	}

	*shift = s;
	*lower = to_q31(lo, s);
	*upper = to_q31(hi, s);
	*epsilon = MAX(1, to_q31(f.lsb, s));
	k_mutex_unlock(&data->lock);

	return 0;
}

int emul_sensor_regmap_init(const struct emul *target, const struct device *parent)
{
	const struct emul_sensor_regmap *desc = target->cfg;
	struct emul_sensor_regmap_data *data = target->data;

	ARG_UNUSED(parent);

	if (desc->regs == NULL || desc->num_regs == 0U || desc->num_regs > 256U) {
		return -EINVAL;
	}
	for (size_t i = 0; i < desc->num_regs; i++) {
		if (reg_bytes(desc, &desc->regs[i]) > 4U) {
			return -EINVAL;
		}
	}
	if (desc->num_channels > 0U && desc->channels == NULL) {
		return -EINVAL;
	}
	k_mutex_init(&data->lock);
	memset(data->regs, 0, sizeof(data->regs));
	data->ptr = 0;
	data->pos = 0;
	for (size_t i = 0; i < desc->num_regs; i++) {
		data->regs[desc->regs[i].addr] = desc->regs[i].reset;
	}
	return 0;
}

const struct i2c_emul_api emul_sensor_regmap_i2c_api = {
	.transfer = transfer,
};

const struct emul_sensor_driver_api emul_sensor_regmap_backend_api = {
	.set_channel = regmap_set_channel,
	.get_sample_range = regmap_get_sample_range,
};
