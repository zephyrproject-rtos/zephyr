/*
 * Copyright (c) 2019, 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>

#include <nrfx.h>

/*
 * Account for MDK inconsistencies
 */

#if !defined(NRF_CTRLAP) && defined(NRF_CTRL_AP_PERI)
#define NRF_CTRLAP NRF_CTRL_AP_PERI
#endif

#if !defined(NRF_GPIOTE0) && defined(NRF_GPIOTE)
#define NRF_GPIOTE0 NRF_GPIOTE
#endif

#if !defined(NRF_I2S0) && defined(NRF_I2S)
#define NRF_I2S0 NRF_I2S
#endif

#if !defined(NRF_P0) && defined(NRF_GPIO)
#define NRF_P0 NRF_GPIO
#endif

#if !defined(NRF_PDM0) && defined(NRF_PDM)
#define NRF_PDM0 NRF_PDM
#endif

#if !defined(NRF_QDEC0) && defined(NRF_QDEC)
#define NRF_QDEC0 NRF_QDEC
#endif

#if !defined(NRF_RADIO) && defined(NRF_RADIOCORE_RADIO)
#define NRF_RADIO NRF_RADIOCORE_RADIO
#endif

#if !defined(NRF_RTC) && defined(NRF_RADIOCORE_RTC)
#define NRF_RTC NRF_RADIOCORE_RTC
#endif

#if !defined(NRF_SWI0) && defined(NRF_SWI_BASE)
#define NRF_SWI0 ((0 * 0x1000) + NRF_SWI_BASE)
#endif

#if !defined(NRF_SWI1) && defined(NRF_SWI_BASE)
#define NRF_SWI1 ((1 * 0x1000) + NRF_SWI_BASE)
#endif

#if !defined(NRF_SWI2) && defined(NRF_SWI_BASE)
#define NRF_SWI2 ((2 * 0x1000) + NRF_SWI_BASE)
#endif

#if !defined(NRF_SWI3) && defined(NRF_SWI_BASE)
#define NRF_SWI3 ((3 * 0x1000) + NRF_SWI_BASE)
#endif

#if !defined(NRF_SWI4) && defined(NRF_SWI_BASE)
#define NRF_SWI4 ((4 * 0x1000) + NRF_SWI_BASE)
#endif

#if !defined(NRF_SWI5) && defined(NRF_SWI_BASE)
#define NRF_SWI5 ((5 * 0x1000) + NRF_SWI_BASE)
#endif

#if !defined(NRF_WDT0) && defined(NRF_WDT)
#define NRF_WDT0 NRF_WDT
#endif

#if defined(CONFIG_SOC_SERIES_NRF51X) || defined(CONFIG_SOC_SERIES_NRF52X)
#if !defined(NRF_POWER_GPREGRET1) && defined(NRF_POWER_BASE)
#define NRF_POWER_GPREGRET1 (0x51c + NRF_POWER_BASE)
#endif

#if !defined(NRF_POWER_GPREGRET2) && defined(NRF_POWER_BASE)
#define NRF_POWER_GPREGRET2 (0x520 + NRF_POWER_BASE)
#endif
#endif

/**
 * Check that a devicetree node's "reg" base address matches the
 * correct value from the MDK.
 *
 * Node reg values are checked against MDK addresses regardless of
 * their status.
 *
 * Using a node label allows the same file to work with multiple SoCs
 * and devicetree configurations.
 *
 * @param lbl lowercase-and-underscores devicetree node label to check
 * @param mdk_addr expected address from the Nordic MDK.
 */
#define CHECK_DT_REG(lbl, mdk_addr)					\
	BUILD_ASSERT(							\
		UTIL_OR(UTIL_NOT(DT_REG_HAS_IDX(DT_NODELABEL(lbl), 0)),	\
			(DT_REG_ADDR(DT_NODELABEL(lbl)) == (uint32_t)(mdk_addr))))

/**
 * If a node label "lbl" might have different addresses depending on
 * its compatible "compat", you can use this macro to pick the right
 * one.
 *
 * @param lbl lowercase-and-underscores devicetree node label to check
 * @param compat lowercase-and-underscores compatible to check
 * @param addr_if_match MDK address to return if "lbl" has compatible "compat"
 * @param addr_if_no_match MDK address to return otherwise
 */
#define NODE_ADDRESS(lbl, compat, addr_if_match, addr_if_no_match)	\
	COND_CODE_1(DT_NODE_HAS_COMPAT(DT_NODELABEL(lbl), compat),	\
		    (addr_if_match), (addr_if_no_match))

#define CHECK_SPI_REG(lbl, num)						\
	CHECK_DT_REG(lbl,						\
		NODE_ADDRESS(lbl, nordic_nrf_spi, NRF_SPI##num,	\
		NODE_ADDRESS(lbl, nordic_nrf_spim, NRF_SPIM##num,	\
			     NRF_SPIS##num)))

#define CHECK_I2C_REG(lbl, num)						\
	CHECK_DT_REG(lbl,						\
		NODE_ADDRESS(lbl, nordic_nrf_twi, NRF_TWI##num,		\
		NODE_ADDRESS(lbl, nordic_nrf_twim, NRF_TWIM##num,	\
			     NRF_TWIS##num)))

#define CHECK_UART_REG(lbl, num)					\
	CHECK_DT_REG(lbl,						\
		NODE_ADDRESS(lbl, nordic_nrf_uart, NRF_UART##num,	\
			     NRF_UARTE##num))

CHECK_DT_REG(acl, NRF_ACL);
CHECK_DT_REG(adc, NODE_ADDRESS(adc, nordic_nrf_adc, NRF_ADC, NRF_SAADC));
CHECK_DT_REG(cpusec_bellboard, NRF_SECDOMBELLBOARD);
CHECK_DT_REG(cpuapp_bellboard, NRF_APPLICATION_BELLBOARD);
CHECK_DT_REG(cpurad_bellboard, NRF_RADIOCORE_BELLBOARD);
CHECK_DT_REG(bprot, NRF_BPROT);
CHECK_DT_REG(ccm, NRF_CCM);
CHECK_DT_REG(ccm030, NRF_RADIOCORE_CCM030);
CHECK_DT_REG(ccm031, NRF_RADIOCORE_CCM031);
CHECK_DT_REG(clock, NRF_CLOCK);
CHECK_DT_REG(comp, NODE_ADDRESS(comp, nordic_nrf_comp, NRF_COMP, NRF_LPCOMP));
CHECK_DT_REG(cryptocell, NRF_CRYPTOCELL);
CHECK_DT_REG(ctrlap, NRF_CTRLAP);
CHECK_DT_REG(dcnf, NRF_DCNF);
CHECK_DT_REG(dppic, NRF_DPPIC);
CHECK_DT_REG(dppic00, NRF_DPPIC00);
CHECK_DT_REG(dppic10, NRF_DPPIC10);
CHECK_DT_REG(dppic20, NRF_DPPIC20);
CHECK_DT_REG(dppic30, NRF_DPPIC30);
CHECK_DT_REG(dppic020, NRF_RADIOCORE_DPPIC020);
CHECK_DT_REG(dppic120, NRF_DPPIC120);
CHECK_DT_REG(dppic130, NRF_DPPIC130);
CHECK_DT_REG(dppic131, NRF_DPPIC131);
CHECK_DT_REG(dppic132, NRF_DPPIC132);
CHECK_DT_REG(dppic133, NRF_DPPIC133);
CHECK_DT_REG(dppic134, NRF_DPPIC134);
CHECK_DT_REG(dppic135, NRF_DPPIC135);
CHECK_DT_REG(dppic136, NRF_DPPIC136);
CHECK_DT_REG(ecb, NRF_ECB);
CHECK_DT_REG(ecb020, NRF_ECB020);
CHECK_DT_REG(ecb030, NRF_RADIOCORE_ECB030);
CHECK_DT_REG(ecb031, NRF_RADIOCORE_ECB031);
CHECK_DT_REG(egu0, NRF_EGU0);
CHECK_DT_REG(egu1, NRF_EGU1);
CHECK_DT_REG(egu2, NRF_EGU2);
CHECK_DT_REG(egu3, NRF_EGU3);
CHECK_DT_REG(egu4, NRF_EGU4);
CHECK_DT_REG(egu5, NRF_EGU5);
CHECK_DT_REG(egu10, NRF_EGU10);
CHECK_DT_REG(egu20, NRF_EGU20);
CHECK_DT_REG(egu020, NRF_RADIOCORE_EGU020);
CHECK_DT_REG(ficr, NRF_FICR);
CHECK_DT_REG(flash_controller, NRF_NVMC);
CHECK_DT_REG(gpio0, NRF_P0);
CHECK_DT_REG(gpio1, NRF_P1);
CHECK_DT_REG(gpio2, NRF_P2);
CHECK_DT_REG(gpio6, NRF_P6);
CHECK_DT_REG(gpio7, NRF_P7);
CHECK_DT_REG(gpio9, NRF_P9);
CHECK_DT_REG(gpiote, NRF_GPIOTE);
CHECK_DT_REG(gpiote0, NRF_GPIOTE0);
CHECK_DT_REG(gpiote1, NRF_GPIOTE1);
CHECK_DT_REG(gpiote20, NRF_GPIOTE20);
CHECK_DT_REG(gpiote30, NRF_GPIOTE30);
CHECK_DT_REG(gpiote130, NRF_GPIOTE130);
CHECK_DT_REG(gpiote131, NRF_GPIOTE131);
CHECK_DT_REG(grtc, NRF_GRTC);
CHECK_DT_REG(cpuapp_hsfll, NRF_APPLICATION_HSFLL);
CHECK_DT_REG(cpurad_hsfll, NRF_RADIOCORE_HSFLL);
CHECK_I2C_REG(i2c0, 0);
CHECK_I2C_REG(i2c1, 1);
CHECK_DT_REG(i2c2, NRF_TWIM2);
CHECK_DT_REG(i2c3, NRF_TWIM3);
CHECK_DT_REG(i2c20, NRF_TWIM20);
CHECK_DT_REG(i2c21, NRF_TWIM21);
CHECK_DT_REG(i2c22, NRF_TWIM22);
CHECK_DT_REG(i2c30, NRF_TWIM30);
CHECK_DT_REG(i2c130, NRF_TWIM130);
CHECK_DT_REG(i2c131, NRF_TWIM131);
CHECK_DT_REG(i2c132, NRF_TWIM132);
CHECK_DT_REG(i2c133, NRF_TWIM133);
CHECK_DT_REG(i2c134, NRF_TWIM134);
CHECK_DT_REG(i2c135, NRF_TWIM135);
CHECK_DT_REG(i2c136, NRF_TWIM136);
CHECK_DT_REG(i2c137, NRF_TWIM137);
CHECK_DT_REG(i2s0, NRF_I2S0);
CHECK_DT_REG(i2s20, NRF_I2S20);
CHECK_DT_REG(ipc, NRF_IPC);
CHECK_DT_REG(cpuapp_ipct, NRF_APPLICATION_IPCT);
CHECK_DT_REG(cpurad_ipct, NRF_RADIOCORE_IPCT);
CHECK_DT_REG(ipct120, NRF_IPCT120);
CHECK_DT_REG(ipct130, NRF_IPCT130);
CHECK_DT_REG(kmu, NRF_KMU);
CHECK_DT_REG(mutex, NRF_MUTEX);
CHECK_DT_REG(mwu, NRF_MWU);
CHECK_DT_REG(nfct, NRF_NFCT);
CHECK_DT_REG(nrf_mpu, NRF_MPU);
CHECK_DT_REG(oscillators, NRF_OSCILLATORS);
CHECK_DT_REG(pdm0, NRF_PDM0);
CHECK_DT_REG(power, NRF_POWER);
CHECK_DT_REG(ppi, NRF_PPI);
CHECK_DT_REG(pwm0, NRF_PWM0);
CHECK_DT_REG(pwm1, NRF_PWM1);
CHECK_DT_REG(pwm2, NRF_PWM2);
CHECK_DT_REG(pwm3, NRF_PWM3);
CHECK_DT_REG(pwm20, NRF_PWM20);
CHECK_DT_REG(pwm21, NRF_PWM21);
CHECK_DT_REG(pwm22, NRF_PWM22);
CHECK_DT_REG(pwm120, NRF_PWM120);
CHECK_DT_REG(pwm130, NRF_PWM130);
CHECK_DT_REG(pwm131, NRF_PWM131);
CHECK_DT_REG(pwm132, NRF_PWM132);
CHECK_DT_REG(pwm133, NRF_PWM133);
CHECK_DT_REG(qdec, NRF_QDEC0);	/* this should be the same node as qdec0 */
CHECK_DT_REG(qdec0, NRF_QDEC0);
CHECK_DT_REG(qdec1, NRF_QDEC1);
CHECK_DT_REG(qdec20, NRF_QDEC20);
CHECK_DT_REG(qdec21, NRF_QDEC21);
CHECK_DT_REG(qdec130, NRF_QDEC130);
CHECK_DT_REG(qdec131, NRF_QDEC131);
CHECK_DT_REG(radio, NRF_RADIO);
CHECK_DT_REG(regulators, NRF_REGULATORS);
CHECK_DT_REG(reset, NRF_RESET);
CHECK_DT_REG(cpuapp_resetinfo, NRF_APPLICATION_RESETINFO);
CHECK_DT_REG(cpurad_resetinfo, NRF_RADIOCORE_RESETINFO);
CHECK_DT_REG(rng, NRF_RNG);
CHECK_DT_REG(rtc, NRF_RTC);
CHECK_DT_REG(rtc0, NRF_RTC0);
CHECK_DT_REG(rtc1, NRF_RTC1);
CHECK_DT_REG(rtc2, NRF_RTC2);
CHECK_DT_REG(rtc130, NRF_RTC130);
CHECK_DT_REG(rtc131, NRF_RTC131);
CHECK_SPI_REG(spi0, 0);
CHECK_SPI_REG(spi1, 1);
CHECK_SPI_REG(spi2, 2);
CHECK_DT_REG(spi3, NRF_SPIM3);
CHECK_DT_REG(spi4, NRF_SPIM4);
CHECK_DT_REG(spi00, NRF_SPIM00);
CHECK_DT_REG(spi20, NRF_SPIM20);
CHECK_DT_REG(spi21, NRF_SPIM21);
CHECK_DT_REG(spi22, NRF_SPIM22);
CHECK_DT_REG(spi30, NRF_SPIM30);
CHECK_DT_REG(spi120, NRF_SPIM120);
CHECK_DT_REG(spi121, NRF_SPIM121);
CHECK_DT_REG(spi130, NRF_SPIM130);
CHECK_DT_REG(spi131, NRF_SPIM131);
CHECK_DT_REG(spi132, NRF_SPIM132);
CHECK_DT_REG(spi133, NRF_SPIM133);
CHECK_DT_REG(spi134, NRF_SPIM134);
CHECK_DT_REG(spi135, NRF_SPIM135);
CHECK_DT_REG(spi136, NRF_SPIM136);
CHECK_DT_REG(spi137, NRF_SPIM137);
CHECK_DT_REG(spu, NRF_SPU);
CHECK_DT_REG(swi0, NRF_SWI0);
CHECK_DT_REG(swi1, NRF_SWI1);
CHECK_DT_REG(swi2, NRF_SWI2);
CHECK_DT_REG(swi3, NRF_SWI3);
CHECK_DT_REG(swi4, NRF_SWI4);
CHECK_DT_REG(swi5, NRF_SWI5);
CHECK_DT_REG(temp, NRF_TEMP);
CHECK_DT_REG(timer0, NRF_TIMER0);
CHECK_DT_REG(timer1, NRF_TIMER1);
CHECK_DT_REG(timer2, NRF_TIMER2);
CHECK_DT_REG(timer3, NRF_TIMER3);
CHECK_DT_REG(timer4, NRF_TIMER4);
CHECK_DT_REG(timer00, NRF_TIMER00);
CHECK_DT_REG(timer10, NRF_TIMER10);
CHECK_DT_REG(timer20, NRF_TIMER20);
CHECK_DT_REG(timer21, NRF_TIMER21);
CHECK_DT_REG(timer22, NRF_TIMER22);
CHECK_DT_REG(timer23, NRF_TIMER23);
CHECK_DT_REG(timer24, NRF_TIMER24);
CHECK_DT_REG(timer020, NRF_RADIOCORE_TIMER020);
CHECK_DT_REG(timer021, NRF_RADIOCORE_TIMER021);
CHECK_DT_REG(timer022, NRF_RADIOCORE_TIMER022);
CHECK_DT_REG(timer120, NRF_TIMER120);
CHECK_DT_REG(timer121, NRF_TIMER121);
CHECK_DT_REG(timer130, NRF_TIMER130);
CHECK_DT_REG(timer131, NRF_TIMER131);
CHECK_DT_REG(timer132, NRF_TIMER132);
CHECK_DT_REG(timer133, NRF_TIMER133);
CHECK_DT_REG(timer134, NRF_TIMER134);
CHECK_DT_REG(timer135, NRF_TIMER135);
CHECK_DT_REG(timer136, NRF_TIMER136);
CHECK_DT_REG(timer137, NRF_TIMER137);
CHECK_UART_REG(uart0, 0);
CHECK_DT_REG(uart1, NRF_UARTE1);
CHECK_DT_REG(uart2, NRF_UARTE2);
CHECK_DT_REG(uart3, NRF_UARTE3);
CHECK_DT_REG(uart00, NRF_UARTE00);
CHECK_DT_REG(uart20, NRF_UARTE20);
CHECK_DT_REG(uart21, NRF_UARTE21);
CHECK_DT_REG(uart22, NRF_UARTE22);
CHECK_DT_REG(uart30, NRF_UARTE30);
CHECK_DT_REG(uart120, NRF_UARTE120);
CHECK_DT_REG(uart130, NRF_UARTE130);
CHECK_DT_REG(uart131, NRF_UARTE131);
CHECK_DT_REG(uart132, NRF_UARTE132);
CHECK_DT_REG(uart133, NRF_UARTE133);
CHECK_DT_REG(uart134, NRF_UARTE134);
CHECK_DT_REG(uart135, NRF_UARTE135);
CHECK_DT_REG(uart136, NRF_UARTE136);
CHECK_DT_REG(uart137, NRF_UARTE137);
CHECK_DT_REG(uicr, NRF_UICR);
CHECK_DT_REG(cpuapp_uicr, NRF_APPLICATION_UICR);
CHECK_DT_REG(cpurad_uicr, NRF_RADIOCORE_UICR);
CHECK_DT_REG(usbd, NRF_USBD);
CHECK_DT_REG(usbhs, NRF_USBHS);
CHECK_DT_REG(usbhs_core, NRF_USBHSCORE0);
CHECK_DT_REG(usbreg, NRF_USBREGULATOR);
CHECK_DT_REG(vmc, NRF_VMC);
CHECK_DT_REG(cpuflpr_clic, NRF_FLPR_VPRCLIC);
#if defined(CONFIG_SOC_NRF54L15)
CHECK_DT_REG(cpuflpr_vpr, NRF_VPR00);
#elif defined(CONFIG_SOC_NRF54H20)
CHECK_DT_REG(cpuflpr_vpr, NRF_VPR121);
CHECK_DT_REG(cpuppr_vpr, NRF_VPR130);
#endif
CHECK_DT_REG(wdt, NRF_WDT0);	/* this should be the same node as wdt0 */
CHECK_DT_REG(wdt0, NRF_WDT0);
CHECK_DT_REG(wdt1, NRF_WDT1);
CHECK_DT_REG(wdt30, NRF_WDT30);
CHECK_DT_REG(wdt31, NRF_WDT31);
CHECK_DT_REG(cpuapp_wdt010, NRF_APPLICATION_WDT010);
CHECK_DT_REG(cpuapp_wdt011, NRF_APPLICATION_WDT011);
CHECK_DT_REG(cpurad_wdt010, NRF_RADIOCORE_WDT010);
CHECK_DT_REG(cpurad_wdt011, NRF_RADIOCORE_WDT011);
CHECK_DT_REG(wdt131, NRF_WDT131);
CHECK_DT_REG(wdt132, NRF_WDT132);

/* nRF51/nRF52-specific addresses */
#if defined(CONFIG_SOC_SERIES_NRF51X) || defined(CONFIG_SOC_SERIES_NRF52X)
CHECK_DT_REG(gpregret1, NRF_POWER_GPREGRET1);
CHECK_DT_REG(gpregret2, NRF_POWER_GPREGRET2);
#endif
