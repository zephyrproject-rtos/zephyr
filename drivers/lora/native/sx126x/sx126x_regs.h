/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_LORA_SX126X_SX126X_REGS_H_
#define ZEPHYR_DRIVERS_LORA_SX126X_SX126X_REGS_H_

#include <stdint.h>
#include <zephyr/sys/util.h>

/* SPI Command Opcodes */

/* Operational Modes Commands */
#define SX126X_CMD_SET_SLEEP                0x84
#define SX126X_CMD_SET_STANDBY              0x80
#define SX126X_CMD_SET_FS                   0xC1
#define SX126X_CMD_SET_TX                   0x83
#define SX126X_CMD_SET_RX                   0x82
#define SX126X_CMD_STOP_TIMER_ON_PREAMBLE   0x9F
#define SX126X_CMD_SET_RX_DUTY_CYCLE        0x94
#define SX126X_CMD_SET_CAD                  0xC5
#define SX126X_CMD_SET_TX_CONTINUOUS_WAVE   0xD1
#define SX126X_CMD_SET_TX_INFINITE_PREAMBLE 0xD2
#define SX126X_CMD_SET_REGULATOR_MODE       0x96
#define SX126X_CMD_CALIBRATE                0x89
#define SX126X_CMD_CALIBRATE_IMAGE          0x98
#define SX126X_CMD_SET_PA_CONFIG            0x95
#define SX126X_CMD_SET_RX_TX_FALLBACK_MODE  0x93

/* Register and Buffer Access Commands */
#define SX126X_CMD_WRITE_REGISTER           0x0D
#define SX126X_CMD_READ_REGISTER            0x1D
#define SX126X_CMD_WRITE_BUFFER             0x0E
#define SX126X_CMD_READ_BUFFER              0x1E

/* DIO and IRQ Control Commands */
#define SX126X_CMD_SET_DIO_IRQ_PARAMS       0x08
#define SX126X_CMD_GET_IRQ_STATUS           0x12
#define SX126X_CMD_CLR_IRQ_STATUS           0x02
#define SX126X_CMD_SET_DIO2_AS_RF_SWITCH    0x9D
#define SX126X_CMD_SET_DIO3_AS_TCXO_CTRL    0x97

/* RF, Modulation and Packet Commands */
#define SX126X_CMD_SET_RF_FREQUENCY         0x86
#define SX126X_CMD_SET_PACKET_TYPE          0x8A
#define SX126X_CMD_GET_PACKET_TYPE          0x11
#define SX126X_CMD_SET_TX_PARAMS            0x8E
#define SX126X_CMD_SET_MODULATION_PARAMS    0x8B
#define SX126X_CMD_SET_PACKET_PARAMS        0x8C
#define SX126X_CMD_SET_CAD_PARAMS           0x88
#define SX126X_CMD_SET_BUFFER_BASE_ADDRESS  0x8F
#define SX126X_CMD_SET_LORA_SYMB_NUM_TIMEOUT 0xA0

/* Status Commands */
#define SX126X_CMD_GET_STATUS               0xC0
#define SX126X_CMD_GET_RSSI_INST            0x15
#define SX126X_CMD_GET_RX_BUFFER_STATUS     0x13
#define SX126X_CMD_GET_PACKET_STATUS        0x14
#define SX126X_CMD_GET_DEVICE_ERRORS        0x17
#define SX126X_CMD_CLR_DEVICE_ERRORS        0x07
#define SX126X_CMD_GET_STATS                0x10
#define SX126X_CMD_RESET_STATS              0x00

/* IRQ Flags (16-bit mask) */
#define SX126X_IRQ_TX_DONE                  BIT(0)
#define SX126X_IRQ_RX_DONE                  BIT(1)
#define SX126X_IRQ_PREAMBLE_DETECTED        BIT(2)
#define SX126X_IRQ_SYNC_WORD_VALID          BIT(3)
#define SX126X_IRQ_HEADER_VALID             BIT(4)
#define SX126X_IRQ_HEADER_ERR               BIT(5)
#define SX126X_IRQ_CRC_ERR                  BIT(6)
#define SX126X_IRQ_CAD_DONE                 BIT(7)
#define SX126X_IRQ_CAD_ACTIVITY_DETECTED    BIT(8)
#define SX126X_IRQ_RX_TX_TIMEOUT            BIT(9)

#define SX126X_IRQ_ALL                      0x03FF

/* Packet Types */
#define SX126X_PACKET_TYPE_GFSK             0x00
#define SX126X_PACKET_TYPE_LORA             0x01

/* Standby Modes */
#define SX126X_STANDBY_RC                   0x00
#define SX126X_STANDBY_XOSC                 0x01

/* Regulator Modes */
#define SX126X_REGULATOR_LDO                0x00
#define SX126X_REGULATOR_DCDC               0x01

/* Sleep Configuration */
#define SX126X_SLEEP_COLD_START             0x00
#define SX126X_SLEEP_WARM_START             0x04

/* Calibration Mask */
#define SX126X_CALIBRATE_RC64K              BIT(0)
#define SX126X_CALIBRATE_RC13M              BIT(1)
#define SX126X_CALIBRATE_PLL                BIT(2)
#define SX126X_CALIBRATE_ADC_PULSE          BIT(3)
#define SX126X_CALIBRATE_ADC_BULK_N         BIT(4)
#define SX126X_CALIBRATE_ADC_BULK_P         BIT(5)
#define SX126X_CALIBRATE_IMAGE              BIT(6)
#define SX126X_CALIBRATE_ALL                0x7F

/* LoRa Bandwidth values (for SET_MODULATION_PARAMS) */
#define SX126X_LORA_BW_7_8                  0x00
#define SX126X_LORA_BW_10_4                 0x08
#define SX126X_LORA_BW_15_6                 0x01
#define SX126X_LORA_BW_20_8                 0x09
#define SX126X_LORA_BW_31_25                0x02
#define SX126X_LORA_BW_41_7                 0x0A
#define SX126X_LORA_BW_62_5                 0x03
#define SX126X_LORA_BW_125                  0x04
#define SX126X_LORA_BW_250                  0x05
#define SX126X_LORA_BW_500                  0x06

/* LoRa Coding Rate values (for SET_MODULATION_PARAMS) */
#define SX126X_LORA_CR_4_5                  0x01
#define SX126X_LORA_CR_4_6                  0x02
#define SX126X_LORA_CR_4_7                  0x03
#define SX126X_LORA_CR_4_8                  0x04

/* LoRa Header Type (for SET_PACKET_PARAMS) */
#define SX126X_LORA_HEADER_EXPLICIT         0x00
#define SX126X_LORA_HEADER_IMPLICIT         0x01

/* LoRa CRC Mode (for SET_PACKET_PARAMS) */
#define SX126X_LORA_CRC_OFF                 0x00
#define SX126X_LORA_CRC_ON                  0x01

/* LoRa IQ Mode (for SET_PACKET_PARAMS) */
#define SX126X_LORA_IQ_STANDARD             0x00
#define SX126X_LORA_IQ_INVERTED             0x01

/* TX Power Ramp Time (for SET_TX_PARAMS) */
#define SX126X_RAMP_10_US                   0x00
#define SX126X_RAMP_20_US                   0x01
#define SX126X_RAMP_40_US                   0x02
#define SX126X_RAMP_80_US                   0x03
#define SX126X_RAMP_200_US                  0x04
#define SX126X_RAMP_800_US                  0x05
#define SX126X_RAMP_1700_US                 0x06
#define SX126X_RAMP_3400_US                 0x07

/* Device Selection (for SET_PA_CONFIG) */
#define SX126X_DEVICE_SEL_SX1262            0x00
#define SX126X_DEVICE_SEL_SX1261            0x01

/* PA Configuration per chip variant (from datasheet Table 13-21) */

/* SX1261: Low power PA, up to +15 dBm */
#define SX1261_MAX_POWER                    15
#define SX1261_MAX_POWER_TX_PARAM           14
#define SX1261_MIN_POWER                    (-17)
#define SX1261_PA_DUTY_CYCLE_HIGH           0x06  /* For +15 dBm at >400 MHz */
#define SX1261_PA_DUTY_CYCLE_LOW            0x04  /* For lower power / <400 MHz */
#define SX1261_HP_MAX                       0x00  /* Not used on SX1261 */

/* SX1262: High power PA, up to +22 dBm */
#define SX1262_MAX_POWER                    22
#define SX1262_MIN_POWER                    (-9)
#define SX1262_PA_DUTY_CYCLE                0x04
#define SX1262_HP_MAX                       0x07

/* PA LUT (always 0x01) */
#define SX126X_PA_LUT                       0x01

/* Register Addresses */

/* LoRa Sync Word */
#define SX126X_REG_LORA_SYNC_WORD_MSB       0x0740
#define SX126X_REG_LORA_SYNC_WORD_LSB       0x0741

/* Sync Word Values */
#define SX126X_LORA_SYNC_WORD_PUBLIC        0x3444
#define SX126X_LORA_SYNC_WORD_PRIVATE       0x1424

/* RX Gain */
#define SX126X_REG_RX_GAIN                  0x08AC
#define SX126X_RX_GAIN_POWER_SAVING         0x94
#define SX126X_RX_GAIN_BOOSTED              0x96

/* TX Clamp Config (workaround for SX1262) */
#define SX126X_REG_TX_CLAMP_CFG             0x08D8

/* IQ Polarity (workaround) */
#define SX126X_REG_IQ_POLARITY              0x0736

/* TX Modulation (workaround for 500kHz BW) */
#define SX126X_REG_TX_MODULATION            0x0889

/* Random Number Generator */
#define SX126X_REG_RANDOM_NUMBER_GEN        0x0819

/* Status Byte Decoding */
#define SX126X_STATUS_CHIP_MODE_MASK        0xE0
#define SX126X_STATUS_CHIP_MODE_SHIFT       5
#define SX126X_STATUS_CMD_STATUS_MASK       0x1E
#define SX126X_STATUS_CMD_STATUS_SHIFT      1

/* Chip Modes */
#define SX126X_CHIP_MODE_UNUSED             0x00
#define SX126X_CHIP_MODE_RFU                0x01
#define SX126X_CHIP_MODE_STBY_RC            0x02
#define SX126X_CHIP_MODE_STBY_XOSC          0x03
#define SX126X_CHIP_MODE_FS                 0x04
#define SX126X_CHIP_MODE_RX                 0x05
#define SX126X_CHIP_MODE_TX                 0x06

/* Command Status */
#define SX126X_CMD_STATUS_RESERVED          0x00
#define SX126X_CMD_STATUS_RFU               0x01
#define SX126X_CMD_STATUS_DATA_AVAILABLE    0x02
#define SX126X_CMD_STATUS_CMD_TIMEOUT       0x03
#define SX126X_CMD_STATUS_CMD_PROC_ERROR    0x04
#define SX126X_CMD_STATUS_EXEC_FAILURE      0x05
#define SX126X_CMD_STATUS_CMD_TX_DONE       0x06

/* Timeout Special Values (in units of 15.625 us) */
#define SX126X_RX_TIMEOUT_CONTINUOUS        0x000000
#define SX126X_RX_TIMEOUT_SINGLE            0xFFFFFF
#define SX126X_TX_TIMEOUT_NONE              0x000000

/* Convert milliseconds to timeout ticks */
#define SX126X_MS_TO_TIMEOUT(ms)            ((uint32_t)((ms) * 64))

/* Crystal Frequency */
#define SX126X_XTAL_FREQ                    32000000UL

/* Frequency calculation: freq_reg = (frequency * 2^25) / XTAL_FREQ */
#define SX126X_FREQ_TO_REG(freq) \
	((uint32_t)(((uint64_t)(freq) << 25) / SX126X_XTAL_FREQ))

/* Maximum payload size */
#define SX126X_MAX_PAYLOAD_LEN              255

/* Buffer size */
#define SX126X_BUFFER_SIZE                  256

#endif /* ZEPHYR_DRIVERS_LORA_SX126X_SX126X_REGS_H_ */
