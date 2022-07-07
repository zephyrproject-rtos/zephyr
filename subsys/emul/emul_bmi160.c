/*
 * Copyright 2020 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Emulator for the Bosch BMI160 accelerometer / gyro. This supports basic
 * init and reading of canned samples. It supports both I2C and SPI buses.
 */

#define DT_DRV_COMPAT bosch_bmi160

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bosch_bmi160);

#include <zephyr/sys/byteorder.h>
#include <bmi160.h>
#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi_emul.h>

/** Run-time data used by the emulator */
struct bmi160_emul_data {
	union {
		/** SPI emulator detail */
		struct spi_emul emul_spi;
		/** I2C emulator detail */
		struct i2c_emul emul_i2c;
	};
	/** BMI160 device being emulated */
	const struct device *dev;
	uint8_t pmu_status;
	/** Current register to read (address) */
	uint32_t cur_reg;
};

/** Static configuration for the emulator */
struct bmi160_emul_cfg {
	/** Label of the SPI bus this emulator connects to */
	const char *bus_label;
	/** Chip registers */
	uint8_t *reg;
	union {
		/** Unit address (chip select ordinal) of emulator */
		uint16_t chipsel;
		/** I2C address of emulator */
		uint16_t addr;
	};
};

/* Names for the PMU components */
static const char *const pmu_name[] = {"acc", "gyr", "mag", "INV"};

static void sample_read(struct bmi160_emul_data *data, union bmi160_sample *buf)
{
	/*
	 * Use hard-coded scales to get values just above 0, 1, 2 and
	 * 3, 4, 5. Values are stored in little endianness.
	 * gyr[x] = 0x0b01  // 3 * 1000000 / BMI160_GYR_SCALE(2000) + 1
	 * gyr[y] = 0x0eac  // 4 * 1000000 / BMI160_GYR_SCALE(2000) + 1
	 * gyr[z] = 0x1257  // 5 * 1000000 / BMI160_GYR_SCALE(2000) + 1
	 * acc[x] = 0x0001  // 0 * 1000000 / BMI160_ACC_SCALE(2) + 1
	 * acc[y] = 0x0689  // 1 * 1000000 / BMI160_ACC_SCALE(2) + 1
	 * acc[z] = 0x0d11  // 2 * 1000000 / BMI160_ACC_SCALE(2) + 1
	 */
	static uint8_t raw_data[] = { 0x01, 0x0b, 0xac, 0x0e, 0x57, 0x12, 0x01, 0x00,
							0x89, 0x06, 0x11, 0x0d };

	LOG_INF("Sample read");
	memcpy(buf->raw, raw_data, ARRAY_SIZE(raw_data));
}

static void reg_write(const struct emul *emulator, int regn, int val)
{
	struct bmi160_emul_data *data = emulator->data;
	const struct bmi160_emul_cfg *cfg = emulator->cfg;

	LOG_INF("write %x = %x", regn, val);
	cfg->reg[regn] = val;
	switch (regn) {
	case BMI160_REG_ACC_CONF:
		LOG_INF("   * acc conf");
		break;
	case BMI160_REG_ACC_RANGE:
		LOG_INF("   * acc range");
		break;
	case BMI160_REG_GYR_CONF:
		LOG_INF("   * gyr conf");
		break;
	case BMI160_REG_GYR_RANGE:
		LOG_INF("   * gyr range");
		break;
	case BMI160_REG_CMD:
		switch (val) {
		case BMI160_CMD_SOFT_RESET:
			LOG_INF("   * soft reset");
			break;
		default:
			if ((val & BMI160_CMD_PMU_BIT) == BMI160_CMD_PMU_BIT) {
				int which = (val & BMI160_CMD_PMU_MASK) >>
					 BMI160_CMD_PMU_SHIFT;
				int shift;
				int pmu_val = val & BMI160_CMD_PMU_VAL_MASK;

				switch (which) {
				case 0:
					shift = BMI160_PMU_STATUS_ACC_POS;
					break;
				case 1:
					shift = BMI160_PMU_STATUS_GYR_POS;
					break;
				case 2:
				default:
					shift = BMI160_PMU_STATUS_MAG_POS;
					break;
				}
				data->pmu_status &= 3 << shift;
				data->pmu_status |= pmu_val << shift;
				LOG_INF("   * pmu %s = %x, new status %x",
					pmu_name[which], pmu_val,
					data->pmu_status);
			} else {
				LOG_INF("Unknown command %x", val);
			}
			break;
		}
		break;
	default:
		LOG_INF("Unknown write %x", regn);
	}
}

static int reg_read(const struct emul *emulator, int regn)
{
	struct bmi160_emul_data *data = emulator->data;
	const struct bmi160_emul_cfg *cfg = emulator->cfg;
	int val;

	LOG_INF("read %x =", regn);
	val = cfg->reg[regn];
	switch (regn) {
	case BMI160_REG_CHIPID:
		LOG_INF("   * get chipid");
		break;
	case BMI160_REG_PMU_STATUS:
		LOG_INF("   * get pmu");
		val = data->pmu_status;
		break;
	case BMI160_REG_STATUS:
		LOG_INF("   * status");
		val |= BMI160_DATA_READY_BIT_MASK;
		break;
	case BMI160_REG_ACC_CONF:
		LOG_INF("   * acc conf");
		break;
	case BMI160_REG_GYR_CONF:
		LOG_INF("   * gyr conf");
		break;
	case BMI160_SPI_START:
		LOG_INF("   * Bus start");
		break;
	case BMI160_REG_ACC_RANGE:
		LOG_INF("   * acc range");
		break;
	case BMI160_REG_GYR_RANGE:
		LOG_INF("   * gyr range");
		break;
	default:
		LOG_INF("Unknown read %x", regn);
	}
	LOG_INF("       = %x", val);

	return val;
}

#if BMI160_BUS_SPI
static int bmi160_emul_io_spi(struct spi_emul *emul,
			      const struct spi_config *config,
			      const struct spi_buf_set *tx_bufs,
			      const struct spi_buf_set *rx_bufs)
{
	struct bmi160_emul_data *data;
	const struct spi_buf *tx, *txd, *rxd;
	unsigned int regn, val;
	int count;

	data = CONTAINER_OF(emul, struct bmi160_emul_data, emul_spi);

	__ASSERT_NO_MSG(tx_bufs || rx_bufs);
	__ASSERT_NO_MSG(!tx_bufs || !rx_bufs ||
			tx_bufs->count == rx_bufs->count);
	count = tx_bufs ? tx_bufs->count : rx_bufs->count;

	switch (count) {
	case 2:
		tx = tx_bufs->buffers;
		txd = &tx_bufs->buffers[1];
		rxd = rx_bufs ? &rx_bufs->buffers[1] : NULL;
		switch (tx->len) {
		case 1:
			regn = *(uint8_t *)tx->buf;
			if ((regn & BMI160_REG_READ) && rxd == NULL) {
				LOG_ERR("Cannot read without rxd");
				return -EPERM;
			}
			switch (txd->len) {
			case 1:
				if (regn & BMI160_REG_READ) {
					regn &= BMI160_REG_MASK;
					val = reg_read(emul->parent, regn);
					*(uint8_t *)rxd->buf = val;
				} else {
					val = *(uint8_t *)txd->buf;
					reg_write(emul->parent, regn, val);
				}
				break;
			case BMI160_SAMPLE_SIZE:
				if (regn & BMI160_REG_READ) {
					sample_read(data, rxd->buf);
				} else {
					LOG_INF("Unknown sample write");
				}
				break;
			default:
				LOG_INF("Unknown A txd->len %d", txd->len);
				break;
			}
			break;
		default:
			LOG_INF("Unknown tx->len %d", tx->len);
			break;
		}
		break;
	default:
		LOG_INF("Unknown tx_bufs->count %d", count);
		break;
	}

	return 0;
}
#endif

#if BMI160_BUS_I2C
static int bmi160_emul_transfer_i2c(struct i2c_emul *emul, struct i2c_msg *msgs,
				    int num_msgs, int addr)
{
	struct bmi160_emul_data *data;
	unsigned int val;

	data = CONTAINER_OF(emul, struct bmi160_emul_data, emul_i2c);

	__ASSERT_NO_MSG(msgs && num_msgs);

	i2c_dump_msgs("emul", msgs, num_msgs, addr);
	switch (num_msgs) {
	case 2:
		if (msgs->flags & I2C_MSG_READ) {
			LOG_ERR("Unexpected read");
			return -EIO;
		}
		if (msgs->len != 1) {
			LOG_ERR("Unexpected msg0 length %d", msgs->len);
			return -EIO;
		}
		data->cur_reg = msgs->buf[0];

		/* Now process the 'read' part of the message */
		msgs++;
		if (msgs->flags & I2C_MSG_READ) {
			switch (msgs->len) {
			case 1:
				val = reg_read(emul->parent, data->cur_reg);
				msgs->buf[0] = val;
				break;
			case BMI160_SAMPLE_SIZE:
				sample_read(data, (void *)msgs->buf);
				break;
			default:
				LOG_ERR("Unexpected msg1 length %d", msgs->len);
				return -EIO;
			}
		} else {
			if (msgs->len != 1) {
				LOG_ERR("Unexpected msg1 length %d", msgs->len);
			}
			reg_write(emul->parent, data->cur_reg, msgs->buf[0]);
		}
		break;
	default:
		LOG_ERR("Invalid number of messages: %d", num_msgs);
		return -EIO;
	}

	return 0;
}
#endif

/* Device instantiation */

#if BMI160_BUS_SPI
static struct spi_emul_api bmi160_emul_api_spi = {
	.io = bmi160_emul_io_spi,
};
#endif

#if BMI160_BUS_I2C
static struct i2c_emul_api bmi160_emul_api_i2c = {
	.transfer = bmi160_emul_transfer_i2c,
};
#endif

static void emul_bosch_bmi160_init(const struct emul *emul,
				   const struct device *parent)
{
	const struct bmi160_emul_cfg *cfg = emul->cfg;
	struct bmi160_emul_data *data = emul->data;
	uint8_t *reg = cfg->reg;

	data->dev = parent;
	data->pmu_status = 0;

	reg[BMI160_REG_CHIPID] = BMI160_CHIP_ID;
}

#if BMI160_BUS_SPI
/**
 * Set up a new BMI160 emulator (SPI)
 *
 * This should be called for each BMI160 device that needs to be emulated. It
 * registers it with the SPI emulation controller.
 *
 * @param emul Emulation information
 * @param parent Device to emulate (must use BMI160 driver)
 * @return 0 indicating success (always)
 */
static int emul_bosch_bmi160_init_spi(const struct emul *emul,
				      const struct device *parent)
{
	const struct bmi160_emul_cfg *cfg = emul->cfg;
	struct bmi160_emul_data *data = emul->data;

	emul_bosch_bmi160_init(emul, parent);
	data->emul_spi.api = &bmi160_emul_api_spi;
	data->emul_spi.chipsel = cfg->chipsel;
	data->emul_spi.parent = emul;

	int rc = spi_emul_register(parent, emul->dev_label, &data->emul_spi);

	return rc;
}
#endif

#if BMI160_BUS_I2C
/**
 * Set up a new BMI160 emulator (I2C)
 *
 * This should be called for each BMI160 device that needs to be emulated. It
 * registers it with the SPI emulation controller.
 *
 * @param emul Emulation information
 * @param parent Device to emulate (must use BMI160 driver)
 * @return 0 indicating success (always)
 */
static int emul_bosch_bmi160_init_i2c(const struct emul *emul,
				      const struct device *parent)
{
	const struct bmi160_emul_cfg *cfg = emul->cfg;
	struct bmi160_emul_data *data = emul->data;

	emul_bosch_bmi160_init(emul, parent);
	data->emul_i2c.api = &bmi160_emul_api_i2c;
	data->emul_i2c.addr = cfg->addr;
	data->emul_i2c.parent = emul;

	int rc = i2c_emul_register(parent, emul->dev_label, &data->emul_i2c);

	return rc;
}
#endif

#define BMI160_EMUL_DATA(n) \
	static uint8_t bmi160_emul_reg_##n[BMI160_REG_COUNT]; \
	static struct bmi160_emul_data bmi160_emul_data_##n;

#define BMI160_EMUL_DEFINE(n, type) \
	EMUL_DEFINE(emul_bosch_bmi160_init_##type, DT_DRV_INST(n), \
		&bmi160_emul_cfg_##n, &bmi160_emul_data_##n)

/* Instantiation macros used when a device is on a SPI bus */
#define BMI160_EMUL_SPI(n) \
	BMI160_EMUL_DATA(n) \
	static const struct bmi160_emul_cfg bmi160_emul_cfg_##n = { \
		.bus_label = DT_INST_BUS_LABEL(n), \
		.reg = bmi160_emul_reg_##n, \
		.chipsel = DT_INST_REG_ADDR(n) \
	}; \
	BMI160_EMUL_DEFINE(n, spi)

#define BMI160_EMUL_I2C(n) \
	BMI160_EMUL_DATA(n) \
	static const struct bmi160_emul_cfg bmi160_emul_cfg_##n = { \
		.bus_label = DT_INST_BUS_LABEL(n), \
		.reg = bmi160_emul_reg_##n, \
		.addr = DT_INST_REG_ADDR(n) \
	}; \
	BMI160_EMUL_DEFINE(n, i2c)

/*
 * Main instantiation macro. Use of COND_CODE_1() selects the right
 * bus-specific macro at preprocessor time.
 */
#define BMI160_EMUL(n) \
	COND_CODE_1(DT_INST_ON_BUS(n, spi), \
		    (BMI160_EMUL_SPI(n)), \
		    (BMI160_EMUL_I2C(n)))

DT_INST_FOREACH_STATUS_OKAY(BMI160_EMUL)
