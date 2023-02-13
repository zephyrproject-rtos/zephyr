/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _OSDP_H_
#define _OSDP_H_

#include <zephyr/kernel.h>
#include <stdint.h>

#include <zephyr/sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OSDP_CMD_TEXT_MAX_LEN          32
#define OSDP_CMD_KEYSET_KEY_MAX_LEN    32
#define OSDP_EVENT_MAX_DATALEN         64

/**
 * @brief Command sent from CP to Control digital output of PD.
 *
 * @param output_no 0 = First Output, 1 = Second Output, etc.
 * @param control_code One of the following:
 *   0 - NOP – do not alter this output
 *   1 - set the permanent state to OFF, abort timed operation (if any)
 *   2 - set the permanent state to ON, abort timed operation (if any)
 *   3 - set the permanent state to OFF, allow timed operation to complete
 *   4 - set the permanent state to ON, allow timed operation to complete
 *   5 - set the temporary state to ON, resume perm state on timeout
 *   6 - set the temporary state to OFF, resume permanent state on timeout
 * @param timer_count Time in units of 100 ms
 */
struct osdp_cmd_output {
	uint8_t output_no;
	uint8_t control_code;
	uint16_t timer_count;
};

/**
 * @brief LED Colors as specified in OSDP for the on_color/off_color parameters.
 */
enum osdp_led_color_e {
	OSDP_LED_COLOR_NONE,
	OSDP_LED_COLOR_RED,
	OSDP_LED_COLOR_GREEN,
	OSDP_LED_COLOR_AMBER,
	OSDP_LED_COLOR_BLUE,
	OSDP_LED_COLOR_SENTINEL
};

/**
 * @brief LED params sub-structure. Part of LED command. See struct osdp_cmd_led
 *
 * @param control_code One of the following:
 * Temporary Control Code:
 *   0 - NOP - do not alter this LED's temporary settings
 *   1 - Cancel any temporary operation and display this LED's permanent state
 *       immediately
 *   2 - Set the temporary state as given and start timer immediately
 * Permanent Control Code:
 *   0 - NOP - do not alter this LED's permanent settings
 *   1 - Set the permanent state as given
 * @param on_count The ON duration of the flash, in units of 100 ms
 * @param off_count The OFF duration of the flash, in units of 100 ms
 * @param on_color Color to set during the ON timer (enum osdp_led_color_e)
 * @param off_color Color to set during the OFF timer (enum osdp_led_color_e)
 * @param timer_count Time in units of 100 ms (only for temporary mode)
 */
struct osdp_cmd_led_params {
	uint8_t control_code;
	uint8_t on_count;
	uint8_t off_count;
	uint8_t on_color;
	uint8_t off_color;
	uint16_t timer_count;
};

/**
 * @brief Sent from CP to PD to control the behaviour of it's on-board LEDs
 *
 * @param reader 0 = First Reader, 1 = Second Reader, etc.
 * @param led_number 0 = first LED, 1 = second LED, etc.
 * @param temporary ephemeral LED status descriptor
 * @param permanent permanent LED status descriptor
 */
struct osdp_cmd_led {
	uint8_t reader;
	uint8_t led_number;
	struct osdp_cmd_led_params temporary;
	struct osdp_cmd_led_params permanent;
};

/**
 * @brief Sent from CP to control the behaviour of a buzzer in the PD.
 *
 * @param reader 0 = First Reader, 1 = Second Reader, etc.
 * @param control_code 0: no tone, 1: off, 2: default tone, 3+ is TBD.
 * @param on_count The ON duration of the flash, in units of 100 ms
 * @param off_count The OFF duration of the flash, in units of 100 ms
 * @param rep_count The number of times to repeat the ON/OFF cycle; 0: forever
 */
struct osdp_cmd_buzzer {
	uint8_t reader;
	uint8_t control_code;
	uint8_t on_count;
	uint8_t off_count;
	uint8_t rep_count;
};

/**
 * @brief Command to manipulate any display units that the PD supports.
 *
 * @param reader 0 = First Reader, 1 = Second Reader, etc.
 * @param control_code One of the following:
 *   1 - permanent text, no wrap
 *   2 - permanent text, with wrap
 *   3 - temp text, no wrap
 *   4 - temp text, with wrap
 * @param temp_time duration to display temporary text, in seconds
 * @param offset_row row to display the first character (1 indexed)
 * @param offset_col column to display the first character (1 indexed)
 * @param length Number of characters in the string
 * @param data The string to display
 */
struct osdp_cmd_text {
	uint8_t reader;
	uint8_t control_code;
	uint8_t temp_time;
	uint8_t offset_row;
	uint8_t offset_col;
	uint8_t length;
	uint8_t data[OSDP_CMD_TEXT_MAX_LEN];
};

/**
 * @brief Sent in response to a COMSET command. Set communication parameters to
 * PD. Must be stored in PD non-volatile memory.
 *
 * @param address Unit ID to which this PD will respond after the change takes
 *             effect.
 * @param baud_rate baud rate value 9600/38400/115200
 */
struct osdp_cmd_comset {
	uint8_t address;
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
	uint8_t type;
	uint8_t length;
	uint8_t data[OSDP_CMD_KEYSET_KEY_MAX_LEN];
};

/**
 * @brief OSDP application exposed commands
 */
enum osdp_cmd_e {
	OSDP_CMD_OUTPUT = 1,
	OSDP_CMD_LED,
	OSDP_CMD_BUZZER,
	OSDP_CMD_TEXT,
	OSDP_CMD_KEYSET,
	OSDP_CMD_COMSET,
	OSDP_CMD_SENTINEL
};

/**
 * @brief OSDP Command Structure. This is a wrapper for all individual OSDP
 * commands.
 *
 * @param id used to select specific commands in union. Type: enum osdp_cmd_e
 * @param led LED command structure
 * @param buzzer buzzer command structure
 * @param text text command structure
 * @param output output command structure
 * @param comset comset command structure
 * @param keyset keyset command structure
 */
struct osdp_cmd {
	sys_snode_t node;
	enum osdp_cmd_e id;
	union {
		struct osdp_cmd_led    led;
		struct osdp_cmd_buzzer buzzer;
		struct osdp_cmd_text   text;
		struct osdp_cmd_output output;
		struct osdp_cmd_comset comset;
		struct osdp_cmd_keyset keyset;
	};
};

/**
 * @brief Various card formats that a PD can support. This is sent to CP
 * when a PD must report a card read.
 */
enum osdp_event_cardread_format_e {
	OSDP_CARD_FMT_RAW_UNSPECIFIED,
	OSDP_CARD_FMT_RAW_WIEGAND,
	OSDP_CARD_FMT_ASCII,
	OSDP_CARD_FMT_SENTINEL
};

/**
 * @brief OSDP event cardread
 *
 * @param reader_no In context of readers attached to current PD, this number
 *        indicated this number. This is not supported by LibOSDP.
 * @param format Format of the card being read.
 *        see `enum osdp_event_cardread_format_e`
 * @param direction Card read direction of PD 0 - Forward; 1 - Backward
 * @param length Length of card data in bytes or bits depending on `format`
 *        (see note).
 * @param data Card data of `length` bytes or bits bits depending on `format`
 *        (see note).
 *
 * @note When `format` is set to OSDP_CARD_FMT_RAW_UNSPECIFIED or
 * OSDP_CARD_FMT_RAW_WIEGAND, the length is expressed in bits. OTOH, when it is
 * set to OSDP_CARD_FMT_ASCII, the length is in bytes. The number of bytes to
 * read from the `data` field must be interpreted accordingly.
 */
struct osdp_event_cardread {
	int reader_no;
	enum osdp_event_cardread_format_e format;
	int direction;
	int length;
	uint8_t data[OSDP_EVENT_MAX_DATALEN];
};

/**
 * @brief OSDP Event Keypad
 *
 * @param reader_no In context of readers attached to current PD, this number
 *                  indicated this number. This is not supported by LibOSDP.
 * @param length Length of keypress data in bytes
 * @param data keypress data of `length` bytes
 */
struct osdp_event_keypress {
	int reader_no;
	int length;
	uint8_t data[OSDP_EVENT_MAX_DATALEN];
};

/**
 * @brief OSDP PD Events
 */
enum osdp_event_type {
	OSDP_EVENT_CARDREAD,
	OSDP_EVENT_KEYPRESS,
	OSDP_EVENT_SENTINEL
};

/**
 * @brief OSDP Event structure.
 *
 * @param type used to select specific event in union. See: enum osdp_event_type
 * @param keypress keypress event structure
 * @param cardread cardread event structure
 */
struct osdp_event {
	sys_snode_t node;
	enum osdp_event_type type;
	union {
		struct osdp_event_keypress keypress;
		struct osdp_event_cardread cardread;
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

#ifdef CONFIG_OSDP_MODE_PD

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

#ifdef CONFIG_OSDP_SC_ENABLED

uint32_t osdp_get_sc_status_mask(void);

#endif

#ifdef __cplusplus
}
#endif

#endif	/* _OSDP_H_ */
