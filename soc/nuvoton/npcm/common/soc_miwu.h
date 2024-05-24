/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NPCM_SOC_MIWU_H_
#define _NUVOTON_NPCM_SOC_MIWU_H_

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

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

#define NPCM_MIWU_TABLE_NONE NPCM_MIWU_TABLE_COUNT

/* Interrupt modes supported by npcm miwu modules */
enum miwu_int_mode {
	NPCM_MIWU_MODE_LEVEL,
	NPCM_MIWU_MODE_EDGE,
};

/* Interrupt trigger modes supported by npcm miwu modules */
enum miwu_int_trig {
	NPCM_MIWU_TRIG_LOW,  /** Edge failing or active low detection */
	NPCM_MIWU_TRIG_HIGH, /** Edge rising or active high detection */
	NPCM_MIWU_TRIG_BOTH, /** Both edge rising and failing detection */
};

/**
 * @brief NPCM wake-up input source structure
 *
 * Used to indicate a Wake-Up Input source (WUI) belongs to which group and bit
 * of Multi-Input Wake-Up Unit (MIWU) modules.
 */
struct npcm_wui {
	uint8_t table : 2;      /** A source belongs to which MIWU table. */
	uint8_t group : 3;      /** A source belongs to which group of MIWU table. */
	uint8_t bit : 3;        /** A source belongs to which bit of MIWU group. */
};

/**
 * Define npcm miwu driver callback handler signature for wake-up input source
 * of generic hardware. Its parameters contain the device issued interrupt
 * and corresponding WUI source.
 */
typedef void (*miwu_dev_callback_handler_t)(const struct device *source,
					    struct npcm_wui *wui);

/**
 * @brief MIWU/GPIO information structure
 *
 * It contains both GPIO and MIWU information which is stored in unused field
 * of struct gpio_port_pins_t since a interested mask of pins is only 8 bits.
 * Beware the size of such structure must equal struct gpio_port_pins_t.
 */
struct miwu_io_params {
	uint8_t pin_mask;       /** A mask of pins the callback is interested in. */
	uint8_t reserved;
	uint8_t gpio_port;      /** GPIO device index */
	struct npcm_wui wui; /** Wake-up input source of GPIO */
};

/**
 * @brief MIWU callback structure for a GPIO input
 *
 * Used to register a GPIO callback in the driver instance callback list.
 * Beware such structure should not be allocated on stack and its size must
 * equal struct gpio_callback.
 *
 * Note: To help setting it, see npcx_miwu_init_gpio_callback() below
 */
struct miwu_io_callback {
	/** Node of single-linked list */
	sys_snode_t node;
	/** Callback function being called when GPIO event occurred */
	gpio_callback_handler_t handler;
	/** GPIO callback parameters used in MIWU ISR */
	struct miwu_io_params params;
};

/**
 * @brief MIWU callback structure for a device input
 *
 * Used to register a generic hardware device callback in the driver instance
 * callback list. Beware such structure should not be allocated on stack.
 *
 * Note: To help setting it, see npcm_miwu_init_dev_callback() below
 */
struct miwu_dev_callback {
	/** Node of single-linked list */
	sys_snode_t node;
	/** Callback function being called when device event occurred */
	miwu_dev_callback_handler_t handler;
	/** Device instance register callback function */
	const struct device *source;
	/* Wake-up input source */
	struct npcm_wui wui;
};

/**
 * @brief Enable interrupt of the wake-up input source
 *
 * @param A pointer on wake-up input source
 */
void npcm_miwu_irq_enable(const struct npcm_wui *wui);

/**
 * @brief Disable interrupt of the wake-up input source
 *
 * @param wui A pointer on wake-up input source
 */
void npcm_miwu_irq_disable(const struct npcm_wui *wui);

/**
 * @brief Connect io to the wake-up input source
 *
 * @param wui A pointer on wake-up input source
 */
void npcm_miwu_io_enable(const struct npcm_wui *wui);

/**
 * @brief Disconnect io to the wake-up input source
 *
 * @param wui A pointer on wake-up input source
 */
void npcm_miwu_io_disable(const struct npcm_wui *wui);

/**
 * @brief Get interrupt state of the wake-up input source
 *
 * @param wui A pointer on wake-up input source
 *
 * @retval 0 if interrupt is disabled, otherwise interrupt is enabled
 */
bool npcm_miwu_irq_get_state(const struct npcm_wui *wui);

/**
 * @brief Get & clear interrupt pending bit of the wake-up input source
 *
 * @param wui A pointer on wake-up input source
 *
 * @retval 1 if interrupt is pending
 */
bool npcm_miwu_irq_get_and_clear_pending(const struct npcm_wui *wui);

/**
 * @brief Configure interrupt type of the wake-up input source
 *
 * @param wui Pointer to wake-up input source for configuring
 * @param mode Interrupt mode supported by NPCM MIWU
 * @param trig Interrupt trigger mode supported by NPCM MIWU
 *
 * @retval 0 If successful
 * @retval -EINVAL Invalid parameters
 */
int npcm_miwu_interrupt_configure(const struct npcm_wui *wui,
				     enum miwu_int_mode mode, enum miwu_int_trig trig);

/**
 * @brief Function to initialize a struct miwu_io_callback properly
 *
 * @param callback Pointer to io callback structure for initialization
 * @param io_wui Pointer to wake-up input IO source
 * @param port GPIO port issued a callback function
 */
void npcm_miwu_init_gpio_callback(struct miwu_io_callback *callback,
				     const struct npcm_wui *io_wui, int port);

/**
 * @brief Function to initialize a struct miwu_dev_callback properly
 *
 * @param callback Pointer to device callback structure for initialization
 * @param dev_wui Pointer to wake-up input device source
 * @param handler A function called when its device input event issued
 * @param source Pointer to device instance issued a callback function
 */
void npcm_miwu_init_dev_callback(struct miwu_dev_callback *callback,
				    const struct npcm_wui *dev_wui,
				    miwu_dev_callback_handler_t handler,
				    const struct device *source);

/**
 * @brief Function to insert or remove a IO callback from a callback list
 *
 * @param callback Pointer to io callback structure
 * @param set A boolean indicating insertion or removal of the callback
 *
 * @retval 0 If successful.
 * @retval -EINVAL Invalid parameters
 */
int npcm_miwu_manage_gpio_callback(struct miwu_io_callback *callback, bool set);


/**
 * @brief Function to insert or remove a device callback from a callback list
 *
 * @param callback Pointer to device callback structure
 * @param set A boolean indicating insertion or removal of the callback
 *
 * @retval 0 If successful.
 * @retval -EINVAL Invalid parameters
 */
int npcm_miwu_manage_dev_callback(struct miwu_dev_callback *cb, bool set);

#ifdef __cplusplus
}
#endif
#endif /* _NUVOTON_NPCM_SOC_MIWU_H_ */
