/*
 * Copyright (c) 2025 Pedro Ramos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ENERGY_METER_ATM90E32AS_ATM90E32AS_H_
#define ZEPHYR_DRIVERS_ENERGY_METER_ATM90E32AS_ATM90E32AS_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/energy_meter.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

/* Status and special registers */
#define ATM90E32AS_REG_METER_EN       0x00U
#define ATM90E32AS_REG_CHANNEL_MAP_I  0x01U
#define ATM90E32AS_REG_CHANNEL_MAP_U  0x02U
#define ATM90E32AS_REG_SAG_PEAK_DET   0x05U
#define ATM90E32AS_REG_OV_TH          0x06U
#define ATM90E32AS_REG_ZX_CONFIG      0x07U
#define ATM90E32AS_REG_SAG_TH         0x08U
#define ATM90E32AS_REG_PHASE_LOSS_TH  0x09U
#define ATM90E32AS_REG_IN_WARN_TH     0x0AU
#define ATM90E32AS_REG_OI_TH          0x0BU
#define ATM90E32AS_REG_FREQ_LO_TH     0x0CU
#define ATM90E32AS_REG_FREQ_HI_TH     0x0DU
#define ATM90E32AS_REG_PM_PWR_CTRL    0x0EU
#define ATM90E32AS_REG_IRQ0_MERGE_CFG 0x0FU

/* Low power mode and segment compensation registers */
#define ATM90E32AS_REG_DETECT_CTRL   0x10U
#define ATM90E32AS_REG_DETECT_TH1    0x11U
#define ATM90E32AS_REG_IDC_OFFSET_A  0x14U
#define ATM90E32AS_REG_UDC_OFFSET_A  0x17U
#define ATM90E32AS_REG_U_GAIN_TAB    0x1AU
#define ATM90E32AS_REG_U_GAIN_TC     0x1BU
#define ATM90E32AS_REG_PHI_FREQ_COMP 0x1CU
#define ATM90E32AS_REG_LOG_IRMS0     0x20U
#define ATM90E32AS_REG_LOG_IRMS1     0x21U
#define ATM90E32AS_REG_F0            0x22U
#define ATM90E32AS_REG_T0            0x23U
/* Phase A segment registers; phases B and C follow at a stride of 4 */
#define ATM90E32AS_REG_PHI_A_IRMS01  0x24U
#define ATM90E32AS_REG_PHI_A_IRMS2   0x25U
#define ATM90E32AS_REG_GAIN_A_IRMS01 0x26U
#define ATM90E32AS_REG_GAIN_A_IRMS2  0x27U
#define ATM90E32AS_SEGMENT_STRIDE    4U

/* Configuration registers */
#define ATM90E32AS_REG_PL_CONST_H 0x31U
#define ATM90E32AS_REG_PL_CONST_L 0x32U
#define ATM90E32AS_REG_MMODE0     0x33U
#define ATM90E32AS_REG_MMODE1     0x34U
#define ATM90E32AS_REG_P_START_TH 0x35U
#define ATM90E32AS_REG_Q_START_TH 0x36U
#define ATM90E32AS_REG_S_START_TH 0x37U
#define ATM90E32AS_REG_P_PHASE_TH 0x38U
#define ATM90E32AS_REG_Q_PHASE_TH 0x39U
#define ATM90E32AS_REG_S_PHASE_TH 0x3AU

/* Energy calibration registers; phases B and C follow at a stride of 2 */
#define ATM90E32AS_REG_P_OFFSET_A  0x41U
#define ATM90E32AS_REG_Q_OFFSET_A  0x42U
#define ATM90E32AS_REG_PQ_GAIN_A   0x47U
#define ATM90E32AS_REG_PHI_A       0x48U
#define ATM90E32AS_CAL_STRIDE      2U
#define ATM90E32AS_REG_P_OFFSET_AF 0x51U
#define ATM90E32AS_REG_P_GAIN_AF   0x54U

/* Measurement calibration registers; phases B and C follow at a stride of 4 */
#define ATM90E32AS_REG_U_GAIN_A    0x61U
#define ATM90E32AS_REG_I_GAIN_A    0x62U
#define ATM90E32AS_REG_U_OFFSET_A  0x63U
#define ATM90E32AS_REG_I_OFFSET_A  0x64U
#define ATM90E32AS_MEAS_CAL_STRIDE 4U

/* EMM status registers */
#define ATM90E32AS_REG_SOFT_RESET     0x70U
#define ATM90E32AS_REG_EMM_STATE0     0x71U
#define ATM90E32AS_REG_EMM_STATE1     0x72U
#define ATM90E32AS_REG_EMM_INT_STATE0 0x73U
#define ATM90E32AS_REG_EMM_INT_STATE1 0x74U
#define ATM90E32AS_REG_EMM_INT_EN0    0x75U
#define ATM90E32AS_REG_EMM_INT_EN1    0x76U
#define ATM90E32AS_REG_LAST_SPI_DATA  0x78U
#define ATM90E32AS_REG_CRC_ERR_STATUS 0x79U
#define ATM90E32AS_REG_CRC_DIGEST     0x7AU
#define ATM90E32AS_REG_CFG_REG_ACC_EN 0x7FU

/* Energy registers, cleared when read */
#define ATM90E32AS_REG_AP_ENERGY_T  0x80U
#define ATM90E32AS_REG_AN_ENERGY_T  0x84U
#define ATM90E32AS_REG_RP_ENERGY_T  0x88U
#define ATM90E32AS_REG_RN_ENERGY_T  0x8CU
#define ATM90E32AS_REG_SA_ENERGY_T  0x90U
#define ATM90E32AS_REG_AP_ENERGY_TF 0xA0U
#define ATM90E32AS_REG_AN_ENERGY_TF 0xA4U
#define ATM90E32AS_REG_AP_ENERGY_TH 0xA8U
#define ATM90E32AS_REG_AN_ENERGY_TH 0xACU

/* Measurement registers */
#define ATM90E32AS_REG_PMEAN_T      0xB0U
#define ATM90E32AS_REG_QMEAN_T      0xB4U
#define ATM90E32AS_REG_SMEAN_T      0xB8U
#define ATM90E32AS_REG_PFMEAN_T     0xBCU
#define ATM90E32AS_REG_PMEAN_T_LSB  0xC0U
#define ATM90E32AS_REG_QMEAN_T_LSB  0xC4U
#define ATM90E32AS_REG_SMEAN_T_LSB  0xC8U
#define ATM90E32AS_REG_PMEAN_TF     0xD0U
#define ATM90E32AS_REG_PMEAN_TH     0xD4U
#define ATM90E32AS_REG_URMS_A       0xD9U
#define ATM90E32AS_REG_IRMS_N       0xDCU
#define ATM90E32AS_REG_IRMS_A       0xDDU
#define ATM90E32AS_REG_PMEAN_TF_LSB 0xE0U
#define ATM90E32AS_REG_PMEAN_TH_LSB 0xE4U
#define ATM90E32AS_REG_URMS_A_LSB   0xE9U
#define ATM90E32AS_REG_IRMS_A_LSB   0xEDU
#define ATM90E32AS_REG_UPEAK_A      0xF1U
#define ATM90E32AS_REG_IPEAK_A      0xF5U
#define ATM90E32AS_REG_FREQ         0xF8U
#define ATM90E32AS_REG_PANGLE_A     0xF9U
#define ATM90E32AS_REG_TEMP         0xFCU
#define ATM90E32AS_REG_UANGLE_A     0xFDU

/* Register values */
#define ATM90E32AS_SOFT_RESET_CMD     0x789AU
#define ATM90E32AS_CFG_ACCESS_ENABLE  0x55AAU
/* Any value other than 0x55AA/0xAA55 returns to normal operation and latches the golden CRC */
#define ATM90E32AS_CFG_ACCESS_DISABLE 0x0000U
#define ATM90E32AS_METER_EN_ON        0x0001U
#define ATM90E32AS_PL_CONST_H_DEFAULT 0x0861U
/* Temp reads this until the first measurement, a few hundred ms after a reset */
#define ATM90E32AS_TEMP_NOT_READY     0xFF00U

/* SPI frame: 16-bit address word (bit 15 set for reads) followed by a 16-bit data word */
#define ATM90E32AS_SPI_READ      BIT(15)
#define ATM90E32AS_SPI_ADDR_MASK GENMASK(14, 0)
#define ATM90E32AS_SPI_OPERATION                                                                   \
	(SPI_OP_MODE_CONTROLLER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPOL |             \
	 SPI_MODE_CPHA)

/* Delay after power on, soft reset or exit from Idle mode (T1 maximum) */
#define ATM90E32AS_RESET_DELAY_MS 40

/* Energy registers are 0x80-0x93 and 0xA0-0xAF; one accumulator per register */
#define ATM90E32AS_ENERGY_COUNT 36U

#define ATM90E32AS_PHASES 3U

/** A register address and the value written to it at initialization. */
struct atm90e32as_reg_val {
	uint8_t addr;
	uint16_t val;
};

enum atm90e32as_irq_line {
	ATM90E32AS_IRQ0,
	ATM90E32AS_IRQ1,
	ATM90E32AS_WARN,
	ATM90E32AS_CF1,
	ATM90E32AS_CF2,
	ATM90E32AS_CF3,
	ATM90E32AS_CF4,
	ATM90E32AS_IRQ_LINES,
};

struct atm90e32as_config {
	struct spi_dt_spec bus;
	const struct atm90e32as_reg_val *init_regs;
	size_t init_regs_len;
	uint32_t meter_constant;
	/* Actual current over the current the chip reads: numerator, denominator */
	uint16_t current_scale[2];
	uint16_t u_gain[ATM90E32AS_PHASES];
	uint16_t i_gain[ATM90E32AS_PHASES];
	struct gpio_dt_spec pm0;
	struct gpio_dt_spec pm1;
#ifdef CONFIG_ATM90E32AS_EVENTS
	struct gpio_dt_spec irq_gpios[ATM90E32AS_IRQ_LINES];
#endif
};

#ifdef CONFIG_ATM90E32AS_EVENTS
struct atm90e32as_irq_ctx {
	const struct device *dev;
	struct gpio_callback gpio_cb;
	struct k_work work;
	struct energy_meter_event event;
};
#endif

struct atm90e32as_data {
	struct k_mutex lock;
	uint64_t energy[ATM90E32AS_ENERGY_COUNT];
#ifdef CONFIG_ATM90E32AS_EVENTS
	struct atm90e32as_irq_ctx irq[ATM90E32AS_IRQ_LINES];
	energy_meter_event_cb_t event_cb;
	void *event_user_data;
#endif
};

/* True when dev is an ATM90E32AS */
bool atm90e32as_is_device(const struct device *dev);

int atm90e32as_reg_read(const struct device *dev, uint16_t addr, uint16_t *val);
int atm90e32as_reg_write(const struct device *dev, uint16_t addr, uint16_t val);

/* Read a register for debugging; energy registers are added to the accumulators */
int atm90e32as_debug_read(const struct device *dev, uint8_t addr, uint16_t *val);

/* Write a register for debugging; configuration registers are unlocked around the write */
int atm90e32as_debug_write(const struct device *dev, uint8_t addr, uint16_t val);

#ifdef CONFIG_ATM90E32AS_EVENTS
int atm90e32as_events_init(const struct device *dev);
int atm90e32as_set_event_callback(const struct device *dev, energy_meter_event_cb_t cb,
				  void *user_data);
void atm90e32as_events_enable(const struct device *dev, bool enable);
#endif

#endif /* ZEPHYR_DRIVERS_ENERGY_METER_ATM90E32AS_ATM90E32AS_H_ */
