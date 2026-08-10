/*
 * Copyright 2021-2022,2024, 2026 NXP.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __UWB_UWBS_TML_IO_H__
#define __UWB_UWBS_TML_IO_H__

#ifdef UWBIOT_USE_FTR_FILE
#include "uwb_iot_ftr.h"
#else
#include "uwb_iot_ftr_default.h"
#endif

/** @defgroup uwb_uwbs_io UWBS Specific IO Constants
 *
 * Each UWBS has different IO Requirements.
 *
 * In order to isolate the interfaces,
 * UWBS specific header file would allow to
 * isolate the usage of these constants.
 *
 * @{
 */

/**
 * @brief IO Pin States
 * @example High or low
 *
 */
typedef enum {
	/** Pin is Low, logic low, zero volt */
	kUWBS_IO_State_Low = 0,

	/** Pin is High, logic high.
	 * Depending on the voltage domain, 1.8v, 3.3v */
	kUWBS_IO_State_High = 1,
	/* Invalid state */
	kUWBS_IO_State_NA = 0x7F,
} uwbs_io_state_t;

/** IO PINs between Host and the UWBS
 *
 * See uwb_bus_io.c
 */

#if UWBIOT_UWBD_SR2XXT
/**
 * @brief Enumeration for different IO pins between Host and the UWBS
 */
typedef enum {
	/** Helios IRQ PIN */
	kUWBS_IO_I_UWBS_IRQ,
	/** Helios Enable*/
	kUWBS_IO_O_ENABLE_HELIOS,
	/** Helios Chip Reset*/
	kUWBS_IO_O_RSTN,
	/** SN220 (Vulcan) VEN PIN */
	kUWBS_IO_O_NFC_GPIO0,
	/** SN220 (Vulcan) RSTN PIN */
	kUWBS_IO_O_NFC_RSTN,
	/** SN220 (Vulcan) IRQ PIN */
	kUWBS_IO_I_SN220_IRQ,
} uwbs_io_t;
#endif // UWBIOT_UWBD_SR2XXT

#if (UWBIOT_UWBD_SR04X)
/**
 * @brief Enumeration for different IO pins between Host and the UWBS
 */
typedef enum {
	/** UWBS IRQ PIN */
	kUWBS_IO_I_UWBS_IRQ,
	/** Chip Select */
	kUWBS_IO_O_UWBS_CS,
	/** Ready IRQ */
	kUWBS_IO_I_UWBS_READY_IRQ,
	/** RSTN */
	kUWBS_IO_O_RSTN,
} uwbs_io_t;
#endif // UWBIOT_UWBD_SR04X

/**
 * @brief Call back function and argument for helios irq
 */
typedef struct {
	void (*fn)(void *args);
	void *args;
} uwbs_io_callback;

/** @} */
#endif // __UWB_UWBS_TML_IO_H__
