/*
 * Copyright(c) 2017, Analogix Semiconductor. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __ANX7533_CHICAGO_CONFIG_H__
#define __ANX7533_CHICAGO_CONFIG_H__

#define I2C_ADDR_SEL	0

#define SIGNAL_HIGH	1
#define SIGNAL_LOW	0

#define VALUE_ON	1
#define VALUE_OFF	0

#define VALUE_SUCCESS	0
#define VALUE_FAILURE	-1
#define VALUE_FAILURE2	-2
#define VALUE_FAILURE3	-3
#define VALUE_FAILURE4	-4
#define VALUE_FAILURE5	-5

#define _BIT0	0x01
#define _BIT1	0x02
#define _BIT2	0x04
#define _BIT3	0x08
#define _BIT4	0x10
#define _BIT5	0x20
#define _BIT6	0x40
#define _BIT7	0x80

#define  MAX_BYTE_COUNT_PER_RECORD_EEPROM   16
#define  MAX_BYTE_COUNT_PER_RECORD_FLASH    16

#define ANX7533_IRQ_QUEUE_SIZE		10

#define VIDEO_STABLE_DELAY_TIME 100

/******************************************************************************
Description: State mechanism emun
******************************************************************************/
enum anx7533_state {
	ANX7533_STATE_NONE,
	ANX7533_STATE_WAITCABLE,
	ANX7533_STATE_WAITOCM,
	ANX7533_STATE_NORMAL,
	ANX7533_STATE_DEBUG
};

struct anx7533_irq_queue {
	uint8_t q0[ANX7533_IRQ_QUEUE_SIZE];
	uint8_t q1[ANX7533_IRQ_QUEUE_SIZE];
	uint8_t irq_q_input;
	uint8_t irq_q_output;
};

#endif  /* _ANX7533_CHICAGO_CONFIG_H_ */

