/*
 * Copyright (c) 2016 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32LX_I2C_H_
#define _STM32LX_I2C_H_

/* 35.6.1 Control register 1 (I2C_CR1) */
union __cr1 {
	uint32_t val;
	struct {
		uint32_t pe :1 __packed;
		uint32_t txie :1 __packed;
		uint32_t rxie :1 __packed;
		uint32_t addrie :1 __packed;
		uint32_t nackie :1 __packed;
		uint32_t stopie :1 __packed;
		uint32_t tcie :1 __packed;
		uint32_t errie :1 __packed;
		uint32_t dnf :4 __packed;
		uint32_t anfoff :1 __packed;
		uint32_t rsvd__13 :1 __packed;
		uint32_t txdmaen :1 __packed;
		uint32_t rxdmaen :1 __packed;
		uint32_t sbc :1 __packed;
		uint32_t nostretch :1 __packed;
		uint32_t wupen :1 __packed;
		uint32_t gcen :1 __packed;
		uint32_t smbhen :1 __packed;
		uint32_t smbden :1 __packed;
		uint32_t alerten :1 __packed;
		uint32_t pecen :1 __packed;
		uint32_t rsvd__24_31 :8 __packed;
	} bit;
};

/* 35.6.2 Control register 2 (I2C_CR2) */
union __cr2 {
	uint32_t val;
	struct {
		uint32_t sadd :10 __packed;
		uint32_t rd_wrn :1 __packed;
		uint32_t add10 :1 __packed;
		uint32_t head10r :1 __packed;
		uint32_t start :1 __packed;
		uint32_t stop :1 __packed;
		uint32_t nack :1 __packed;
		uint32_t nbytes :8 __packed;
		uint32_t reload :1 __packed;
		uint32_t autoend :1 __packed;
		uint32_t pecbyte :1 __packed;
		uint32_t rsvd__27_31 :5 __packed;
	} bit;
};

union __oar1 {
	uint32_t val;
	struct {
		uint32_t oa1 :10 __packed;
		uint32_t oa1mode :1 __packed;
		uint32_t rsvd__11_14 :4 __packed;
		uint32_t oa1en :1 __packed;
		uint32_t rsvd__16_31 :16 __packed;
	} bit;
};

union __oar2 {
	uint32_t val;
	struct {
		uint32_t rsvd__0 :1 __packed;
		uint32_t oa2 :7 __packed;
		uint32_t oa2msk :3 __packed;
		uint32_t rsvd__11_14 :4 __packed;
		uint32_t oa2en :1 __packed;
		uint32_t rsvd__16_31 :16 __packed;
	} bit;
};

union __timingr {
	uint32_t val;
	struct {
		uint32_t scll :8 __packed;
		uint32_t sclh :8 __packed;
		uint32_t sdadel :4 __packed;
		uint32_t scldel :4 __packed;
		uint32_t rsvd__24_27 :4 __packed;
		uint32_t presc :4 __packed;
	} bit;
};

union __timeoutr {
	uint32_t val;
	struct {
		uint32_t timeouta :12 __packed;
		uint32_t tidle :1 __packed;
		uint32_t rsvd__13_14 :1 __packed;
		uint32_t timouten :1 __packed;
		uint32_t timeoutb :12 __packed;
		uint32_t rsvd__28_30 :1 __packed;
		uint32_t texten :1 __packed;
	} bit;
};

union __isr {
	uint32_t val;
	struct {
		uint32_t txe :1 __packed;
		uint32_t txis :1 __packed;
		uint32_t rxne :1 __packed;
		uint32_t addr :1 __packed;
		uint32_t nackf :1 __packed;
		uint32_t stopf :1 __packed;
		uint32_t tc :1 __packed;
		uint32_t tcr :1 __packed;
		uint32_t berr :1 __packed;
		uint32_t arlo :1 __packed;
		uint32_t ovr :1 __packed;
		uint32_t pecerr :1 __packed;
		uint32_t timeout :1 __packed;
		uint32_t alert :1 __packed;
		uint32_t rsvd__14 :1 __packed;
		uint32_t busy :1 __packed;
		uint32_t dir :1 __packed;
		uint32_t addcode :7 __packed;
		uint32_t rsvd__24_31 :8 __packed;
	} bit;
};

union __icr {
	uint32_t val;
	struct {
		uint32_t rsvd__0_2 :3 __packed;
		uint32_t addr :1 __packed;
		uint32_t nack :1 __packed;
		uint32_t stop :1 __packed;
		uint32_t rsvd__6_7 :2 __packed;
		uint32_t berr :1 __packed;
		uint32_t arlo :1 __packed;
		uint32_t ovr :1 __packed;
		uint32_t pec :1 __packed;
		uint32_t timeout :1 __packed;
		uint32_t alert :1 __packed;
		uint32_t rsvd__14_31 :17 __packed;
	} bit;
};

union __pecr {
	uint32_t val;
	struct {
		uint32_t pec:8 __packed;
		uint32_t rsvd__9_31 :24 __packed;
	} bit;
};

union __dr {
	uint32_t val;
	struct {
		uint32_t data:8 __packed;
		uint32_t rsvd__9_31 :24 __packed;
	} bit;
};

/* 35.7.12 I2C register map */
struct i2c_stm32lx {
	union __cr1 cr1;
	union __cr2 cr2;
	union __oar1 oar1;
	union __oar2 oar2;
	union __timingr timingr;
	union __timeoutr timeoutr;
	union __isr isr;
	union __icr icr;
	union __pecr pecr;
	union __dr rxdr;
	uint32_t txdr;
};

typedef void (*irq_config_func_t)(struct device *port);

/* device config */
struct i2c_stm32lx_config {
	void *base;
	irq_config_func_t irq_config_func;
	/* clock subsystem driving this peripheral */
	struct stm32_pclken pclken;
};

/* driver data */
struct i2c_stm32lx_data {
	/* clock device */
	struct device *clock;
	/* Device config */
	union dev_config dev_config;
	/* ISR Sync */
	struct k_sem device_sync_sem;
	/* Current message data */
	struct {
		struct i2c_msg *msg;
		unsigned int len;
		uint8_t *buf;
		unsigned int is_err;
		unsigned int is_nack;
		unsigned int is_write;
	} current;
};

#endif	/* _STM32LX_UART_H_ */
