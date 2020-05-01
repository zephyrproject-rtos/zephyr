/*
 * Copyright (c) 2016-2017 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The BLE SW link layer defines 10 GPIO pins for debug related
 * information.
 *
 * Mapping:
 *
 * DIO29 : DEBUG_CPU_SLEEP
 * DIO4  : DEBUG_TICKER_ISR, DEBUG_TICKER_TASK
 * DIO21 : DEBUG_TICKER_JOB
 * DIO27 : DEBUG_RADIO_PREPARE_A, DEBUG_RADIO_CLOSE_A
 * DIO24 : DEBUG_RADIO_PREPARE_S, DEBUG_RADIO_START_S, DEBUG_RADIO_CLOSE_S
 * DIO22 : DEBUG_RADIO_PREPARE_O, DEBUG_RADIO_START_O, DEBUG_RADIO_CLOSE_O
 * DIO28 : DEBUG_RADIO_PREPARE_M, DEBUG_RADIO_START_M, DEBUG_RADIO_CLOSE_M
 * DIO23 : DEBUG_RADIO_ISR
 * DIO5  : DEBUG_RADIO_RX (This is driven by the RF core, not software)
 * DIO30 : DEBUG_RADIO_TX (This is driven by the RF core, not software)
 *
 */
#ifdef CONFIG_BT_CTLR_DEBUG_PINS

#include <drivers/gpio.h>
#include <driverlib/gpio.h>
#include <driverlib/ioc.h>
#include <driverlib/rfc.h>
#include <inc/hw_rfc_dbell.h>

#define DEBUG0_PIN IOID_29
#define DEBUG1_PIN IOID_4
#define DEBUG2_PIN IOID_21
#define DEBUG3_PIN IOID_27
#define DEBUG4_PIN IOID_24 /* BLE TX Window */
#define DEBUG5_PIN IOID_22
#define DEBUG6_PIN IOID_28
#define DEBUG7_PIN IOID_23 /* ISR */
#define DEBUG8_PIN IOID_5  /* RX */
#define DEBUG9_PIN IOID_30 /* TX */

extern struct device *ti_ble_debug_port;

#define GPIO_setClearDio(pin, set)                                     \
	do {                                                               \
		if (set) {                                                     \
			GPIO_setDio(pin);                                          \
		} else {                                                       \
			GPIO_clearDio(pin);                                        \
		}	                                                           \
	} while (0)

#define DEBUG_INIT()                                                   \
	do {                                                               \
		/* There is only 1 gpio controller on the cc1352r */           \
		ti_ble_debug_port =                                            \
			device_get_binding(DT_ALIAS_LED0_GPIOS_CONTROLLER);        \
		gpio_pin_configure(ti_ble_debug_port, DEBUG0_PIN,              \
				   GPIO_DIR_OUT);                                      \
		gpio_pin_configure(ti_ble_debug_port, DEBUG1_PIN,              \
				   GPIO_DIR_OUT);                                      \
		gpio_pin_configure(ti_ble_debug_port, DEBUG2_PIN,              \
				   GPIO_DIR_OUT);                                      \
		gpio_pin_configure(ti_ble_debug_port, DEBUG3_PIN,              \
				   GPIO_DIR_OUT);                                      \
		gpio_pin_configure(ti_ble_debug_port, DEBUG4_PIN,              \
				   GPIO_DIR_OUT);                                      \
		gpio_pin_configure(ti_ble_debug_port, DEBUG5_PIN,              \
				   GPIO_DIR_OUT);                                      \
		gpio_pin_configure(ti_ble_debug_port, DEBUG6_PIN,              \
				   GPIO_DIR_OUT);                                      \
		gpio_pin_configure(ti_ble_debug_port, DEBUG7_PIN,              \
				   GPIO_DIR_OUT);                                      \
		gpio_pin_configure(ti_ble_debug_port, DEBUG8_PIN,              \
				   GPIO_DIR_OUT);                                      \
		gpio_pin_configure(ti_ble_debug_port, DEBUG9_PIN,              \
				   GPIO_DIR_OUT);                                      \
		GPIO_setClearDio(DEBUG0_PIN, 0);                               \
		GPIO_setClearDio(DEBUG1_PIN, 0);                               \
		GPIO_setClearDio(DEBUG2_PIN, 0);                               \
		GPIO_setClearDio(DEBUG3_PIN, 0);                               \
		GPIO_setClearDio(DEBUG4_PIN, 0);                               \
		GPIO_setClearDio(DEBUG5_PIN, 0);                               \
		GPIO_setClearDio(DEBUG6_PIN, 0);                               \
		GPIO_setClearDio(DEBUG7_PIN, 0);                               \
        GPIO_setClearDio(DEBUG8_PIN, 0);                               \
        GPIO_setClearDio(DEBUG9_PIN, 0);                               \
        IOCPortConfigureSet(DEBUG8_PIN, IOC_PORT_RFC_GPO0,             \
                            IOC_STD_OUTPUT);                           \
        IOCPortConfigureSet(DEBUG4_PIN, IOC_PORT_RFC_GPO2,             \
                            IOC_STD_OUTPUT);                           \
        IOCPortConfigureSet(DEBUG9_PIN, IOC_PORT_RFC_GPO3,             \
                            IOC_STD_OUTPUT);                           \
    } while (0)

#define DEBUG_CPU_SLEEP(flag) GPIO_setClearDio(DEBUG0_PIN, flag)

#define DEBUG_TICKER_ISR(flag) GPIO_setClearDio(DEBUG1_PIN, flag)

#define DEBUG_TICKER_TASK(flag) GPIO_setClearDio(DEBUG1_PIN, flag)

#define DEBUG_TICKER_JOB(flag) GPIO_setClearDio(DEBUG2_PIN, flag)

#define DEBUG_RADIO_ISR(flag) GPIO_setClearDio(DEBUG7_PIN, flag)

#define DEBUG_RADIO_XTAL(flag) GPIO_setClearDio(DEBUG8_PIN, flag)

#define DEBUG_RADIO_ACTIVE(flag) GPIO_setClearDio(DEBUG9_PIN, flag)

#define DEBUG_RADIO_CLOSE(flag)                                        \
	do {                                                               \
		if (!flag) {                                                   \
			GPIO_setClearDio(DEBUG3_PIN, flag);                        \
			GPIO_setClearDio(DEBUG4_PIN, flag);                        \
			GPIO_setClearDio(DEBUG5_PIN, flag);                        \
			GPIO_setClearDio(DEBUG6_PIN, flag);                        \
		}                                                              \
	} while (0)

#define DEBUG_RADIO_PREPARE_A(flag) GPIO_setClearDio(DEBUG3_PIN, flag)

#define DEBUG_RADIO_START_A(flag) GPIO_setClearDio(DEBUG3_PIN, flag)

#define DEBUG_RADIO_CLOSE_A(flag) GPIO_setClearDio(DEBUG3_PIN, flag)

#if 0
#define DEBUG_RADIO_PREPARE_S(flag) GPIO_setClearDio(DEBUG4_PIN, flag)
#define DEBUG_RADIO_START_S(flag) GPIO_setClearDio(DEBUG4_PIN, flag)
#define DEBUG_RADIO_CLOSE_S(flag) GPIO_setClearDio(DEBUG4_PIN, flag)
#else
#define DEBUG_RADIO_PREPARE_S(flag)
#define DEBUG_RADIO_START_S(flag)
#define DEBUG_RADIO_CLOSE_S(flag)
#endif

#define DEBUG_RADIO_PREPARE_O(flag) GPIO_setClearDio(DEBUG5_PIN, flag)

#define DEBUG_RADIO_START_O(flag) GPIO_setClearDio(DEBUG5_PIN, flag)

#define DEBUG_RADIO_CLOSE_O(flag) GPIO_setClearDio(DEBUG5_PIN, flag)

#define DEBUG_RADIO_PREPARE_M(flag) GPIO_setClearDio(DEBUG6_PIN, flag)

#define DEBUG_RADIO_START_M(flag) GPIO_setClearDio(DEBUG6_PIN, flag)

#define DEBUG_RADIO_CLOSE_M(flag) GPIO_setClearDio(DEBUG6_PIN, flag)

#else

#define DEBUG_INIT()
#define DEBUG_CPU_SLEEP(flag)
#define DEBUG_TICKER_ISR(flag)
#define DEBUG_TICKER_TASK(flag)
#define DEBUG_TICKER_JOB(flag)
#define DEBUG_RADIO_ISR(flag)
#define DEBUG_RADIO_HCTO(flag)
#define DEBUG_RADIO_XTAL(flag)
#define DEBUG_RADIO_ACTIVE(flag)
#define DEBUG_RADIO_CLOSE(flag)
#define DEBUG_RADIO_PREPARE_A(flag)
#define DEBUG_RADIO_START_A(flag)
#define DEBUG_RADIO_CLOSE_A(flag)
#define DEBUG_RADIO_PREPARE_S(flag)
#define DEBUG_RADIO_START_S(flag)
#define DEBUG_RADIO_CLOSE_S(flag)
#define DEBUG_RADIO_PREPARE_O(flag)
#define DEBUG_RADIO_START_O(flag)
#define DEBUG_RADIO_CLOSE_O(flag)
#define DEBUG_RADIO_PREPARE_M(flag)
#define DEBUG_RADIO_START_M(flag)
#define DEBUG_RADIO_CLOSE_M(flag)

#endif
