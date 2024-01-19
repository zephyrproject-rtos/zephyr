/*
 * Copyright (c) 2023 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/can_mcan.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(can_tcan4x5x, CONFIG_CAN_LOG_LEVEL);

#define DT_DRV_COMPAT ti_tcan4x5x

/*
 * The register definitions correspond to those found in the TI TCAN4550-Q1 datasheet, revision D
 * June 2022 (SLLSEZ5D).
 */

/* Device ID1 register */
#define CAN_TCAN4X5X_DEVICE_ID1 0x0000

/* Device ID2 register */
#define CAN_TCAN4X5X_DEVICE_ID2 0x0004

/* Revision register */
#define CAN_TCAN4X5X_REVISION                0x0008
#define CAN_TCAN4X5X_REVISION_SPI_2_REVISION GENMASK(31, 24)
#define CAN_TCAN4X5X_REVISION_REV_ID_MAJOR   GENMASK(15, 8)
#define CAN_TCAN4X5X_REVISION_REV_ID_MINOR   GENMASK(7, 0)

/* Status register */
#define CAN_TCAN4X5X_STATUS                          0x000c
#define CAN_TCAN4X5X_STATUS_INTERNAL_READ_ERROR      BIT(29)
#define CAN_TCAN4X5X_STATUS_INTERNAL_WRITE_ERROR     BIT(28)
#define CAN_TCAN4X5X_STATUS_INTERNAL_ERROR_LOG_WRITE BIT(27)
#define CAN_TCAN4X5X_STATUS_READ_FIFO_UNDERFLOW      BIT(26)
#define CAN_TCAN4X5X_STATUS_READ_FIFO_EMPTY          BIT(25)
#define CAN_TCAN4X5X_STATUS_WRITE_FIFO_OVERFLOW      BIT(24)
#define CAN_TCAN4X5X_STATUS_SPI_END_ERROR            BIT(21)
#define CAN_TCAN4X5X_STATUS_INVALID_COMMAND          BIT(20)
#define CAN_TCAN4X5X_STATUS_WRITE_OVERFLOW           BIT(19)
#define CAN_TCAN4X5X_STATUS_WRITE_UNDERFLOW          BIT(18)
#define CAN_TCAN4X5X_STATUS_READ_OVERFLOW            BIT(17)
#define CAN_TCAN4X5X_STATUS_READ_UNDERFLOW           BIT(16)
#define CAN_TCAN4X5X_STATUS_WRITE_FIFO_AVAILABLE     BIT(5)
#define CAN_TCAN4X5X_STATUS_READ_FIFO_AVAILABLE      BIT(4)
#define CAN_TCAN4X5X_STATUS_INTERNAL_ACCESS_ACTIVE   BIT(3)
#define CAN_TCAN4X5X_STATUS_INTERNAL_ERROR_INTERRUPT BIT(2)
#define CAN_TCAN4X5X_STATUS_SPI_ERROR_INTERRUPT      BIT(1)
#define CAN_TCAN4X5X_STATUS_INTERRUPT                BIT(0)

/* Mask of clearable status register bits */
#define CAN_TCAN4X5X_STATUS_CLEAR_ALL                                                              \
	(CAN_TCAN4X5X_STATUS_INTERNAL_READ_ERROR | CAN_TCAN4X5X_STATUS_INTERNAL_WRITE_ERROR |      \
	 CAN_TCAN4X5X_STATUS_INTERNAL_ERROR_LOG_WRITE | CAN_TCAN4X5X_STATUS_READ_FIFO_UNDERFLOW |  \
	 CAN_TCAN4X5X_STATUS_READ_FIFO_EMPTY | CAN_TCAN4X5X_STATUS_WRITE_FIFO_OVERFLOW |           \
	 CAN_TCAN4X5X_STATUS_SPI_END_ERROR | CAN_TCAN4X5X_STATUS_INVALID_COMMAND |                 \
	 CAN_TCAN4X5X_STATUS_WRITE_OVERFLOW | CAN_TCAN4X5X_STATUS_WRITE_UNDERFLOW |                \
	 CAN_TCAN4X5X_STATUS_READ_OVERFLOW | CAN_TCAN4X5X_STATUS_READ_UNDERFLOW)

/* SPI Error Status Mask register */
#define CAN_TCAN4X5X_SPI_ERROR_STATUS_MASK                          0x0010
#define CAN_TCAN4X5X_SPI_ERROR_STATUS_MASK_INTERNAL_READ_ERROR      BIT(29)
#define CAN_TCAN4X5X_SPI_ERROR_STATUS_MASK_INTERNAL_WRITE_ERROR     BIT(28)
#define CAN_TCAN4X5X_SPI_ERROR_STATUS_MASK_INTERNAL_ERROR_LOG_WRITE BIT(27)
#define CAN_TCAN4X5X_SPI_ERROR_STATUS_MASK_READ_FIFO_UNDERFLOW      BIT(26)
#define CAN_TCAN4X5X_SPI_ERROR_STATUS_MASK_READ_FIFO_EMPTY          BIT(25)
#define CAN_TCAN4X5X_SPI_ERROR_STATUS_MASK_WRITE_FIFO_OVERFLOW      BIT(24)
#define CAN_TCAN4X5X_SPI_ERROR_STATUS_MASK_SPI_END_ERROR            BIT(21)
#define CAN_TCAN4X5X_SPI_ERROR_STATUS_MASK_INVALID_COMMAND          BIT(20)
#define CAN_TCAN4X5X_SPI_ERROR_STATUS_MASK_WRITE_OVERFLOW           BIT(19)
#define CAN_TCAN4X5X_SPI_ERROR_STATUS_MASK_WRITE_UNDERFLOW          BIT(18)
#define CAN_TCAN4X5X_SPI_ERROR_STATUS_MASK_READ_OVERFLOW            BIT(17)
#define CAN_TCAN4X5X_SPI_ERROR_STATUS_MASK_READ_UNDERFLOW           BIT(16)

/* Modes of Operation and Pin Configurations register */
#define CAN_TCAN4X5X_MODE_CONFIG                  0x0800
#define CAN_TCAN4X5X_MODE_CONFIG_WAKE_CONFIG      GENMASK(31, 30)
#define CAN_TCAN4X5X_MODE_CONFIG_WD_TIMER         GENMASK(29, 28)
#define CAN_TCAN4X5X_MODE_CONFIG_CLK_REF          BIT(27)
#define CAN_TCAN4X5X_MODE_CONFIG_GPO2_CONFIG      GENMASK(23, 22)
#define CAN_TCAN4X5X_MODE_CONFIG_TEST_MODE_EN     BIT(21)
#define CAN_TCAN4X5X_MODE_CONFIG_NWKRQ_VOLTAGE    BIT(19)
#define CAN_TCAN4X5X_MODE_CONFIG_WD_BIT_SET       BIT(18)
#define CAN_TCAN4X5X_MODE_CONFIG_WD_ACTION        GENMASK(17, 16)
#define CAN_TCAN4X5X_MODE_CONFIG_GPIO1_CONFIG     GENMASK(15, 14)
#define CAN_TCAN4X5X_MODE_CONFIG_FAIL_SAFE_EN     BIT(13)
#define CAN_TCAN4X5X_MODE_CONFIG_GPIO1_GPO_CONFIG GENMASK(11, 10)
#define CAN_TCAN4X5X_MODE_CONFIG_INH_DIS          BIT(9)
#define CAN_TCAN4X5X_MODE_CONFIG_NWKRQ_CONFIG     BIT(8)
#define CAN_TCAN4X5X_MODE_CONFIG_MODE_SEL         GENMASK(7, 6)
#define CAN_TCAN4X5X_MODE_CONFIG_WD_EN            BIT(3)
#define CAN_TCAN4X5X_MODE_CONFIG_DEVICE_RESET     BIT(2)
#define CAN_TCAN4X5X_MODE_CONFIG_SWE_DIS          BIT(1)
#define CAN_TCAN4X5X_MODE_CONFIG_TEST_MODE_CONFIG BIT(0)

/* Timestamp Prescaler register */
#define CAN_TCAN4X5X_TIMESTAMP_PRESCALER      0x0804
#define CAN_TCAN4X5X_TIMESTAMP_PRESCALER_MASK GENMASK(7, 0)

/* Test Register and Scratch Pad */
#define CAN_TCAN4X5X_TEST_SCRATCH_PAD             0x0808
#define CAN_TCAN4X5X_TEST_SCRATCH_PAD_READ_WRITE  GENMASK(31, 16)
#define CAN_TCAN4X5X_TEST_SCRATCH_PAD_SCRATCH_PAD GENMASK(15, 0)

/* Test register */
#define CAN_TCAN4X5X_TEST                       0x0808
#define CAN_TCAN4X5X_TEST_ECC_ERR_FORCE_BIT_SEL GENMASK(21, 16)
#define CAN_TCAN4X5X_TEST_ECC_ERR_FORCE         BIT(12)
#define CAN_TCAN4X5X_TEST_ECC_ERR_CHECK         BIT(11)

/* Interrupts register */
#define CAN_TCAN4X5X_IR           0x0820
#define CAN_TCAN4X5X_IR_CANBUSNOM BIT(31)
#define CAN_TCAN4X5X_IR_SMS       BIT(23)
#define CAN_TCAN4X5X_IR_UVSUP     BIT(22)
#define CAN_TCAN4X5X_IR_UVIO      BIT(21)
#define CAN_TCAN4X5X_IR_PWRON     BIT(20)
#define CAN_TCAN4X5X_IR_TSD       BIT(19)
#define CAN_TCAN4X5X_IR_WDTO      BIT(18)
#define CAN_TCAN4X5X_IR_ECCERR    BIT(16)
#define CAN_TCAN4X5X_IR_CANINT    BIT(15)
#define CAN_TCAN4X5X_IR_LWU       BIT(14)
#define CAN_TCAN4X5X_IR_WKERR     BIT(13)
#define CAN_TCAN4X5X_IR_CANSLNT   BIT(10)
#define CAN_TCAN4X5X_IR_CANDOM    BIT(8)
#define CAN_TCAN4X5X_IR_GLOBALERR BIT(7)
#define CAN_TCAN4X5X_IR_WKRQ      BIT(6)
#define CAN_TCAN4X5X_IR_CANERR    BIT(5)
#define CAN_TCAN4X5X_IR_SPIERR    BIT(3)
#define CAN_TCAN4X5X_IR_M_CAN_INT BIT(1)
#define CAN_TCAN4X5X_IR_VTWD      BIT(0)

/* Mask of clearable interrupts register bits */
#define CAN_TCAN4X5X_IR_CLEAR_ALL                                                                  \
	(CAN_TCAN4X5X_IR_SMS | CAN_TCAN4X5X_IR_UVSUP | CAN_TCAN4X5X_IR_UVIO |                      \
	 CAN_TCAN4X5X_IR_PWRON | CAN_TCAN4X5X_IR_TSD | CAN_TCAN4X5X_IR_WDTO |                      \
	 CAN_TCAN4X5X_IR_ECCERR | CAN_TCAN4X5X_IR_CANINT | CAN_TCAN4X5X_IR_LWU |                   \
	 CAN_TCAN4X5X_IR_WKERR | CAN_TCAN4X5X_IR_CANSLNT | CAN_TCAN4X5X_IR_CANDOM)

/* MCAN Interrupts register */
#define CAN_TCAN4X5X_MCAN_IR      0x0824
#define CAN_TCAN4X5X_MCAN_IR_ARA  BIT(29)
#define CAN_TCAN4X5X_MCAN_IR_PED  BIT(28)
#define CAN_TCAN4X5X_MCAN_IR_PEA  BIT(27)
#define CAN_TCAN4X5X_MCAN_IR_WDI  BIT(26)
#define CAN_TCAN4X5X_MCAN_IR_BO   BIT(25)
#define CAN_TCAN4X5X_MCAN_IR_EW   BIT(24)
#define CAN_TCAN4X5X_MCAN_IR_EP   BIT(23)
#define CAN_TCAN4X5X_MCAN_IR_ELO  BIT(22)
#define CAN_TCAN4X5X_MCAN_IR_BEU  BIT(21)
#define CAN_TCAN4X5X_MCAN_IR_BEC  BIT(20)
#define CAN_TCAN4X5X_MCAN_IR_DRX  BIT(19)
#define CAN_TCAN4X5X_MCAN_IR_TOO  BIT(18)
#define CAN_TCAN4X5X_MCAN_IR_MRAF BIT(17)
#define CAN_TCAN4X5X_MCAN_IR_TSW  BIT(16)
#define CAN_TCAN4X5X_MCAN_IR_TEFL BIT(15)
#define CAN_TCAN4X5X_MCAN_IR_TEFF BIT(14)
#define CAN_TCAN4X5X_MCAN_IR_TEFW BIT(13)
#define CAN_TCAN4X5X_MCAN_IR_TEFN BIT(12)
#define CAN_TCAN4X5X_MCAN_IR_TFE  BIT(11)
#define CAN_TCAN4X5X_MCAN_IR_TCF  BIT(10)
#define CAN_TCAN4X5X_MCAN_IR_TC   BIT(9)
#define CAN_TCAN4X5X_MCAN_IR_HPM  BIT(8)
#define CAN_TCAN4X5X_MCAN_IR_RF1L BIT(7)
#define CAN_TCAN4X5X_MCAN_IR_RF1F BIT(6)
#define CAN_TCAN4X5X_MCAN_IR_RF1W BIT(5)
#define CAN_TCAN4X5X_MCAN_IR_RF1N BIT(4)
#define CAN_TCAN4X5X_MCAN_IR_RF0L BIT(3)
#define CAN_TCAN4X5X_MCAN_IR_RF0F BIT(2)
#define CAN_TCAN4X5X_MCAN_IR_RF0W BIT(1)
#define CAN_TCAN4X5X_MCAN_IR_RF0N BIT(0)

/* Interrupt Enables register */
#define CAN_TCAN4X5X_IE         0x0830
#define CAN_TCAN4X5X_IE_UVSUP   BIT(22)
#define CAN_TCAN4X5X_IE_UVIO    BIT(21)
#define CAN_TCAN4X5X_IE_TSD     BIT(19)
#define CAN_TCAN4X5X_IE_ECCERR  BIT(16)
#define CAN_TCAN4X5X_IE_CANINT  BIT(15)
#define CAN_TCAN4X5X_IE_LWU     BIT(14)
#define CAN_TCAN4X5X_IE_CANSLNT BIT(10)
#define CAN_TCAN4X5X_IE_CANDOM  BIT(8)

/* Bosch M_CAN registers base address */
#define CAN_TCAN4X5X_MCAN_BASE 0x1000

/* Bosch M_CAN Message RAM base address and size */
#define CAN_TCAN4X5X_MRAM_BASE 0x8000
#define CAN_TCAN4X5X_MRAM_SIZE 2048

/* TCAN4x5x SPI OP codes */
#define CAN_TCAN4X5X_WRITE_B_FL 0x61
#define CAN_TCAN4X5X_READ_B_FL  0x41

/* TCAN4x5x timing requirements */
#define CAN_TCAN4X5X_T_MODE_STBY_NOM_US 70
#define CAN_TCAN4X5X_T_WAKE_US          50
#define CAN_TCAN4X5X_T_PULSE_WIDTH_US   30
#define CAN_TCAN4X5X_T_RESET_US         1000

/*
 * Only compile in support for the optional GPIOs if at least one enabled tcan4x5x device tree node
 * has them. Only the INT GPIO is required.
 */
#define TCAN4X5X_RST_GPIO_SUPPORT   DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
#define TCAN4X5X_NWKRQ_GPIO_SUPPORT DT_ANY_INST_HAS_PROP_STATUS_OKAY(device_state_gpios)
#define TCAN4X5X_WAKE_GPIO_SUPPORT  DT_ANY_INST_HAS_PROP_STATUS_OKAY(device_wake_gpios)

struct tcan4x5x_config {
	struct spi_dt_spec spi;
#if TCAN4X5X_RST_GPIO_SUPPORT
	struct gpio_dt_spec rst_gpio;
#endif /* TCAN4X5X_RST_GPIO_SUPPORT */
#if TCAN4X5X_NWKRQ_GPIO_SUPPORT
	struct gpio_dt_spec nwkrq_gpio;
#endif /* TCAN4X5X_NWKRQ_GPIO_SUPPORT */
#if TCAN4X5X_WAKE_GPIO_SUPPORT
	struct gpio_dt_spec wake_gpio;
#endif /* TCAN4X5X_WAKE_GPIO_SUPPORT */
	struct gpio_dt_spec int_gpio;
	uint32_t clk_freq;
};

struct tcan4x5x_data {
	struct gpio_callback int_gpio_cb;
	struct k_thread int_thread;
	struct k_sem int_sem;

	K_KERNEL_STACK_MEMBER(int_stack, CONFIG_CAN_TCAN4X5X_THREAD_STACK_SIZE);
};

static int tcan4x5x_read(const struct device *dev, uint16_t addr, void *dst, size_t len)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct tcan4x5x_config *tcan_config = mcan_config->custom;
	size_t len32 = len / sizeof(uint32_t);
	uint32_t *dst32 = (uint32_t *)dst;
	uint8_t cmd[4] = {CAN_TCAN4X5X_READ_B_FL, addr >> 8U & 0xFF, addr & 0xFF,
			  len32 == 256 ? 0U : len32};
	uint8_t global_status;
	const struct spi_buf tx_bufs[] = {
		{.buf = &cmd, .len = sizeof(cmd)},
	};
	const struct spi_buf rx_bufs[] = {
		{.buf = &global_status, .len = sizeof(global_status)},
		{.buf = NULL, .len = 3},
		{.buf = dst, .len = len},
	};
	const struct spi_buf_set tx = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs),
	};
	const struct spi_buf_set rx = {
		.buffers = rx_bufs,
		.count = ARRAY_SIZE(rx_bufs),
	};
	int err;
	int i;

	if (len == 0) {
		return 0;
	}

	/* Maximum transfer size is 256 32-bit words */
	__ASSERT_NO_MSG(len % 4 == 0);
	__ASSERT_NO_MSG(len32 <= 256);

	err = spi_transceive_dt(&tcan_config->spi, &tx, &rx);
	if (err != 0) {
		LOG_ERR("failed to read addr %u, len %d (err %d)", addr, len, err);
		return err;
	}

	__ASSERT_NO_MSG((global_status & CAN_TCAN4X5X_IR_SPIERR) == 0U);

	for (i = 0; i < len32; i++) {
		dst32[i] = sys_be32_to_cpu(dst32[i]);
	}

	return 0;
}

static int tcan4x5x_write(const struct device *dev, uint16_t addr, const void *src, size_t len)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct tcan4x5x_config *tcan_config = mcan_config->custom;
	size_t len32 = len / sizeof(uint32_t);
	uint32_t src32[len32];
	uint8_t cmd[4] = {CAN_TCAN4X5X_WRITE_B_FL, addr >> 8U & 0xFF, addr & 0xFF,
			  len32 == 256 ? 0U : len32};
	uint8_t global_status;
	const struct spi_buf tx_bufs[] = {
		{.buf = &cmd, .len = sizeof(cmd)},
		{.buf = &src32, .len = len},
	};
	const struct spi_buf rx_bufs[] = {
		{.buf = &global_status, .len = sizeof(global_status)},
	};
	const struct spi_buf_set tx = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs),
	};
	const struct spi_buf_set rx = {
		.buffers = rx_bufs,
		.count = ARRAY_SIZE(rx_bufs),
	};
	int err;
	int i;

	if (len == 0) {
		return 0;
	}

	/* Maximum transfer size is 256 32-bit words */
	__ASSERT_NO_MSG(len % 4 == 0);
	__ASSERT_NO_MSG(len32 <= 256);

	for (i = 0; i < len32; i++) {
		src32[i] = sys_cpu_to_be32(((uint32_t *)src)[i]);
	}

	err = spi_transceive_dt(&tcan_config->spi, &tx, &rx);
	if (err != 0) {
		LOG_ERR("failed to write addr %u, len %d (err %d)", addr, len, err);
		return err;
	}

	__ASSERT_NO_MSG((global_status & CAN_TCAN4X5X_IR_SPIERR) == 0U);

	return 0;
}

static inline int tcan4x5x_read_tcan_reg(const struct device *dev, uint16_t reg, uint32_t *val)
{
	return tcan4x5x_read(dev, reg, val, sizeof(uint32_t));
}

static inline int tcan4x5x_write_tcan_reg(const struct device *dev, uint16_t reg, uint32_t val)
{
	return tcan4x5x_write(dev, reg, &val, sizeof(uint32_t));
}

static int tcan4x5x_read_mcan_reg(const struct device *dev, uint16_t reg, uint32_t *val)
{
	return tcan4x5x_read(dev, CAN_TCAN4X5X_MCAN_BASE + reg, val, sizeof(uint32_t));
}

static int tcan4x5x_write_mcan_reg(const struct device *dev, uint16_t reg, uint32_t val)
{
	return tcan4x5x_write(dev, CAN_TCAN4X5X_MCAN_BASE + reg, &val, sizeof(uint32_t));
}

static int tcan4x5x_read_mcan_mram(const struct device *dev, uint16_t offset, void *dst, size_t len)
{
	return tcan4x5x_read(dev, CAN_TCAN4X5X_MRAM_BASE + offset, dst, len);
}

static int tcan4x5x_write_mcan_mram(const struct device *dev, uint16_t offset, const void *src,
				    size_t len)
{
	return tcan4x5x_write(dev, CAN_TCAN4X5X_MRAM_BASE + offset, src, len);
}

static int tcan4x5x_clear_mcan_mram(const struct device *dev, uint16_t offset, size_t len)
{
	static const uint8_t buf[256] = {0};
	size_t pending;
	size_t upto;
	int err;

	for (upto = 0; upto < len; upto += pending) {
		pending = MIN(len - upto, sizeof(buf));

		err = tcan4x5x_write_mcan_mram(dev, offset, &buf, pending);
		if (err != 0) {
			LOG_ERR("failed to clear message RAM (err %d)", err);
			return err;
		}

		offset += pending;
	}

	return 0;
}

static int tcan4x5x_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct tcan4x5x_config *tcan_config = mcan_config->custom;

	*rate = tcan_config->clk_freq;

	return 0;
}

static void tcan4x5x_int_gpio_callback_handler(const struct device *port, struct gpio_callback *cb,
					       gpio_port_pins_t pins)
{
	struct tcan4x5x_data *tcan_data = CONTAINER_OF(cb, struct tcan4x5x_data, int_gpio_cb);

	k_sem_give(&tcan_data->int_sem);
}

static void tcan4x5x_int_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev = p1;
	struct can_mcan_data *mcan_data = dev->data;
	struct tcan4x5x_data *tcan_data = mcan_data->custom;
	uint32_t status;
	uint32_t ir;
	int err;

	while (true) {
		k_sem_take(&tcan_data->int_sem, K_FOREVER);

		err = tcan4x5x_read_tcan_reg(dev, CAN_TCAN4X5X_IR, &ir);
		if (err != 0) {
			LOG_ERR("failed to read interrupt register (err %d)", err);
			continue;
		}

		while (ir != 0U) {
			err = tcan4x5x_write_tcan_reg(dev, CAN_TCAN4X5X_IR,
						      ir & CAN_TCAN4X5X_IR_CLEAR_ALL);
			if (err != 0) {
				LOG_ERR("failed to write interrupt register (err %d)", err);
				break;
			}

			if ((ir & CAN_TCAN4X5X_IR_SPIERR) != 0U) {
				err = tcan4x5x_read_tcan_reg(dev, CAN_TCAN4X5X_STATUS, &status);
				if (err != 0) {
					LOG_ERR("failed to read status register (err %d)", err);
					continue;
				}

				LOG_ERR("SPIERR, status = 0x%08x", status);

				err = tcan4x5x_write_tcan_reg(dev, CAN_TCAN4X5X_STATUS, status &
							      CAN_TCAN4X5X_STATUS_CLEAR_ALL);
				if (err != 0) {
					LOG_ERR("failed to write status register (err %d)", err);
					continue;
				}
			}

			if ((ir & CAN_TCAN4X5X_IR_M_CAN_INT) != 0U) {
				can_mcan_line_0_isr(dev);
				can_mcan_line_1_isr(dev);
			}

			err = tcan4x5x_read_tcan_reg(dev, CAN_TCAN4X5X_IR, &ir);
			if (err != 0) {
				LOG_ERR("failed to read interrupt register (err %d)", err);
				break;
			}
		}
	}
}

static int tcan4x5x_wake(const struct device *dev)
{
#if TCAN4X5X_WAKE_GPIO_SUPPORT
	const struct can_mcan_config *mcan_config = dev->config;
	const struct tcan4x5x_config *tcan_config = mcan_config->custom;
	int wake_needed = 1;
	int err;

#if TCAN4X5X_NWKRQ_GPIO_SUPPORT
	if (tcan_config->wake_gpio.port != NULL && tcan_config->nwkrq_gpio.port != NULL) {
		wake_needed = gpio_pin_get_dt(&tcan_config->nwkrq_gpio);

		if (wake_needed < 0) {
			LOG_ERR("failed to get nWKRQ status (err %d)", wake_needed);
			return wake_needed;
		};
	}
#endif /* TCAN4X5X_NWKRQ_GPIO_SUPPORT */
	if (tcan_config->wake_gpio.port != NULL && wake_needed != 0) {
		err = gpio_pin_set_dt(&tcan_config->wake_gpio, 1);
		if (err != 0) {
			LOG_ERR("failed to assert WAKE GPIO (err %d)", err);
			return err;
		}

		k_busy_wait(CAN_TCAN4X5X_T_WAKE_US);

		err = gpio_pin_set_dt(&tcan_config->wake_gpio, 0);
		if (err != 0) {
			LOG_ERR("failed to deassert WAKE GPIO (err %d)", err);
			return err;
		}
	}
#endif /* TCAN4X5X_WAKE_GPIO_SUPPORT*/

	return 0;
}

static int tcan4x5x_reset(const struct device *dev)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct tcan4x5x_config *tcan_config = mcan_config->custom;
	int err;

	err = tcan4x5x_wake(dev);
	if (err != 0) {
		return err;
	}

#if TCAN4X5X_RST_GPIO_SUPPORT
	if (tcan_config->rst_gpio.port != NULL) {
		err = gpio_pin_set_dt(&tcan_config->rst_gpio, 1);
		if (err != 0) {
			LOG_ERR("failed to assert RST GPIO (err %d)", err);
			return err;
		}

		k_busy_wait(CAN_TCAN4X5X_T_PULSE_WIDTH_US);

		err = gpio_pin_set_dt(&tcan_config->rst_gpio, 0);
		if (err != 0) {
			LOG_ERR("failed to deassert RST GPIO (err %d)", err);
			return err;
		}
	} else {
#endif /* TCAN4X5X_RST_GPIO_SUPPORT */
		err = tcan4x5x_write_tcan_reg(dev, CAN_TCAN4X5X_MODE_CONFIG,
					      CAN_TCAN4X5X_MODE_CONFIG_DEVICE_RESET);
		if (err != 0) {
			LOG_ERR("failed to initiate SW reset (err %d)", err);
			return err;
		}
#if TCAN4X5X_RST_GPIO_SUPPORT
	}
#endif /* TCAN4X5X_RST_GPIO_SUPPORT */

	k_busy_wait(CAN_TCAN4X5X_T_RESET_US);

	return 0;
}

static int tcan4x5x_init(const struct device *dev)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct tcan4x5x_config *tcan_config = mcan_config->custom;
	struct can_mcan_data *mcan_data = dev->data;
	struct tcan4x5x_data *tcan_data = mcan_data->custom;
	k_tid_t tid;
	uint32_t reg;
	int err;

	/* Initialize int_sem to 1 to ensure any pending IRQ is serviced */
	k_sem_init(&tcan_data->int_sem, 1, 1);

	if (!spi_is_ready_dt(&tcan_config->spi)) {
		LOG_ERR("SPI bus not ready");
		return -ENODEV;
	}

#if TCAN4X5X_RST_GPIO_SUPPORT
	if (tcan_config->rst_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&tcan_config->rst_gpio)) {
			LOG_ERR("RST GPIO not ready");
			return -ENODEV;
		}

		err = gpio_pin_configure_dt(&tcan_config->rst_gpio, GPIO_OUTPUT_INACTIVE);
		if (err != 0) {
			LOG_ERR("failed to configure RST GPIO (err %d)", err);
			return -ENODEV;
		}
	}
#endif /* TCAN4X5X_RST_GPIO_SUPPORT */

#if TCAN4X5X_NWKRQ_GPIO_SUPPORT
	if (tcan_config->nwkrq_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&tcan_config->nwkrq_gpio)) {
			LOG_ERR("nWKRQ GPIO not ready");
			return -ENODEV;
		}

		err = gpio_pin_configure_dt(&tcan_config->nwkrq_gpio, GPIO_INPUT);
		if (err != 0) {
			LOG_ERR("failed to configure nWKRQ GPIO (err %d)", err);
			return -ENODEV;
		}
	}
#endif /* TCAN4X5X_NWKRQ_GPIO_SUPPORT */

#if TCAN4X5X_WAKE_GPIO_SUPPORT
	if (tcan_config->wake_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&tcan_config->wake_gpio)) {
			LOG_ERR("WAKE GPIO not ready");
			return -ENODEV;
		}

		err = gpio_pin_configure_dt(&tcan_config->wake_gpio, GPIO_OUTPUT_INACTIVE);
		if (err != 0) {
			LOG_ERR("failed to configure WAKE GPIO (err %d)", err);
			return -ENODEV;
		}
	}
#endif /* TCAN4X5X_WAKE_GPIO_SUPPORT */

	if (!gpio_is_ready_dt(&tcan_config->int_gpio)) {
		LOG_ERR("nINT GPIO not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&tcan_config->int_gpio, GPIO_INPUT);
	if (err != 0) {
		LOG_ERR("failed to configure nINT GPIO (err %d)", err);
		return -ENODEV;
	}

	gpio_init_callback(&tcan_data->int_gpio_cb, tcan4x5x_int_gpio_callback_handler,
			   BIT(tcan_config->int_gpio.pin));

	err = gpio_add_callback_dt(&tcan_config->int_gpio, &tcan_data->int_gpio_cb);
	if (err != 0) {
		LOG_ERR("failed to add nINT GPIO callback (err %d)", err);
		return -ENODEV;
	}

	/* Initialize nINT GPIO callback and interrupt handler thread to ACK any early SPIERR */
	err = gpio_pin_interrupt_configure_dt(&tcan_config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (err != 0) {
		LOG_ERR("failed to configure nINT GPIO interrupt (err %d)", err);
		return -ENODEV;
	}

	tid = k_thread_create(&tcan_data->int_thread, tcan_data->int_stack,
			      K_KERNEL_STACK_SIZEOF(tcan_data->int_stack),
			      tcan4x5x_int_thread, (void *)dev, NULL, NULL,
			      CONFIG_CAN_TCAN4X5X_THREAD_PRIO, 0, K_NO_WAIT);
	k_thread_name_set(tid, "tcan4x5x");

	/* Reset TCAN */
	err = tcan4x5x_reset(dev);
	if (err != 0) {
		return -ENODEV;
	}

#if CONFIG_CAN_LOG_LEVEL >= LOG_LEVEL_DBG
	uint32_t info[3];

	/* Read DEVICE_ID1, DEVICE_ID2, and REVISION registers */
	err = tcan4x5x_read(dev, CAN_TCAN4X5X_DEVICE_ID1, &info, sizeof(info));
	if (err != 0) {
		return -EIO;
	}

	LOG_DBG("%c%c%c%c%c%c%c%c, SPI 2 rev. %lu, device rev. ID %lu.%lu",
		(char)FIELD_GET(GENMASK(7, 0), info[0]), (char)FIELD_GET(GENMASK(15, 8), info[0]),
		(char)FIELD_GET(GENMASK(23, 16), info[0]),
		(char)FIELD_GET(GENMASK(31, 24), info[0]), (char)FIELD_GET(GENMASK(7, 0), info[1]),
		(char)FIELD_GET(GENMASK(15, 8), info[1]), (char)FIELD_GET(GENMASK(23, 16), info[1]),
		(char)FIELD_GET(GENMASK(31, 24), info[1]), FIELD_GET(GENMASK(31, 24), info[2]),
		FIELD_GET(GENMASK(15, 8), info[2]), FIELD_GET(GENMASK(7, 0), info[2]));
#endif /* CONFIG_CAN_LOG_LEVEL >= LOG_LEVEL_DBG */

	/* Set TCAN4x5x mode normal */
	err = tcan4x5x_read_tcan_reg(dev, CAN_TCAN4X5X_MODE_CONFIG, &reg);
	if (err != 0) {
		LOG_ERR("failed to read configuration register (err %d)", err);
		return -ENODEV;
	}

	reg &= ~(CAN_TCAN4X5X_MODE_CONFIG_MODE_SEL);
	reg |= FIELD_PREP(CAN_TCAN4X5X_MODE_CONFIG_MODE_SEL, 0x02);
	reg |= CAN_TCAN4X5X_MODE_CONFIG_WAKE_CONFIG;

	if (tcan_config->clk_freq == MHZ(20)) {
		/* 20 MHz frequency reference */
		reg &= ~(CAN_TCAN4X5X_MODE_CONFIG_CLK_REF);
	} else {
		/* 40 MHz frequency reference */
		reg |= CAN_TCAN4X5X_MODE_CONFIG_CLK_REF;
	}

	err = tcan4x5x_write_tcan_reg(dev, CAN_TCAN4X5X_MODE_CONFIG, reg);
	if (err != 0) {
		LOG_ERR("failed to write configuration register (err %d)", err);
		return -ENODEV;
	}

	/* Wait for standby to normal mode switch */
	k_busy_wait(CAN_TCAN4X5X_T_MODE_STBY_NOM_US);

	/* Configure Message RAM */
	err = can_mcan_configure_mram(dev, CAN_TCAN4X5X_MRAM_BASE, CAN_TCAN4X5X_MRAM_BASE);
	if (err != 0) {
		return -EIO;
	}

	/* Initialize M_CAN */
	err = can_mcan_init(dev);
	if (err != 0) {
		LOG_ERR("failed to initialize mcan (err %d)", err);
		return err;
	}

	return 0;
}

static const struct can_driver_api tcan4x5x_driver_api = {
	.get_capabilities = can_mcan_get_capabilities,
	.start = can_mcan_start,
	.stop = can_mcan_stop,
	.set_mode = can_mcan_set_mode,
	.set_timing = can_mcan_set_timing,
	.send = can_mcan_send,
	.add_rx_filter = can_mcan_add_rx_filter,
	.remove_rx_filter = can_mcan_remove_rx_filter,
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	.recover = can_mcan_recover,
#endif /* CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */
	.get_state = can_mcan_get_state,
	.set_state_change_callback = can_mcan_set_state_change_callback,
	.get_core_clock = tcan4x5x_get_core_clock,
	.get_max_filters = can_mcan_get_max_filters,
	.timing_min = CAN_MCAN_TIMING_MIN_INITIALIZER,
	.timing_max = CAN_MCAN_TIMING_MAX_INITIALIZER,
#ifdef CONFIG_CAN_FD_MODE
	.set_timing_data = can_mcan_set_timing_data,
	.timing_data_min = CAN_MCAN_TIMING_DATA_MIN_INITIALIZER,
	.timing_data_max = CAN_MCAN_TIMING_DATA_MAX_INITIALIZER,
#endif /* CONFIG_CAN_FD_MODE */
};

static const struct can_mcan_ops tcan4x5x_ops = {
	.read_reg = tcan4x5x_read_mcan_reg,
	.write_reg = tcan4x5x_write_mcan_reg,
	.read_mram = tcan4x5x_read_mcan_mram,
	.write_mram = tcan4x5x_write_mcan_mram,
	.clear_mram = tcan4x5x_clear_mcan_mram,
};

#if TCAN4X5X_RST_GPIO_SUPPORT
#define TCAN4X5X_RST_GPIO_INIT(inst)                                                               \
	.rst_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),
#else /* TCAN4X5X_RST_GPIO_SUPPORT */
#define TCAN4X5X_RST_GPIO_INIT(inst)
#endif /* !TCAN4X5X_RST_GPIO_SUPPORT */

#if TCAN4X5X_NWKRQ_GPIO_SUPPORT
#define TCAN4X5X_NWKRQ_GPIO_INIT(inst)                                                             \
	.nwkrq_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, device_state_gpios, {0}),
#else /* TCAN4X5X_NWKRQ_GPIO_SUPPORT */
#define TCAN4X5X_NWKRQ_GPIO_INIT(inst)
#endif /* !TCAN4X5X_NWKRQ_GPIO_SUPPORT */

#if TCAN4X5X_WAKE_GPIO_SUPPORT
#define TCAN4X5X_WAKE_GPIO_INIT(inst)                                                              \
	.wake_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, device_wake_gpios, {0}),
#else /* TCAN4X5X_WAKE_GPIO_SUPPORT */
#define TCAN4X5X_WAKE_GPIO_INIT(inst)
#endif /* !TCAN4X5X_WAKE_GPIO_SUPPORT */

#define TCAN4X5X_INIT(inst)                                                                        \
	BUILD_ASSERT(CAN_MCAN_DT_INST_MRAM_OFFSET(inst) == 0, "MRAM offset must be 0");            \
	BUILD_ASSERT(CAN_MCAN_DT_INST_MRAM_ELEMENTS_SIZE(inst) <= CAN_TCAN4X5X_MRAM_SIZE,          \
		     "Insufficient Message RAM size to hold elements");                            \
                                                                                                   \
	CAN_MCAN_DT_INST_BUILD_ASSERT_MRAM_CFG(inst);                                              \
	CAN_MCAN_DT_INST_CALLBACKS_DEFINE(inst, tcan4x5x_cbs_##inst);                              \
                                                                                                   \
	static const struct tcan4x5x_config tcan4x5x_config_##inst = {                             \
		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8), 0),                             \
		.int_gpio = GPIO_DT_SPEC_INST_GET(inst, int_gpios),                                \
		.clk_freq = DT_INST_PROP(inst, clock_frequency),                                   \
		TCAN4X5X_RST_GPIO_INIT(inst)                                                       \
		TCAN4X5X_NWKRQ_GPIO_INIT(inst)                                                     \
		TCAN4X5X_WAKE_GPIO_INIT(inst)                                                      \
	};                                                                                         \
                                                                                                   \
	static const struct can_mcan_config can_mcan_config_##inst = CAN_MCAN_DT_CONFIG_INST_GET(  \
		inst, &tcan4x5x_config_##inst, &tcan4x5x_ops, &tcan4x5x_cbs_##inst);               \
                                                                                                   \
	static struct tcan4x5x_data tcan4x5x_data_##inst;                                          \
                                                                                                   \
	static struct can_mcan_data can_mcan_data_##inst =                                         \
		CAN_MCAN_DATA_INITIALIZER(&tcan4x5x_data_##inst);                                  \
                                                                                                   \
	CAN_DEVICE_DT_INST_DEFINE(inst, tcan4x5x_init, NULL, &can_mcan_data_##inst,                \
				  &can_mcan_config_##inst, POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,  \
				  &tcan4x5x_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TCAN4X5X_INIT)
