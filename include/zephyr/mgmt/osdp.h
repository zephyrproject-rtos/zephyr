/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Open Supervised Device Protocol (OSDP) public API header file.
 */

#ifndef ZEPHYR_INCLUDE_MGMT_OSDP_H_
#define ZEPHYR_INCLUDE_MGMT_OSDP_H_

#include <zephyr/kernel.h>
#include <stdint.h>

#include <zephyr/sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OSDP_CMD_TEXT_MAX_LEN          32 /**< Max length of text for text command */
#define OSDP_CMD_KEYSET_KEY_MAX_LEN    32 /**< Max length of key data for keyset command */
#define OSDP_EVENT_MAX_DATALEN         64 /**< Max length of event data */

/**
 * @brief Command sent from CP to Control digital output of PD.
 */
struct osdp_cmd_output {
	/**
	 * Output number.
	 *
	 * 0 = First Output, 1 = Second Output, etc.
	 */
	uint8_t output_no;
	/**
	 * Control code.
	 *
	 * - 0 - NOP – do not alter this output
	 * - 1 - set the permanent state to OFF, abort timed operation (if any)
	 * - 2 - set the permanent state to ON, abort timed operation (if any)
	 * - 3 - set the permanent state to OFF, allow timed operation to complete
	 * - 4 - set the permanent state to ON, allow timed operation to complete
	 * - 5 - set the temporary state to ON, resume perm state on timeout
	 * - 6 - set the temporary state to OFF, resume permanent state on timeout
	 */
	uint8_t control_code;
	/**
	 * Time in units of 100 ms
	 */
	uint16_t timer_count;
};

/**
 * @brief LED Colors as specified in OSDP for the on_color/off_color parameters.
 */
enum osdp_led_color_e {
	OSDP_LED_COLOR_NONE,     /**< No color */
	OSDP_LED_COLOR_RED,      /**< Red */
	OSDP_LED_COLOR_GREEN,    /**< Green */
	OSDP_LED_COLOR_AMBER,    /**< Amber */
	OSDP_LED_COLOR_BLUE,     /**< Blue */
	OSDP_LED_COLOR_SENTINEL  /**< Max value */
};

/**
 * @brief LED params sub-structure. Part of LED command. See @ref osdp_cmd_led.
 */
struct osdp_cmd_led_params {
	/** Control code.
	 *
	 * Temporary Control Code:
	 * - 0 - NOP - do not alter this LED's temporary settings.
	 * - 1 - Cancel any temporary operation and display this LED's permanent state immediately.
	 * - 2 - Set the temporary state as given and start timer immediately.
	 *
	 * Permanent Control Code:
	 * - 0 - NOP - do not alter this LED's permanent settings.
	 * - 1 - Set the permanent state as given.
	 */
	uint8_t control_code;
	/**
	 * The ON duration of the flash, in units of 100 ms.
	 */
	uint8_t on_count;
	/**
	 * The OFF duration of the flash, in units of 100 ms.
	 */
	uint8_t off_count;
	/**
	 * Color to set during the ON timer (see @ref osdp_led_color_e).
	 */
	uint8_t on_color;
	/**
	 * Color to set during the OFF timer (see @ref osdp_led_color_e).
	 */
	uint8_t off_color;
	/**
	 * Time in units of 100 ms (only for temporary mode).
	 */
	uint16_t timer_count;
};

/**
 * @brief Sent from CP to PD to control the behaviour of it's on-board LEDs
 */
struct osdp_cmd_led {
	/**
	 * Reader number. 0 = First Reader, 1 = Second Reader, etc.
	 */
	uint8_t reader;
	/**
	 * LED number. 0 = first LED, 1 = second LED, etc.
	 */
	uint8_t led_number;
	/**
	 * Ephemeral LED status descriptor.
	 */
	struct osdp_cmd_led_params temporary;
	/**
	 * Permanent LED status descriptor.
	 */
	struct osdp_cmd_led_params permanent;
};

/**
 * @brief Sent from CP to control the behaviour of a buzzer in the PD.
 */
struct osdp_cmd_buzzer {
	/**
	 * Reader number. 0 = First Reader, 1 = Second Reader, etc.
	 */
	uint8_t reader;
	/**
	 * Control code.
	 * - 0 - no tone
	 * - 1 - off
	 * - 2 - default tone
	 * - 3+ - TBD
	 */
	uint8_t control_code;
	/**
	 * The ON duration of the sound, in units of 100 ms.
	 */
	uint8_t on_count;
	/**
	 * The OFF duration of the sound, in units of 100 ms.
	 */
	uint8_t off_count;
	/**
	 * The number of times to repeat the ON/OFF cycle; 0: forever.
	 */
	uint8_t rep_count;
};

/**
 * @brief Command to manipulate any display units that the PD supports.
 */
struct osdp_cmd_text {
	/**
	 * Reader number. 0 = First Reader, 1 = Second Reader, etc.
	 */
	uint8_t reader;
	/**
	 * Control code.
	 * - 1 - permanent text, no wrap
	 * - 2 - permanent text, with wrap
	 * - 3 - temp text, no wrap
	 * - 4 - temp text, with wrap
	 */
	uint8_t control_code;
	/**
	 * Duration to display temporary text, in seconds
	 */
	uint8_t temp_time;
	/**
	 * Row to display the first character (1-indexed)
	 */
	uint8_t offset_row;
	/**
	 * Column to display the first character (1-indexed)
	 */
	uint8_t offset_col;
	/**
	 * Number of characters in the string
	 */
	uint8_t length;
	/**
	 * The string to display
	 */
	uint8_t data[OSDP_CMD_TEXT_MAX_LEN];
};

/**
 * @brief Sent in response to a COMSET command. Set communication parameters to
 * PD. Must be stored in PD non-volatile memory.
 */
struct osdp_cmd_comset {
	/**
	 * Unit ID to which this PD will respond after the change takes effect.
	 */
	uint8_t address;
	/**
	 * Baud rate.
	 *
	 * Valid values: 9600, 19200, 38400, 115200, 230400.
	 */
	uint32_t baud_rate;
};

/**
 * @brief This command transfers an encryption key from the CP to a PD.
 *
 * @param type Type of keys:
 *   - 0x01 – Secure Channel Base Key
 * @param length Number of bytes of key data - (Key Length in bits + 7) / 8
 * @param data Key data
 */
struct osdp_cmd_keyset {
	/**
	 * Type of keys:
	 * - 0x01 – Secure Channel Base Key
	 */
	uint8_t type;
	/**
	 * Number of bytes of key data - (Key Length in bits + 7) / 8
	 */
	uint8_t length;
	/**
	 * Key data
	 */
	uint8_t data[OSDP_CMD_KEYSET_KEY_MAX_LEN];
};

/**
 * @brief OSDP application exposed commands
 */
enum osdp_cmd_e {
	OSDP_CMD_OUTPUT = 1,  /**< Output control command */
	OSDP_CMD_LED,         /**< Reader LED control command */
	OSDP_CMD_BUZZER,      /**< Reader buzzer control command */
	OSDP_CMD_TEXT,        /**< Reader text output command */
	OSDP_CMD_KEYSET,      /**< Encryption Key Set Command */
	OSDP_CMD_COMSET,      /**< PD Communication Configuration Command */
	OSDP_CMD_SENTINEL     /**< Max command value */
};

/**
 * @brief OSDP Command Structure. This is a wrapper for all individual OSDP
 * commands.
 */
struct osdp_cmd {
	/** @cond INTERNAL_HIDDEN */
	sys_snode_t node;
	/** @endcond */
	/**
	 * Command ID.
	 * Used to select specific commands in union.
	 */
	enum osdp_cmd_e id;
	/** Command */
	union {
		struct osdp_cmd_led    led;    /**< LED command structure */
		struct osdp_cmd_buzzer buzzer; /**< Buzzer command structure */
		struct osdp_cmd_text   text;   /**< Text command structure */
		struct osdp_cmd_output output; /**< Output command structure */
		struct osdp_cmd_comset comset; /**< Comset command structure */
		struct osdp_cmd_keyset keyset; /**< Keyset command structure */
	};
};

/**
 * @brief Various card formats that a PD can support. This is sent to CP
 * when a PD must report a card read.
 */
enum osdp_event_cardread_format_e {
	OSDP_CARD_FMT_RAW_UNSPECIFIED, /**< Unspecified card format */
	OSDP_CARD_FMT_RAW_WIEGAND,     /**< Wiegand card format */
	OSDP_CARD_FMT_ASCII,           /**< ASCII card format */
	OSDP_CARD_FMT_SENTINEL         /**< Max card format value */
};

/**
 * @brief OSDP event cardread
 *
 * @note When @a format is set to OSDP_CARD_FMT_RAW_UNSPECIFIED or
 * OSDP_CARD_FMT_RAW_WIEGAND, the length is expressed in bits. OTOH, when it is
 * set to OSDP_CARD_FMT_ASCII, the length is in bytes. The number of bytes to
 * read from the @a data field must be interpreted accordingly.
 */
struct osdp_event_cardread {
	/**
	 * Reader number. 0 = First Reader, 1 = Second Reader, etc.
	 */
	int reader_no;
	/**
	 * Format of the card being read.
	 */
	enum osdp_event_cardread_format_e format;
	/**
	 * Direction of data in @a data array.
	 * - 0 - Forward
	 * - 1 - Backward
	 */
	int direction;
	/**
	 * Length of card data in bytes or bits depending on @a format
	 */
	int length;
	/**
	 * Card data of @a length bytes or bits bits depending on @a format
	 */
	uint8_t data[OSDP_EVENT_MAX_DATALEN];
};

/**
 * @brief OSDP Event Keypad
 */
struct osdp_event_keypress {
	/**
	 * Reader number.
	 * In context of readers attached to current PD, this number indicated this number. This is
	 * not supported by LibOSDP.
	 */
	int reader_no;
	/**
	 * Length of keypress data in bytes
	 */
	int length;
	/**
	 * Keypress data of @a length bytes
	 */
	uint8_t data[OSDP_EVENT_MAX_DATALEN];
};

/**
 * @brief OSDP PD Events
 */
enum osdp_event_type {
	OSDP_EVENT_CARDREAD,  /**< Card read event */
	OSDP_EVENT_KEYPRESS,  /**< Keypad press event */
	OSDP_EVENT_SENTINEL   /**< Max event value */
};

/**
 * @brief OSDP Event structure.
 */
struct osdp_event {
	/** @cond INTERNAL_HIDDEN */
	sys_snode_t node;
	/** @endcond */
	/**
	 * Event type.
	 * Used to select specific event in union.
	 */
	enum osdp_event_type type;
	/** Event */
	union {
		struct osdp_event_keypress keypress; /**< Keypress event structure */
		struct osdp_event_cardread cardread; /**< Card read event structure */
	};
};

/**
 * @brief Callback for PD command notifications. After it has been registered
 * with `osdp_pd_set_command_callback`, this method is invoked when the PD
 * receives a command from the CP.
 *
 * @param arg pointer that will was passed to the arg param of
 * `osdp_pd_set_command_callback`.
 * @param cmd pointer to the received command.
 *
 * @retval 0 if LibOSDP must send a `osdp_ACK` response
 * @retval -ve if LibOSDP must send a `osdp_NAK` response
 * @retval +ve and modify the passed `struct osdp_cmd *cmd` if LibOSDP must send
 * a specific response. This is useful for sending manufacturer specific
 * reply ``osdp_MFGREP``.
 */
typedef int (*pd_command_callback_t)(void *arg, struct osdp_cmd *cmd);

/**
 * @brief Callback for CP event notifications. After it has been registered with
 * `osdp_cp_set_event_callback`, this method is invoked when the CP receives an
 * event from the PD.
 *
 * @param arg Opaque pointer provided by the application during callback
 *            registration.
 * @param pd the offset (0-indexed) of this PD in kconfig `OSDP_PD_ADDRESS_LIST`
 * @param ev pointer to osdp_event struct (filled by libosdp).
 *
 * @retval 0 on handling the event successfully.
 * @retval -ve on errors.
 */
typedef int (*cp_event_callback_t)(void *arg, int pd, struct osdp_event *ev);

#if defined(CONFIG_OSDP_MODE_PD) || defined(__DOXYGEN__)

/**
 * @name Peripheral Device mode APIs
 * @note These are only available when @kconfig{CONFIG_OSDP_MODE_PD} is enabled.
 * @{
 */

/**
 * @brief Set callback method for PD command notification. This callback is
 * invoked when the PD receives a command from the CP.
 *
 * @param cb The callback function's pointer
 * @param arg A pointer that will be passed as the first argument of `cb`
 */
void osdp_pd_set_command_callback(pd_command_callback_t cb, void *arg);

/**
 * @brief API to notify PD events to CP. These events are sent to the CP as an
 * alternate response to a POLL command.
 *
 * @param event pointer to event struct. Must be filled by application.
 *
 * @retval 0 on success
 * @retval -1 on failure
 */
int osdp_pd_notify_event(const struct osdp_event *event);

/**
 * @}
 */

#else /* CONFIG_OSDP_MODE_PD */

/**
 * @brief Generic command enqueue API.
 *
 * @param pd the offset (0-indexed) of this PD in kconfig `OSDP_PD_ADDRESS_LIST`
 * @param cmd command pointer. Must be filled by application.
 *
 * @retval 0 on success
 * @retval -1 on failure
 *
 * @note This method only adds the command on to a particular PD's command
 * queue. The command itself can fail due to various reasons.
 */
int osdp_cp_send_command(int pd, struct osdp_cmd *cmd);


/**
 * @brief Set callback method for CP event notification. This callback is
 * invoked when the CP receives an event from the PD.
 *
 * @param cb The callback function's pointer
 * @param arg A pointer that will be passed as the first argument of `cb`
 */
void osdp_cp_set_event_callback(cp_event_callback_t cb, void *arg);

#endif /* CONFIG_OSDP_MODE_PD */

#if defined(CONFIG_OSDP_SC_ENABLED) || defined(__DOXYGEN__)

/**
 * @name OSDP Secure Channel APIs
 * @note These are only available when @kconfig{CONFIG_OSDP_SC_ENABLED} is
 * enabled.
 * @{
 */

/**
 * Get the current SC status mask.
 * @return SC status mask
 */
uint32_t osdp_get_sc_status_mask(void);

/**
 * @}
 */

#endif

#ifdef __cplusplus
}
#endif

#endif	/* ZEPHYR_INCLUDE_MGMT_OSDP_H_ */
