/*
 * Copyright (c) 2020, Seagate Technology LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_I2C_I2C_LPC11U6X_H_
#define ZEPHYR_DRIVERS_I2C_I2C_LPC11U6X_H_

#include <zephyr/drivers/pinctrl.h>

#define PINCTRL_STATE_FAST_PLUS PINCTRL_STATE_PRIV_START

#define LPC11U6X_I2C_CONTROL_AA           (1 << 2)
#define LPC11U6X_I2C_CONTROL_SI           (1 << 3)
#define LPC11U6X_I2C_CONTROL_STOP         (1 << 4)
#define LPC11U6X_I2C_CONTROL_START        (1 << 5)
#define LPC11U6X_I2C_CONTROL_I2C_EN       (1 << 6)

/* I2C controller states */
#define LPC11U6X_I2C_MASTER_TX_START               0x08
#define LPC11U6X_I2C_MASTER_TX_RESTART             0x10
#define LPC11U6X_I2C_MASTER_TX_ADR_ACK             0x18
#define LPC11U6X_I2C_MASTER_TX_ADR_NACK            0x20
#define LPC11U6X_I2C_MASTER_TX_DAT_ACK             0x28
#define LPC11U6X_I2C_MASTER_TX_DAT_NACK            0x30
#define LPC11U6X_I2C_MASTER_TX_ARB_LOST            0x38

#define LPC11U6X_I2C_MASTER_RX_ADR_ACK             0x40
#define LPC11U6X_I2C_MASTER_RX_ADR_NACK            0x48
#define LPC11U6X_I2C_MASTER_RX_DAT_ACK             0x50
#define LPC11U6X_I2C_MASTER_RX_DAT_NACK            0x58

#define LPC11U6X_I2C_SLAVE_RX_ADR_ACK              0x60
#define LPC11U6X_I2C_SLAVE_RX_ARB_LOST_ADR_ACK     0x68
#define LPC11U6X_I2C_SLAVE_RX_GC_ACK               0x70
#define LPC11U6X_I2C_SLAVE_RX_ARB_LOST_GC_ACK      0x78
#define LPC11U6X_I2C_SLAVE_RX_DAT_ACK              0x80
#define LPC11U6X_I2C_SLAVE_RX_DAT_NACK             0x88
#define LPC11U6X_I2C_SLAVE_RX_GC_DAT_ACK           0x90
#define LPC11U6X_I2C_SLAVE_RX_GC_DAT_NACK          0x98
#define LPC11U6X_I2C_SLAVE_RX_STOP                 0xA0

#define LPC11U6X_I2C_SLAVE_TX_ADR_ACK              0xA8
#define LPC11U6X_I2C_SLAVE_TX_ARB_LOST_ADR_ACK     0xB0
#define LPC11U6X_I2C_SLAVE_TX_DAT_ACK              0xB8
#define LPC11U6X_I2C_SLAVE_TX_DAT_NACK             0xC0
#define LPC11U6X_I2C_SLAVE_TX_LAST_BYTE            0xC8

/* Transfer Status */
#define LPC11U6X_I2C_STATUS_BUSY                   0x01
#define LPC11U6X_I2C_STATUS_OK                     0x02
#define LPC11U6X_I2C_STATUS_FAIL                   0x03
#define LPC11U6X_I2C_STATUS_INACTIVE               0x04

struct lpc11u6x_i2c_regs {
	volatile uint32_t con_set;           /* Control set */
	volatile const uint32_t stat;        /* Status */
	volatile uint32_t dat;               /* Data */
	volatile uint32_t addr0;             /* Slave address 0 */
	volatile uint32_t sclh;              /* SCL Duty Cycle */
	volatile uint32_t scll;              /* SCL Duty Cycle */
	volatile uint32_t con_clr;           /* Control clear */
	volatile uint32_t mm_ctrl;           /* Monitor mode control */
	volatile uint32_t addr[3];           /* Slave address {1,2,3} */
	volatile const uint32_t data_buffer; /* Data buffer */
	volatile uint32_t mask[4];           /* Slave address mask */
};

struct lpc11u6x_i2c_config {
	struct lpc11u6x_i2c_regs *base;
	const struct device *clock_dev;
	void (*irq_config_func)(const struct device *dev);
	uint32_t clkid;
	const struct pinctrl_dev_config *pincfg;
};

struct lpc11u6x_i2c_current_transfer {
	struct i2c_msg *msgs;
	uint8_t *curr_buf;
	uint8_t curr_len;
	uint8_t nr_msgs;
	uint8_t addr;
	uint8_t status;
};

struct lpc11u6x_i2c_data {
	struct lpc11u6x_i2c_current_transfer transfer;
	struct i2c_target_config *slave;
	struct k_sem completion;
	struct k_mutex mutex;
};

#endif /* ZEPHYR_DRIVERS_I2C_I2C_LPC11U6X_H_ */
