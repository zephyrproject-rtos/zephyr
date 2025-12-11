/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_system.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(stm32_usb_pwr, CONFIG_STM32_USB_COMMON_LOG_LEVEL);

/*
 * Keep track of whether power is already
 * enabled here to simplify the USB drivers.
 */
static atomic_t usb_pwr_refcount = ATOMIC_INIT(0);

int stm32_usb_pwr_enable(void)
{
	atomic_val_t old = atomic_inc(&usb_pwr_refcount);

	if (old > 0) {
		/* Already enabled - nothing to do */
		return 0;
	}

#if defined(CONFIG_SOC_SERIES_STM32H7X)
	LL_PWR_EnableUSBVoltageDetector();

	/* Per AN2606: USBREGEN not supported when running in FS mode. */
	LL_PWR_DisableUSBReg();
	while (!LL_PWR_IsActiveFlag_USB()) {
		LOG_INF("PWR not active yet");
		k_msleep(100);
	}
#elif defined(CONFIG_SOC_SERIES_STM32U5X)
	/* Sequence to enable the power of the OTG HS on a stm32U5 serie : Enable VDDUSB */
	__ASSERT_NO_MSG(LL_AHB3_GRP1_IsEnabledClock(LL_AHB3_GRP1_PERIPH_PWR));

	/* Check that power range is 1 or 2 */
	if (LL_PWR_GetRegulVoltageScaling() < LL_PWR_REGU_VOLTAGE_SCALE2) {
		LOG_ERR("Wrong Power range to use USB OTG HS");
		return -EIO;
	}

	LL_PWR_EnableVddUSB();

	#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otghs)
		/* Configure VOSR register of USB HSTransceiverSupply(); */
		LL_PWR_EnableUSBPowerSupply();
		LL_PWR_EnableUSBEPODBooster();
		while (LL_PWR_IsActiveFlag_USBBOOST() != 1) {
			/* Wait for USB EPOD BOOST ready */
		}
	#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otghs) */
#elif defined(CONFIG_SOC_SERIES_STM32N6X)
	/* Enable Vdd33USB voltage monitoring */
	LL_PWR_EnableVddUSBMonitoring();
	while (!LL_PWR_IsActiveFlag_USB33RDY()) {
		/* Wait for Vdd33USB ready */
	}

	/* Enable VDDUSB */
	LL_PWR_EnableVddUSB();
#elif defined(CONFIG_SOC_SERIES_STM32WBAX)
	/* Remove VDDUSB power isolation */
	LL_PWR_EnableVddUSB();

	/* Make sure that voltage scaling is Range 1 */
	__ASSERT_NO_MSG(LL_PWR_GetRegulCurrentVOS() == LL_PWR_REGU_VOLTAGE_SCALE1);

	/* Enable VDD11USB */
	LL_PWR_EnableVdd11USB();

	/* Enable USB OTG internal power */
	LL_PWR_EnableUSBPWR();

	while (!LL_PWR_IsActiveFlag_VDD11USBRDY()) {
		/* Wait for VDD11USB supply to be ready */
	}

	/* Enable USB OTG booster */
	LL_PWR_EnableUSBBooster();

	while (!LL_PWR_IsActiveFlag_USBBOOSTRDY()) {
		/* Wait for USB OTG booster to be ready */
	}
#elif defined(PWR_USBSCR_USB33SV) || defined(PWR_SVMCR_USV)
	/*
	 * VDDUSB independent USB supply (PWR clock is on)
	 * with LL_PWR_EnableVDDUSB function (higher case)
	 */
	LL_PWR_EnableVDDUSB();
#elif defined(PWR_CR2_USV)
	/*cd
	 * Required for at least STM32L4 devices as they electrically
	 * isolate USB features from VDDUSB. It must be enabled before
	 * USB can function. Refer to section 5.1.3 in DM00083560 or
	 * DM00310109.
	 */
	LL_PWR_EnableVddUSB();
#endif
	return 0;
}

int stm32_usb_pwr_disable(void)
{
	atomic_val_t old = atomic_dec(&usb_pwr_refcount);

	if (old > 1) {
		/* There are other users - don't disable now */
		return 0;
	}

#if defined(CONFIG_SOC_SERIES_STM32H7X)
	LL_PWR_DisableUSBVoltageDetector();
#elif defined(CONFIG_SOC_SERIES_STM32U5X)
	#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otghs)
		LL_PWR_DisableUSBEPODBooster();
		while (LL_PWR_IsActiveFlag_USBBOOST() != 0) {
			/* Wait for USB EPOD BOOST off */
		}

		LL_PWR_DisableUSBPowerSupply();
	#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otghs) */

	LL_PWR_DisableVddUSB();
#elif defined(CONFIG_SOC_SERIES_STM32N6X)
	/* Disable Vdd33USB voltage monitoring */
	LL_PWR_DisableVddUSBMonitoring();

	/* Disable VDDUSB */
	LL_PWR_DisableVddUSB();
#elif defined(CONFIG_SOC_SERIES_STM32WBAX)
	/* Disable USB OTG booster */
	LL_PWR_DisableUSBBooster();

	while (LL_PWR_IsActiveFlag_USBBOOSTRDY()) {
		/* Wait until USB OTG booster is off */
	}

	/* Disable USB OTG internal power */
	LL_PWR_DisableUSBPWR();

	/* Disable VDD11USB */
	LL_PWR_DisableVdd11USB();

	while (LL_PWR_IsActiveFlag_VDD11USBRDY()) {
		/* Wait until VDD11USB supply is off */
	}

	/* Enable VDDUSB power isolation */
	LL_PWR_DisableVddUSB();
#elif defined(PWR_USBSCR_USB33SV) || defined(PWR_SVMCR_USV)
	/* Enable VDDUSB power isolation */
	LL_PWR_DisableVDDUSB();
#elif defined(PWR_CR2_USV)
	/* Enable VDDUSB power isolation */
	LL_PWR_DisableVddUSB();
#endif

	return 0;
}
