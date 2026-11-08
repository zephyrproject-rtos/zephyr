/*
 * Copyright (c) 2025 Pedro Ramos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ENERGY_METER_ATM90E26_H_
#define ZEPHYR_DRIVERS_ENERGY_METER_ATM90E26_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/energy_meter.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#ifdef CONFIG_ATM90E26_BUS_SPI
#include <zephyr/drivers/spi.h>
#endif

/* Status and special registers */
#define ATM90E26_REG_SOFT_RESET 0x00U
#define ATM90E26_REG_SYS_STATUS 0x01U
#define ATM90E26_REG_FUNC_EN    0x02U
#define ATM90E26_REG_SAG_TH     0x03U
#define ATM90E26_REG_LAST_DATA  0x06U

/* Metering calibration registers, protected by CS1 */
#define ATM90E26_REG_CAL_START  0x20U
#define ATM90E26_REG_PL_CONST_H 0x21U
#define ATM90E26_REG_MMODE      0x2BU
#define ATM90E26_REG_CS1        0x2CU
#define ATM90E26_METER_CAL_LEN  (ATM90E26_REG_MMODE - ATM90E26_REG_PL_CONST_H + 1U)

/* Measurement calibration registers, protected by CS2 */
#define ATM90E26_REG_ADJ_START  0x30U
#define ATM90E26_REG_U_GAIN     0x31U
#define ATM90E26_REG_Q_OFFSET_N 0x3AU
#define ATM90E26_REG_CS2        0x3BU
#define ATM90E26_MEAS_CAL_LEN   (ATM90E26_REG_Q_OFFSET_N - ATM90E26_REG_U_GAIN + 1U)

/* Energy registers, cleared when read */
#define ATM90E26_REG_AP_ENERGY 0x40U
#define ATM90E26_REG_AN_ENERGY 0x41U
#define ATM90E26_REG_AT_ENERGY 0x42U
#define ATM90E26_REG_RP_ENERGY 0x43U
#define ATM90E26_REG_RN_ENERGY 0x44U
#define ATM90E26_REG_RT_ENERGY 0x45U
#define ATM90E26_ENERGY_COUNT  6U
#define ATM90E26_REG_EN_STATUS 0x46U

/* Measurement registers */
#define ATM90E26_REG_IRMS    0x48U
#define ATM90E26_REG_URMS    0x49U
#define ATM90E26_REG_PMEAN   0x4AU
#define ATM90E26_REG_QMEAN   0x4BU
#define ATM90E26_REG_FREQ    0x4CU
#define ATM90E26_REG_POWERF  0x4DU
#define ATM90E26_REG_PANGLE  0x4EU
#define ATM90E26_REG_SMEAN   0x4FU
#define ATM90E26_REG_IRMS2   0x68U
#define ATM90E26_REG_PMEAN2  0x6AU
#define ATM90E26_REG_QMEAN2  0x6BU
#define ATM90E26_REG_POWERF2 0x6DU
#define ATM90E26_REG_PANGLE2 0x6EU
#define ATM90E26_REG_SMEAN2  0x6FU

/* Register values */
#define ATM90E26_SOFT_RESET_CMD     0x789AU
#define ATM90E26_CAL_START_CMD      0x5678U
#define ATM90E26_CAL_CHECK_CMD      0x8765U
#define ATM90E26_PL_CONST_H_DEFAULT 0x0015U
#define ATM90E26_SYS_STATUS_CAL_ERR GENMASK(15, 14)
#define ATM90E26_SYS_STATUS_ADJ_ERR GENMASK(13, 12)

/* Bit 7 of the address byte selects a read on both buses */
#define ATM90E26_READ      BIT(7)
#define ATM90E26_ADDR_MASK GENMASK(6, 0)

#define ATM90E26_RESET_DELAY_MS 5

enum atm90e26_irq_line {
	ATM90E26_IRQ,
	ATM90E26_WARN,
	ATM90E26_CF1,
	ATM90E26_CF2,
	ATM90E26_IRQ_LINES,
};

struct atm90e26_bus_io {
	/** Check that the bus is ready and usable for the chip. */
	int (*check)(const struct device *dev);
	/**
	 * Read a register. Returns -EBADMSG when the chip answered but the value may be
	 * corrupted, so the caller can recover it from LastData.
	 */
	int (*read)(const struct device *dev, uint8_t addr, uint16_t *val);
	/** Write a register. */
	int (*write)(const struct device *dev, uint8_t addr, uint16_t val);
};

union atm90e26_bus {
#ifdef CONFIG_ATM90E26_BUS_SPI
	struct spi_dt_spec spi;
#endif
#ifdef CONFIG_ATM90E26_BUS_UART
	const struct device *uart;
#endif
};

struct atm90e26_config {
	union atm90e26_bus bus;
	const struct atm90e26_bus_io *bus_io;
	uint32_t meter_constant;
	/* Actual current over the current the chip reads: numerator, denominator */
	uint16_t current_scale[2];
	uint16_t func_en;
	uint16_t sag_th;
	uint16_t meter_cal[ATM90E26_METER_CAL_LEN];
	uint16_t meas_cal[ATM90E26_MEAS_CAL_LEN];
#ifdef CONFIG_ATM90E26_EVENTS
	struct gpio_dt_spec irq_gpios[ATM90E26_IRQ_LINES];
#endif
};

#ifdef CONFIG_ATM90E26_EVENTS
struct atm90e26_irq_ctx {
	const struct device *dev;
	struct gpio_callback gpio_cb;
	struct k_work work;
	struct energy_meter_event event;
};
#endif

struct atm90e26_data {
	struct k_mutex lock;
	uint64_t energy[ATM90E26_ENERGY_COUNT];
#ifdef CONFIG_ATM90E26_BUS_UART
	struct k_sem rx_done;
	uint8_t rx_buf[3];
	size_t rx_len;
	size_t rx_expected;
	uint32_t baudrate;
#endif
#ifdef CONFIG_ATM90E26_EVENTS
	struct atm90e26_irq_ctx irq[ATM90E26_IRQ_LINES];
	energy_meter_event_cb_t event_cb;
	void *event_user_data;
#endif
};

/* Checksum of a block of calibration registers, as stored in CS1 and CS2 */
uint16_t atm90e26_checksum(const uint16_t *regs, size_t len);

#ifdef CONFIG_ATM90E26_BUS_SPI
#define ATM90E26_SPI_OPERATION                                                                     \
	(SPI_OP_MODE_CONTROLLER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPOL |             \
	 SPI_MODE_CPHA)
extern const struct atm90e26_bus_io atm90e26_bus_io_spi;
#endif

#ifdef CONFIG_ATM90E26_BUS_UART
#define ATM90E26_UART_START 0xFEU
extern const struct atm90e26_bus_io atm90e26_bus_io_uart;
#endif

/* True when dev is an ATM90E26 */
bool atm90e26_is_device(const struct device *dev);

/* Read a register for debugging; clear-on-read registers are handled as in normal reads */
int atm90e26_debug_read(const struct device *dev, uint8_t addr, uint16_t *val);

/* Write a register for debugging; calibration registers are rewritten as a whole block */
int atm90e26_debug_write(const struct device *dev, uint8_t addr, uint16_t val);

#ifdef CONFIG_ATM90E26_EVENTS
int atm90e26_events_init(const struct device *dev);
int atm90e26_set_event_callback(const struct device *dev, energy_meter_event_cb_t cb,
				void *user_data);
void atm90e26_events_enable(const struct device *dev, bool enable);
#endif

#endif /* ZEPHYR_DRIVERS_ENERGY_METER_ATM90E26_H_ */
