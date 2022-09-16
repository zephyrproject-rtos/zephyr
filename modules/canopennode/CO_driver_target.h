/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODULES_CANOPENNODE_CO_DRIVER_H
#define ZEPHYR_MODULES_CANOPENNODE_CO_DRIVER_H

/*
 * Zephyr RTOS CAN driver interface and configuration for CANopenNode
 * CANopen protocol stack.
 *
 * See CANopenNode/stack/drvTemplate/CO_driver.h for API description.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/zephyr.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/toolchain.h>

/* Use static variables instead of calloc() */
#define CO_USE_GLOBALS

/* Use Zephyr provided crc16 implementation */
#define CO_USE_OWN_CRC16

/* Use SDO buffer size from Kconfig */
#define CO_SDO_BUFFER_SIZE CONFIG_CANOPENNODE_SDO_BUFFER_SIZE

/* Use trace buffer size from Kconfig */
#define CO_TRACE_BUFFER_SIZE_FIXED CONFIG_CANOPENNODE_TRACE_BUFFER_SIZE

#ifdef CONFIG_CANOPENNODE_LEDS
#define CO_USE_LEDS 1
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define CO_LITTLE_ENDIAN
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define CO_BIG_ENDIAN
#else
#error "Unsupported endianness"
#endif

typedef bool          bool_t;
typedef float         float32_t;
typedef long double   float64_t;
typedef char          char_t;
typedef unsigned char oChar_t;
typedef unsigned char domain_t;

typedef struct canopen_rx_msg {
	uint8_t data[8];
	uint16_t ident;
	uint8_t DLC;
} CO_CANrxMsg_t;

typedef void (*CO_CANrxBufferCallback_t)(void *object,
					 const CO_CANrxMsg_t *message);

typedef struct canopen_rx {
	int filter_id;
	void *object;
	CO_CANrxBufferCallback_t pFunct;
	uint16_t ident;
} CO_CANrx_t;

typedef struct canopen_tx {
	uint8_t data[8];
	uint16_t ident;
	uint8_t DLC;
	bool_t rtr : 1;
	bool_t bufferFull : 1;
	bool_t syncFlag : 1;
} CO_CANtx_t;

typedef struct canopen_module {
	const struct device *dev;
	CO_CANrx_t *rx_array;
	CO_CANtx_t *tx_array;
	uint16_t rx_size;
	uint16_t tx_size;
	uint32_t errors;
	void *em;
	bool_t configured : 1;
	bool_t CANnormal : 1;
	bool_t first_tx_msg : 1;
} CO_CANmodule_t;

void canopen_send_lock(void);
void canopen_send_unlock(void);
#define CO_LOCK_CAN_SEND()   canopen_send_lock()
#define CO_UNLOCK_CAN_SEND() canopen_send_unlock()

void canopen_emcy_lock(void);
void canopen_emcy_unlock(void);
#define CO_LOCK_EMCY()   canopen_emcy_lock()
#define CO_UNLOCK_EMCY() canopen_emcy_unlock()

void canopen_od_lock(void);
void canopen_od_unlock(void);
#define CO_LOCK_OD()   canopen_od_lock()
#define CO_UNLOCK_OD() canopen_od_unlock()

/*
 * CANopenNode RX callbacks run in interrupt context, no memory
 * barrier needed.
 */
#define CANrxMemoryBarrier()
#define IS_CANrxNew(rxNew) ((uintptr_t)rxNew)
#define SET_CANrxNew(rxNew) { CANrxMemoryBarrier(); rxNew = (void *)1L; }
#define CLEAR_CANrxNew(rxNew) { CANrxMemoryBarrier(); rxNew = (void *)0L; }

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_MODULES_CANOPENNODE_CO_DRIVER_H */
