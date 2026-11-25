/*
 * Copyright (c) 2026 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NPCM_SOC_MIWU_H_
#define _NUVOTON_NPCM_SOC_MIWU_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

enum miwu_table {
	NPCM_MIWU_TABLE_0,
	NPCM_MIWU_TABLE_1,
	NPCM_MIWU_TABLE_2,
	NPCM_MIWU_TABLE_COUNT
};

enum miwu_group {
	NPCM_MIWU_GROUP_1,
	NPCM_MIWU_GROUP_2,
	NPCM_MIWU_GROUP_3,
	NPCM_MIWU_GROUP_4,
	NPCM_MIWU_GROUP_5,
	NPCM_MIWU_GROUP_6,
	NPCM_MIWU_GROUP_7,
	NPCM_MIWU_GROUP_8,
	NPCM_MIWU_GROUP_COUNT
};

/* Table of the pseudo wake-up input 'wui_none', which has no MIWU mapping */
#define NPCM_MIWU_TABLE_NONE NPCM_MIWU_TABLE_COUNT

/* Interrupt modes supported by npcm miwu modules */
enum miwu_int_mode {
	NPCM_MIWU_MODE_LEVEL,
	NPCM_MIWU_MODE_EDGE,
};

/* Interrupt trigger modes supported by npcm miwu modules */
enum miwu_int_trig {
	NPCM_MIWU_TRIG_LOW,  /** Edge falling or active low detection */
	NPCM_MIWU_TRIG_HIGH, /** Edge rising or active high detection */
	NPCM_MIWU_TRIG_BOTH, /** Both edge rising and falling detection */
};

/**
 * @brief NPCM wake-up input source structure
 *
 * Used to indicate a Wake-Up Input source (WUI) belongs to which group and bit
 * of Multi-Input Wake-Up Unit (MIWU) modules.
 */
struct npcm_wui {
	uint8_t table: 2; /** A source belongs to which MIWU table. */
	uint8_t group: 3; /** A source belongs to which group of MIWU table. */
	uint8_t bit: 3;   /** A source belongs to which bit of MIWU group. */
};

/**
 * Define npcm miwu driver callback handler signature for a wake-up input
 * source. Its parameters contain the device that registered the callback and
 * the WUI source that issued the event.
 */
typedef void (*miwu_dev_callback_handler_t)(const struct device *source, struct npcm_wui *wui);

/**
 * @brief MIWU callback structure for a wake-up input source
 *
 * Used to register the callback of a device in the callback list of the MIWU
 * group its wake-up input belongs to. Each structure serves one wake-up input
 * and must not be allocated on stack.
 *
 * Note: To help setting it, see npcm_miwu_init_dev_callback() and
 *       npcm_miwu_manage_callback() below
 */
struct miwu_callback {
	/** Node of single-linked list */
	sys_snode_t node;
	/** Callback function being called when the wake-up input event occurred */
	miwu_dev_callback_handler_t handler;
	/** Device instance that registered the callback */
	const struct device *source;
	/** Wake-up input source of the callback */
	struct npcm_wui wui;
};

/**
 * @brief Enable interrupt of the wake-up input source
 *
 * Does nothing if the wake-up input source has no MIWU mapping.
 *
 * @param wui A pointer on wake-up input source
 */
void npcm_miwu_irq_enable(const struct npcm_wui *wui);

/**
 * @brief Disable interrupt of the wake-up input source
 *
 * Does nothing if the wake-up input source has no MIWU mapping.
 *
 * @param wui A pointer on wake-up input source
 */
void npcm_miwu_irq_disable(const struct npcm_wui *wui);

/**
 * @brief Connect io to the wake-up input source
 *
 * Does nothing if the wake-up input source has no MIWU mapping.
 *
 * @param wui A pointer on wake-up input source
 */
void npcm_miwu_io_enable(const struct npcm_wui *wui);

/**
 * @brief Disconnect io to the wake-up input source
 *
 * Does nothing if the wake-up input source has no MIWU mapping.
 *
 * @param wui A pointer on wake-up input source
 */
void npcm_miwu_io_disable(const struct npcm_wui *wui);

/**
 * @brief Get interrupt state of the wake-up input source
 *
 * @param wui A pointer on wake-up input source
 *
 * @retval true Interrupt is enabled
 * @retval false Interrupt is disabled, or the source has no MIWU mapping
 */
bool npcm_miwu_irq_get_state(const struct npcm_wui *wui);

/**
 * @brief Get & clear interrupt pending bit of the wake-up input source
 *
 * Works on every mapped source, including one that no interrupt is routed
 * from, so the owner of the interrupt it is routed to can poll it.
 *
 * @param wui A pointer on wake-up input source
 *
 * @retval true Interrupt was pending and has been cleared
 * @retval false Interrupt was not pending, or the source has no MIWU mapping
 */
bool npcm_miwu_irq_get_and_clear_pending(const struct npcm_wui *wui);

/**
 * @brief Configure interrupt type of the wake-up input source
 *
 * The request is checked before any register is changed, so a rejected
 * request leaves the previous configuration in place.
 *
 * @param wui Pointer to wake-up input source for configuring
 * @param mode Interrupt mode supported by NPCM MIWU
 * @param trig Interrupt trigger mode supported by NPCM MIWU
 *
 * @retval 0 Success
 * @retval -EINVAL The source has no MIWU mapping, or the mode does not support
 *                 the trigger (level mode with NPCM_MIWU_TRIG_BOTH)
 */
int npcm_miwu_interrupt_configure(const struct npcm_wui *wui, enum miwu_int_mode mode,
				  enum miwu_int_trig trig);

/**
 * @brief Function to initialize a struct miwu_callback properly
 *
 * @param callback Pointer to callback structure for initialization
 * @param dev_wui Pointer to wake-up input source of the device
 * @param handler A function called when the wake-up input event occurred
 * @param source Pointer to device instance registering the callback, passed
 *               back to the handler
 */
void npcm_miwu_init_dev_callback(struct miwu_callback *callback, const struct npcm_wui *dev_wui,
				 miwu_dev_callback_handler_t handler, const struct device *source);

/**
 * @brief Function to insert or remove a miwu callback from a callback list
 *
 * A callback can only be inserted for a wake-up input source that the MIWU
 * group-interrupt map routes to an interrupt.
 *
 * @param cb Pointer to miwu callback structure
 * @param set A boolean indicating insertion or removal of the callback
 *
 * @retval 0 Success
 * @retval -EINVAL Wake-up input source of the callback has no MIWU mapping, or
 *                 the callback to remove is not in the callback list
 * @retval -ENOTSUP No interrupt is routed from the wake-up input source of the
 *                  callback to insert
 */
int npcm_miwu_manage_callback(struct miwu_callback *cb, bool set);

#ifdef __cplusplus
}
#endif
#endif /* _NUVOTON_NPCM_SOC_MIWU_H_ */
