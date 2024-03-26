/*
 * Copyright (c) 2022 Google LLC.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef I3C_RENESAS_IMX3112_REGS_H_
#define I3C_RENESAS_IMX3112_REGS_H_

#include <stdint.h>
#include <zephyr/toolchain.h>

/* ---------------- */
/* Register Fields. */
/* ---------------- */

#define RF_DEVICE_TYPE GENMASK(7, 0)

#define RF_VENDOR_ID GENMASK(7, 0)

/* Local Interface - IO voltage */
#define RF_LOCAL_INF_IO_LEVEL GENMASK(4, 2)
/* Local Interface - Pull Up Resistor Configuration */
#define RF_LOCAL_INF_PULLUP_CONF BIT(5)

/* LSCL_0 Pull-up Resistor Select */
#define RF_LSCL_0_PU_RES GENMASK(1, 0)
/* LSDA_0 Pull-up Resistor Select */
#define RF_LSDA_0_PU_RES GENMASK(3, 2)
/* LSCL_1 Pull-up Resistor Select */
#define RF_LSCL_1_PU_RES GENMASK(5, 4)
/* LSDA_1 Pull-up Resistor Select */
#define RF_LSDA_1_PU_RES GENMASK(7, 6)

/* Burst length for read pointer address for PEC calculations */
#define RF_DEF_READ_ADDRESS_POINT_BL BIT(1)
/* Default read pointer starting address */
#define RF_DEF_RD_ADDR_POINT_START GENMASK(3, 2)
/* Default read address pointer enable */
#define RF_DEF_RD_ADDR_POINT_EN BIT(4)
/* Interface Selection */
#define RF_INF_SEL BIT(5)
/* Parity (T bit) Disable */
#define RF_PAR_DIS BIT(6)
/* PEC enable */
#define RF_PEC_EN BIT(7)

/* LSDA_0 IO Port 0 Enable */
#define RF_SCL_SDA_0_EN BIT(6)
/* LSDA_1 IO Port 1 Enable */
#define RF_SCL_SDA_1_EN BIT(7)

/* Write/Read Operation Port 0 Selection */
#define RF_SCL_SDA_0_SEL BIT(6)
/* Write/Read Operation Port 1 Selection */
#define RF_SCL_SDA_1_SEL BIT(7)

/* ------------------------------ */
/* Enums used in register fields. */
/* ------------------------------ */

#define RE_INTERNAL_PULLUP_RESISTOR 0
#define RE_EXTERNAL_PULLUP_RESISTOR 1

#define RE_INF_SEL_I2C 0
#define RE_INF_SEL_I3C 1

/* ----------------- */
/* Register structs. */
/* ----------------- */

struct i3c_renesas_imx3112_registers {
	/* 0x0: (MR0/1) The device type harcoded by renesas */
	uint8_t device_type[2];
	/* 0x2: (MR2) Split between major and minor */
	uint8_t device_revision;
	/* 0x3: (MR3/4) Jedec standard renesas code */
	uint8_t vendor_id[2];
	uint8_t __reserved0[9];
	/* 0xe: (MR14) Device configuration for Resistance, Voltage */
	uint8_t local_interface_cfg;
	/* 0xf: (MR15) */
	uint8_t pullup_resistor_config;
	uint8_t __reserved1[2];
	/* 0x12: (MR18) PEC/parity/protocol selection */
	uint8_t device_cfg;
	/* 0x13: (MR19) */
	uint8_t clear_temp_sensor_alarm;
	/* 0x14: (MR20) Clear PEC/parity errors */
	uint8_t clear_ecc_error;
	uint8_t __reserved2[5];
	/* 0x1a: (MR26) Enable/Disable temp sensor */
	uint8_t temp_sensor_cfg;
	/* 0x1b: (MR27) IBI configuration */
	uint8_t interrupt_cfg;
	/* 0x1c: (MR28/29) */
	uint8_t temp_hi_limit_cfg[2];
	/* 0x1e: (MR30/31) */
	uint8_t temp_lo_limit_cfg[2];
	/* 0x20: (MR32/33) Must be > temp_hi_limit_cfg */
	uint8_t temp_crit_hi_limit_cfg[2];
	/* 0x22: (MR34/35) Must be < temp_lo_limit_cfg */
	uint8_t temp_crit_lo_limit_cfg[2];
	uint8_t __reserved3[12];
	/* 0x30: (MR48) IBI status */
	uint8_t device_status;
	/* 0x31: (MR49/50) (oC) */
	uint8_t current_temperature[2];
	/* 0x33: (MR51) Alarms for temperature thresholds */
	uint8_t temperature_status;
	/* 0x34: (MR52) PEC/Parity errors */
	uint8_t error_status;
	uint8_t __reserved4[11];
	/* 0x40: (MR64) Enable/Disable SCL/SDA lines for each port */
	uint8_t mux_config;
	/* 0x41: (MR65) Select a port for passthrough */
	uint8_t mux_select;
	uint8_t __reserved5[62];
};
BUILD_ASSERT(sizeof(struct i3c_renesas_imx3112_registers) == 128);

#endif /* I3C_RENESAS_IMX3112_REGS_H_ */
