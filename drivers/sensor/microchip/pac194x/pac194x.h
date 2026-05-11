/*
 * Copyright (c) 2026 Google, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PAC194X_H
#define PAC194X_H

#define PAC194X_MAX_CHANNELS			4

/* --- Special Commands (Register addresses with no data) --- */
/** REFRESH command: Updates readable registers and resets accumulators */
#define PAC194X_REG_REFRESH			0x00
/** REFRESH_V command: Updates readable registers without resetting accumulators */
#define PAC194X_REG_REFRESH_V			0x1F
/** REFRESH_G command: Global Refresh for multiple devices on the bus */
#define PAC194X_REG_REFRESH_G			0x1E

/* --- Configuration and Status Registers --- */
/** CTRL Register: Sample rate and channel configuration (Default: 0700h) */
#define PAC194X_REG_CTRL			0x01
#define PAC194X_CTRL_SAMPLE_MODE_POS		  12
#define PAC194X_CTRL_SAMPLE_MODE_MASK		  (0xF << PAC194X_CTRL_SAMPLE_MODE_POS)
#define PAC194X_CTRL_SAMPLE_1024_ADAPTIVE	  0x0
#define PAC194X_CTRL_SAMPLE_256_ADAPTIVE	  0x1
#define PAC194X_CTRL_SAMPLE_64_ADAPTIVE		  0x2
#define PAC194X_CTRL_SAMPLE_8_ADAPTIVE		  0x3
#define PAC194X_CTRL_SAMPLE_1024		  0x4
#define PAC194X_CTRL_SAMPLE_256			  0x5
#define PAC194X_CTRL_SAMPLE_64			  0x6
#define PAC194X_CTRL_SAMPLE_8			  0x7
#define PAC194X_CTRL_SAMPLE_SINGLE_SHOT		  0x8
#define PAC194X_CTRL_SAMPLE_SINGLE_8X		  0x9
#define PAC194X_CTRL_SAMPLE_FAST		  0xA
#define PAC194X_CTRL_SAMPLE_BURST		  0xB
#define PAC194X_CTRL_SAMPLE_SLEEP		  0xF
#define PAC194X_CTRL_CHANNEL_N_OFF_POS		  4
#define PAC194X_CTRL_CHANNEL_N_OFF_MASK		  (0xF << PAC194X_CTRL_CHANNEL_N_OFF_POS)
#define PAC194X_CTRL_CHANNEL_N_OFF(n)		  \
	(((1 << (3-(n))) << PAC194X_CTRL_CHANNEL_N_OFF_POS) & PAC194X_CTRL_CHANNEL_N_OFF_MASK)

/** Accumulation Count: Number of items accumulated (4 bytes) */
#define PAC194X_REG_ACC_COUNT			0x02

/* --- Result Registers (Require Block Read) --- */
/* Accumulators (VACCn) - 7 bytes each */
#define PAC194X_REG_VACC1			0x03
#define PAC194X_REG_VACC2			0x04
#define PAC194X_REG_VACC3			0x05
#define PAC194X_REG_VACC4			0x06

/* Bus Voltage (VBUSn) - 2 bytes */
#define PAC194X_REG_VBUS1			0x07
#define PAC194X_REG_VBUS2			0x08
#define PAC194X_REG_VBUS3			0x09
#define PAC194X_REG_VBUS4			0x0A

/* Sense Resistor Voltage (VSENSEn) - 2 bytes */
#define PAC194X_REG_VSENSE1			0x0B
#define PAC194X_REG_VSENSE2			0x0C
#define PAC194X_REG_VSENSE3			0x0D
#define PAC194X_REG_VSENSE4			0x0E

/* Rolling Averages (8 most recent samples) */
#define PAC194X_REG_VBUS1_AVG			0x0F
#define PAC194X_REG_VBUS2_AVG			0x10
#define PAC194X_REG_VBUS3_AVG			0x11
#define PAC194X_REG_VBUS4_AVG			0x12

#define PAC194X_REG_VSENSE1_AVG			0x13
#define PAC194X_REG_VSENSE2_AVG			0x14
#define PAC194X_REG_VSENSE3_AVG			0x15
#define PAC194X_REG_VSENSE4_AVG			0x16

/* --- Advanced Configuration --- */
#define PAC194X_REG_SMBUS_SETTINGS		0x1C
#define PAC194X_REG_NEG_PWR_FSR			0x1D  /* Bidirectional Range (FSR) Configuration */
#define PAC194X_NEG_PWR_FSR_UNIPOLAR_FSR	  0
#define PAC194X_NEG_PWR_FSR_BIPOLAR_FSR		  1
#define PAC194X_NEG_PWR_FSR_BIPOLAR_HALF_FSR	  2
#define PAC194X_REG_SLOW			0x20  /* SLOW pin configuration */
#define PAC194X_REG_ACCUM_CONFIG		0x25  /* Accumulation mode (Power/V/I) */
#define PAC194X_ACCUM_CONFIG_VPOWER		  0
#define PAC194X_ACCUM_CONFIG_VSENSE		  1
#define PAC194X_ACCUM_CONFIG_VBUS		  2
#define PAC194X_REG_ALERT_STATUS		0x26  /* ALERT flags status */

/* --- ALERT Limits (OC/UC/OV/UV) --- */
#define PAC194X_REG_OC_LIMIT1			0x30
#define PAC194X_REG_UC_LIMIT1			0x34
#define PAC194X_REG_OV_LIMIT1			0x38
#define PAC194X_REG_UV_LIMIT1			0x3C

/* --- Device Identification --- */
#define PAC194X_REG_PRODUCT_ID			0xFD
#define PAC194X_PRODUCT_ID_1941			  0x68
#define PAC194X_PRODUCT_ID_1942			  0x69
#define PAC194X_PRODUCT_ID_1943			  0x6A
#define PAC194X_PRODUCT_ID_1944			  0x6B
#define PAC194X_PRODUCT_ID_1941_2		  0x6C
#define PAC194X_PRODUCT_ID_1942_2		  0x6D
#define PAC194X_PRODUCT_ID_1951			  0x78
#define PAC194X_PRODUCT_ID_1952			  0x79
#define PAC194X_PRODUCT_ID_1953			  0x7A
#define PAC194X_PRODUCT_ID_1954			  0x7B
#define PAC194X_PRODUCT_ID_1951_2		  0x7C
#define PAC194X_PRODUCT_ID_1952_2		  0x7D
#define PAC194X_REG_MANUF_ID			0xFE
#define PAC194X_REG_MANUF_ID_MICROCHIP		0x54
#define PAC194X_REG_REVISION_ID			0xFF

/** Default Microchip Manufacturer ID */
#define PAC194X_POR_MANUF_ID			0x54

#endif /* PAC194X_H */
