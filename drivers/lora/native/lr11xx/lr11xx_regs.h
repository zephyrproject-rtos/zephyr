/*
 * Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_LORA_LR11XX_LR11XX_REGS_H_
#define ZEPHYR_DRIVERS_LORA_LR11XX_LR11XX_REGS_H_

#include <stdint.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/minmax.h>

/* SPI Command Opcodes
 * On Lora gen3+, they are dual byte.
 */

/* Produce Opcode sequence for use in SPI array */
#define LR11XX_OPCODE(_op)			((_op) >> 8), ((_op) & 0xff)

/* Register and Memory Access Commands */
#define LR11XX_CMD_WRITE_REGMEM			0x0105
#define LR11XX_CMD_READ_REGMEM			0x0106
#define LR11XX_CMD_WRITE_BUFFER			0x0109
#define LR11XX_CMD_READ_BUFFER			0x010a
#define LR11XX_CMD_CLEAR_BUFFER			0x010b
#define LR11XX_CMD_RMW_REGMEM			0x010c

/* System and Status Commands */
#define LR11XX_CMD_GET_STATUS			0x0100
#define LR11XX_CMD_GET_VERSION			0x0101
#define LR11XX_CMD_GET_ERRORS			0x010d
#define LR11XX_CMD_CLEAR_ERRORS			0x010e
#define LR11XX_CMD_CALIBRATE			0x010f
#define LR11XX_CMD_SET_REGULATOR_MODE		0x0110
#define LR11XX_CMD_CALIBRATE_IMAGE		0x0111
#define LR11XX_CMD_SET_DIO_AS_RF_SWITCH		0x0112
#define LR11XX_CMD_SET_DIO_IRQ_PARAMS		0x0113
#define LR11XX_CMD_CLEAR_IRQ			0x0114
#define LR11XX_CMD_CONFIG_LF_CLOCK		0x0116
#define LR11XX_CMD_SET_TCXO_MODE		0x0117
#define LR11XX_CMD_REBOOT			0x0118
#define LR11XX_CMD_GET_VBAT			0x0119
#define LR11XX_CMD_GET_TEMP			0x011a
#define LR11XX_CMD_SET_SLEEP			0x011b
#define LR11XX_CMD_SET_STANDBY			0x011c
#define LR11XX_CMD_SET_FS			0x011d
#define LR11XX_CMD_GET_RANDOM_NUMBER		0x0120
#define LR11XX_CMD_ERASE_INFO_PAGE		0x0121
#define LR11XX_CMD_WRITE_INFO_PAGE		0x0122
#define LR11XX_CMD_READ_INFO_PAGE		0x0123
#define LR11XX_CMD_GET_CHIP_EUI			0x0125
#define LR11XX_CMD_ENABLE_SPI_CRC		0x0128
#define LR11XX_CMD_DRIVE_DIO_SLEEP_MODE		0x012a

/* Radio Commands */
#define LR11XX_CMD_RESET_STATS			0x0200
#define LR11XX_CMD_GET_STATS			0x0201
#define LR11XX_CMD_GET_PACKET_TYPE		0x0202
#define LR11XX_CMD_GET_RX_BUFFER_STATUS		0x0203
#define LR11XX_CMD_GET_PACKET_STATUS		0x0204
#define LR11XX_CMD_GET_RSSI_INST		0x0205
#define LR11XX_CMD_SET_GFSK_SYNCWORD		0x0206
#define LR11XX_CMD_SET_LORA_PUBLIC_NETWORK	0x0208
#define LR11XX_CMD_SET_RX			0x0209
#define LR11XX_CMD_SET_TX			0x020a
#define LR11XX_CMD_SET_RF_FREQUENCY		0x020b
#define LR11XX_CMD_AUTO_TX_RX			0x020c
#define LR11XX_CMD_SET_CAD_PARAMS		0x020d
#define LR11XX_CMD_SET_PACKET_TYPE		0x020e
#define LR11XX_CMD_SET_MODULATION_PARAMS	0x020f
#define LR11XX_CMD_SET_PACKET_PARAMS		0x0210
#define LR11XX_CMD_SET_TX_PARAMS		0x0211
#define LR11XX_CMD_SET_PACKET_ADDR		0x0212
#define LR11XX_CMD_SET_RX_TX_FALLBACK_MODE	0x0213
#define LR11XX_CMD_SET_RX_DUTY_CYCLE		0x0214
#define LR11XX_CMD_SET_PA_CONFIG		0x0215
#define LR11XX_CMD_STOP_TIMEOUT_ON_PREAMBLE	0x0217
#define LR11XX_CMD_SET_CAD			0x0218
#define LR11XX_CMD_SET_TX_CONTINUOUS_WAVE	0x0219
#define LR11XX_CMD_SET_TX_INFINITE_PREAMBLE	0x021a
#define LR11XX_CMD_SET_LORA_SYNCH_TIMEOUT	0x021b
#define LR11XX_CMD_SET_GFSK_CRC_PARAMS		0x0224
#define LR11XX_CMD_SET_GFSK_WHIT_PARAMS		0x0225
#define LR11XX_CMD_SET_RX_BOOSTED		0x0227
#define LR11XX_CMD_SET_RSSI_CALIBRATION		0x0229
#define LR11XX_CMD_SET_LORA_SYNCWORD		0x022b
#define LR11XX_CMD_LRFHSS_BUILD_FRAME		0x022c
#define LR11XX_CMD_SET_LRFHSS_SYNCWORD		0x022d
#define LR11XX_CMD_GET_LORA_RX_HEADER_INFOS	0x0230
#define LR11XX_CMD_RADIO_APPLY_PATCH		0x0234

/* IRQ Flags (32-bit mask) */
#define LR11XX_IRQ_TX_DONE			BIT(2)
#define LR11XX_IRQ_RX_DONE			BIT(3)
#define LR11XX_IRQ_PREAMBLE_DETECTED		BIT(4)
#define LR11XX_IRQ_SYNC_WORD_VALID		BIT(5)
#define LR11XX_IRQ_HEADER_ERR			BIT(6)
#define LR11XX_IRQ_CRC_ERR			BIT(7)
#define LR11XX_IRQ_CAD_DONE			BIT(8)
#define LR11XX_IRQ_CAD_ACTIVITY_DETECTED	BIT(9)
#define LR11XX_IRQ_RX_TX_TIMEOUT		BIT(10)
#define LR11XX_IRQ_LRFHSS_HOP			BIT(11)
#define LR11XX_IRQ_LOW_BATTERY			BIT(21)
#define LR11XX_IRQ_CMD_ERR			BIT(22)
#define LR11XX_IRQ_ERR				BIT(23)
#define LR11XX_IRQ_FSK_LEN_ERR			BIT(24)
#define LR11XX_IRQ_FSK_ADDR_ERR			BIT(25)
#define LR11XX_IRQ_LORA_RX_TIMESTAMP		BIT(27)

#define LR11XX_IRQ_ALL				0xbe00ffc

/* Packet Types (SetPacketType, GetPacketType) */
#define LR11XX_PACKET_TYPE_NONE			0x00
#define LR11XX_PACKET_TYPE_GFSK			0x01
#define LR11XX_PACKET_TYPE_LORA			0x02
#define LR11XX_PACKET_TYPE_SIGFOX		0x03
#define LR11XX_PACKET_TYPE_LRFHSS		0x04

/* Standby Modes (SetStandby) */
#define LR11XX_STANDBY_RC			0x00
#define LR11XX_STANDBY_XOSC			0x01

/* Regulator Modes (SetRegMode) */
#define LR11XX_REGULATOR_LDO			0x00
#define LR11XX_REGULATOR_DCDC			0x01

/* Sleep Configuration */
#define LR11XX_SLEEP_COLD_START			0x00
#define LR11XX_SLEEP_WARM_START			0x01
#define LR11XX_SLEEP_RTC_WAKE			0x02

/* Calibration Mask (Calibrate) */
#define LR11XX_CALIBRATE_LF_RC			BIT(0)
#define LR11XX_CALIBRATE_HF_RC			BIT(1)
#define LR11XX_CALIBRATE_PLL			BIT(2)
#define LR11XX_CALIBRATE_ADC			BIT(3)
#define LR11XX_CALIBRATE_IMAGE			BIT(4)
#define LR11XX_CALIBRATE_PLL_TX			BIT(5)
#define LR11XX_CALIBRATE_ALL			\
	(LR11XX_CALIBRATE_LF_RC | LR11XX_CALIBRATE_HF_RC | LR11XX_CALIBRATE_PLL \
	 | LR11XX_CALIBRATE_ADC | LR11XX_CALIBRATE_IMAGE | LR11XX_CALIBRATE_PLL_TX)

/* LoRa sub-GHZ Bandwidth values (SetModulationParams) */
#define LR11XX_LORA_BW_10_4			0x08
#define LR11XX_LORA_BW_15_6			0x01
#define LR11XX_LORA_BW_20_8			0x09
#define LR11XX_LORA_BW_31_25			0x02
#define LR11XX_LORA_BW_41_7			0x0a
#define LR11XX_LORA_BW_62_5			0x03
#define LR11XX_LORA_BW_125			0x04
#define LR11XX_LORA_BW_250			0x05
#define LR11XX_LORA_BW_500			0x06

/* LoRa 2.4G Bandwidth values (SetModulationParams) */
#define LR11XX_LORA_BW_200			0x0d
#define LR11XX_LORA_BW_400			0x0e
#define LR11XX_LORA_BW_800			0x0f

/* LoRa Coding Rate values (SetModulationParams) */
#define LR11XX_LORA_CR_4_5			0x01
#define LR11XX_LORA_CR_4_6			0x02
#define LR11XX_LORA_CR_4_7			0x03
#define LR11XX_LORA_CR_4_8			0x04
/* Long Interleaver */
#define LR11XX_LORA_CR_4_5_2			0x05
#define LR11XX_LORA_CR_4_6_2			0x06
#define LR11XX_LORA_CR_4_8_2			0x07

/* LoRa Header Type (SetPacketParams) */
#define LR11XX_LORA_HEADER_EXPLICIT		0x00
#define LR11XX_LORA_HEADER_IMPLICIT		0x01

/* LoRa CRC Mode (SetPacketParams) */
#define LR11XX_LORA_CRC_OFF			0x00
#define LR11XX_LORA_CRC_ON			0x01

/* LoRa IQ Mode (SetPacketParams) */
#define LR11XX_LORA_IQ_STANDARD			0x00
#define LR11XX_LORA_IQ_INVERTED			0x01

/* TX Power Ramp Time (SetTxParams) */
#define LR11XX_RAMP_16_US			0x00
#define LR11XX_RAMP_32_US			0x01
#define LR11XX_RAMP_48_US			0x02
#define LR11XX_RAMP_64_US			0x03
#define LR11XX_RAMP_80_US			0x04
#define LR11XX_RAMP_96_US			0x05
#define LR11XX_RAMP_112_US			0x06
#define LR11XX_RAMP_128_US			0x07
#define LR11XX_RAMP_144_US			0x08
#define LR11XX_RAMP_160_US			0x09
#define LR11XX_RAMP_176_US			0x0a
#define LR11XX_RAMP_192_US			0x0b
#define LR11XX_RAMP_208_US			0x0c
#define LR11XX_RAMP_240_US			0x0d
#define LR11XX_RAMP_272_US			0x0e
#define LR11XX_RAMP_304_US			0x0f

/* PA Configuration per chip variant (LR1121 UM v2.2) */

#define LR1121_MAX_POWER_LF			22
#define LR1121_MIN_POWER_LF			(-17)
#define LR1121_MAX_POWER_HF			13
#define LR1121_MIN_POWER_HF			(-18)
/* 14 dBm with LP PA, 22 dBM with HP PA */
#define LR1121_PA_DUTY_CYCLE_LF			0x04
/* 13 dBm with HF PA */
#define LR1121_PA_DUTY_CYCLE_HF			0x00
/* Threshold to switch between LP and HP Low-Frequency PA */
#define LR1121_LF_PA_THRESHOLD			14

/* Sync Word Values */
#define LR11XX_LORA_SYNC_WORD_PUBLIC		0x34
#define LR11XX_LORA_SYNC_WORD_PRIVATE		0x12

/* Status Stat1 (MISO byte 0 on all commands) */
#define LR11XX_STAT1_HAS_IRQ			BIT(0)
#define LR11XX_STAT1_CMD_MSK			GENMASK(3, 1)
#define LR11XX_STAT1_CMD_OFF			1U
#define LR11XX_STAT1_CMD_FAIL			0x0
#define LR11XX_STAT1_CMD_PERR			0x1
#define LR11XX_STAT1_CMD_OK			0x2
#define LR11XX_STAT1_CMD_DAT			0x3

#define LR11XX_STAT1_CMD_GET(_v)		\
	(((_v) & LR11XX_STAT1_CMD_MSK) >> LR11XX_STAT1_CMD_OFF)

/* Status Stat2 (MISO byte 1 on all commands) */
#define LR11XX_STAT2_IS_FLASH			BIT(0)
#define LR11XX_STAT2_MODE_MSK			GENMASK(3, 1)
#define LR11XX_STAT2_MODE_OFF			1U
#define LR11XX_STAT2_MODE_SLEEP			0x0
#define LR11XX_STAT2_MODE_STANDBY_RC		0x1
#define LR11XX_STAT2_MODE_STANDBY_XOSC		0x2
#define LR11XX_STAT2_MODE_FS			0x3
#define LR11XX_STAT2_MODE_RX			0x4
#define LR11XX_STAT2_MODE_TX			0x5
#define LR11XX_STAT2_RESET_MSK			GENMASK(7, 4)
#define LR11XX_STAT2_RESET_OFF			4U
#define LR11XX_STAT2_RESET_NONE			0x0
#define LR11XX_STAT2_RESET_ANALOG		0x1
#define LR11XX_STAT2_RESET_EXTERNAL		0x2
#define LR11XX_STAT2_RESET_SYSTEM		0x3
#define LR11XX_STAT2_RESET_WATCHDOG		0x4
#define LR11XX_STAT2_RESET_WAKEUP		0x5
#define LR11XX_STAT2_RESET_RTC			0x6

#define LR11XX_STAT2_MODE_GET(_v)		\
	(((_v) & LR11XX_STAT2_MODE_MSK) >> LR11XX_STAT2_MODE_OFF)

#define LR11XX_STAT2_RESET_GET(_v)		\
	(((_v) & LR11XX_STAT2_RESET_MSK) >> LR11XX_STAT2_RESET_OFF)

#define LR11XX_PA_LP				0x0
#define LR11XX_PA_HP				0x1
#define LR11XX_PA_HF				0x2

/* LF clock frequency = 32.768KHz */
#define LR11XX_LF_FREQ				32768UL
/* HF clock frequency = 32 MHz */
#define LR11XX_XTAL_FREQ			MHZ(32UL)

/* RX until a reception occurs, then back to standby */
#define LR11XX_RX_TIMEOUT_SINGLE		0x000000
/* RX until command is received to change the mode */
#define LR11XX_RX_TIMEOUT_CONTINUOUS		0xffffff
/* No time out */
#define LR11XX_TX_TIMEOUT_NONE			0x000000

/* Convert milliseconds to timeout ticks, the maximum value is 512s (24-bit value)
 * Clamped to 0xfffffe to avoid the possibility of accidentally selecting a special timeout.
 */
#define LR11XX_MS_TO_TIMEOUT(_ms)	\
	min(z_tmcvt_32((_ms), Z_HZ_ms, LR11XX_LF_FREQ, true, true, false), 0xfffffe)

/* Maximum payload size */
#define LR11XX_MAX_PAYLOAD_LEN			255

/* Buffer size */
#define LR11XX_BUFFER_SIZE			256

/* 1 GHz */
#define LR11XX_SUBGHZ_THRESHOLD			MHZ(1000UL)

#define LR11XX_IRQ_SIZE				(sizeof(uint32_t))

#define LR11XX_NOT_API_ERR(_err)		(-(int)(0x40000000 | (_err)))

#endif /* ZEPHYR_DRIVERS_LORA_LR11XX_LR11XX_REGS_H_ */
