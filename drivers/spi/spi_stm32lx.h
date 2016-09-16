/*
 * Copyright (c) 2016 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32LX_SPI_H_
#define _STM32LX_SPI_H_

/* 38.6.1 Control register 1 (SPI_CR1) */
union __cr1 {
	u32_t val;
	struct {
		u32_t cpha :1 __packed;
		u32_t cpol :1 __packed;
		u32_t mstr :1 __packed;
		u32_t br :3 __packed;
		u32_t spe :1 __packed;
		u32_t lsbfirst :1 __packed;
		u32_t ssi :1 __packed;
		u32_t ssm :1 __packed;
		u32_t rxonly :1 __packed;
		u32_t crcl :1 __packed;
		u32_t crcnext :1 __packed;
		u32_t crcen :1 __packed;
		u32_t bidioe :1 __packed;
		u32_t bidimode :1 __packed;
		u32_t rsvd__16_31 :16 __packed;
	} bit;
};

/* 38.6.2 Control register 2 (SPI_CR2) */
union __cr2 {
	u32_t val;
	struct {
		u32_t rxdmaen :1 __packed;
		u32_t txdmaen :1 __packed;
		u32_t ssoe :1 __packed;
		u32_t nssp :1 __packed;
		u32_t frf :1 __packed;
		u32_t errie :1 __packed;
		u32_t rxneie :1 __packed;
		u32_t txeie :1 __packed;
		u32_t ds :4 __packed;
		u32_t frxth :1 __packed;
		u32_t ldma_rx :1 __packed;
		u32_t ldma_tx :1 __packed;
		u32_t rsvd__15_31 :17 __packed;
	} bit;
};

union __sr {
	u32_t val;
	struct {
		u32_t rxne :1 __packed;
		u32_t txe :1 __packed;
		u32_t rsvd__2_3 :2 __packed;
		u32_t crcerr :1 __packed;
		u32_t modf :1 __packed;
		u32_t ovr :1 __packed;
		u32_t bsy :1 __packed;
		u32_t fre :1 __packed;
		u32_t frlvl :2 __packed;
		u32_t ftlvl :2 __packed;
		u32_t rsvd__13_31 :19 __packed;
	} bit;
};

/* 36.6.8 SPI register map */
struct spi_stm32lx {
	union __cr1 cr1;
	union __cr2 cr2;
	union __sr sr;
	union {
		u8_t dr_as_u8;
		u16_t dr_as_u16;
		u32_t dr_as_u32;
	} dr;
	u32_t crcpr;
	u32_t rxcrcpr;
	u32_t txcrcpr;
};

typedef void (*irq_config_func_t)(struct device *port);

/* device config */
struct spi_stm32lx_config {
	void *base;
	irq_config_func_t irq_config_func;
	/* clock subsystem driving this peripheral */
	struct stm32_pclken pclken;
};

/* driver data */
struct spi_stm32lx_data {
	/* clock device */
	struct device *clock;
	/* ISR Sync */
	struct k_sem device_sync_sem;
	/* Current message data */
	struct {
		unsigned int is_slave;
		unsigned int rx_len;
		u8_t *rx_buf;
		unsigned int tx_len;
		const u8_t *tx_buf;
		unsigned int is_err;
	} current;
};

#endif	/* _STM32LX_SPI_H_ */
