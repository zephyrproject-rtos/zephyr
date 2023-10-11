/*
 * USB audio class core header
 *
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Audio Device Class public header
 *
 * Header follows below documentation:
 * - USB Device Class Definition for Audio Devices (audio10.pdf)
 *
 * Additional documentation considered a part of USB Audio v1.0:
 * - USB Device Class Definition for Audio Data Formats (frmts10.pdf)
 * - USB Device Class Definition for Terminal Types (termt10.pdf)
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_AUDIO_H_
#define ZEPHYR_INCLUDE_USB_CLASS_AUDIO_H_

#include <zephyr/usb/usb_ch9.h>
#include <zephyr/device.h>
#include <zephyr/net/buf.h>
#include <zephyr/sys/util.h>

/** Audio Interface Subclass Codes.
 * Refer to Table A-2 from audio10.pdf
 */
enum usb_audio_int_subclass_codes {
	USB_AUDIO_SUBCLASS_UNDEFINED	= 0x00,
	USB_AUDIO_AUDIOCONTROL		= 0x01,
	USB_AUDIO_AUDIOSTREAMING	= 0x02,
	USB_AUDIO_MIDISTREAMING		= 0x03
};

/** Audio Class-Specific AC Interface Descriptor Subtypes.
 * Refer to Table A-5 from audio10.pdf
 */
enum usb_audio_cs_ac_int_desc_subtypes {
	USB_AUDIO_AC_DESCRIPTOR_UNDEFINED	= 0x00,
	USB_AUDIO_HEADER			= 0x01,
	USB_AUDIO_INPUT_TERMINAL		= 0x02,
	USB_AUDIO_OUTPUT_TERMINAL		= 0x03,
	USB_AUDIO_MIXER_UNIT			= 0x04,
	USB_AUDIO_SELECTOR_UNIT			= 0x05,
	USB_AUDIO_FEATURE_UNIT			= 0x06,
	USB_AUDIO_PROCESSING_UNIT		= 0x07,
	USB_AUDIO_EXTENSION_UNIT		= 0x08
};

/** Audio Class-Specific AS Interface Descriptor Subtypes
 * Refer to Table A-6 from audio10.pdf
 */
enum usb_audio_cs_as_int_desc_subtypes {
	USB_AUDIO_AS_DESCRIPTOR_UNDEFINED	= 0x00,
	USB_AUDIO_AS_GENERAL			= 0x01,
	USB_AUDIO_FORMAT_TYPE			= 0x02,
	USB_AUDIO_FORMAT_SPECIFIC		= 0x03
};

/** Audio Class-Specific Request Codes
 * Refer to Table A-9 from audio10.pdf
 */
enum usb_audio_cs_req_codes {
	USB_AUDIO_REQUEST_CODE_UNDEFINED	= 0x00,
	USB_AUDIO_SET_CUR			= 0x01,
	USB_AUDIO_GET_CUR			= 0x81,
	USB_AUDIO_SET_MIN			= 0x02,
	USB_AUDIO_GET_MIN			= 0x82,
	USB_AUDIO_SET_MAX			= 0x03,
	USB_AUDIO_GET_MAX			= 0x83,
	USB_AUDIO_SET_RES			= 0x04,
	USB_AUDIO_GET_RES			= 0x84,
	USB_AUDIO_SET_MEM			= 0x05,
	USB_AUDIO_GET_MEM			= 0x85,
	USB_AUDIO_GET_STAT			= 0xFF
};

/** Feature Unit Control Selectors
 * Refer to Table A-11 from audio10.pdf
 */
enum usb_audio_fucs {
	USB_AUDIO_FU_CONTROL_UNDEFINED		= 0x00,
	USB_AUDIO_FU_MUTE_CONTROL		= 0x01,
	USB_AUDIO_FU_VOLUME_CONTROL		= 0x02,
	USB_AUDIO_FU_BASS_CONTROL		= 0x03,
	USB_AUDIO_FU_MID_CONTROL		= 0x04,
	USB_AUDIO_FU_TREBLE_CONTROL		= 0x05,
	USB_AUDIO_FU_GRAPHIC_EQUALIZER_CONTROL	= 0x06,
	USB_AUDIO_FU_AUTOMATIC_GAIN_CONTROL	= 0x07,
	USB_AUDIO_FU_DELAY_CONTROL		= 0x08,
	USB_AUDIO_FU_BASS_BOOST_CONTROL		= 0x09,
	USB_AUDIO_FU_LOUDNESS_CONTROL		= 0x0A
};

/** USB Terminal Types
 * Refer to Table 2-1 - Table 2-4 from termt10.pdf
 */
enum usb_audio_terminal_types {
	/**
	 * @name USB Terminal Types
	 * @{
	 */
	/** USB undefined */
	USB_AUDIO_USB_UNDEFINED   = 0x0100,
	/** USB streaming */
	USB_AUDIO_USB_STREAMING   = 0x0101,
	/** USB vendor specific */
	USB_AUDIO_USB_VENDOR_SPEC = 0x01FF,
	/** @} */

	/**
	 * @name Input Terminal Types
	 * @{
	 */
	/** Input undefined */
	USB_AUDIO_IN_UNDEFINED      = 0x0200,
	/** Microphone */
	USB_AUDIO_IN_MICROPHONE     = 0x0201,
	/** Desktop microphone */
	USB_AUDIO_IN_DESKTOP_MIC    = 0x0202,
	/** Personal microphone */
	USB_AUDIO_IN_PERSONAL_MIC   = 0x0203,
	/** Omni directional microphone */
	USB_AUDIO_IN_OM_DIR_MIC     = 0x0204,
	/** Microphone array */
	USB_AUDIO_IN_MIC_ARRAY      = 0x0205,
	/** Processing microphone array */
	USB_AUDIO_IN_PROC_MIC_ARRAY = 0x0205,
	/** @} */

	/**
	 * @name Output Terminal Types
	 * @{
	 */
	/** Output undefined */
	USB_AUDIO_OUT_UNDEFINED		= 0x0300,
	/** Speaker */
	USB_AUDIO_OUT_SPEAKER		= 0x0301,
	/** Headphones */
	USB_AUDIO_OUT_HEADPHONES	= 0x0302,
	/** Head mounted display audio */
	USB_AUDIO_OUT_HEAD_AUDIO	= 0x0303,
	/** Desktop speaker */
	USB_AUDIO_OUT_DESKTOP_SPEAKER	= 0x0304,
	/** Room speaker */
	USB_AUDIO_OUT_ROOM_SPEAKER	= 0x0305,
	/** Communication speaker */
	USB_AUDIO_OUT_COMM_SPEAKER	= 0x0306,
	/** Low frequency effects speaker */
	USB_AUDIO_OUT_LOW_FREQ_SPEAKER	= 0x0307,
	/** @} */

	/**
	 * @name Bi-directional Terminal Types
	 * @{
	 */
	/** Bidirectional undefined */
	USB_AUDIO_IO_UNDEFINED			= 0x0400,
	/** Handset */
	USB_AUDIO_IO_HANDSET			= 0x0401,
	/** Headset */
	USB_AUDIO_IO_HEADSET			= 0x0402,
	/** Speakerphone, no echo reduction */
	USB_AUDIO_IO_SPEAKERPHONE_ECHO_NONE	= 0x0403,
	/** Speakerphone, echo reduction */
	USB_AUDIO_IO_SPEAKERPHONE_ECHO_SUP	= 0x0404,
	/** Speakerphone, echo cancellation */
	USB_AUDIO_IO_SPEAKERPHONE_ECHO_CAN	= 0x0405,
};

/**
 * @brief Audio device direction.
 */
enum usb_audio_direction {
	USB_AUDIO_IN = 0x00,
	USB_AUDIO_OUT = 0x01
};

/**
 * @brief Feature Unit event structure.
 *
 * The event structure is used by feature_update_cb in order to inform the App
 * whenever the Host has modified one of the device features.
 */
struct usb_audio_fu_evt {
	/**
	 * The device direction that has been changed.
	 * Applicable for Headset device only.
	 */
	enum usb_audio_direction dir;
	/** Control selector feature that has been changed. */
	enum usb_audio_fucs cs;
	/**
	 * Device channel that has been changed.
	 * If 0xFF, then all channels have been changed.
	 */
	uint8_t channel;
	/** Length of the val field. */
	uint8_t val_len;
	/** Value of the feature that has been set. */
	const void *val;
};

/**
 * @brief Callback type used to inform the app that data were requested
 *	  from the device and may be send to the Host.
 *	  For sending the data usb_audio_send() API function should be used.
 *
 * @note User may not use this callback and may try to send in 1ms task
 *	 instead. Sending every 1ms may be unsuccessful and may return -EAGAIN
 *	 if Host did not required data.
 *
 * @param dev The device for which data were requested by the Host.
 */
typedef void (*usb_audio_data_request_cb_t)(const struct device *dev);

/**
 * @brief Callback type used to inform the app that data were successfully
 *	  send/received.
 *
 * @param dev	 The device for which the callback was called.
 * @param buffer Pointer to the net_buf data chunk that was successfully
 *		 send/received. If the application uses data_written_cb and/or
 *		 data_received_cb callbacks it is responsible for freeing the
 *		 buffer by itself.
 * @param size	 Amount of data that were successfully send/received.
 */
typedef void (*usb_audio_data_completion_cb_t)(const struct device *dev,
					       struct net_buf *buffer,
					       size_t size);

/**
 * @brief Callback type used to inform the app that Host has changed
 *	  one of the features configured for the device.
 *	  Applicable for all devices.
 *
 * @warning Host may not use all of configured features.
 *
 * @param dev USB Audio device
 * @param evt Pointer to an event to be parsed by the App.
 *	      Pointer struct is temporary and is valid only during the
 *	      execution of this callback.
 */
typedef void (*usb_audio_feature_updated_cb_t)(const struct device *dev,
					       const struct usb_audio_fu_evt *evt);

/**
 * @brief Audio callbacks used to interact with audio devices by user App.
 *
 * usb_audio_ops structure contains all relevant callbacks to interact with
 * USB Audio devices. Each of this callbacks is optional and may be left NULL.
 * This will not break the stack but could make USB Audio device useless.
 * Depending on the device some of those callbacks are necessary to make USB
 * device work as expected sending/receiving data. For more information refer
 * to callback documentation above.
 */
struct usb_audio_ops {
	/** Callback called when data could be send */
	usb_audio_data_request_cb_t data_request_cb;

	/** Callback called when data were successfully written with sending
	 * capable device. Applicable for headset and microphone. Unused for
	 * headphones.
	 */
	usb_audio_data_completion_cb_t data_written_cb;

	/** Callback called when data were successfully received by receive
	 * capable device. Applicable for headset and headphones. Unused for
	 * microphone.
	 */
	usb_audio_data_completion_cb_t data_received_cb;

	/** Callback called when features were modified by the Host */
	usb_audio_feature_updated_cb_t feature_update_cb;
};

/** @brief Get the frame size that is accepted by the Host.
 *
 * This function returns the frame size for Input Devices that is expected
 * by the Host. Returned value rely on Input Device configuration:
 * - number of channels
 * - sampling frequency
 * - sample resolution
 * Device configuration is done via DT overlay.
 *
 * @param dev The Input device that is asked for frame size.
 *
 * @warning Do not use with OUT only devices (Headphones).
 *	    For OUT only devices this function shall return 0.
 */
size_t usb_audio_get_in_frame_size(const struct device *dev);

/**
 * @brief Register the USB Audio device and make it usable.
 *	  This must be called in order to make the device work
 *	  and respond to all relevant requests.
 *
 * @param dev USB Audio device
 * @param ops USB audio callback structure. Callback are used to
 *	      inform the user about what is happening
 */
void usb_audio_register(const struct device *dev,
			const struct usb_audio_ops *ops);

/**
 * @brief Send data using USB Audio device
 *
 * @param dev    USB Audio device which will send the data
 *		 over its ISO IN endpoint
 * @param buffer Pointer to the buffer that should be send. User is
 *		 responsible for managing the buffer for Input devices. In case
 *		 of sending error user must decide if the buffer should be
 *		 dropped or retransmitted.
 *		 After the buffer was sent successfully it is passed to the
 *		 data_written_cb callback if the application uses one or
 *		 automatically freed otherwise.
 *		 User must provide proper net_buf chunk especially when
 *		 it comes to its size. This information can be obtained
 *		 using usb_audio_get_in_frame_size() API function.
 *
 * @param len    Length of the data to be send
 *
 * @return 0 on success, negative error on fail
 */
int usb_audio_send(const struct device *dev, struct net_buf *buffer,
		   size_t len);

#endif /* ZEPHYR_INCLUDE_USB_CLASS_AUDIO_H_ */
