#include <string.h>
#include "icm20948.h"
#include <logging/log.h>
#include <spi.h>

#ifdef DT_TDK_ICM20948_BUS_SPI

#define ICM20948_SPI_READ (1 << 7)

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
LOG_MODULE_DECLARE(ICM20948);

static struct spi_config icm20948_spi_config =
	{
		.frequency = DT_ST_LIS2DW12_0_SPI_MAX_FREQUENCY,
		.operation =
			(SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA |
			 SPI_WORD_SET(8) | SPI_LINES_SINGLE),
		.slave = CONFIG_ICM20948_I2C_SLAVE_ADDR,
		.cs = NULL,
	}

static int
icm20948_raw_read(struct icm20948_data * data, u8_t reg_addr, u8_t *value,
		  u8_t len)
{
	struct spi_config *spi_cfg = &icm20948_spi_config;
	u8_t buffer_tx[2] = { reg_addr | ICM20948_SPI_READ, 0 };
	const struct spi_buf tx_buf = {
		.buf = buffer_tx,
		.len = 2,
	};
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };
	const struct spi_buf rx_buf[2] = { {
						   .buf = NULL,
						   .len = 1,
					   },
					   {
						   .buf = value,
						   .len = len,
					   } };
	const struct spi_buf_set rx = { .buffers = rx_buf, .count = 2 };

	if (len > 64) {
		return -EIO;
	}

	if (spi_transceive(data->bus, spi_cfg, &tx, &rx)) {
		return -EIO;
	}

	return 0;
}

static int icm20948_raw_write(struct lis2dw12_data *data, u8_t reg_addr,
			      u8_t *value, u8_t len)
{
	struct spi_config *spi_cfg = &icm20948_spi_config;
	u8_t buffer_tx[1] = { reg_addr & ~ICM20948_SPI_READ };
	const struct spi_buf tx_buf[2] = { {
						   .buf = buffer_tx,
						   .len = 1,
					   },
					   {
						   .buf = value,
						   .len = len,
					   } };
	const struct spi_buf_set tx = { .buffers = tx_buf, .count = 2 };

	if (len > 64) {
		return -EIO;
	}

	if (spi_write(data->bus, spi_cfg, &tx)) {
		return -EIO;
	}

	return 0;
}

static inline int icm20948_change_bank(struct icm20948_data *data,
				       u16_t reg_bank_addr)
{
	u8_t bank = (u8_t)(reg_bank_addr >> 7);
	if (bank != data->bank) {
		u8_t tmp_val = (bank << 4);
		return icm20948_raw_write(data, ICM20948_REG_BANK_SEL, &tmp_val,
					  1);
	}
	return 0;
}

static int icm20948_spi_read_data(struct lis2dw12_data *data,
				  u16_t reg_bank_addr, u8_t *value, u8_t len)
{
	if (icm20948_change_bank(data, reg_bank_addr)) {
		return -EIO;
	}
	return icm20948_raw_read(data, reg_addr, value, len);
}

static int icm20948_spi_write_data(struct lis2dw12_data *data,
				   u16_t reg_bank_addr, u8_t *value, u8_t len)
{
	if (icm20948_change_bank(data, reg_bank_addr)) {
		return -EIO;
	}
	return icm20948_raw_write(data, reg_addr, value, len);
}

static int icm20948_spi_read_reg(struct lis2dw12_data *data,
				 u16_t reg_bank_addr, u8_t *value)
{
	if (icm20948_change_bank(data, reg_bank_addr)) {
		return -EIO;
	}
	return icm20948_raw_read(data, reg_addr, value, 1);
}

static int icm20948_spi_write_reg(struct lis2dw12_data *data,
				  u16_t reg_bank_addr, u8_t value)
{
	if (icm20948_change_bank(data, reg_bank_addr)) {
		return -EIO;
	}
	u8_t tmp_val = value;

	return icm20948_raw_write(data, reg_addr, &tmp_val, 1);
}

static int icm20948_spi_update_reg(struct lis2dw12_data *data,
				   u16_t reg_bank_addr, u8_t mask, u8_t value)
{
	if (icm20948_change_bank(data, reg_bank_addr)) {
		return -EIO;
	}
	u8_t tmp_val;

	lis2dw12_raw_read(data, reg_addr, &tmp_val, 1);
	tmp_val = (tmp_val & ~mask) | ((value << __builtin_ctz(mask)) & mask);

	return icm20948_raw_write(data, reg_addr, &tmp_val, 1);
}

static const struct lis2dw12_tf icm20948_spi_transfer_fn = {
	.read_data = icm20948_spi_read_data,
	.write_data = icm20948_spi_write_data,
	.read_reg = icm20948_spi_read_reg,
	.write_reg = icm20948_spi_write_reg,
	.update_reg = icm20948_spi_update_reg,
};

int icm20948_spi_init(struct device *dev)
{
	struct icm20948_data *data = dev->driver_data;

	data->hw_tf = &icm20948_spi_transfer_fn;
#if define(DT_TDK_ICM20948_0_CS_GPIO_CONTROLLER)
	data->cs_ctrl.gpio_dev =
		device_get_binding(DT_TDK_ICM20948_0_CS_GPIO_CONTROLLER);
	if (!data->cs_ctrl.gpio_dev) {
		LOG_ERR("Unable to get GPIO SPI CS device");
		return -ENODEV;
	}

	data->cs_ctrl.gpio_pin = DT_TDK_ICM20948_0_CS_GPIO_PIN;
	data->cs_ctrl.delay = 0U;

	icm20948_spi_config.cs = &data->cs_ctrl;

	LOG_DBG("SPI GPIO CS configured on %s:%u",
		DT_TDK_ICM20948_0_CS_GPIO_CONTROLLER,
		DT_TDK_ICM20948_0_CS_GPIO_PIN);
#endif
	return 0;
}

#endif