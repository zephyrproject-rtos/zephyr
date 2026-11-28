/*
 * Copyright (c) 2025 Franz Parzer <Franz.Parzer@htl-steyr.ac.at>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* AP33772S I2C USB PD3.1 EPR SINK CONTROLLER driver definitions */

#ifndef ZEPHYR_DRIVERS_SENSOR_AP33772S_H
#define ZEPHYR_DRIVERS_SENSOR_AP33772S_H

#include <zephyr/drivers/gpio.h>

#define AP33772S_MAX_SUPPORTED_VOLTAGE   28
#define AP33772S_UPDATE_PDO_TIMEOUT_MSEC 500

/* Define the maximum number of PDO entries you expect */
#define AP33772S_MAX_PDO 13

#define AP33772S_READ_BUFF_LENGTH  128
#define AP33772S_WRITE_BUFF_LENGTH 6
#define AP33772S_SRCPDO_LENGTH     28

#define AP33772S_CMD_STATUS   0x01 /* Reset to 0 after every Read */
#define AP33772S_CMD_MASK     0x02
#define AP33772S_CMD_OPMODE   0x03
#define AP33772S_CMD_CONFIG   0x04
#define AP33772S_CMD_PDCONFIG 0x05
#define AP33772S_CMD_SYSTEM   0x06
#define AP33772S_VOUTCTL_MASK (BIT(1) | BIT(0))
#define AP33772S_VOUTCTL_AUTO (0)
#define AP33772S_VOUTCTL_OFF  (BIT(0))
#define AP33772S_VOUTCTL_ON   (BIT(1))
/* Temperature setting register */
#define AP33772S_CMD_TR25     0x0C
#define AP33772S_CMD_TR50     0x0D
#define AP33772S_CMD_TR75     0x0E
#define AP33772S_CMD_TR100    0x0F
/* Power reading related */
#define AP33772S_CMD_VOLTAGE  0x11
#define AP33772S_CMD_CURRENT  0x12
#define AP33772S_CMD_TEMP     0x13
#define AP33772S_CMD_VREQ     0x14
#define AP33772S_CMD_IREQ     0x15

#define AP33772S_CMD_VSELMIN 0x16 /* Minimum Selection Voltage */
#define AP33772S_CMD_UVPTHR  0x17
#define AP33772S_CMD_OVPTHR  0x18
#define AP33772S_CMD_OCPTHR  0x19
#define AP33772S_CMD_OTPTHR  0x1A
#define AP33772S_CMD_DRTHR   0x1B

#define AP33772S_CMD_SRCPDO 0x20 /* Get All PD Source Power Capabilities */

#define AP33772S_CMD_PD_REQMSG 0x31
#define AP33772S_CMD_PD_CMDMSG 0x32
#define AP33772S_CMD_PD_MSGRLT 0x33

typedef enum {
	STARTED_BIT = 0,
	READY_BIT = 1,
	NEWPDO_BIT = 2,
	UVP_BIT = 3,
	OVP_BIT = 4,
	OCP_BIT = 5,
	OTP_BIT = 6,
} AP33772S_BIT;

typedef struct {
	union {
		struct {
			uint16_t vol_max: 8;      /* Bits 7:0, VOLTAGE_MAX field */
			uint16_t peak_current: 2; /* Bits 9:8, PEAK_CURRENT field */
			uint16_t cur_max: 4;      /* Bits 13:10, CURRENT_MAX field */
			uint16_t type: 1;         /* Bit 14, TYPE field */
			uint16_t detect: 1;       /* Bit 15, DETECT field */
		} fixed;
		struct {
			uint16_t vol_max: 8;     /* Bits 7:0, VOLTAGE_MAX field */
			uint16_t voltage_min: 2; /* Bits 9:8, VOLTAGE_MIN field */
			uint16_t cur_max: 4;     /* Bits 13:10, CURRENT_MAX field */
			uint16_t type: 1;        /* Bit 14, TYPE field */
			uint16_t detect: 1;      /* Bit 15, DETECT field */
		} pps;
		struct {
			uint16_t vol_max: 8;     /* Bits 7:0, VOLTAGE_MAX field */
			uint16_t voltage_min: 2; /* Bits 9:8, VOLTAGE_MIN field */
			uint16_t cur_max: 4;     /* Bits 13:10, CURRENT_MAX field */
			uint16_t type: 1;        /* Bit 14, TYPE field */
			uint16_t detect: 1;      /* Bit 15, DETECT field */
		} avs;
		struct {
			uint8_t byte0;
			uint8_t byte1;
		};
	};
} ap33772s_pdo_fld;

typedef struct {
	union {
		struct {
			uint16_t voltage_sel: 8; /* Bits 7:0, Output Voltage Select */
			uint16_t current_sel: 4; /* Bits 11:8, Operating Current Select */
			uint16_t pdo_index: 4;   /* Bits 15:12, Source PDO index select */
		} req_fld;
		struct {
			uint8_t byte0;
			uint8_t byte1;
		};
	};
} ap33772s_rdo_dat;

struct ap33772s_config {
	struct i2c_dt_spec i2c;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	struct gpio_dt_spec *int_gpio;
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios) */
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(flip_gpios)
	struct gpio_dt_spec *flip_gpio;
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(flip_gpios) */
	const uint16_t epr_delay_ms;
	const uint16_t poll_interval_ms;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(tr25)
	const uint16_t tr25;
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(tr25) */
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(tr50)
	const uint16_t tr50;
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(tr50) */
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(tr75)
	const uint16_t tr75;
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(tr75) */
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(tr100)
	const uint16_t tr100;
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(tr100) */
};

struct ap33772s_data {
	struct gpio_callback int_gpio_cb;
	struct k_work epr_work;
	struct k_work poll_work;
	const struct device *dev;
	const struct sensor_trigger *data_ready_trigger;
	sensor_trigger_handler_t data_ready_handler;
	struct k_timer poll_timer;
	struct k_timer epr_timer;
	ap33772s_pdo_fld pdo_arr[AP33772S_MAX_PDO];
	struct sensor_value vout_voltage;
	struct sensor_value vout_current;
	uint32_t temperature; /* Celsius */
	ap33772s_rdo_dat selected_pd_reqmsg;
	struct sensor_value sel_vol;
	struct sensor_value sel_cur;
	struct k_sem update_pdo_sem;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_AP33772S_H */
