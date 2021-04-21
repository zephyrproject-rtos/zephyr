/*
 * Copyright (c) 2020 Jefferson Lee.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <arduino_nano_33_ble.h>

/*
 * this method roughly follows the steps here:
 * https://github.com/arduino/ArduinoCore-nRF528x-mbedos/blob/6216632cc70271619ad43547c804dabb4afa4a00/variants/ARDUINO_NANO33BLE/variant.cpp#L136
 */

static int board_internal_sensors_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	struct arduino_gpio_t gpios;

	arduino_gpio_init(&gpios);

	arduino_gpio_pinMode(&gpios, ARDUINO_LEDPWR, GPIO_OUTPUT);
	arduino_gpio_digitalWrite(&gpios, ARDUINO_LEDPWR, 1);

	CoreDebug->DEMCR = 0;
	NRF_CLOCK->TRACECONFIG = 0;

	/*
	 * Arduino uses software to disable RTC1,
	 * but I disabled it using DeviceTree
	 */
	/* nrf_rtc_event_disable(NRF_RTC1, NRF_RTC_INT_COMPARE0_MASK); */
	/* nrf_rtc_int_disable(NRF_RTC1, NRF_RTC_INT_COMPARE0_MASK); */

	NRF_PWM_Type * PWM[] = {
		NRF_PWM0, NRF_PWM1, NRF_PWM2, NRF_PWM3
	};

	for (unsigned int i = 0; i < (ARRAY_SIZE(PWM)); i++) {
		PWM[i]->ENABLE = 0;
		PWM[i]->PSEL.OUT[0] = 0xFFFFFFFFUL;
	}

	/*
	 * the PCB designers decided to use GPIO's
	 * as power pins for the internal sensors
	 */
	arduino_gpio_pinMode(&gpios, ARDUINO_INTERNAL_VDD_ENV_ENABLE, GPIO_OUTPUT);
	arduino_gpio_pinMode(&gpios, ARDUINO_INTERNAL_I2C_PULLUP, GPIO_OUTPUT);
	arduino_gpio_digitalWrite(&gpios, ARDUINO_INTERNAL_VDD_ENV_ENABLE, 1);
	arduino_gpio_digitalWrite(&gpios, ARDUINO_INTERNAL_I2C_PULLUP, 1);
	return 0;
}
SYS_INIT(board_internal_sensors_init, PRE_KERNEL_1, 32);
