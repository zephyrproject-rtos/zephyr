/*
 * Copyright (c) 2019, 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/devicetree.h>

/*
 * Account for MDK inconsistencies
 */

#if !defined(NRF_CTRLAP) && defined(NRF_CTRL_AP_PERI)
#define NRF_CTRLAP NRF_CTRL_AP_PERI
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
		UTIL_OR(UTIL_NOT(DT_NODE_EXISTS(DT_NODELABEL(lbl))),	\
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
CHECK_DT_REG(bprot, NRF_BPROT);
CHECK_DT_REG(ccm, NRF_CCM);
CHECK_DT_REG(clock, NRF_CLOCK);
CHECK_DT_REG(comp, NODE_ADDRESS(comp, nordic_nrf_comp, NRF_COMP, NRF_LPCOMP));
CHECK_DT_REG(cryptocell, NRF_CRYPTOCELL);
CHECK_DT_REG(ctrlap, NRF_CTRLAP);
CHECK_DT_REG(dcnf, NRF_DCNF);
CHECK_DT_REG(dppic, NRF_DPPIC);
CHECK_DT_REG(ecb, NRF_ECB);
CHECK_DT_REG(egu0, NRF_EGU0);
CHECK_DT_REG(egu1, NRF_EGU1);
CHECK_DT_REG(egu2, NRF_EGU2);
CHECK_DT_REG(egu3, NRF_EGU3);
CHECK_DT_REG(egu4, NRF_EGU4);
CHECK_DT_REG(egu5, NRF_EGU5);
CHECK_DT_REG(ficr, NRF_FICR);
CHECK_DT_REG(flash_controller, NRF_NVMC);
CHECK_DT_REG(gpio0, NRF_P0);
CHECK_DT_REG(gpio1, NRF_P1);
CHECK_DT_REG(gpiote, NRF_GPIOTE);
CHECK_I2C_REG(i2c0, 0);
CHECK_I2C_REG(i2c1, 1);
CHECK_DT_REG(i2c2, NRF_TWIM2);
CHECK_DT_REG(i2c3, NRF_TWIM3);
CHECK_DT_REG(i2s0, NRF_I2S0);
CHECK_DT_REG(ipc, NRF_IPC);
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
CHECK_DT_REG(qdec, NRF_QDEC0);	/* this should be the same node as qdec0 */
CHECK_DT_REG(qdec0, NRF_QDEC0);
CHECK_DT_REG(qdec1, NRF_QDEC1);
CHECK_DT_REG(radio, NRF_RADIO);
CHECK_DT_REG(regulators, NRF_REGULATORS);
CHECK_DT_REG(reset, NRF_RESET);
CHECK_DT_REG(rng, NRF_RNG);
CHECK_DT_REG(rtc0, NRF_RTC0);
CHECK_DT_REG(rtc1, NRF_RTC1);
CHECK_DT_REG(rtc2, NRF_RTC2);
CHECK_SPI_REG(spi0, 0);
CHECK_SPI_REG(spi1, 1);
CHECK_SPI_REG(spi2, 2);
CHECK_DT_REG(spi3, NRF_SPIM3);
CHECK_DT_REG(spi4, NRF_SPIM4);
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
CHECK_UART_REG(uart0, 0);
CHECK_DT_REG(uart1, NRF_UARTE1);
CHECK_DT_REG(uart2, NRF_UARTE2);
CHECK_DT_REG(uart3, NRF_UARTE3);
CHECK_DT_REG(uicr, NRF_UICR);
CHECK_DT_REG(usbd, NRF_USBD);
CHECK_DT_REG(usbreg, NRF_USBREGULATOR);
CHECK_DT_REG(vmc, NRF_VMC);
CHECK_DT_REG(wdt, NRF_WDT0);	/* this should be the same node as wdt0 */
CHECK_DT_REG(wdt0, NRF_WDT0);
CHECK_DT_REG(wdt1, NRF_WDT1);
