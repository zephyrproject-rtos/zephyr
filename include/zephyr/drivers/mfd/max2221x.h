/*
 * Copyright (c) 2025 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for MAX2221X MFD driver
 * @ingroup mfd_interface_max2221x
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_MAX2221X_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_MAX2221X_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/device.h>
#include <zephyr/sys/util.h>

/**
 * @defgroup mfd_interface_max2221x MFD MAX2221X
 * @ingroup mfd_interfaces
 * @{
 */

/**
 * @name MAX2221X SPI Transaction Fields
 * @{
 */
/** SPI transaction register address field mask. */
#define MAX2221X_SPI_TRANS_ADDR GENMASK(6, 0)
/** SPI transaction read/write direction bit. */
#define MAX2221X_SPI_TRANS_DIR  BIT(7)
/** @} */

/**
 * @name MAX2221X Register Addresses
 * @{
 */
/** Global control register address. */
#define MAX2221X_REG_GLOBAL_CTRL     0x00
/** Global configuration register address. */
#define MAX2221X_REG_GLOBAL_CFG      0x01
/** Status register address. */
#define MAX2221X_REG_STATUS          0x02
/** Demagnetization duty cycle high-to-low register address. */
#define MAX2221X_REG_DC_H2L          0x03
/** Voltage monitor register address. */
#define MAX2221X_REG_VM_MONITOR      0x05
/** Voltage monitor threshold register address. */
#define MAX2221X_REG_VM_THRESHOLD    0x06
/**
 * @brief Channel duty cycle low-to-high configuration register address
 * @param x Channel number
 */
#define MAX2221X_REG_CFG_DC_L2H(x)   (0x09 + ((x) * 0xE))
/**
 * @brief Channel duty cycle high configuration register address
 * @param x Channel number
 */
#define MAX2221X_REG_CFG_DC_H(x)     (0x0A + ((x) * 0xE))
/**
 * @brief Channel duty cycle low configuration register address
 * @param x Channel number
 */
#define MAX2221X_REG_CFG_DC_L(x)     (0x0B + ((x) * 0xE))
/**
 * @brief Channel time low-to-high configuration register address
 * @param x Channel number
 */
#define MAX2221X_REG_CFG_TIME_L2H(x) (0x0C + ((x) * 0xE))
/**
 * @brief Channel control 0 configuration register address
 * @param x Channel number
 */
#define MAX2221X_REG_CFG_CTRL0(x)    (0x0D + ((x) * 0xE))
/**
 * @brief Channel control 1 configuration register address
 * @param x Channel number
 */
#define MAX2221X_REG_CFG_CTRL1(x)    (0x0E + ((x) * 0xE))
/**
 * @brief Channel PWM duty cycle register address
 * @param x Channel number
 */
#define MAX2221X_REG_PWM_DUTY(x)     (0x49 + ((x) * 0xE))
/** Fault 0 register address. */
#define MAX2221X_REG_FAULT0          0x65
/** Fault 1 register address. */
#define MAX2221X_REG_FAULT1          0x66
/** @} */

/**
 * @name MAX2221X Global Configuration Masks
 * @{
 */
/** Master chopping frequency mask in global configuration register. */
#define MAX2221X_F_PWM_M_MASK GENMASK(7, 4)
/** Channel 0 control enable mask. */
#define MAX2221X_CNTL0_MASK   BIT(0)
/** Channel 1 control enable mask. */
#define MAX2221X_CNTL1_MASK   BIT(1)
/** Channel 2 control enable mask. */
#define MAX2221X_CNTL2_MASK   BIT(2)
/** Channel 3 control enable mask. */
#define MAX2221X_CNTL3_MASK   BIT(3)
/** @} */

/**
 * @name MAX2221X Global Control Masks
 * @{
 */
/** Active mode enable mask. */
#define MAX2221X_ACTIVE_MASK      BIT(15)
/** Under-voltage monitor mask. */
#define MAX2221X_M_UVM_MASK       BIT(8)
/** VDR/VDRDUTY mode selection mask. */
#define MAX2221X_VDRNVDRDUTY_MASK BIT(4)
/** @} */

/**
 * @name MAX2221X Status Register Masks
 * @{
 */
/** Status bit 3 mask. */
#define MAX2221X_STATUS_STT3_MASK     BIT(14)
/** Status bit 2 mask. */
#define MAX2221X_STATUS_STT2_MASK     BIT(13)
/** Status bit 1 mask. */
#define MAX2221X_STATUS_STT1_MASK     BIT(12)
/** Status bit 0 mask. */
#define MAX2221X_STATUS_STT0_MASK     BIT(11)
/** Minimum on-time fault status mask. */
#define MAX2221X_STATUS_MIN_T_ON_MASK BIT(10)
/** Resistance measurement status mask. */
#define MAX2221X_STATUS_RES_MASK      BIT(9)
/** Inductance measurement status mask. */
#define MAX2221X_STATUS_IND_MASK      BIT(8)
/** Over-temperature fault status mask. */
#define MAX2221X_STATUS_OVT_MASK      BIT(7)
/** Over-current protection fault status mask. */
#define MAX2221X_STATUS_OCP_MASK      BIT(6)
/** Open-load fault status mask. */
#define MAX2221X_STATUS_OLF_MASK      BIT(5)
/** Hit current not reached fault status mask. */
#define MAX2221X_STATUS_HHF_MASK      BIT(4)
/** DPM error fault status mask. */
#define MAX2221X_STATUS_DPM_MASK      BIT(3)
/** Communication error fault status mask. */
#define MAX2221X_STATUS_COMER_MASK    BIT(2)
/** Under-voltage monitor fault status mask. */
#define MAX2221X_STATUS_UVM_MASK      BIT(1)
/** @} */

/**
 * @name MAX2221X VM Monitor Masks
 * @{
 */
/** Voltage monitor measurement value mask. */
#define MAX2221X_VM_MONITOR_MASK GENMASK(12, 0)
/** @} */

/**
 * @name MAX2221X VM Threshold Masks
 * @{
 */
/** Upper voltage monitor threshold mask. */
#define MAX2221X_VM_THLD_UP_MASK   GENMASK(7, 4)
/** Lower voltage monitor threshold mask. */
#define MAX2221X_VM_THLD_DOWN_MASK GENMASK(3, 0)
/** @} */

/**
 * @name MAX2221X Channel Control 0 Masks
 * @{
 */
/** Control mode selection mask. */
#define MAX2221X_CTRL_MODE_MASK GENMASK(15, 14)
/** Ramp-down enable mask. */
#define MAX2221X_RDWE_MASK      BIT(10)
/** Ramp-middle enable mask. */
#define MAX2221X_RMDE_MASK      BIT(9)
/** Ramp-up enable mask. */
#define MAX2221X_RUPE_MASK      BIT(8)
/** Ramp slew rate value mask. */
#define MAX2221X_RAMP_MASK      GENMASK(7, 0)
/** @} */

/**
 * @name MAX2221X Channel Control 1 Masks
 * @{
 */
/** Channel chopping frequency divisor mask. */
#define MAX2221X_F_PWM_MASK     GENMASK(9, 8)
/** Output slew rate selection mask. */
#define MAX2221X_SLEW_RATE_MASK GENMASK(5, 4)
/** Digital gain selection mask. */
#define MAX2221X_GAIN_MASK      GENMASK(3, 2)
/** Sense scaling factor selection mask. */
#define MAX2221X_SNSF_MASK      GENMASK(1, 0)
/** @} */

/**
 * @name MAX2221X Fault 0 Register Masks
 * @{
 */
/** DPM fault on channel 3 mask. */
#define MAX2221X_FAULT_DPM3_MASK BIT(15)
/** DPM fault on channel 2 mask. */
#define MAX2221X_FAULT_DPM2_MASK BIT(14)
/** DPM fault on channel 1 mask. */
#define MAX2221X_FAULT_DPM1_MASK BIT(13)
/** DPM fault on channel 0 mask. */
#define MAX2221X_FAULT_DPM0_MASK BIT(12)
/** Open-load fault on channel 3 mask. */
#define MAX2221X_FAULT_OLF3_MASK BIT(11)
/** Open-load fault on channel 2 mask. */
#define MAX2221X_FAULT_OLF2_MASK BIT(10)
/** Open-load fault on channel 1 mask. */
#define MAX2221X_FAULT_OLF1_MASK BIT(9)
/** Open-load fault on channel 0 mask. */
#define MAX2221X_FAULT_OLF0_MASK BIT(8)
/** Hit current not reached fault on channel 3 mask. */
#define MAX2221X_FAULT_HHF3_MASK BIT(7)
/** Hit current not reached fault on channel 2 mask. */
#define MAX2221X_FAULT_HHF2_MASK BIT(6)
/** Hit current not reached fault on channel 1 mask. */
#define MAX2221X_FAULT_HHF1_MASK BIT(5)
/** Hit current not reached fault on channel 0 mask. */
#define MAX2221X_FAULT_HHF0_MASK BIT(4)
/** Over-current protection fault on channel 3 mask. */
#define MAX2221X_FAULT_OCP3_MASK BIT(3)
/** Over-current protection fault on channel 2 mask. */
#define MAX2221X_FAULT_OCP2_MASK BIT(2)
/** Over-current protection fault on channel 1 mask. */
#define MAX2221X_FAULT_OCP1_MASK BIT(1)
/** Over-current protection fault on channel 0 mask. */
#define MAX2221X_FAULT_OCP0_MASK BIT(0)
/** @} */

/**
 * @name MAX2221X Fault 1 Register Masks
 * @{
 */
/** Resistance measurement fault on channel 3 mask. */
#define MAX2221X_FAULT_RES3_MASK  BIT(10)
/** Resistance measurement fault on channel 2 mask. */
#define MAX2221X_FAULT_RES2_MASK  BIT(9)
/** Resistance measurement fault on channel 1 mask. */
#define MAX2221X_FAULT_RES1_MASK  BIT(8)
/** Resistance measurement fault on channel 0 mask. */
#define MAX2221X_FAULT_RES0_MASK  BIT(7)
/** Over-temperature fault mask. */
#define MAX2221X_FAULT_OVT_MASK   BIT(6)
/** Communication error fault mask. */
#define MAX2221X_FAULT_COMER_MASK BIT(5)
/** Under-voltage monitor fault mask. */
#define MAX2221X_FAULT_UVM_MASK   BIT(4)
/** Inductance measurement fault on channel 3 mask. */
#define MAX2221X_FAULT_IND3_MASK  BIT(3)
/** Inductance measurement fault on channel 2 mask. */
#define MAX2221X_FAULT_IND2_MASK  BIT(2)
/** Inductance measurement fault on channel 1 mask. */
#define MAX2221X_FAULT_IND1_MASK  BIT(1)
/** Inductance measurement fault on channel 0 mask. */
#define MAX2221X_FAULT_IND0_MASK  BIT(0)
/** @} */

/** Number of MAX2221X channels. */
#define MAX2221X_NUM_CHANNELS 4

/**
 * @brief Read register from max2221x
 *
 * @param dev max2221x mfd device
 * @param addr register address to read from
 * @param value pointer to buffer for received data
 * @retval 0 If successful
 * @retval -errno In case of any error (see spi_transceive_dt())
 */
int max2221x_reg_read(const struct device *dev, uint8_t addr, uint16_t *value);

/**
 * @brief Write register to max2221x
 *
 * @param dev max2221x mfd device
 * @param addr register address to write to
 * @param value content to write
 * @retval 0 If successful
 * @retval -errno In case of any error (see spi_write_dt())
 */
int max2221x_reg_write(const struct device *dev, uint8_t addr, uint16_t value);

/**
 * @brief Update register in max2221x
 *
 * @param dev max2221x mfd device
 * @param addr register address to update
 * @param mask mask to apply to the register
 * @param val value to write to the register
 * @retval 0 If successful
 * @retval -errno In case of any error (see spi_transceive_dt())
 */
int max2221x_reg_update(const struct device *dev, uint8_t addr, uint16_t mask, uint16_t val);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_MAX2221X_H_ */
