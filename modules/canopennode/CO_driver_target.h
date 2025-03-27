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

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/toolchain.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/dsp/types.h> /* float32_t, float64_t */

/* Use static variables instead of calloc() */
#define CO_USE_GLOBALS

/* Use Zephyr provided crc16 implementation */
#define CO_CONFIG_CRC16 (CO_CONFIG_CRC16_ENABLE | CO_CONFIG_CRC16_EXTERNAL)

#ifdef CONFIG_LITTLE_ENDIAN
#define CO_LITTLE_ENDIAN
#else
#define CO_BIG_ENDIAN
#endif

#define CO_SWAP_16(x) sys_le16_to_cpu(x)
#define CO_SWAP_32(x) sys_le32_to_cpu(x)
#define CO_SWAP_64(x) sys_le64_to_cpu(x)

typedef bool          bool_t;
typedef char          char_t;
typedef unsigned char oChar_t;
typedef unsigned char domain_t;

BUILD_ASSERT(sizeof(float32_t) >= 4);
BUILD_ASSERT(sizeof(float64_t) >= 8);

typedef struct canopen_rx_msg {
	uint8_t data[8];
	uint16_t ident;
	uint8_t DLC;
} CO_CANrxMsg_t;

static inline uint16_t
CO_CANrxMsg_readIdent(const CO_CANrxMsg_t *rxMsg)
{
	return rxMsg->ident;
}

static inline uint8_t
CO_CANrxMsg_readDLC(const CO_CANrxMsg_t *rxMsg)
{
	return rxMsg->DLC;
}

static inline uint8_t *
CO_CANrxMsg_readData(const CO_CANrxMsg_t *rxMsg)
{
	return ((CO_CANrxMsg_t *)(rxMsg))->data;
}

typedef void (*CO_CANrxBufferCallback_t)(void *object, void *message);

typedef struct canopen_rx {
	int filter_id;
	void *object;
	CO_CANrxBufferCallback_t pFunct;
	uint16_t ident;
	uint16_t mask;
#ifdef CONFIG_CAN_ACCEPT_RTR
	bool rtr;
#endif /* CONFIG_CAN_ACCEPT_RTR */
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
	void *em;
	CO_CANrx_t *rx_array;
	CO_CANtx_t *tx_array;
	uint16_t rx_size;
	uint16_t tx_size;
	uint16_t CANerrorStatus;
	uint32_t errors;
	bool_t configured : 1;
	bool_t CANnormal : 1;
	bool_t first_tx_msg : 1;
} CO_CANmodule_t;

/**
 * @brief CANopen object dictionary storage types.
 */
enum canopen_storage {
	CANOPEN_STORAGE_RAM,
	CANOPEN_STORAGE_ROM,
	CANOPEN_STORAGE_EEPROM,
};

typedef struct canopen_storage_entry {
	void *addr;
	size_t len;
	uint8_t subIndexOD;
	uint8_t attr;

	enum canopen_storage type;
} CO_storage_entry_t;

void canopen_send_lock(void);
void canopen_send_unlock(void);
#define CO_LOCK_CAN_SEND(module)   canopen_send_lock()
#define CO_UNLOCK_CAN_SEND(module) canopen_send_unlock()

void canopen_emcy_lock(void);
void canopen_emcy_unlock(void);
#define CO_LOCK_EMCY(module)   canopen_emcy_lock()
#define CO_UNLOCK_EMCY(module) canopen_emcy_unlock()

void canopen_od_lock(void);
void canopen_od_unlock(void);
#define CO_LOCK_OD(module)   canopen_od_lock()
#define CO_UNLOCK_OD(module) canopen_od_unlock()

/*
 * CANopenNode RX callbacks run in interrupt context, no memory
 * barrier needed.
 */
#define CO_MemoryBarrier()
#define CO_FLAG_READ(rxNew)  ((rxNew) != NULL)
#define CO_FLAG_SET(rxNew)   { CO_MemoryBarrier(); rxNew = (void *)1L; }
#define CO_FLAG_CLEAR(rxNew) { CO_MemoryBarrier(); rxNew = (void *)0L; }

/*
 * CANopenNode firmware download macro definition override
 */
void canopen_set_firmware_download(bool_t in_progress);
bool_t canopen_firmware_download_in_progress(void);

#define CO_STATUS_FIRMWARE_DOWNLOAD_IN_PROGRESS \
	canopen_firmware_download_in_progress()

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_MODULES_CANOPENNODE_CO_DRIVER_H */
