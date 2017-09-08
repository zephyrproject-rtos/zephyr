/*
 * Copyright (c) 2017 RDA
 * Copyright (c) 2016-2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "soc_cfg.h"

#define CLK_FREQ_40M            (0)
#define CLK_FREQ_80M            (1)
#define CLK_FREQ_160M           (2)

#define ADDR2REG(addr)          (*((volatile unsigned int *)(addr)))

#define RF_SPI_REG              ADDR2REG(0x4001301C)
#define TRAP_CTRL_REG           ADDR2REG(0x40011000)
#define TRAP0_SRC_REG           ADDR2REG(0x40011004)
#define TRAP0_DST_REG           ADDR2REG(0x40011024)
#define TRAP1_SRC_REG           ADDR2REG(0x40011008)
#define TRAP1_DST_REG           ADDR2REG(0x40011028)

#define TRAP_RAM_BASE           (0x00100000)
#define SYS_CPU_CLK             CLK_FREQ_160M
#define AHB_BUS_CLK             CLK_FREQ_80M

static inline void wr_rf_usb_reg(unsigned char a, unsigned short d, int isusb)
{
	while (RF_SPI_REG & (0x1 << 31))
		;
	while (RF_SPI_REG & (0x1 << 31))
		;
	RF_SPI_REG = (unsigned int)d | ((unsigned int)a << 16) |
		(0x1 << 25) | ((isusb) ? (0x1 << 27) : 0);
}

static inline void rd_rf_usb_reg(unsigned char a, unsigned short *d, int isusb)
{
	while (RF_SPI_REG & (0x01 << 31))
		;
	while (RF_SPI_REG & (0x01 << 31))
		;
	RF_SPI_REG = ((unsigned int)a << 16) | (0x01 << 24) |
		(0x01 << 25) | ((isusb) ? (0x01 << 27) : 0);
	__asm__ volatile ("nop");

	while (RF_SPI_REG & (0x01 << 31))
		;
	while (RF_SPI_REG & (0x01 << 31))
		;
	*d = (unsigned short)(RF_SPI_REG & 0xFFFF);
}

/* Config UART0 RX pin */
static inline int rda_ccfg_gp26(void)
{
	int ret = 0;
	unsigned short val = 0;

	rd_rf_usb_reg(0xcd, &val, 0);
	ret = (int)((val >> 6) & 0x1);
	if (!ret) {
		wr_rf_usb_reg(0xcd, (val | (0x1 << 6)), 0);
	}

	return ret;
}

/* Config USB */
static inline void rda_ccfg_usb(void)
{
	wr_rf_usb_reg(0x89, 0xeedd, 1);
}

/* Config PMU */
static inline void rda_ccfg_pmu(void)
{
	unsigned short val = 0;

	rd_rf_usb_reg(0xc0, &val, 0);
	wr_rf_usb_reg(0xc0, (val | (0x1 << 2)), 0); /* set sido_ctrl_comp2 */
}

/* Power down the debug-usage I2C */
static inline void rda_ccfg_pdi2c(void)
{
	wr_rf_usb_reg(0xa1, 0, 0);
}

/* Config CPU & Bus clock */
static inline void rda_ccfg_ck(void)
{
	unsigned short val = 0;

	rd_rf_usb_reg(0xa4, &val, 0);
	/* HCLK inv */
	val |=  (0x1 << 12);
	/* Config CPU clock */
	val &= ~(0x3 << 10);
	val |=  (SYS_CPU_CLK << 10); /* 2'b00:40M, 2'b01:80M, 2'b1x:160M */
	/* Config BUS clock */
	val &= ~(0x1 << 9);
	val |=  (AHB_BUS_CLK << 9);  /* 1'b0:40M, 1'b1:80M */
	wr_rf_usb_reg(0xa4, val, 0);

	/* Trap baud rate config for bootrom */
	/* Matrix between BusClk & TrapVal:  */
	/*                                   */
	/* -----+---------+--------+-------- */
	/*  C\T | 0x2814  | 0x2834 | 0x282C  */
	/* -----+---------+--------+-------- */
	/*  40M | 921600  | 460800 | 230400  */
	/* -----+------------------+-------- */
	/*  80M | 1843200 | 921600 | 460800  */
	/* -----+---------+--------+-------- */
	/*                                   */
	TRAP0_SRC_REG  = 0x00001ca4;
	TRAP0_DST_REG  = 0x00002834;

	/* Trap simple delay after ICACHE en */
	TRAP1_SRC_REG  = 0x00001eb4;
	TRAP1_DST_REG  = 0x00005a8c;
	/* Enable Trap0, Trap1 */
	TRAP_CTRL_REG |= (0x01 | (0x01 << 1));
}

/* Handle abort booting */
static inline void rda_ccfg_abort_hdlr(void)
{
	unsigned short val = 0;

	rd_rf_usb_reg(0xa1, &val, 0);
	if (0 != ((val >> 2) & 0x1)) {
		unsigned short val2 = 0;

		rd_rf_usb_reg(0xb2, &val2, 0);
		wr_rf_usb_reg(0xb2, (val2 | (0x1 << 11)), 0);
		rda_ccfg_ck();
		/* delay */
		for (val = 0; val < 0xff; val++) {
			;
		}
		wr_rf_usb_reg(0xb2, (val2 & ~(0x1 << 11)), 0);
	}
}

/* Power up the always-on timer */
void rda_ccfg_aontmr(void)
{
	unsigned short val = 0;

	rd_rf_usb_reg(0xa3, &val, 0);
	wr_rf_usb_reg(0xa3, (val | (0x1 << 12)), 0);
}

/* Config GPIO6 to dig core */
void rda_ccfg_gp6(void)
{
	unsigned short val = 0;

	rd_rf_usb_reg(0xcd, &val, 0);
	wr_rf_usb_reg(0xcd, (val | (0x1 << 11)), 0);
}

/* Config GPIO7 to dig core */
void rda_ccfg_gp7(void)
{
	unsigned short val = 0;

	rd_rf_usb_reg(0xb0, &val, 0);
	wr_rf_usb_reg(0xb0, (val | (0x1 << 14)), 0);
}

/* Set some core config when booting */
int rda_ccfg_boot(void)
{
	int ret = rda_ccfg_gp26();

	if (!ret) {
		rda_ccfg_usb();
		rda_ccfg_pmu();
		rda_ccfg_pdi2c();
		rda_ccfg_ck();
	}
	rda_ccfg_abort_hdlr();

	return ret;
}

/* Reset CPU & Bus clock config */
void rda_ccfg_ckrst(void)
{
	unsigned short val = 0;

	rd_rf_usb_reg(0xa4, &val, 0);
	/* HCLK inv */
	val &= ~(0x1 << 12);
	/* Config CPU clock */
	val &= ~(0x3 << 10);
	val |=  (0 << 10); /* 2'b00:40M, 2'b01:80M, 2'b1x:160M */
	/* Config BUS clock */
	val &= ~(0x1 << 9);
	val |=  (0 << 9);  /* 1'b0:40M, 1'b1:80M */
	wr_rf_usb_reg(0xa4, val, 0);
}

/* Init ADC module */
void rda_ccfg_adc_init(unsigned char ch)
{
	unsigned short val = 0;

	rd_rf_usb_reg(0xa3, &val, 0); /* adc en */
	wr_rf_usb_reg(0xa3, (val | (0x1 << 3)), 0);
	rd_rf_usb_reg(0xd8, &val, 0); /* clk 6p5m en */
	wr_rf_usb_reg(0xd8, (val | (0x1 << 15)), 0);
	rd_rf_usb_reg(0xb7, &val, 0); /* clk 26m en */
	wr_rf_usb_reg(0xb7, (val | (0x1 << 14)), 0);

	if (0 == (ch >> 1)) {
		rd_rf_usb_reg(0xb2, &val, 0);
		val &= ~(0x3 << 8);
		wr_rf_usb_reg(0xb2, (val | (0x1 << (9 - ch))), 0);
	}
}

/* Read ADC value */
unsigned short rda_ccfg_adc_read(unsigned char ch)
{
	unsigned short val = 0;

	rd_rf_usb_reg(0xb6, &val, 0); /* channel select */
	val &= ~((0x3) << 12);
	wr_rf_usb_reg(0xb6, (val | ((ch & 0x3) << 12)), 0);

	rd_rf_usb_reg(0xb6, &val, 0); /* set read en */
	wr_rf_usb_reg(0xb6, (val | (0x1 << 2)), 0);
	/* delay */
	for (val = 0; val < 0xff; val++) {
		;
	}
	rd_rf_usb_reg(0xb6, &val, 0); /* clr read en */
	wr_rf_usb_reg(0xb6, (val & ~(0x1 << 2)), 0);

	do {
		rd_rf_usb_reg(0xb7, &val, 0); /* finish loop flag */
	} while (0 == (val & (0x1 << 10)));

	return (val & 0x03ff);
}

