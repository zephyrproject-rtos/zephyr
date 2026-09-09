/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_npm10xx_uicr

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/otp.h>
#include <zephyr/drivers/otp/npm10xx.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/math_extras.h>

LOG_MODULE_REGISTER(otp_npm10xx, CONFIG_OTP_LOG_LEVEL);

#define NPM10_OTP_TASKS      0xECU
#define NPM10_OTP_EVENTS_SET 0xEDU
#define NPM10_OTP_EVENTS_CLR 0xEEU
#define NPM10_OTP_PROGMODE   0xF1U
#define NPM10_OTP_LOCK       0xF2U
#define NPM10_OTP_READLOCK   0xF3U
#define NPM10_OTP_REQUEST    0xF4U
#define NPM10_OTP_ADDR0      0xF5U
#define NPM10_OTP_READDATA   0xF7U

/* TASKS (0xEC) */
#define OTP_TASKS_PROG_Msk BIT(0)
#define OTP_TASKS_READ_Msk BIT(1)

/* EVENTS_SET (0xED) / EVENTS_CLR (0xEE) */
#define OTP_EVENTS_USERPROGMODE_Msk    BIT(0)
#define OTP_EVENTS_USERPROGSUCC_Msk    BIT(1)
#define OTP_EVENTS_USERLOCKERR_Msk     BIT(2)
#define OTP_EVENTS_USERADDRERR_Msk     BIT(3)
#define OTP_EVENTS_USERREADMODE_Msk    BIT(4)
#define OTP_EVENTS_USERREAD_Msk        BIT(5)
#define OTP_EVENTS_USERREADCOMPT_Msk   BIT(6)
#define OTP_EVENTS_USERREADADDRERR_Msk BIT(7)
#define OTP_EVENTS_ALL_Msk             0xFFU

/* PROGMODE (0xF1) */
#define OTP_PROGMODE_STATUS_Msk BIT(0)
#define OTP_PROGMODE_READ_Msk   BIT(1)

/* READLOCK (0xF3) */
#define OTP_READLOCK_STATE BIT(0)

/* REQUEST (0xF4) */
#define OTP_REQUEST_USERPROGMODE_Msk BIT(0)
#define OTP_REQUEST_USERREADMODE_Msk BIT(1)
#define OTP_REQUEST_MARGINMODE_Msk   (BIT_MASK(2) << 4)

#define OTP_POLL_INTERVAL_US 1000
#define OTP_POLL_COUNT       10U

#define LOCK_BIT_BYTE_IDX (OTP_NPM10XX_UICR_LOCK_BIT / 8)
#define LOCK_BIT_BYTE_POS (OTP_NPM10XX_UICR_LOCK_BIT % 8)

struct otp_npm10xx_config {
	struct i2c_dt_spec i2c;
	uint8_t margin_mode;
};

struct otp_npm10xx_data {
	struct k_mutex lock;
};

/* Poll a register until all bits in mask are set, or timeout. */
static int npm10xx_otp_poll_set(const struct i2c_dt_spec *i2c, uint8_t reg, uint8_t mask)
{
	int i, ret;
	uint8_t val;

	for (i = 0U; i < OTP_POLL_COUNT; i++) {
		ret = i2c_reg_read_byte_dt(i2c, reg, &val);
		if (ret < 0) {
			return ret;
		}

		if ((val & mask) == mask) {
			return 0;
		}

		k_busy_wait(OTP_POLL_INTERVAL_US);
	}

	return -ETIMEDOUT;
}

/* Poll a register until any bit in mask is set, returning the read value in *out, or timeout. */
static int npm10xx_otp_poll_any(const struct i2c_dt_spec *i2c, uint8_t reg, uint8_t mask,
				uint8_t *out)
{
	int i, ret;
	uint8_t val;

	for (i = 0U; i < OTP_POLL_COUNT; i++) {
		ret = i2c_reg_read_byte_dt(i2c, reg, &val);
		if (ret < 0) {
			return ret;
		}

		if ((val & mask) != 0U) {
			*out = val;
			return 0;
		}

		k_busy_wait(OTP_POLL_INTERVAL_US);
	}

	return -ETIMEDOUT;
}

#if defined(CONFIG_OTP_PROGRAM)
/* Poll a register until all bits in mask are cleared, or timeout. */
static int npm10xx_otp_poll_clear(const struct i2c_dt_spec *i2c, uint8_t reg, uint8_t mask)
{
	int i, ret;
	uint8_t val;

	for (i = 0U; i < OTP_POLL_COUNT; i++) {
		ret = i2c_reg_read_byte_dt(i2c, reg, &val);
		if (ret < 0) {
			return ret;
		}

		if ((val & mask) == 0U) {
			return 0;
		}

		k_busy_wait(OTP_POLL_INTERVAL_US);
	}

	return -ETIMEDOUT;
}
#endif /* CONFIG_OTP_PROGRAM */

static int npm10xx_uicr_do_read(const struct i2c_dt_spec *i2c, uint8_t *buf, uint8_t start,
				uint8_t len, uint8_t margin)
{
	int ret;
	uint8_t events, bit_addr;

	/* UICR reading sequence, ref. datasheet chapter 4.2.12 */
	/* (Step 1 is outside of driver's control) */
	/* 2. Power up the OTP memory in read mode
	 * 3. Optional: select margin read level (same register, single write is okay)
	 */
	ret = i2c_reg_write_byte_dt(i2c, NPM10_OTP_REQUEST,
				    OTP_REQUEST_USERREADMODE_Msk |
					    FIELD_PREP(OTP_REQUEST_MARGINMODE_Msk, margin));
	if (ret < 0) {
		return ret;
	}

	for (uint8_t i = 0U; i < len; i++) {
		bit_addr = (start + i) * 8U;

		/* 4. Clear events */
		ret = i2c_reg_write_byte_dt(i2c, NPM10_OTP_EVENTS_CLR, OTP_EVENTS_ALL_Msk);
		if (ret < 0) {
			goto exit_read_mode;
		}

		/* 5. Write start address */
		ret = i2c_reg_write_byte_dt(i2c, NPM10_OTP_ADDR0, bit_addr);
		if (ret < 0) {
			goto exit_read_mode;
		}

		/* 6. Activate UICR read */
		ret = i2c_reg_write_byte_dt(i2c, NPM10_OTP_TASKS, OTP_TASKS_READ_Msk);
		if (ret < 0) {
			goto exit_read_mode;
		}

		/* 7. Poll events and errors. Once USERREAD=1, an 8-bit chunk has been read */
		ret = npm10xx_otp_poll_any(i2c, NPM10_OTP_EVENTS_SET,
					   OTP_EVENTS_USERREAD_Msk | OTP_EVENTS_USERREADADDRERR_Msk,
					   &events);
		if (ret < 0) {
			LOG_ERR("UICR read chunk %u timed out (%d)", i, ret);
			goto exit_read_mode;
		}

		if (events & OTP_EVENTS_USERREADADDRERR_Msk) {
			LOG_ERR("UICR read address error at bit %u", bit_addr);
			ret = -EIO;
			goto exit_read_mode;
		}

		/* 8. Read READDATA */
		ret = i2c_reg_read_byte_dt(i2c, NPM10_OTP_READDATA, &buf[i]);
		if (ret < 0) {
			goto exit_read_mode;
		}

		/* 9. Poll USERREADCOMPT */
		ret = npm10xx_otp_poll_set(i2c, NPM10_OTP_EVENTS_SET, OTP_EVENTS_USERREADCOMPT_Msk);
		if (ret < 0) {
			LOG_ERR("UICR read completion timed out at chunk %u (%d)", i, ret);
			goto exit_read_mode;
		}

		/* 10. Repeat incrementing the start address by 8 */
	}

	/* 11. Exit reading mode */
exit_read_mode: {
	int exit_ret = i2c_reg_write_byte_dt(i2c, NPM10_OTP_REQUEST, 0U);

	if (ret == 0) {
		ret = exit_ret;
	}
}

	return ret;
}

static int otp_npm10xx_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	const struct otp_npm10xx_config *config = dev->config;
	struct otp_npm10xx_data *dev_data = dev->data;
	size_t end;
	int ret;

	if ((offset < 0) || size_add_overflow((size_t)offset, len, &end) ||
	    (end > OTP_NPM10XX_UICR_SIZE)) {
		LOG_ERR("UICR read out of bounds [0,%u)", OTP_NPM10XX_UICR_SIZE);
		return -EINVAL;
	}

	if (len == 0U) {
		return 0;
	}

	(void)k_mutex_lock(&dev_data->lock, K_FOREVER);
	ret = npm10xx_uicr_do_read(&config->i2c, data, (uint8_t)offset, (uint8_t)len,
				   config->margin_mode);
	(void)k_mutex_unlock(&dev_data->lock);

	return ret;
}

#if defined(CONFIG_OTP_PROGRAM)
/* Program a single UICR bit at the given absolute bit address. */
static int npm10xx_uicr_bit_program(const struct i2c_dt_spec *i2c, uint8_t bit_addr)
{
	uint8_t events;
	int ret;

	/* UICR programming sequence, ref. datasheet chapter 4.2.12 */
	/* 6. Clear events */
	ret = i2c_reg_write_byte_dt(i2c, NPM10_OTP_EVENTS_CLR, OTP_EVENTS_ALL_Msk);
	if (ret < 0) {
		return ret;
	}

	/* 7. Set UICR bit address in OTP.ADDR0 */
	ret = i2c_reg_write_byte_dt(i2c, NPM10_OTP_ADDR0, bit_addr);
	if (ret < 0) {
		return ret;
	}

	if (IS_ENABLED(CONFIG_OTP_NPM10XX_DRY_RUN)) {
		LOG_INF("Dry run: would program UICR bit %u", bit_addr);
		return 0;
	}

	/* 8. Activate UICR bit programming by setting OTP.TASKS.PROG */
	ret = i2c_reg_write_byte_dt(i2c, NPM10_OTP_TASKS, OTP_TASKS_PROG_Msk);
	if (ret < 0) {
		return ret;
	}

	/* 9. Poll events and errors. OTP.EVENTS_UICR_SET.USERPROGSUCC indicates success */
	ret = npm10xx_otp_poll_any(i2c, NPM10_OTP_EVENTS_SET,
				   OTP_EVENTS_USERPROGSUCC_Msk | OTP_EVENTS_USERLOCKERR_Msk |
					   OTP_EVENTS_USERADDRERR_Msk,
				   &events);
	if (ret < 0) {
		LOG_ERR("UICR bit %u: no programming result (%d)", bit_addr, ret);
		return ret;
	}

	if (events & OTP_EVENTS_USERLOCKERR_Msk) {
		LOG_ERR("UICR bit %u: lock error", bit_addr);
		return -EACCES;
	}

	if (events & OTP_EVENTS_USERADDRERR_Msk) {
		LOG_ERR("UICR bit %u: address error", bit_addr);
		return -EINVAL;
	}

	if (!(events & OTP_EVENTS_USERPROGSUCC_Msk)) {
		LOG_ERR("UICR bit %u: programming failed (events 0x%02x)", bit_addr, events);
		return -EIO;
	}

	return 0;
}

static int otp_npm10xx_program(const struct device *dev, off_t offset, const void *data, size_t len)
{
	const struct otp_npm10xx_config *config = dev->config;
	struct otp_npm10xx_data *dev_data = dev->data;
	const uint8_t *src = data;
	size_t end;
	int ret;
	uint8_t reg;

	if ((offset < 0) || size_add_overflow((size_t)offset, len, &end) ||
	    (end > OTP_NPM10XX_UICR_SIZE)) {
		LOG_ERR("UICR write out of bounds [0,%u)", OTP_NPM10XX_UICR_SIZE);
		return -EINVAL;
	}

	if (len == 0U) {
		return 0;
	}

	(void)k_mutex_lock(&dev_data->lock, K_FOREVER);

	ret = i2c_reg_read_byte_dt(&config->i2c, NPM10_OTP_READLOCK, &reg);
	if (ret < 0) {
		goto unlock;
	}

	if (reg & OTP_READLOCK_STATE) {
		LOG_ERR("UICR locked, programming not possible");
		ret = -EACCES;
		goto unlock;
	}

	/* UICR programming sequence, ref. datasheet chapter 4.2.12 */
	/* (Steps 1-3 are outside of driver's control) */
	/* 4. Power up the OTP memory by setting OTP.REQUEST.USERPROGMODE */
	ret = i2c_reg_write_byte_dt(&config->i2c, NPM10_OTP_REQUEST, OTP_REQUEST_USERPROGMODE_Msk);
	if (ret < 0) {
		goto unlock;
	}

	/* 5. Poll OTP.PROGMODE.STATUS before proceeding */
	ret = npm10xx_otp_poll_set(&config->i2c, NPM10_OTP_PROGMODE, OTP_PROGMODE_STATUS_Msk);
	if (ret < 0) {
		LOG_ERR("UICR program mode not entered - is VBUS supplied? (%d)", ret);
		goto exit_prog_mode;
	}

	for (size_t byte = 0U; byte < len; byte++) {
		uint8_t val = src[byte];

		for (uint8_t bit = 0U; bit < 8U; bit++) {
			uint8_t bit_addr;

			if ((val & BIT(bit)) == 0U) {
				continue;
			}

			bit_addr = (uint8_t)((offset + byte) * 8U + bit);
			if (bit_addr == OTP_NPM10XX_UICR_LOCK_BIT) {
				/* Lock bit should be burned last */
				continue;
			}

			/* Steps 6-9 */
			ret = npm10xx_uicr_bit_program(&config->i2c, bit_addr);
			if (ret < 0) {
				goto exit_prog_mode;
			}

			/* 10.Repeat steps 6-9 until all desired bits have been programmed */
		}
	}

	if ((offset <= LOCK_BIT_BYTE_IDX) && (end > LOCK_BIT_BYTE_IDX) &&
	    (src[LOCK_BIT_BYTE_IDX - offset] & BIT(LOCK_BIT_BYTE_POS))) {
		/* 11. Prevent further UICR programming by programming UICR lock bit OTP.LOCK.STATE
		 *	- Repeat steps 6 to 9 to program UICR lock bit
		 */
		LOG_WRN("Locking UICR from further writing");
		ret = npm10xx_uicr_bit_program(&config->i2c, OTP_NPM10XX_UICR_LOCK_BIT);
	}

exit_prog_mode: {
	int exit_ret;

	/* 12.Exit programming mode: OTP.REQUEST.USERPROGMODE=0 */
	exit_ret = i2c_reg_write_byte_dt(&config->i2c, NPM10_OTP_REQUEST, 0U);
	if (ret == 0) {
		ret = exit_ret;
	}

	/* 13.Poll and check that OTP.PROGMODE.STATUS=0 before proceeding */
	exit_ret =
		npm10xx_otp_poll_clear(&config->i2c, NPM10_OTP_PROGMODE, OTP_PROGMODE_STATUS_Msk);
	if (ret == 0) {
		ret = exit_ret;
	}
}
	/* (Steps 14-15 are outside of driver's control) */

unlock:
	(void)k_mutex_unlock(&dev_data->lock);

	return ret;
}
#endif /* CONFIG_OTP_PROGRAM */

static int otp_npm10xx_init(const struct device *dev)
{
	const struct otp_npm10xx_config *config = dev->config;
	struct otp_npm10xx_data *dev_data = dev->data;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus is not ready");
		return -ENODEV;
	}

	(void)k_mutex_init(&dev_data->lock);

	return 0;
}

static DEVICE_API(otp, otp_npm10xx_api) = {
#if defined(CONFIG_OTP_PROGRAM)
	.program = otp_npm10xx_program,
#endif
	.read = otp_npm10xx_read,
};

#define OTP_NPM10XX_DEFINE(n)                                                                      \
	static const struct otp_npm10xx_config otp_npm10xx_config_##n = {                          \
		.i2c = I2C_DT_SPEC_GET(DT_INST_PARENT(n)),                                         \
		.margin_mode = DT_INST_ENUM_IDX(n, margin_mode),                                   \
	};                                                                                         \
                                                                                                   \
	static struct otp_npm10xx_data otp_npm10xx_data_##n;                                       \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, otp_npm10xx_init, NULL, &otp_npm10xx_data_##n,                    \
			      &otp_npm10xx_config_##n, POST_KERNEL,                                \
			      CONFIG_OTP_NPM10XX_INIT_PRIORITY, &otp_npm10xx_api);

DT_INST_FOREACH_STATUS_OKAY(OTP_NPM10XX_DEFINE)
