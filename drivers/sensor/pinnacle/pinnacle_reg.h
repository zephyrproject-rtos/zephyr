/*
 * Copyright (c) 2024 Ilia Kharin
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_PINNACLE_PINNACLE_REG_H_
#define ZEPHYR_DRIVERS_SENSOR_PINNACLE_PINNACLE_REG_H_

#include <zephyr/sys/util.h>

/*
 * Register Access Protocol Standard Registers.
 * Standard registers have 5-bit addresses, BIT[4:0], that range from
 * 0x00 to 0x1F. For reading, a register address has to be combined with
 * 0xA0 for reading and 0x80 for writing bits, BIT[7:5].
 */
#define PINNACLE_REG_FIRMWARE_ID	(0x00)  /* R */
#define PINNACLE_REG_FIRMWARE_VERSION	(0x01)	/* R */
#define PINNACLE_REG_STATUS1		(0x02)	/* R/W */
#define PINNACLE_REG_SYS_CONFIG1	(0x03)	/* R/W */
#define PINNACLE_REG_FEED_CONFIG1	(0x04)	/* R/W */
#define PINNACLE_REG_FEED_CONFIG2	(0x05)	/* R/W */
#define PINNACLE_REG_FEED_CONFIG3	(0x06)	/* R/W */
#define PINNACLE_REG_CAL_CONFIG1	(0x07)	/* R/W */
#define PINNACLE_REG_PS2_AUX_CONTROL	(0x08)	/* R/W */
#define PINNACLE_REG_SAMPLE_RATE	(0x09)	/* R/W */
#define PINNACLE_REG_Z_IDLE		(0x0A)	/* R/W */
#define PINNACLE_REG_Z_SCALER		(0x0B)	/* R/W */
#define PINNACLE_REG_SLEEP_INTERVAL	(0x0C)	/* R/W */
#define PINNACLE_REG_SLEEP_TIMER	(0x0D)	/* R/W */
#define PINNACLE_REG_EMI_THRESHOLD	(0x0E)	/* R/W */
#define PINNACLE_REG_PACKET_BYTE0	(0x12)	/* R */
#define PINNACLE_REG_PACKET_BYTE1	(0x13)	/* R */
#define PINNACLE_REG_PACKET_BYTE2	(0x14)	/* R */
#define PINNACLE_REG_PACKET_BYTE3	(0x15)	/* R */
#define PINNACLE_REG_PACKET_BYTE4	(0x16)	/* R */
#define PINNACLE_REG_PACKET_BYTE5	(0x17)	/* R */
#define PINNACLE_REG_GPIO_A_CTRL	(0x18)	/* R/W */
#define PINNACLE_REG_GPIO_A_DATA	(0x19)	/* R/W */
#define PINNACLE_REG_GPIO_B_CTRL_DATA	(0x1A)	/* R/W */
#define PINNACLE_REG_ERA_VALUE		(0x1B)	/* R/W */
#define PINNACLE_REG_ERA_ADDR_HIGH	(0x1C)	/* R/W */
#define PINNACLE_REG_ERA_ADDR_LOW	(0x1D)	/* R/W */
#define PINNACLE_REG_ERA_ADDR_CTRL	(0x1E)	/* R/W */
#define PINNACLE_REG_PRODUCT_ID		(0x1F)	/* R */

/* Firmware ASIC ID value */
#define PINNACLE_FIRMWARE_ID		(0x07)

/* Status1 definition */
#define PINNACLE_STATUS1_SW_DR			BIT(2)
#define PINNACLE_STATUS1_SW_CC			BIT(3)

/* SysConfig1 definition */
#define PINNACLE_SYS_CONFIG1_RESET		BIT(0)
#define PINNACLE_SYS_CONFIG1_SHUTDOWN		BIT(1)
#define PINNACLE_SYS_CONFIG1_LOW_POWER_MODE	BIT(2)

/* FeedConfig1 definition */
#define PINNACLE_FEED_CONFIG1_FEED_EN		BIT(0)

/* FeedConfig2 definition */
#define PINNACLE_FEED_CONFIG2_INTELLIMOUSE_EN	BIT(0)

/* Special definitions */
#define PINNACLE_FB	(0xFB) /* Filler byte */
#define PINNACLE_FC	(0xFC) /* Auto-increment byte */

/* Read and write masks */
#define PINNACLE_READ_MSK	(0xA0)
#define PINNACLE_WRITE_MSK	(0x80)

/* Read and write register addresses */
#define PINNACLE_READ_REG(addr) (PINNACLE_READ_MSK|addr)
#define PINNACLE_WRITE_REG(addr) (PINNACLE_WRITE_MSK|addr)

#endif /* ZEPHYR_DRIVERS_SENSOR_PINNACLE_PINNACLE_REG_H_ */
