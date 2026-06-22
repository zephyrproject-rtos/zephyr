/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Renesas RX DTC Driver Header File
 */

#ifndef ZEPHYR_DRIVERS_MISC_RENESAS_RX_DTC_H_
#define ZEPHYR_DRIVERS_MISC_RENESAS_RX_DTC_H_

#include <zephyr/drivers/gpio.h>

/**
 * @name Renesas RX DTC driver definitions.
 * @{
 */
#define DTC_PLACE_IN_SECTION(x)                                                                    \
	__attribute__((section(x)))                                                                \
	__attribute__((__used__)) /**< Attribute to place variable in a specific section. */
#define DTC_ALIGN_VARIABLE(x) __aligned(x) /**< Attribute to align variable. */
#define DTC_SECTION_ATTRIBUTE                                                                      \
	DTC_PLACE_IN_SECTION(".dtc_vector_table") /**< DTC vector table section attribute. */

#define DTC_VECTOR_TABLE_ENTRIES (CONFIG_NUM_IRQS) /**< Number of DTC vector entries. */
#define DTC_PRV_ACT_BIT_MASK     (1 << 15)         /**< DTC Active flag (DTCSTS.ACT) bit mask */
#define DTC_PRV_VECT_NR_MASK     (0x00FF)          /**< DTC-Activating Vector Number bits mask */
#define DTC_PRV_MASK_CRAL        (0xFFU)           /**< Counter Register A Lower Byte Mask */
#define DTC_PRV_OFFSET_CRAH      (8U)              /**< Counter Register A Upper Byte Offset */

#define DTC_MAX_NORMAL_TRANSFER_LENGTH                                                             \
	(0x10000) /**< Max configurable number of transfers in NORMAL MODE */
/**< Max number of transfers per repeat for REPEAT MODE */
#define DTC_MAX_REPEAT_TRANSFER_LENGTH                                                             \
	(0x100) /**< Max number of transfers per repeat for REPEAT MODE */
/**< Max number of transfers per block in BLOCK MODE */
#define DTC_MAX_BLOCK_TRANSFER_LENGTH                                                              \
	(0x100) /**< Max number of transfers per block in BLOCK MODE */
#define DTC_MAX_BLOCK_COUNT                                                                        \
	(0x10000) /**< Max configurable number of blocks to transfer in BLOCK MODE */

/** @} */

/** @brief Transfer address mode. */
typedef enum e_transfer_addr_mode {
	TRANSFER_ADDR_MODE_FIXED = 0,       /**< Mode fixed. */
	TRANSFER_ADDR_MODE_OFFSET = 1,      /**< Mode offset. */
	TRANSFER_ADDR_MODE_INCREMENTED = 2, /**< Mode incremented. */
	TRANSFER_ADDR_MODE_DECREMENTED = 3  /**< Mode decremented. */
} transfer_addr_mode_t;

/** @brief Area that repeats during repeat transfer mode. */
typedef enum e_transfer_repeat_area {
	TRANSFER_REPEAT_AREA_DESTINATION = 0, /**< Destination repeats. */
	TRANSFER_REPEAT_AREA_SOURCE = 1       /**< Source repeats. */
} transfer_repeat_area_t;

/** @brief Transfer interrupt type. */
typedef enum e_transfer_irq {
	TRANSFER_IRQ_END = 0, /**< Interrupt at end of transfer. */
	TRANSFER_IRQ_EACH = 1 /**< Interrupt on each transfer. */
} transfer_irq_t;

/**< Interrupt on each transfer. */
typedef enum e_transfer_chain_mode {
	TRANSFER_CHAIN_MODE_DISABLED = 0, /**< Chaining disabled. */
	TRANSFER_CHAIN_MODE_EACH = 2,     /**< Chaining disabled. */
	TRANSFER_CHAIN_MODE_END = 3       /**< Chain at end. */
} transfer_chain_mode_t;

/** @brief Transfer data size. */
typedef enum e_transfer_size {
	TRANSFER_SIZE_1_BYTE = 0, /**< 1-byte transfer. */
	TRANSFER_SIZE_2_BYTE = 1, /**< 2-byte transfer. */
	TRANSFER_SIZE_4_BYTE = 2  /**< 4-byte transfer. */
} transfer_size_t;

/** @brief Transfer mode. */
typedef enum e_transfer_mode {
	TRANSFER_MODE_NORMAL = 0,      /**< Normal transfer mode. */
	TRANSFER_MODE_REPEAT = 1,      /**< Repeat transfer mode. */
	TRANSFER_MODE_BLOCK = 2,       /**< Block transfer mode. */
	TRANSFER_MODE_REPEAT_BLOCK = 3 /**< Repeat block transfer mode. */
} transfer_mode_t;

/** @brief DTC activation status. */
typedef enum e_dtc_act_status {
	DTC_ACT_IDLE = 0,        /**< Idle state. */
	DTC_ACT_CONFIGURED = 1,  /**< Configured state. */
	DTC_ACT_IN_PROGRESS = 2, /**< In-progress state. */
} dtc_act_status_t;

/** @brief Transfer information structure. */
typedef struct st_transfer_info {
	union {
		struct {
			unsigned int: 16;
			unsigned int: 2;

			transfer_addr_mode_t dest_addr_mode: 2; /**< Destination address mode. */

			transfer_repeat_area_t repeat_area: 1; /**< Destination address mode. */

			transfer_irq_t irq: 1; /**< IRQ type. */

			transfer_chain_mode_t chain_mode: 2; /**< Chain mode. */

			unsigned int: 2;

			transfer_addr_mode_t src_addr_mode: 2; /**< Source address mode. */

			transfer_size_t size: 2; /**< Transfer data size. */

			transfer_mode_t mode: 2; /**< Transfer mode. */
		} transfer_settings_word_b;

		uint32_t transfer_settings_word; /**< Raw settings. */
	};

	void const *volatile p_src;   /**< Source address. */
	void *volatile p_dest;        /**< Destination address. */
	volatile uint16_t num_blocks; /**< Number of blocks. */
	volatile uint16_t length;     /**< Transfer length. */
} transfer_info_t;

/** @brief Detailed transfer properties. */
typedef struct st_transfer_properties {
	uint32_t block_count_max;           /**< Maximum number of blocks */
	uint32_t block_count_remaining;     /**< Number of blocks remaining */
	uint32_t transfer_length_max;       /**< Maximum number of transfers */
	uint32_t transfer_length_remaining; /**< Number of transfers remaining */
} transfer_properties_t;

/** @brief DTC transfer status. */
struct dtc_transfer_status {
	uint8_t activation_irq; /**< Activation IRQ number. */
	bool in_progress;       /**< Transfer in progress flag. */
};

/**
 * @brief Turn off module DTC.
 * @param dev DTC device instance.
 *
 * @retval 0 On success.
 */
int dtc_renesas_rx_off(const struct device *dev);

/**
 * @brief Turn on module DTC.
 * @param dev DTC device instance.
 *
 * @retval 0 On success.
 */
int dtc_renesas_rx_on(const struct device *dev);

/**
 * @brief Configure the p_info state and write p_info to DTC vector table.
 *
 * @param dev DTC device instance.
 * @param activation_irq activation source.
 * @param p_info transfer info.
 *
 * @retval 0 On success.
 * @retval -EINVAL if activation source is invalid.
 */
int dtc_renesas_rx_configuration(const struct device *dev, uint8_t activation_irq,
				 transfer_info_t *p_info);

/**
 * @brief Enable transfer in ICU on this activation source.
 *
 * @param activation_irq activation source.
 *
 * @retval 0 On success.
 * @retval -EINVAL if activation source is invalid.
 */
int dtc_renesas_rx_enable_transfer(uint8_t activation_irq);

/**
 * @brief Disable transfer in ICU on this activation source.
 *
 * @param activation_irq activation source.
 *
 * @retval 0 On success.
 * @retval -EINVAL if activation source is invalid.
 */
int dtc_renesas_rx_disable_transfer(uint8_t activation_irq);

/**
 * @brief Start transfers on this activation source.
 *
 * @param dev DTC device instance.
 * @param activation_irq activation source.
 *
 * @retval 0 On success.
 * @retval -EINVAL if activation source is invalid.
 * @retval -EACCES if this activation source in dtc vector table is not configured.
 */
int dtc_renesas_rx_start_transfer(const struct device *dev, uint8_t activation_irq);

/**
 * @brief Stop transfers on this activation source.
 *
 * @param dev DTC device instance.
 * @param activation_irq activation source.
 *
 * @retval 0 On success.
 * @retval -EINVAL if activation source is invalid.
 * @retval -EACCES if this activation source in dtc vector table is not configured.
 */
int dtc_renesas_rx_stop_transfer(const struct device *dev, uint8_t activation_irq);

/**
 * @brief Reset transfer on this activation source.
 *
 * @param dev             DTC device instance.
 * @param activation_irq  Activation source IRQ number.
 * @param p_src           Pointer to the source address for the transfer.
 * @param p_dest          Pointer to the destination address for the transfer.
 * @param num_transfers   Number of data units to transfer.
 *
 * @retval 0              On success.
 * @retval -EINVAL        If activation source is invalid.
 */
int dtc_renesas_rx_reset_transfer(const struct device *dev, uint8_t activation_irq,
				  void const *p_src, void *p_dest, uint16_t const num_transfers);

/**
 * @brief Get status transfer of DTC module and store it into status pointer.
 *
 * @param dev DTC device instance.
 * @param status DTC status.
 */
void dtc_renesas_rx_get_transfer_status(const struct device *dev,
					struct dtc_transfer_status *status);

/**
 * @brief Get information about this transfer and store it into p_properties.
 *
 * @param dev               DTC device instance.
 * @param activation_irq    Activation source IRQ number.
 * @param p_properties      Pointer to structure to receive driver-specific transfer properties.
 *
 * @retval 0        On success.
 * @retval -EINVAL  If activation source is invalid.
 */
int dtc_renesas_rx_info_get(const struct device *dev, uint8_t activation_irq,
			    transfer_properties_t *const p_properties);

static ALWAYS_INLINE bool is_valid_activation_irq(uint8_t activation_irq)
{
	if (activation_irq < 0 || activation_irq > DTC_VECTOR_TABLE_ENTRIES) {
		return false;
	}

	return true;
}

#endif /* ZEPHYR_DRIVERS_MISC_RENESAS_RX_DTC_H_ */
