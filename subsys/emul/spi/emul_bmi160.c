/*
 * Copyright 2020 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bosch_bmi160

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(bosch_bmi160);

#include <sys/byteorder.h>
#include <bmi160.h>
#include <device.h>
#include <emul.h>
#include <drivers/spi.h>
#include <drivers/spi_emul.h>

/** Run-time data used by the emulator */
struct bmi160_emul_data {
	/** SPI emulator detail */
	struct spi_emul emul;
	/** BMI160 device being emulated */
	const struct device *dev;
	/** Configuration information */
	const struct bmi160_emul_cfg *cfg;
	uint8_t pmu_status;
};

/** Static configuration for the emulator */
struct bmi160_emul_cfg {
	/** Label of the SPI bus this emulator connects to */
	const char *spi_label;
	/** Pointer to run-time data */
	struct bmi160_emul_data *data;
	/** Chip registers */
	uint8_t *reg;
	/** Unit address (chip select ordinal) of emulator */
	uint16_t chipsel;
};

/* Names for the PMU components */
static const char *const pmu_name[] = {"acc", "gyr", "mag", "INV"};

static void sample_read(struct bmi160_emul_data *data, union bmi160_sample *buf)
{
	int i;

	buf->dummy_byte = 0;
	for (i = 0; i < BMI160_AXES; i++) {
		/*
		 * Use hard-coded scales to get values just above 0, 1, 2 and
		 * 3, 4, 5.
		  */
		buf->acc[i] = sys_cpu_to_le16(i * 1000000 / 598 + 1);
		buf->gyr[i] = sys_cpu_to_le16((i + 3) * 1000000 / 1065 + 1);
	}
}

static void reg_write(const struct bmi160_emul_cfg *cfg, int regn, int val)
{
	struct bmi160_emul_data *data = cfg->data;

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

static int reg_read(const struct bmi160_emul_cfg *cfg, int regn)
{
	struct bmi160_emul_data *data = cfg->data;
	int val;

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
		LOG_INF("   * SPI start");
		break;
	default:
		LOG_INF("Unknown read %x", regn);
	}

	return val;
}

static int bmi160_emul_io(struct spi_emul *emul,
			  const struct spi_config *config,
			  const struct spi_buf_set *tx_bufs,
			  const struct spi_buf_set *rx_bufs)
{
	struct bmi160_emul_data *data;
	const struct bmi160_emul_cfg *cfg;
	const struct spi_buf *tx, *txd, *rxd;
	unsigned int regn, val;
	int count;

	data = CONTAINER_OF(emul, struct bmi160_emul_data, emul);
	cfg = data->cfg;

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
			switch (txd->len) {
			case 1:
				if (regn & BMI160_REG_READ) {
					regn &= BMI160_REG_MASK;
					LOG_INF("read %x =", regn);
					val = reg_read(cfg, regn);
					*(uint8_t *)rxd->buf = val;
					LOG_INF("       = %x", val);
				} else {
					val = *(uint8_t *)txd->buf;
					LOG_INF("write %x = %x", regn, val);
					reg_write(cfg, regn, val);
				}
				break;
			case BMI160_SAMPLE_SIZE:
				if (regn & BMI160_REG_READ) {
					LOG_INF("Sample read");
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

/* Device instantiation */

static struct spi_emul_api bmi160_emul_api = {
	.io = bmi160_emul_io,
};

/**
 * Set up a new BMI160 emulator
 *
 * This should be called for each BMI160 device that needs to be emulated. It
 * registers it with the SPI emulation controller.
 *
 * @param emul Emulation information
 * @param parent Device to emulate (must use BMI160 driver)
 * @return 0 indicating success (always)
 */
static int emul_bosch_bmi160_init(const struct emul *emul,
				  const struct device *parent)
{
	const struct bmi160_emul_cfg *cfg = emul->cfg;
	struct bmi160_emul_data *data = cfg->data;
	uint8_t *reg = cfg->reg;

	data->emul.api = &bmi160_emul_api;
	data->emul.chipsel = cfg->chipsel;
	data->dev = parent;
	data->cfg = cfg;
	data->pmu_status = 0;

	reg[BMI160_REG_CHIPID] = BMI160_CHIP_ID;

	int rc = spi_emul_register(parent, emul->dev_label, &data->emul);

	return rc;
}

#define BMI160_EMUL(n) \
	static uint8_t bmi160_emul_reg_##n[BMI160_REG_COUNT]; \
	static struct bmi160_emul_data bmi160_emul_data_##n; \
	static const struct bmi160_emul_cfg bmi160_emul_cfg_##n = { \
		.spi_label = DT_INST_BUS_LABEL(n), \
		.data = &bmi160_emul_data_##n, \
		.reg = bmi160_emul_reg_##n, \
		.chipsel = DT_INST_REG_ADDR(n) \
	}; \
	EMUL_DEFINE(emul_bosch_bmi160_init, DT_DRV_INST(n), \
		&bmi160_emul_cfg_##n)

DT_INST_FOREACH_STATUS_OKAY(BMI160_EMUL)
