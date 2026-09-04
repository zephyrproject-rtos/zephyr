/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADI_AD5940_AD5940_H_
#define ZEPHYR_DRIVERS_SENSOR_ADI_AD5940_AD5940_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/ad5940.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#ifdef CONFIG_AD5940_STREAM
#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/rtio/rtio.h>
#endif

#define DT_DRV_COMPAT adi_ad5940

#define SPICMD_SETADDR				0x20u
#define SPICMD_READREG				0x6Du
#define SPICMD_WRITEREG				0x2Du
#define SPICMD_READFIFO				0x5Fu

#define AD5940_REG_SILICON_0908			0x0908u
#define AD5940_REG_SILICON_0C08			0x0C08u

/* ALLON domain (always-on, no clock required) */
#define AD5940_REG_PWRMOD			0x0A00u
#define  AD5940_PWRMOD_PWRMOD_MSK		GENMASK(1, 0)
#define  AD5940_PWRMOD_SEQSLP_MSK		BIT(3)

#define AD5940_REG_PWRKEY			0x0A04u
#define AD5940_REG_OSCKEY			0x0A0Cu
#define AD5940_OSCKEY_UNLOCK			0xCB14u

#define AD5940_REG_OSCCON			0x0A10u
#define  AD5940_OSCCON_LFOSCOK_MSK		BIT(8)
#define  AD5940_OSCCON_HFOSCEN_MSK		BIT(1)
#define  AD5940_OSCCON_LFOSCEN_MSK		BIT(0)

#define AD5940_REG_ALLON_TMRCON			0x0A1Cu
#define  AD5940_ALLON_TMRCON_TMRINTEN_MSK	BIT(0)

#define AD5940_REG_EI2CON			0x0A28u

#define AD5940_REG_RSTSTA			0x0A40u
#define  AD5940_RSTSTA_MMRSWRST_MSK		BIT(1)

#define AD5940_REG_RSTCONKEY			0x0A5Cu
#define AD5940_REG_CLKEN0			0x0A70u

/* AFECON bank (ID, clocking, reset) */
#define AD5940_REG_ADIID			0x0400u
#define AD5940_REG_CHIPID			0x0404u

#define AD5940_REG_CLKCON0			0x0408u
#define  AD5940_CLKCON0_ADCCLKDIV_MSK		GENMASK(9, 6)
#define  AD5940_CLKCON0_SYSCLKDIV_MSK		GENMASK(5, 0)

#define AD5940_REG_CLKEN1			0x0410u

#define AD5940_REG_CLKSEL			0x0414u
#define  AD5940_CLKSEL_ADCCLKSEL_MSK		GENMASK(3, 2)
#define  AD5940_CLKSEL_SYSCLKSEL_MSK		GENMASK(1, 0)

#define AD5940_REG_SWRSTCON			0x0424u

#define AD5940_REG_TRIGSEQ			0x0430u
#define  AD5940_TRIGSEQ_SEQID_MSK		GENMASK(1, 0)


/* Wakeup timer sequencer */
#define AD5940_REG_WUPTMRCON			0x0800u
#define  AD5940_TMRCON_WUPTEN_MSK		BIT(0)
#define  AD5940_TMRCON_ENDSEQ_MSK		GENMASK(4, 1)

#define AD5940_REG_SEQORDER			0x0804u
#define AD5940_REG_SEQ0WUPL			0x0808u
#define AD5940_REG_SEQ0WUPH			0x080Cu
#define AD5940_REG_SEQ0SLEEPL			0x0810u
#define AD5940_REG_SEQ0SLEEPH			0x0814u
#define AD5940_REG_SEQ1WUPL			0x0818u
#define AD5940_REG_SEQ1WUPH			0x081Cu
#define AD5940_REG_SEQ1SLEEPL			0x0820u
#define AD5940_REG_SEQ1SLEEPH			0x0824u
#define AD5940_REG_SEQ2WUPL			0x0828u
#define AD5940_REG_SEQ2WUPH			0x082Cu
#define AD5940_REG_SEQ2SLEEPL			0x0830u
#define AD5940_REG_SEQ2SLEEPH			0x0834u
#define AD5940_REG_SEQ3WUPL			0x0838u
#define AD5940_REG_SEQ3WUPH			0x083Cu
#define AD5940_REG_SEQ3SLEEPL			0x0840u
#define AD5940_REG_SEQ3SLEEPH			0x0844u

/* AFE main registers */
#define AD5940_REG_AFECON			0x2000u
#define  AD5940_AFECON_DACBUFEN_MSK		BIT(21)
#define  AD5940_AFECON_DACREFEN_MSK		BIT(20)
#define  AD5940_AFECON_SINC2EN_MSK		BIT(16)
#define  AD5940_AFECON_DFTEN_MSK		BIT(15)
#define  AD5940_AFECON_WAVEGENEN_MSK		BIT(14)
#define  AD5940_AFECON_TIAEN_MSK		BIT(11)
#define  AD5940_AFECON_INAMPEN_MSK		BIT(10)
#define  AD5940_AFECON_EXBUFEN_MSK		BIT(9)
#define  AD5940_AFECON_ADCCONVEN_MSK		BIT(8)
#define  AD5940_AFECON_ADCEN_MSK		BIT(7)
#define  AD5940_AFECON_DACEN_MSK		BIT(6)
#define  AD5940_AFECON_HPREFDIS_MSK		BIT(5)

#define AD5940_REG_SEQCON			0x2004u
#define  AD5940_SEQCON_SEQWRTMR_MSK		GENMASK(15, 8)
#define  AD5940_SEQCON_SEQHALTFIFOEMPTY_MSK	BIT(1)
#define  AD5940_SEQCON_SEQEN_MSK		BIT(0)

#define AD5940_REG_FIFOCON			0x2008u
#define  AD5940_FIFOCON_DATAFIFOSRCSEL_MSK	GENMASK(15, 13)
#define  AD5940_FIFOCON_DATAFIFOEN_MSK		BIT(11)

#define AD5940_REG_SWCON			0x200Cu
#define  AD5940_SWCON_SWSOURCESEL_MSK		BIT(16)
#define  AD5940_SWCON_TMUXCON_MSK		GENMASK(15, 12)
#define  AD5940_SWCON_NMUXCON_MSK		GENMASK(11, 8)
#define  AD5940_SWCON_PMUXCON_MSK		GENMASK(7, 4)
#define  AD5940_SWCON_DMUXCON_MSK		GENMASK(3, 0)

#define AD5940_REG_HSDACCON			0x2010u
#define  AD5940_HSDACCON_ATTENEN_MSK		BIT(9)
#define  AD5940_HSDACCON_Rate_MSK		GENMASK(8, 1)
#define  AD5940_HSDACCON_GAINX5_MSK		BIT(0)

#define AD5940_REG_WGCON			0x2014u
#define  AD5940_WGCON_TYPESEL_MSK		GENMASK(2, 1)

#define AD5940_REG_WGFCW			0x2030u
#define  AD5940_WGFCW_SINEFCW_MSK		GENMASK(23, 0)

#define AD5940_REG_WGPHASE			0x2034u
#define AD5940_REG_WGOFFSET			0x2038u
#define AD5940_REG_WGAMPLITUDE			0x203Cu

#define AD5940_REG_ADCFILTERCON			0x2044u
#define  AD5940_ADCFILTERCON_DFTCLKENB_MSK	BIT(18)
#define  AD5940_ADCFILTERCON_DACWAVECLKENB_MSK	BIT(17)
#define  AD5940_ADCFILTERCON_SINC2CLKENB_MSK	BIT(16)
#define  AD5940_ADCFILTERCON_AVRGNUM_MSK	GENMASK(15, 14)
#define  AD5940_ADCFILTERCON_SINC3OSR_MSK	GENMASK(13, 12)
#define  AD5940_ADCFILTERCON_SINC2OSR_MSK	GENMASK(11, 8)
#define  AD5940_ADCFILTERCON_AVRGEN_MSK		BIT(7)
#define  AD5940_ADCFILTERCON_SINC3BYP_MSK	BIT(6)
#define  AD5940_ADCFILTERCON_LPFBYPEN_MSK	BIT(4)
#define  AD5940_ADCFILTERCON_ADCSAMPLERATE_MSK	BIT(0)

#define AD5940_REG_LPREFBUFCON			0x2050u
#define AD5940_REG_SEQCNT			0x2064u
#define AD5940_REG_DATAFIFORD			0x206Cu
#define  AD5940_FIFO_CHID_MSK			GENMASK(22, 16)
#define  AD5940_FIFO_SEQID_MSK			GENMASK(24, 23)

#define AD5940_REG_CMDFIFOWRITE			0x2070u
#define AD5940_REG_ADCDAT			0x2074u
#define  AD5940_ADCDAT_DATA_MSK			GENMASK(15, 0)

#define AD5940_REG_DFTREAL			0x2078u
#define  AD5940_DFTREAL_DATA_MSK		GENMASK(17, 0)

#define AD5940_REG_DFTIMAG			0x207Cu
#define  AD5940_DFTIMAG_DATA_MSK		GENMASK(17, 0)

#define AD5940_REG_SINC2DAT			0x2080u
#define  AD5940_SINC2DAT_DATA_MSK		GENMASK(15, 0)

#define AD5940_REG_DFTCON			0x20D0u
#define  AD5940_DFTCON_DFTINSEL_MSK		GENMASK(21, 20)
#define  AD5940_DFTCON_DFTNUM_MSK		GENMASK(7, 4)
#define  AD5940_DFTCON_HANNINGEN_MSK		BIT(0)

#define AD5940_REG_LPTIASW0			0x20E4u
#define  AD5940_LPTIASW0_RECAL_MSK		BIT(15)
#define  AD5940_LPTIASW0_SW13_MSK		BIT(13)
#define  AD5940_LPTIASW0_SW12_MSK		BIT(12)
#define  AD5940_LPTIASW0_SW11_MSK		BIT(11)
#define  AD5940_LPTIASW0_SW10_MSK		BIT(10)
#define  AD5940_LPTIASW0_SW9_MSK		BIT(9)
#define  AD5940_LPTIASW0_SW8_MSK		BIT(8)
#define  AD5940_LPTIASW0_SW7_MSK		BIT(7)
#define  AD5940_LPTIASW0_SW6_MSK		BIT(6)
#define  AD5940_LPTIASW0_SW5_MSK		BIT(5)
#define  AD5940_LPTIASW0_SW4_MSK		BIT(4)
#define  AD5940_LPTIASW0_SW3_MSK		BIT(3)
#define  AD5940_LPTIASW0_SW2_MSK		BIT(2)
#define  AD5940_LPTIASW0_SW1_MSK		BIT(1)
#define  AD5940_LPTIASW0_SW0_MSK		BIT(0)

#define AD5940_REG_LPTIACON0			0x20ECu
#define  AD5940_LPTIACON0_TIARF_MSK		GENMASK(15, 13)
#define  AD5940_LPTIACON0_TIARL_MSK		GENMASK(12, 10)
#define  AD5940_LPTIACON0_TIAGAIN_MSK		GENMASK(9, 5)
#define  AD5940_LPTIACON0_IBOOST_MSK		GENMASK(4, 3)

#define AD5940_REG_HSRTIACON			0x20F0u
#define  AD5940_HSRTIACON_CTIACON_MSK		GENMASK(12, 5)
#define  AD5940_HSRTIACON_RTIACON_MSK		GENMASK(3, 0)

#define AD5940_REG_HSTIACON			0x20FCu
#define  AD5940_HSTIACON_VBIASSEL_MSK		GENMASK(1, 0)

#define AD5940_REG_SEQSLPLOCK			0x2118u
#define AD5940_REG_SEQTRGSLP			0x211Cu
#define AD5940_REG_LPDACDAT0			0x2120u
#define AD5940_REG_LPDACSW0			0x2124u
#define  AD5940_LPDACSW0_LPMODEDIS_MSK		BIT(5)
#define  AD5940_LPDACSW0_SW4_MSK		BIT(4)
#define  AD5940_LPDACSW0_SW3_MSK		BIT(3)
#define  AD5940_LPDACSW0_SW2_MSK		BIT(2)
#define  AD5940_LPDACSW0_SW1_MSK		BIT(1)
#define  AD5940_LPDACSW0_SW0_MSK		BIT(0)

#define AD5940_REG_LPDACCON0			0x2128u
#define AD5940_REG_DSWFULLCON			0x2150u
#define  AD5940_DSWFULLCON_D8_MSK		BIT(7)
#define  AD5940_DSWFULLCON_D7_MSK		BIT(6)
#define  AD5940_DSWFULLCON_D5_MSK		BIT(4)
#define  AD5940_DSWFULLCON_D4_MSK		BIT(3)
#define  AD5940_DSWFULLCON_D3_MSK		BIT(2)
#define  AD5940_DSWFULLCON_D2_MSK		BIT(1)
#define  AD5940_DSWFULLCON_DR0_MSK		BIT(0)

#define AD5940_REG_NSWFULLCON			0x2154u
#define  AD5940_NSWFULLCON_NL_MSK		BIT(10)
#define  AD5940_NSWFULLCON_NR1_MSK		BIT(9)
#define  AD5940_NSWFULLCON_AIN1_MSK		BIT(1)

#define AD5940_REG_PSWFULLCON			0x2158u
#define  AD5940_PSWFULLCON_PL2_MSK		BIT(14)
#define  AD5940_PSWFULLCON_PL_MSK		BIT(11)
#define  AD5940_PSWFULLCON_CE0_MSK		BIT(10)
#define  AD5940_PSWFULLCON_PR0_MSK		BIT(0)

#define AD5940_REG_TSWFULLCON			0x215Cu
#define  AD5940_TSWFULLCON_TR1_MSK		BIT(11)
#define  AD5940_TSWFULLCON_T10_MSK		BIT(9)
#define  AD5940_TSWFULLCON_T9_MSK		BIT(8)
#define  AD5940_TSWFULLCON_T7_MSK		BIT(6)
#define  AD5940_TSWFULLCON_T6_MSK		BIT(5)
#define  AD5940_TSWFULLCON_T5_MSK		BIT(4)
#define  AD5940_TSWFULLCON_T4_MSK		BIT(3)
#define  AD5940_TSWFULLCON_T3_MSK		BIT(2)
#define  AD5940_TSWFULLCON_T2_MSK		BIT(1)
#define  AD5940_TSWFULLCON_T1_MSK		BIT(0)

#define AD5940_REG_BUFSENCON			0x2180u
#define  AD5940_BUFSENCON_V1P1HPADCEN_MSK	BIT(4)
#define  AD5940_BUFSENCON_V1P8HPADCEN_MSK	BIT(0)

#define AD5940_REG_ADCCON			0x21A8u
#define  AD5940_ADCCON_GNPGA_MSK		GENMASK(18, 16)
#define  AD5940_ADCCON_MUXSELN_MSK		GENMASK(12, 8)
#define  AD5940_ADCCON_MUXSELP_MSK		GENMASK(5, 0)

#define AD5940_REG_STATSVAR			0x21C0u
#define AD5940_REG_STATSCON			0x21C4u
#define  AD5940_STATSCON_STDDEV_MSK		GENMASK(11, 7)
#define  AD5940_STATSCON_SAMPLENUM_MSK		GENMASK(6, 4)
#define  AD5940_STATSCON_STATSEN_MSK		BIT(0)

#define AD5940_REG_STATSMEAN			0x21C8u
#define AD5940_REG_SEQ0INFO			0x21CCu
#define  AD5940_SEQINFO_INSTNUM_MSK		GENMASK(26, 16)
#define  AD5940_SEQINFO_STARTADDR_MSK		GENMASK(10, 0)

#define AD5940_REG_SEQ2INFO			0x21D0u
#define AD5940_REG_CMDFIFOWADDR			0x21D4u
#define AD5940_REG_CMDDATACON			0x21D8u

#define AD5940_REG_DATAFIFOTHRES		0x21E0u
#define  AD5940_DATAFIFOTHRES_HIGHTHRES_MSK	GENMASK(26, 16)

#define AD5940_REG_SEQ3INFO			0x21E4u
#define AD5940_REG_SEQ1INFO			0x21E8u
#define AD5940_REG_REPEATADCCNV			0x21F0u
#define AD5940_REG_FIFOCNTSTA			0x2200u
#define  AD5940_FIFOCNTSTA_DATAFIFOCNTSTA_MSK	GENMASK(26, 16)

#define AD5940_REG_ADCOFFSETGN1			0x2244u
#define AD5940_REG_PMBW				0x22F0u
#define  AD5940_PMBW_SYSBW_MSK			GENMASK(3, 2)
#define  AD5940_PMBW_SYSHS_MSK			BIT(0)

#define AD5940_REG_SWMUX			0x235Cu
#define  AD5940_SWMUX_CMMUX_MSK			BIT(3)

#define AD5940_REG_ADCBUFCON			0x238Cu

#define AD5940_SEQSLPLOCK_KEY			0xA47E5u

#define AD5940_FIFO_BURST_MAX_WORDS		64u

#define AD5940_REG_GP0CON			0x0000u
#define AD5940_REG_GP0OEN			0x0004u
#define AD5940_REG_GP0PE			0x0008u
#define AD5940_REG_GP0IEN			0x000Cu
#define AD5940_REG_GP0IN			0x0010u
#define AD5940_REG_GP0OUT			0x0014u

#define AD5940_GP0CON_PIN0_INT0			0x0000u

#define AD5940_REG_INTCPOL			0x3000u
#define AD5940_REG_INTCCLR			0x3004u
#define AD5940_REG_INTCSEL0			0x3008u
#define AD5940_REG_INTCSEL1			0x300Cu
#define AD5940_REG_INTCFLAG0			0x3010u
#define AD5940_REG_INTCFLAG1			0x3014u

#define AD5940_TMRCON_ENDSEQ_A			0x0u
#define AD5940_TMRCON_ENDSEQ_B			0x1u

/* SINC3OSR field values */
#define AD5940_SINC3OSR_5			0u
#define AD5940_SINC3OSR_4			1u
#define AD5940_SINC3OSR_2			2u

/* SINC2OSR field values */
#define AD5940_SINC2OSR_22			0u
#define AD5940_SINC2OSR_44			1u
#define AD5940_SINC2OSR_89			2u
#define AD5940_SINC2OSR_178			3u
#define AD5940_SINC2OSR_267			4u
#define AD5940_SINC2OSR_533			5u
#define AD5940_SINC2OSR_640			6u
#define AD5940_SINC2OSR_667			7u
#define AD5940_SINC2OSR_800			8u
#define AD5940_SINC2OSR_889			9u
#define AD5940_SINC2OSR_1067			10u
#define AD5940_SINC2OSR_1333			11u

/* ADC PGA gain codes */
#define AD5940_ADCPGA_1				0u
#define AD5940_ADCPGA_1P5			1u
#define AD5940_ADCPGA_2				2u
#define AD5940_ADCPGA_4				3u
#define AD5940_ADCPGA_9				4u

/* DFT input source */
#define AD5940_DFTSRC_SINC2NOTCH		0u
#define AD5940_DFTSRC_SINC3			1u
#define AD5940_DFTSRC_ADCRAW			2u
#define AD5940_DFTSRC_AVG			3u

/* Data-flow output type for ClksCalculate */
#define AD5940_DATATYPE_ADCRAW			0u
#define AD5940_DATATYPE_SINC3			1u
#define AD5940_DATATYPE_SINC2			2u
#define AD5940_DATATYPE_DFT			3u

/* ADCCON (0x21A8) */
#define AD5940_ADCCON_GNPGA_MSK			GENMASK(18, 16)
#define AD5940_ADCCON_MUXSELN_MSK		GENMASK(12, 8)
#define AD5940_ADCCON_MUXSELP_MSK		GENMASK(5, 0)

/* ADCCON MUXSELP values */
#define AD5940_ADCCON_MUXSELP_HSTIA_P		0x01u
#define AD5940_ADCCON_MUXSELP_AIN2		0x06u
#define AD5940_ADCCON_MUXSELP_LPTIA_P		0x21u
#define AD5940_ADCCON_MUXSELP_P_NODE		0x24u

/* ADCCON MUXSELN values */
#define AD5940_ADCCON_MUXSELN_HSTIA_N		0x01u
#define AD5940_ADCCON_MUXSELN_AIN3		0x07u
#define AD5940_ADCCON_MUXSELN_N_NODE		0x14u
#define AD5940_ADCCON_MUXSELN_LPTIA_N		0x02u
#define AD5940_ADCCON_MUXSELN_VBIAS		0x08u
#define AD5940_ADCCON_MUXSELN_VZERO0		0x10u

/* Recommended LPTIASW0 value for normal amperometric mode */
#define AD5940_LPTIASW0_AMPER_NORMAL \
	(AD5940_LPTIASW0_SW13_MSK | AD5940_LPTIASW0_SW12_MSK | \
	 AD5940_LPTIASW0_SW5_MSK  | AD5940_LPTIASW0_SW3_MSK  | \
	 AD5940_LPTIASW0_SW2_MSK)

/* SYSBW field values  */
#define AD5940_PMBW_SYSBW_NO_ACTION		0u
#define AD5940_PMBW_SYSBW_50KHZ			1u
#define AD5940_PMBW_SYSBW_100KHZ		2u
#define AD5940_PMBW_SYSBW_250KHZ		3u

/* STATSCON SAMPLENUM field values  */
#define AD5940_STATSSAMPLE_0			0u
#define AD5940_STATSSAMPLE_4			1u
#define AD5940_STATSSAMPLE_8			2u
#define AD5940_STATSSAMPLE_16			3u
#define AD5940_STATSSAMPLE_32			4u
#define AD5940_STATSSAMPLE_64			5u
#define AD5940_STATSSAMPLE_128			6u

/* AD5940_DFT_CHID_TOP_MSK: the 5-bit DFT channel-ID field at bits[22:18].
 * AD5940_DFT_CHID_TOP_DFT: the DFT result marker. Validate that a word is a
 * DFT result by checking (word >> 18) & 0x1F == 0x1F.
 */
#define AD5940_DFT_CHID_TOP_MSK			GENMASK(22, 18)
#define AD5940_DFT_CHID_TOP_DFT			0x1Fu

/* FIFO source-select LSB positions (for shift-based FIFOCON construction) */
#define AD5940_AFE_FIFOCON_DATAFIFOSRCSEL_LSB	13u
#define AD5940_AFE_FIFOCON_DATAFIFOSIZE_LSB	9u
#define AD5940_AFE_FIFOCON_DATAFIFOMODE_LSB	6u

/* Init sequence named constants */
#define AD5940_INIT_SILICON_0908		0x02C9u
#define AD5940_INIT_SILICON_0C08		0x206Cu
#define AD5940_INIT_REPEATADCCNV		0x0010u
#define AD5940_INIT_CLKEN1			0x02C9u
#define AD5940_INIT_EI2CON			0x0009u
#define AD5940_INIT_ADCBUFCON			0x0104u
#define AD5940_INIT_PWRMOD			0x8009u
#define AD5940_INIT_PMBW			0x0000u

/* Key constants */
#define AD5940_ADI_ID				0x4144u
#define AD5940_CHIP_ID_S1			0x5500u
#define AD5940_CHIP_ID_S2			0x5501u
#define AD5940_CHIP_ID_S3			0x5502u

#define AD5940_RSTCONKEY_UNLOCK			0x12EAu
#define AD5940_SWRST_KEY			0xA158u
#define AD5940_PWRKEY1				0x4859u
#define AD5940_PWRKEY2				0xF27Bu
#define AD5940_SLEEP_KEY			0x82ADu
#define AD5940_HIBERNATE_KEY			0xA59Fu

/* CLKSEL source values */
#define AD5940_CLKSEL_HFOSC			0u

/* PWRMOD mode values */
#define AD5940_PWRMOD_ACTIVE			0u

/* Interrupt source bits (INTCFLAG0 / INTCSEL0) */
#define AFEINTSRC_DFTRDY			BIT(1)
#define AFEINTSRC_ENDSEQ			BIT(15)
#define AFEINTSRC_DATAFIFOTHRESH		BIT(25)
#define AFEINTSRC_ALLINT			0xFFFFFFFFu

/* FIFO source selectors */
#define AD5940_FIFOSRC_ADC			1u
#define AD5940_FIFOSRC_DFT			2u
#define AD5940_FIFOSRC_SINC2			3u
#define AD5940_FIFOSRC_MEAN			5u

/* FIFO size selectors */
#define AD5940_FIFOSIZE_4KB			2u

/* FIFO mode selectors */
#define AD5940_FIFOMODE_FIFO			2u

/* Waveform generator types */
#define AD5940_WGTYPE_SINE			2u

/* System constants */
#define AD5940_SYS_CLK_HZ			16000000u
#define AD5940_FCW_SHIFT			30u
#define AD5940_WAKEUP_RETRIES			30u
#define AD5940_WAKEUP_DELAY_US			200u

/* Low-frequency oscillator used by the wakeup timer. */
#define AD5940_LFOSC_HZ				32000u

/* 16-bit field mask for registers whose upper half is reserved. */
#define AD5940_REG16_MSK			0xFFFFu

/* Byte mask/shift helpers for the big-endian SPI payload. */
#define AD5940_BYTE_MSK				0xFFu
#define AD5940_BYTE_BITS			8u

/* Register width boundary */
#define AD5940_REG32_ADDR_MIN			0x1000u
#define AD5940_REG32_ADDR_MAX			0x3014u

/* SPI frame sizes, in bytes. */
#define AD5940_SPI_ADDR_FRAME_LEN		3u
#define AD5940_SPI_REG_RD_LEN			6u
#define AD5940_SPI_WR32_LEN			5u
#define AD5940_SPI_WR16_LEN			3u
#define AD5940_SPI_RD_DATA_OFFSET		2u

/* Fast-burst FIFO frame: 1 command byte + 6 dummy bytes precede the payload,
 * and the trailing two words must carry AD5940_FIFO_BURST_END on MOSI to
 * terminate the burst.
 */
#define AD5940_FIFO_BURST_HDR_LEN		7u
#define AD5940_FIFO_BURST_END_LEN		8u
#define AD5940_FIFO_BURST_END			0x44u

/* Word count at or above which the fast-burst path is used. */
#define AD5940_FIFO_BURST_MIN_WORDS		4u

/* ADC transfer function. The 16-bit ADC is offset-binary: code 0x8000 is 0 V
 * differential, and full scale is +/-1.835 V at PGA gain 1.
 */
#define AD5940_ADC_MIDSCALE_CODE		0x8000u
#define AD5940_ADC_HALF_SCALE			32768.0f
#define AD5940_ADC_VREF_V			1.835f

/* PGA gain values, indexed by AD5940_ADCPGA_*. */
#define AD5940_ADCPGA_GAIN_1			1.0f
#define AD5940_ADCPGA_GAIN_1P5			1.5f
#define AD5940_ADCPGA_GAIN_2			2.0f
#define AD5940_ADCPGA_GAIN_4			4.0f
#define AD5940_ADCPGA_GAIN_9			9.0f

/* Waveform generator amplitude. WGAMPLITUDE is a 12-bit word (0..2047 usable)
 * spanning the 800 mVpp HSDAC output range before the excitation buffer.
 */
#define AD5940_WG_AMP_MAX_WORD			0x7FFu
#define AD5940_WG_AMP_FULL_SCALE		2047u
#define AD5940_WG_FULL_SCALE_MVPP		800u

/* Excitation-buffer full-scale swing with ExcitBufGain=2 and HsDacGain=1. */
#define AD5940_EXCIT_BUF_FS_MVPP		1600.0f

/* RTIA-calibration excitation target: 80% of the 1.8 Vpp HSTIA compliance
 * window (1800 * 0.8), scaled by RCAL/RTIA. Mirrors no-OS ad5940_HSRtiaCal.
 */
#define AD5940_CAL_EXCIT_MVPP_BASE		1440.0f

/* Minimum calibration amplitude word. Below this both cal DFT captures sit at
 * the ADC noise floor and the RTIA/RCAL ratio is meaningless.
 */
#define AD5940_CAL_MIN_AMP_WORD			32u

/* Default RTIA (ohms) used when the RTIA selector is out of table range. */
#define AD5940_RTIA_DEFAULT_OHMS		10000u

/* Fixed-point scaling used by the streaming decoder for the V/I ratio. */
#define AD5940_DECODER_Q_SHIFT			14
#define AD5940_DECODER_Q_SCALE			16384.0f

/* SYSCLK-to-ADC-clock ratio used by ad5940_clks_calculate(). */
#define AD5940_RATIO_SYS2ADC			20.0f

/* DFTNUM register field to DFT point count: 0 => 4 points, 11 => 8192 points. */
#define AD5940_DFT_NUM_SHIFT_BASE		2u
#define AD5940_DFT_NUM_TO_POINTS(n)		(1u << ((n) + AD5940_DFT_NUM_SHIFT_BASE))

/* Extra SYSCLK cycles the DFT engine needs beyond the filter chain latency. */
#define AD5940_DFT_OVERHEAD_CLKS		25u

/* Maximum AVGNUM selector value (0..3 => 2/4/8/16 averages). */
#define AD5940_AVGNUM_MAX			3u

/* Waveform generator excitation amplitude limit in mVpp (DT/attr range check). */
#define AD5940_EXCIT_MAX_MVPP			800

/* Excitation frequency limits enforced on the freq attribute, in Hz. */
#define AD5940_EXCIT_MIN_FREQ_HZ		0.015f
#define AD5940_EXCIT_MAX_FREQ_HZ		200000.0f

/* LPDAC (12-bit) bias output: 0.2 V to 2.4 V, i.e. a 2200 mV span. */
#define AD5940_LPDAC_MIN_MV			200u
#define AD5940_LPDAC_MAX_MV			2200u
#define AD5940_LPDAC_SPAN_MV			2200u
#define AD5940_LPDAC_FULL_SCALE			4095u

/* Widest 8-bit attribute range (settling cycles, sequence counts, etc.). */
#define AD5940_U8_MAX				255

/* Minimum HSDAC update-rate divider (HSDACCON Rate field). */
#define AD5940_HSDAC_RATE_MIN			7

/* Cyclic-voltammetry scan-rate limit, in mV/s. */
#define AD5940_CV_SCAN_RATE_MAX			10000

/* Post-reset settle time before the init sequence may be written. */
#define AD5940_RESET_SETTLE_US			500u

/* Wakeup timer default period, in LFOSC ticks (~128 ms at 32 kHz). */
#define AD5940_SEQ_SLEEP_TICKS			4096u

/* Polling budgets. Each iteration busy-waits AD5940_POLL_STEP_US. */
#define AD5940_POLL_STEP_US			100u
#define AD5940_DFTRDY_POLL_COUNT		2000
#define AD5940_FIFO_POLL_COUNT			1000

/* Analog settle time between powering the excitation path and starting a
 * conversion (~800 SYSCLK cycles at 16 MHz).
 */
#define AD5940_ANALOG_SETTLE_US			50u

/* Drain time for the sinc3 pipeline after the DFT engine is stopped. */
#define AD5940_PIPELINE_DRAIN_US		10u

/* Raw ADC settle: SINC-filtered ADCDAT needs a few conversions to stabilise. */
#define AD5940_ADC_SETTLE_ITERS			50
#define AD5940_ADC_SETTLE_STEP_US		20u

#define AD5940_MV_PER_V				1000.0f
#define AD5940_MICRO_PER_UNIT			1000000.0f
#define AD5940_NSEC_PER_SEC			1000000000u
#define AD5940_PI_F				3.14159265358979323846f
#define AD5940_ROUND_F				0.5f

/* SPI unlock word that must be clocked out after a software reset. */
#define AD5940_SPI_UNLOCK_KEY			0xDEADBEEFu
#define AD5940_SPI_UNLOCK_LEN			4u

/* LFOSC stabilisation poll budget after enabling the oscillators. */
#define AD5940_OSC_POLL_COUNT			100

#define AD5940_ADC_SELFCAL_MAX_OFFSET		4096

#define AD5940_EIS_WORDS_PER_FRAME		4u
#define AD5940_FIFO_WORD_BYTES			4u
#define AD5940_FIFO_DRAIN_MAX_ITERS		64

#define AD5940_STREAM_WARMUP_TICKS		4u

/* DFT results are 18-bit two's complement in DFTREAL/DFTIMAG. */
#define AD5940_DFT_DATA_BITS			18u

/* q31 shift used by the decoder for current readings, and the left shift that
 * maps a 16-bit ADC code into that representation.
 */
#define AD5940_CURRENT_Q31_SHIFT		20
#define AD5940_CURRENT_CODE_SHIFT		4

/* Fallback values used when the matching devicetree property is absent. */
#define AD5940_DEF_RCAL_OHMS			200
#define AD5940_DEF_INT_PIN			0
#define AD5940_DEF_EIS_FREQ_HZ			1000
#define AD5940_DEF_EIS_AMPLITUDE_MVPP		200
#define AD5940_DEF_EIS_RTIA_SEL			4
#define AD5940_DEF_EIS_SETTLING_CYCLES		16
#define AD5940_DEF_EIS_DFT_NUM			11
#define AD5940_DEF_EIS_DFT_SRC			1
#define AD5940_DEF_EIS_SINC3_OSR		2
#define AD5940_DEF_EIS_EXCIT_BUF_GAIN		0
#define AD5940_DEF_EIS_HSDAC_GAIN		0
#define AD5940_DEF_EIS_HSDAC_RATE		7
#define AD5940_DEF_EIS_CTIA_SEL			0
#define AD5940_DEF_EIS_POWER_MODE		0
#define AD5940_DEF_AMPER_BIAS_MV		1200
#define AD5940_DEF_AMPER_LPAMP_RTIA		2
#define AD5940_DEF_SWEEP_START_HZ		1000
#define AD5940_DEF_SWEEP_STOP_HZ		100000
#define AD5940_DEF_SWEEP_POINTS			20

/* SPI word size used for every AD5940 transaction. */
#define AD5940_SPI_WORD_BITS			8

/* Sequencer SRAM depth, in 32-bit command words. */
#define AD5940_SEQ_SRAM_WORDS			512u

/* Wakeup/sleep tick counters are 20-bit and split across a low 16-bit register
 * and a high nibble register.
 */
#define AD5940_TICK_HI_SHIFT			16u
#define AD5940_TICK_HI_MSK			0xFu

/* Hardware reset pulse width on the RESET pin. */
#define AD5940_RESET_PULSE_US			200u

/* SEQTRGSLP: allow the sequencer to trigger sleep on SEQxSLEEP timeout. */
#define AD5940_SEQTRGSLP_EN			0x0001u

/* CMDDATACON: command FIFO in stream mode, data FIFO in FIFO mode, both
 * 2 kB. Field layout (0<<9)|(2<<6)|(1<<3)|(1<<0).
 */
#define AD5940_CMDDATACON_DEFAULT		0x089u

/* Field limits validated by the attribute setters. */
#define AD5940_GPIO_PIN_MAX			7u
#define AD5940_GPIO_FUNC_MAX			3u
#define AD5940_GPIO_FUNC_BITS			2u
#define AD5940_GPIO_FUNC_MSK			0x3u
#define AD5940_DFT_NUM_MAX			12
#define AD5940_DFT_SRC_MAX			3
#define AD5940_SINC3_OSR_MAX			2
#define AD5940_RTIA_SEL_MAX			8
#define AD5940_RTIA_SEL_MSK			0xFu
#define AD5940_CTIA_SEL_MSK			0x1Fu
#define AD5940_CTIA_SEL_MAX			31
#define AD5940_SWEEP_POINTS_MIN			2
#define AD5940_ODR_MIN_HZ			1

#define AD5940_MICRO_SCALE_F			1e-6f
#define AD5940_DECADE_BASE_F			10.0f

#define AD5940_SEQ_WAIT_CLKS(n) (0x00000000u | ((uint32_t)(n) & 0x3FFFFFFFu))
#define AD5940_SEQ_WR_REG(a, d) (0x80000000u | ((((uint32_t)(a) >> 2u) & 0x7Fu) << 24u) \
				 | ((uint32_t)(d) & 0xFFFFFFu))
#define AD5940_SEQ_STOP()       AD5940_SEQ_WR_REG(AD5940_REG_SEQCON, 0u)
#define AD5940_SEQ_SLP()        AD5940_SEQ_WR_REG(AD5940_REG_SEQTRGSLP, 1u)

#define AD5940_AFECON_ANALOG (AD5940_AFECON_DACREFEN_MSK | AD5940_AFECON_SINC2EN_MSK | \
			      AD5940_AFECON_WAVEGENEN_MSK | AD5940_AFECON_TIAEN_MSK | \
			      AD5940_AFECON_INAMPEN_MSK | AD5940_AFECON_EXBUFEN_MSK | \
			      AD5940_AFECON_ADCEN_MSK | AD5940_AFECON_DACEN_MSK)
#define AD5940_AFECON_CONV (AD5940_AFECON_ANALOG | AD5940_AFECON_ADCCONVEN_MSK | \
			    AD5940_AFECON_DFTEN_MSK)
#define AD5940_AFECON_OFF (AD5940_AFECON_DACREFEN_MSK | AD5940_AFECON_SINC2EN_MSK | \
			   AD5940_AFECON_TIAEN_MSK | AD5940_AFECON_INAMPEN_MSK | \
			   AD5940_AFECON_EXBUFEN_MSK | AD5940_AFECON_DACEN_MSK)

#define AD5940_SEQ_SETTLE_CLKS 800u
#define AD5940_SEQ_DFT_WAIT_CLKS 327805u

#define AD5940_SEQ_WAKEUP_COUNTS 1u
#define AD5940_SEQ_SLEEP_COUNTS 4096u

#define AD5940_AFECON_CAL_RUN (AD5940_AFECON_DACREFEN_MSK | AD5940_AFECON_SINC2EN_MSK | \
			       AD5940_AFECON_DFTEN_MSK | AD5940_AFECON_WAVEGENEN_MSK | \
			       AD5940_AFECON_TIAEN_MSK | AD5940_AFECON_INAMPEN_MSK | \
			       AD5940_AFECON_EXBUFEN_MSK | AD5940_AFECON_ADCCONVEN_MSK | \
			       AD5940_AFECON_ADCEN_MSK | AD5940_AFECON_DACEN_MSK)
#define AD5940_AFECON_CAL_IDLE (AD5940_AFECON_DACREFEN_MSK | AD5940_AFECON_SINC2EN_MSK | \
				AD5940_AFECON_WAVEGENEN_MSK | AD5940_AFECON_TIAEN_MSK | \
				AD5940_AFECON_INAMPEN_MSK | AD5940_AFECON_EXBUFEN_MSK | \
				AD5940_AFECON_DACEN_MSK)
#define AD5940_AFECON_CAL_SETTLE (AD5940_AFECON_CAL_IDLE | AD5940_AFECON_ADCEN_MSK)

#define AD5940_SEQ_ADCMUX_VOLT (AD5940_ADCCON_MUXSELP_AIN2 | \
					(AD5940_ADCCON_MUXSELN_AIN3 << 8u))
#define AD5940_SEQ_ADCMUX_RTIA (AD5940_ADCCON_MUXSELP_HSTIA_P | \
					(AD5940_ADCCON_MUXSELN_HSTIA_N << 8u))

#define AD5940_ADCMUX_WORD(p, n) \
	(FIELD_PREP(AD5940_ADCCON_MUXSELP_MSK, (p)) | \
	 FIELD_PREP(AD5940_ADCCON_MUXSELN_MSK, (n)))

#define AD5940_DT_SEQ_ADCMUX_VOLT \
	AD5940_ADCMUX_WORD(DT_INST_PROP(0, adccon_muxselp_v), DT_INST_PROP(0, adccon_muxseln_v))
#define AD5940_DT_SEQ_ADCMUX_RTIA \
	AD5940_ADCMUX_WORD(DT_INST_PROP(0, adccon_muxselp_i), DT_INST_PROP(0, adccon_muxseln_i))

struct ad5940_fifo_hdr {
	uint8_t  is_fifo;
	uint8_t  data_type;
	uint8_t  req_samples;
	uint32_t int_status;
	uint32_t sample_set_size;
	uint16_t fifo_byte_count;
	uint16_t odr;
	uint64_t timestamp;
	float    rtia_mag_ohms;
	float    rtia_phase_rad;
	float    freq_hz;
};

struct ad5940_eis_config {
	float    freq_hz;
	uint32_t amplitude_mvpp;
	uint8_t  excit_buf_gain;
	uint8_t  ctia_sel;
	uint8_t  dft_num;
	uint8_t  dft_src;
	uint8_t  hsdac_gain;
	uint8_t  hsdac_update_rate;
	uint8_t  rtia_sel;
	uint8_t  power_mode;
	uint8_t  settling_cycles;
	uint8_t  sinc3_osr;
	bool     hanning_win;
};

struct ad5940_sweep_config {
	float    start_freq_hz;
	float    stop_freq_hz;
	uint16_t points;
	bool     log_scale;
	bool     needs_recal;
};

struct ad5940_amper_config {
	uint32_t bias_mv;
	uint8_t  lpamp_rtia;
	uint16_t sinc2_osr;
};

struct ad5940_cv_config {
	int32_t  vertex1_mv;
	int32_t  vertex2_mv;
	int32_t  init_mv;
	int32_t  scan_rate_mv_s;
	uint8_t  cycles;
};

struct ad5940_adc_config {
	uint16_t sinc2_osr;
	uint8_t  pga_gain;
};

struct ad5940_clks_cal_info {
	uint32_t data_type;
	uint32_t dft_src;
	uint32_t data_count;
	uint32_t sinc2_osr;
	uint32_t sinc3_osr;
	uint32_t avg_num;
	float    ratio_sys2adc;
};

struct ad5940_result {
	int32_t  dft_real;
	int32_t  dft_imag;
	float    magnitude_ohm;
	float    phase_rad;
	int32_t  adc_raw;
	int32_t  current_na;
	float    potential_mv;
	int32_t  sinc2;
	int32_t  variance;
	int32_t  mean;
};

#ifdef CONFIG_AD5940_SEQGEN
struct ad5940_seq_reg_info {
	uint16_t reg_addr;
	uint32_t reg_data;
};

struct ad5940_seqgen {
	bool     recording;
	uint32_t *buf;
	uint16_t buf_sz;
	uint16_t seq_len;
	uint16_t reg_count;
	int      last_error;
};
#endif /* CONFIG_AD5940_SEQGEN */

struct ad5940_config {
	struct spi_dt_spec     spi;
	struct gpio_dt_spec    int_gpio;
	struct gpio_dt_spec    reset_gpio;
#ifdef CONFIG_AD5940_STREAM
	struct rtio_iodev     *spi_iodev;
#endif
	uint32_t               rcal_ohms;
	uint8_t                clock_source;
	bool                   hfosc_32mhz;
	uint8_t                int_ad5940_pin;
	bool                   int_ad5940_pull_en;

	uint32_t               def_eis_freq_hz;
	uint32_t               def_eis_amplitude_mvpp;
	uint8_t                def_eis_rtia_sel;
	uint8_t                def_eis_settling_cycles;
	uint8_t                def_eis_dft_num;
	uint8_t                def_eis_dft_src;
	bool                   def_eis_hanning_win;
	uint8_t                def_eis_sinc3_osr;
	uint8_t                def_eis_excit_buf_gain;
	uint8_t                def_eis_hsdac_gain;
	uint8_t                def_eis_hsdac_update_rate;
	uint8_t                def_eis_ctia_sel;
	uint8_t                def_eis_power_mode;
	uint32_t               def_bias_mv;
	uint8_t                def_lpamp_rtia;
	uint32_t               def_sweep_start_hz;
	uint32_t               def_sweep_stop_hz;
	uint16_t               def_sweep_points;
	bool                   def_sweep_log_scale;
	uint32_t               sw_dsw;
	uint32_t               sw_psw;
	uint32_t               sw_nsw;
	uint32_t               sw_tsw;
	uint32_t               sw_swmux;
	uint8_t                adc_mux_v_p;
	uint8_t                adc_mux_v_n;
	uint8_t                adc_mux_i_p;
	uint8_t                adc_mux_i_n;
};

struct ad5940_data {
	enum ad5940_mode       mode;

	struct ad5940_eis_config    eis;
	struct ad5940_sweep_config  sweep;
	struct ad5940_amper_config  amper;
	struct ad5940_cv_config     cv;
	struct ad5940_adc_config    adc_cfg;

	uint16_t               chip_id;
	uint16_t               adi_id;
	uint32_t               odr_hz;

	float                  rtia_cal[2];

	/*
	 * Excitation frequency (Hz) of the most recently emitted stream buffer,
	 * cached when its FIFO header is stamped. Read back via
	 * SENSOR_ATTR_AD5940_EIS_LAST_FREQ_HZ to label a streamed reading with
	 * the frequency that produced it — unlike SENSOR_ATTR_AD5940_EIS_FREQ_HZ,
	 * which returns the NEXT sweep point (the callback advances at its end).
	 */
	float                  last_stream_freq_hz;

#ifdef CONFIG_AD5940_ADC_SELFCAL
	struct {
		bool     valid;
		uint8_t  pga_gain;
		int32_t  offset;
		float    gain;
	} adc_cal;
#endif

#ifdef CONFIG_AD5940_FREQUENCY_SWEEP_ENABLE
	float  rtia_table[CONFIG_AD5940_MAX_SWEEP_POINTS][2];
	uint32_t sweep_current_idx;
	bool sweep_enabled;
#endif

	struct ad5940_result   result;

	bool                   initialized;
	bool                   calibrated;
	uint8_t                fifo_src;

	uint32_t               seq_meas_addr;

#ifdef CONFIG_AD5940_SEQGEN
	struct ad5940_seqgen   seqgen;
	uint32_t               seqgen_buf[CONFIG_AD5940_SEQGEN_BUF_WORDS];
#endif

	uint16_t               seq_slot_addr[4];
	uint16_t               seq_slot_len[4];
	uint16_t               seq_slot_init_addr[4];
	uint16_t               seq_slot_init_len[4];
	uint8_t                seq_slot_fifo[4];
	bool                   seq_slot_loaded[4];
	uint32_t               seq_sram_next;

	struct k_mutex         lock;

	struct gpio_callback   gpio_cb;
	sensor_trigger_handler_t trigger_handler;
	const struct sensor_trigger *trigger;
	const struct device   *dev;

#if defined(CONFIG_AD5940_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_AD5940_THREAD_STACK_SIZE);
	struct k_thread        thread;
	struct k_sem           gpio_sem;
#elif defined(CONFIG_AD5940_TRIGGER_GLOBAL_THREAD)
	struct k_work          work;
#endif

#ifdef CONFIG_AD5940_STREAM
	struct rtio             *rtio_ctx;
	struct rtio_iodev_sqe *active_sqe;
	uint64_t               timestamp_ns;
	bool                   timer_running;
#endif
};

int ad5940_reg_write(const struct device *dev, uint16_t addr, uint32_t val);
int ad5940_reg_read(const struct device *dev, uint16_t addr, uint32_t *val);
int ad5940_fifo_read_words(const struct device *dev, uint32_t *words, uint16_t count);
int ad5940_fifo_flush(const struct device *dev);
int ad5940_spi_unlock(const struct device *dev);
int ad5940_wakeup(const struct device *dev);
int ad5940_enter_sleep(const struct device *dev);
int ad5940_gpio_cfg(const struct device *dev, uint8_t pin, uint8_t func,
		    bool output_en, bool input_en, bool pull_en, bool value);
int ad5940_gpio_set(const struct device *dev, uint8_t pin, bool value);
int ad5940_seq_load(const struct device *dev, enum ad5940_seq_id id,
		    const struct ad5940_seq_cfg *cfg);
int ad5940_seq_start(const struct device *dev, enum ad5940_seq_id id);
int ad5940_seq_stop(const struct device *dev);
int ad5940_seq_trigger(const struct device *dev, enum ad5940_seq_id id);
int ad5940_hw_reset(const struct device *dev);

#ifdef CONFIG_AD5940_FREQUENCY_SWEEP_ENABLE
void ad5940_advance_sweep_freq(const struct device *dev);
#endif

#ifdef CONFIG_SENSOR_ASYNC_API
int ad5940_get_decoder(const struct device *dev,
		       const struct sensor_decoder_api **decoder);
#endif

#ifdef CONFIG_AD5940_TRIGGER
int ad5940_trigger_init(const struct device *dev);
int ad5940_trigger_set(const struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler);
void ad5940_stream_irq_handler(const struct device *dev);
void ad5940_trigger_resubmit(const struct device *dev);
#endif

#ifdef CONFIG_AD5940_STREAM
void ad5940_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);
void ad5940_submit_stream(const struct device *dev,
			  struct rtio_iodev_sqe *iodev_sqe);
void ad5940_stream_stop_timer(const struct device *dev);
void ad5940_stream_prime(const struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_ADI_AD5940_AD5940_H_ */
